#include "startup.h"

#include <actcraft/actions/commandregistry.h>
#include <actcraft/core/model.h>
#include <actcraft/export/officeconvert.h>
#include <actcraft/export/pdfexport.h>
#include <actcraft/export/pdfimport.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/documenttabs.h>
#include <actcraft/ui/fileexchange.h>

#include <QDir>
#include <QFile>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

// PDF d'essai de deux pages, produit par notre export (L5-T23) et commité.
QString fixturePdf()
{
    return QStringLiteral(ACTCRAFT_IMPORT_DATA) + QStringLiteral("/deux-pages.pdf");
}

// Convertisseur factice : « cmake -E copy » du PDF d'essai vers {out}. Un chemin de la valeur ne contient pas
// de « | » (séparateur de la variable).
QByteArray fakeConverter(const QString& source)
{
    return (QStringLiteral(ACTCRAFT_CMAKE) + QStringLiteral("|-E|copy|") + source + QStringLiteral("|{out}"))
        .toLocal8Bit();
}

// L'interface réelle (Main.qml) : les commandes passent par leurs gestionnaires, comme au menu et aux
// scénarios.
struct Shell
{
    QQmlApplicationEngine engine;
    QQuickWindow* window = nullptr;
    ui::FileExchange* exchange = nullptr;
    ui::DocumentTabs* tabs = nullptr;
    ui::DocumentController* document = nullptr;

    bool start()
    {
        window = loadMainWindow(engine, StartupOptions{});
        if (!window || !QTest::qWaitForWindowExposed(window)) {
            return false;
        }
        exchange = window->findChild<ui::FileExchange*>(QStringLiteral("appExchange"));
        tabs = window->findChild<ui::DocumentTabs*>(QStringLiteral("appTabs"));
        document = window->findChild<ui::DocumentController*>(QStringLiteral("documentController"));
        return exchange && tabs && document;
    }
    bool run(const QString& id, const QVariant& argument = QVariant())
    {
        QVariant result;
        return QMetaObject::invokeMethod(window, "runCommand", Q_RETURN_ARG(QVariant, result),
                                         Q_ARG(QVariant, QVariant(id)), Q_ARG(QVariant, argument)) &&
               result.toBool();
    }
};

// Une présentation factice (le convertisseur factice ne la lit pas, mais la conversion exige qu'elle existe).
QString dummyPresentation(const QTemporaryDir& dir, const QString& name)
{
    const QString path = dir.filePath(name);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write("pptx") != 4) {
        return {};
    }
    return path;
}

// Les entrées du sous-menu « Importer » du menu Fichier.
QStringList importEntries(const ui::Commands& commands)
{
    QStringList out;
    for (const QVariant& menu : commands.menus()) {
        for (const QVariant& item : menu.toMap().value(QStringLiteral("items")).toList()) {
            const QVariantMap map = item.toMap();
            if (map.value(QStringLiteral("id")).toString() != QLatin1String("import")) {
                continue;
            }
            for (const QVariant& sub : map.value(QStringLiteral("items")).toList()) {
                out << sub.toMap().value(QStringLiteral("id")).toString();
            }
        }
    }
    return out;
}

} // namespace

class TestUiImport : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase()
    {
        qunsetenv("ACTCRAFT_OFFICE_CONVERTER");
        qunsetenv("ACTCRAFT_POWERSHELL");
    }
    void cleanup()
    {
        qunsetenv("ACTCRAFT_OFFICE_CONVERTER");
        for (const char* variable : {"TMPDIR", "TMP", "TEMP"}) {
            qunsetenv(variable);
        }
    }

    void fixtureHasTwoPages()
    {
        QFile file(fixturePdf());
        QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(fixturePdf()));
        QCOMPARE(exporting::countPdfPages(file.readAll()), 2);
    }

    void powerpointThroughFakeConverter()
    {
        if (!exporting::pdfImportAvailable()) {
            QSKIP("Qt PDF absent : import PDF indisponible dans cette construction");
        }
        // Le dossier temporaire de la conversion naît sous TMPDIR/TMP/TEMP : un dossier à nous, vide d'abord.
        QTemporaryDir scratch;
        QVERIFY(scratch.isValid());
        for (const char* variable : {"TMPDIR", "TMP", "TEMP"}) {
            qputenv(variable, QFile::encodeName(scratch.path()));
        }
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString deck = dummyPresentation(dir, QStringLiteral("cours.pptx"));
        QVERIFY(!deck.isEmpty());
        qputenv("ACTCRAFT_OFFICE_CONVERTER", fakeConverter(fixturePdf()));

        Shell s;
        QVERIFY(s.start());
        QVERIFY(s.exchange->powerpointAvailable());
        QSignalSpy finished(s.exchange, &ui::FileExchange::finished);
        QSignalSpy busy(s.exchange, &ui::FileExchange::busyChanged);
        QCOMPARE(s.tabs->rowCount(), 1);
        QVERIFY(s.run(QStringLiteral("file.import-powerpoint-images"), deck));
        QVERIFY(s.exchange->isBusy());
        QTRY_COMPARE_WITH_TIMEOUT(s.tabs->rowCount(), 2, 20000);
        QTRY_VERIFY_WITH_TIMEOUT(!s.exchange->isBusy() && finished.count() == 1, 20000);
        QCOMPARE(s.tabs->currentIndex(), 1);
        QCOMPARE(s.document->pageCount(), 2);
        QCOMPARE(s.document->title(), QStringLiteral("cours"));
        QVERIFY(finished.first().first().toString().contains(QStringLiteral("cours.pptx")));
        QVERIFY(busy.count() >= 2); // occupé, puis libre
        // Le PDF converti et son dossier sont supprimés après l'import.
        QVERIFY2(QDir(scratch.path()).isEmpty(), qPrintable(QDir(scratch.path()).entryList().join(' ')));
        QVERIFY(QFile::exists(deck)); // la présentation n'est pas touchée
        for (const char* variable : {"TMPDIR", "TMP", "TEMP"}) {
            qunsetenv(variable);
        }
    }

    void failureShowsBanner()
    {
        if (!exporting::pdfImportAvailable()) {
            QSKIP("Qt PDF absent : import PDF indisponible dans cette construction");
        }
        QTemporaryDir scratch;
        QVERIFY(scratch.isValid());
        for (const char* variable : {"TMPDIR", "TMP", "TEMP"}) {
            qputenv(variable, QFile::encodeName(scratch.path()));
        }
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString deck = dummyPresentation(dir, QStringLiteral("cassee.pptx"));
        // Le convertisseur copie un fichier qui n'est pas un PDF : la conversion réussit, la lecture échoue.
        const QString notPdf = dir.filePath(QStringLiteral("faux.pdf"));
        QFile bad(notPdf);
        QVERIFY(bad.open(QIODevice::WriteOnly));
        bad.write("ceci n'est pas un PDF");
        bad.close();
        qputenv("ACTCRAFT_OFFICE_CONVERTER", fakeConverter(notPdf));

        Shell s;
        QVERIFY(s.start());
        QSignalSpy failed(s.exchange, &ui::FileExchange::failed);
        QVERIFY(s.run(QStringLiteral("file.import-powerpoint-images"), deck));
        QTRY_VERIFY_WITH_TIMEOUT(failed.count() == 1 && !s.exchange->isBusy(), 20000);
        QVERIFY(failed.first().first().toString().contains(QStringLiteral("impossible")));
        QCOMPARE(s.tabs->rowCount(), 1);
        QVERIFY2(QDir(scratch.path()).isEmpty(), qPrintable(QDir(scratch.path()).entryList().join(' ')));

        // Présentation introuvable : refus visible aussi, rien ne reste occupé.
        failed.clear();
        QVERIFY(s.run(QStringLiteral("file.import-powerpoint-images"),
                      dir.filePath(QStringLiteral("absente.pptx"))));
        QTRY_VERIFY_WITH_TIMEOUT(failed.count() == 1 && !s.exchange->isBusy(), 20000);
        QCOMPARE(s.tabs->rowCount(), 1);
        for (const char* variable : {"TMPDIR", "TMP", "TEMP"}) {
            qunsetenv(variable);
        }
    }

    void refusedWithoutConverter()
    {
        // Aucun convertisseur : variable absente, PATH vide, pas de couture ; LibreOffice installé dans
        // /Applications ou PowerPoint (Windows) rendraient la valeur vraie : le test ne juge que la
        // cohérence.
        Shell s;
        QVERIFY(s.start());
        QCOMPARE(s.exchange->powerpointAvailable(), exporting::findConverter().has_value());
        qputenv("ACTCRAFT_OFFICE_CONVERTER", fakeConverter(fixturePdf()));
        QVERIFY(s.exchange->powerpointAvailable());
    }

    void menuHidesUnavailableImports()
    {
        ui::Commands commands;
        commands.setProfileDir(QTemporaryDir().path());
        commands.setPlatform(actions::Platform::Windows);
        QCOMPARE(importEntries(commands), QStringList({QStringLiteral("file.import-pdf"),
                                                       QStringLiteral("file.import-powerpoint-images")}));
        // Une entrée indisponible disparaît du menu, sans toucher à l'autre.
        QVERIFY(QMetaObject::invokeMethod(&commands, "setHidden",
                                          Q_ARG(QString, QStringLiteral("file.import-powerpoint-images")),
                                          Q_ARG(bool, true)));
        QCOMPARE(importEntries(commands), QStringList({QStringLiteral("file.import-pdf")}));
        QVERIFY(QMetaObject::invokeMethod(
            &commands, "setHidden", Q_ARG(QString, QStringLiteral("file.import-pdf")), Q_ARG(bool, true)));
        QVERIFY(importEntries(commands).isEmpty()); // sous-menu vide : omis
        QVERIFY(QMetaObject::invokeMethod(
            &commands, "setHidden", Q_ARG(QString, QStringLiteral("file.import-pdf")), Q_ARG(bool, false)));
        QVERIFY(QMetaObject::invokeMethod(&commands, "setHidden",
                                          Q_ARG(QString, QStringLiteral("file.import-powerpoint-images")),
                                          Q_ARG(bool, false)));
        QCOMPARE(importEntries(commands).size(), 2);
        // Le convertisseur n'est offert qu'aux postes de bureau : Android n'a ni l'un ni l'autre.
        for (const actions::Platform platform : {actions::Platform::MacOS, actions::Platform::Linux}) {
            commands.setPlatform(platform);
            QVERIFY(commands.isAvailable(QStringLiteral("file.import-powerpoint-images")));
        }
        commands.setPlatform(actions::Platform::Android);
        QVERIFY(!commands.isAvailable(QStringLiteral("file.import-powerpoint-images")));
    }
};

QTEST_MAIN(TestUiImport)
#include "tst_ui_import.moc"
