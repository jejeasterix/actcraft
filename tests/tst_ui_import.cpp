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

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QLibraryInfo>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <array>
#include <optional>

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

// Vrai si l'entrée figure quelque part dans les menus (sous-menus compris).
bool inMenus(const QVariantList& items, const QString& id)
{
    for (const QVariant& v : items) {
        const QVariantMap m = v.toMap();
        if (m.value(QStringLiteral("id")).toString() == id) {
            return true;
        }
        if (inMenus(m.value(QStringLiteral("items")).toList(), id)) {
            return true;
        }
    }
    return false;
}

// Faux « soffice » exécutable dans un dossier à part, pour mettre une machine « avec LibreOffice » sous le
// test.
QString fakeSofficeDirectory(QTemporaryDir& dir)
{
    QFile file(dir.filePath(QStringLiteral("soffice")));
    if (!file.open(QIODevice::WriteOnly) || file.write("#!/bin/sh\nexit 0\n") <= 0) {
        return {};
    }
    file.close();
    file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    return dir.path();
}

// PATH ne laissant voir aucun convertisseur : le dossier donné d'abord. Sous Windows, les DLL de Qt (plugins
// QML chargés à la demande, plateforme) se résolvent par le PATH : un PATH réduit au seul dossier vide fait
// échouer le chargement de Main.qml. On y garde donc les dossiers de Qt, de l'exécutable de test et de
// System32 ; aucun ne contient soffice (findConverter() cherche « soffice » sur le PATH, puis sous
// %ProgramFiles%\LibreOffice, puis PowerPoint par le registre). Hors Windows : le dossier seul.
QByteArray pathWithoutConverter(const QString& directory)
{
    QStringList entries{QDir::toNativeSeparators(directory)};
#ifdef Q_OS_WIN
    entries << QDir::toNativeSeparators(QLibraryInfo::path(QLibraryInfo::BinariesPath))
            << QDir::toNativeSeparators(QCoreApplication::applicationDirPath())
            << QDir::toNativeSeparators(qEnvironmentVariable("SystemRoot", QStringLiteral("C:\\Windows")) +
                                        QStringLiteral("\\System32"));
#endif
    return QFile::encodeName(entries.join(QDir::listSeparator()));
}

// PATH, TMPDIR, TMP, TEMP : valeurs d'origine mémorisées au départ, remises telles quelles (ou retirées si
// elles n'existaient pas) à la fin de chaque cas.
struct SavedEnvironment
{
    static constexpr std::array<const char*, 4> kNames{"PATH", "TMPDIR", "TMP", "TEMP"};
    std::array<std::optional<QByteArray>, 4> saved;

    void remember()
    {
        for (std::size_t i = 0; i < kNames.size(); ++i) {
            saved[i] = qEnvironmentVariableIsSet(kNames[i]) ? std::optional<QByteArray>(qgetenv(kNames[i]))
                                                            : std::nullopt;
        }
    }
    void restore() const
    {
        for (std::size_t i = 0; i < kNames.size(); ++i) {
            if (saved[i]) {
                qputenv(kNames[i], *saved[i]);
            } else {
                qunsetenv(kNames[i]);
            }
        }
    }
};

SavedEnvironment g_environment;

void pointTempAt(const QString& directory)
{
    for (const char* variable : {"TMPDIR", "TMP", "TEMP"}) {
        qputenv(variable, QFile::encodeName(directory));
    }
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
        g_environment.remember();
    }
    void cleanup()
    {
        qunsetenv("ACTCRAFT_OFFICE_CONVERTER");
        g_environment.restore();
    }

    // L5-C13 : l'entrée « Présentation en images » n'est dans le menu que si un convertisseur existe.
    void importEntryHiddenWithoutConverter()
    {
        if (!exporting::pdfImportAvailable()) {
            QSKIP("Qt PDF absent : import PDF indisponible dans cette construction");
        }
        const QString id = QStringLiteral("file.import-powerpoint-images");
        // PATH réduit à un dossier vide : aucun soffice, quelle que soit la machine. LibreOffice dans
        // /Applications ou PowerPoint (Windows) restent hors d'atteinte du test : si l'un d'eux est détecté,
        // l'entrée doit alors être visible, et le test ne juge que cette cohérence.
        QTemporaryDir emptyDir;
        QVERIFY(emptyDir.isValid());
        qputenv("PATH", pathWithoutConverter(emptyDir.path()));
        {
            Shell s;
            QVERIFY(s.start());
            auto* commands = s.window->findChild<ui::Commands*>(QStringLiteral("commands"));
            QVERIFY(commands);
            QCOMPARE(inMenus(commands->menus(), id), s.exchange->powerpointAvailable());
            if (!exporting::findConverter().has_value()) {
                QVERIFY(!s.exchange->powerpointAvailable());
                QVERIFY(!inMenus(commands->menus(), id));
            }
            QVERIFY(inMenus(commands->menus(), QStringLiteral("file.import-pdf")));
#ifndef Q_OS_WIN
            // Un soffice exécutable sur le PATH (machine avec LibreOffice) : l'entrée apparaît. Sur une
            // machine où LibreOffice est installé hors du PATH (/Applications sur Mac), findConverter() est
            // déjà vrai avec le PATH vide : le faux soffice ne prouverait rien, ce bloc seul est passé (pas
            // un talon : le reste du test s'exécute).
            if (exporting::findConverter().has_value()) {
                qInfo("Bloc « faux soffice » passé : un convertisseur est déjà trouvé avec le PATH vide.");
            } else {
                QTemporaryDir sofficeDir;
                QVERIFY(sofficeDir.isValid());
                QVERIFY(!fakeSofficeDirectory(sofficeDir).isEmpty());
                qputenv("PATH", pathWithoutConverter(sofficeDir.path()));
                emit s.exchange->converterChanged();
                QVERIFY(s.exchange->powerpointAvailable());
                QVERIFY(inMenus(commands->menus(), id));
                qputenv("PATH", pathWithoutConverter(emptyDir.path()));
            }
#endif
            // Le convertisseur apparaît, et l'objet l'annonce : l'entrée revient.
            qputenv("ACTCRAFT_OFFICE_CONVERTER", fakeConverter(fixturePdf()));
            emit s.exchange->converterChanged();
            QVERIFY(inMenus(commands->menus(), id));
        }
        qputenv("ACTCRAFT_OFFICE_CONVERTER", fakeConverter(fixturePdf()));
        Shell s;
        QVERIFY(s.start());
        auto* commands = s.window->findChild<ui::Commands*>(QStringLiteral("commands"));
        QVERIFY(commands);
        QVERIFY(inMenus(commands->menus(), id));
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
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString deck = dummyPresentation(dir, QStringLiteral("cours.pptx"));
        QVERIFY(!deck.isEmpty());
        qputenv("ACTCRAFT_OFFICE_CONVERTER", fakeConverter(fixturePdf()));
        pointTempAt(scratch.path());

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
    }

    void failureShowsBanner()
    {
        if (!exporting::pdfImportAvailable()) {
            QSKIP("Qt PDF absent : import PDF indisponible dans cette construction");
        }
        QTemporaryDir scratch;
        QVERIFY(scratch.isValid());
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
        pointTempAt(scratch.path());

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
    }

    void availabilityFollowsTheConverter()
    {
        // Sans la variable de test, la détection regarde la machine (soffice sur le PATH, LibreOffice dans
        // /Applications, PowerPoint sous Windows) : le test ne juge que la cohérence, sur toute machine.
        Shell s;
        QVERIFY(s.start());
        // Sans import PDF construit, rien n'est offert, convertisseur ou non.
        QCOMPARE(s.exchange->powerpointAvailable(),
                 exporting::pdfImportAvailable() && exporting::findConverter().has_value());
        qputenv("ACTCRAFT_OFFICE_CONVERTER", fakeConverter(fixturePdf()));
        QCOMPARE(s.exchange->powerpointAvailable(), exporting::pdfImportAvailable());
    }

    void menuHidesUnavailableImports()
    {
        ui::Commands commands;
        QTemporaryDir profile; // vivant pendant tout le test : le dossier existe encore
        QVERIFY(profile.isValid());
        commands.setProfileDir(profile.path());
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
