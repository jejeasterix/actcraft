#include <actcraft/actions/commandregistry.h>
#include <actcraft/actions/profilestore.h>
#include <actcraft/ui/profilecontroller.h>

#include <actcraft/ui/commands.h>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using namespace actcraft;

class TstUiProfiles : public QObject
{
    Q_OBJECT
    static actions::ProfileStore builtins()
    {
        return actions::ProfileStore(QString(), QStringLiteral(":/actcraft/actions/profiles"),
                                     ui::ProfileController::builtinOrder());
    }
private slots:
    void fiveBuiltinsInOrder()
    {
        QCOMPARE(builtins().ids(),
                 (QStringList{QStringLiteral("en-classe"), QStringLiteral("preparation"),
                              QStringLiteral("lecture-ecriture"), QStringLiteral("calcul-geometrie"),
                              QStringLiteral("son-image")}));
    }
    void pinnedCommandsExist()
    {
        actions::CommandRegistry registry;
        QString error;
        QVERIFY(registry.loadDeclarations(actions::defaultCommandsJson(), error));
        auto store = builtins();
        for (const QString& id : store.ids()) {
            const auto p = store.load(id, error);
            QVERIFY2(p, qPrintable(error));
            QVERIFY2(!p->label.isEmpty(), qPrintable(id));
            QVERIFY2(p->label != id, qPrintable(id + QStringLiteral(" : libellé de talon")));
            for (const QString& command : p->pinned)
                QVERIFY2(registry.find(command), qPrintable(id + QStringLiteral(" : ") + command));
        }
    }
    void pinnedListsAreTheAgreedOnes()
    {
        QString error;
        auto store = builtins();
        QCOMPARE(store.load(QStringLiteral("en-classe"), error)->pinned,
                 (QStringList{QStringLiteral("browser.resources"), QStringLiteral("present.curtain"),
                              QStringLiteral("present.spotlight-circle"), QStringLiteral("view.page-zoom"),
                              QStringLiteral("tool.clock")}));
        QVERIFY(store.load(QStringLiteral("preparation"), error)->pinned.isEmpty());
        QCOMPARE(store.load(QStringLiteral("lecture-ecriture"), error)->pinned,
                 (QStringList{QStringLiteral("tool.keyboard"), QStringLiteral("tool.handwriting"),
                              QStringLiteral("tool.ticker"), QStringLiteral("spell.check")}));
        QCOMPARE(store.load(QStringLiteral("calcul-geometrie"), error)->pinned,
                 (QStringList{QStringLiteral("math.ruler"), QStringLiteral("math.set-square"),
                              QStringLiteral("math.protractor"), QStringLiteral("math.compass"),
                              QStringLiteral("math.xy-origin"), QStringLiteral("tool.dice"),
                              QStringLiteral("tool.calculator")}));
        QCOMPARE(store.load(QStringLiteral("son-image"), error)->pinned,
                 (QStringList{QStringLiteral("tool.camera-area"), QStringLiteral("tool.camera-polygon"),
                              QStringLiteral("tool.camera-freehand"), QStringLiteral("tool.camera-window"),
                              QStringLiteral("tool.camera-screen"), QStringLiteral("insert.image")}));
    }
    void pinnedFilteredByPlatform()
    {
        actions::CommandRegistry registry;
        QString error;
        QVERIFY(registry.loadDeclarations(actions::defaultCommandsJson(), error));
        auto store = builtins();
        int filtered = 0;
        for (const QString& id : store.ids()) {
            const auto p = store.load(id, error);
            QVERIFY(p);
            for (const QString& command : p->pinned)
                if (!registry.isAvailable(command, actions::Platform::Android))
                    ++filtered;
        }
        QVERIFY(filtered >= 1); // au moins le clavier à l'écran ou la reconnaissance d'écriture
    }
    void namesAreOurs()
    {
        auto store = builtins();
        QString error;
        // Noms de l'original lus hors dépôt (jamais écrits ici) : une ligne par nom.
        const QString list = qEnvironmentVariable("ACTCRAFT_REFERENCE_NAMES");
        QFile file(list);
        if (list.isEmpty() || !file.open(QIODevice::ReadOnly | QIODevice::Text))
            QSKIP("ACTCRAFT_REFERENCE_NAMES absent : liste hors dépôt non fournie");
        QStringList theirs;
        for (const QString& line :
             QString::fromUtf8(file.readAll()).toLower().split(u'\n', Qt::SkipEmptyParts))
            theirs << line.trimmed();
        for (const QString& id : store.ids())
            QVERIFY2(!theirs.contains(store.load(id, error)->label.toLower()), qPrintable(id));
    }
    void preparationOpensThePageBrowser()
    {
        QString error;
        const auto p = builtins().load(QStringLiteral("preparation"), error);
        QVERIFY(p);
        QVERIFY(p->browsers.visible);
        QCOMPARE(p->browsers.current, QStringLiteral("pages"));
        QVERIFY(p->browsers.pinned);
        QCOMPARE(p->palette.swatches, 24);
        const auto c = builtins().load(QStringLiteral("en-classe"), error);
        QVERIFY(c);
        QVERIFY(c->fitPage);
        QVERIFY(!c->browsers.visible);
        QVERIFY(c->trashVisible);
    }
    void otherProfilesShareTheClassroomShape()
    {
        QString error;
        auto store = builtins();
        for (const QString& id : store.ids()) {
            if (id == QStringLiteral("preparation"))
                continue;
            const auto p = store.load(id, error);
            QVERIFY(p);
            QVERIFY2(p->fitPage, qPrintable(id));
            QVERIFY2(!p->browsers.visible, qPrintable(id));
            QVERIFY2(p->trashVisible, qPrintable(id));
            QVERIFY2(p->palette.swatches == 16, qPrintable(id));
            QVERIFY2(p->browsers.current == QStringLiteral("pages"), qPrintable(id));
        }
    }

    // Amendement 9 : un profil fourni dont seul <dossier>/layout.json a changé est « modifié », et « Revenir à
    // l'origine » retire ce fichier. Le patron écarté (`hasUserCopy` seul) ne regarde que <id>.json.
    void layoutAloneMarksTheBuiltinModified()
    {
        QTemporaryDir root;
        ui::Commands commands;
        ui::ProfileController controller(
            actions::ProfileStore(root.filePath(QStringLiteral("profiles")),
                                  QStringLiteral(":/actcraft/actions/profiles"),
                                  ui::ProfileController::builtinOrder()),
            nullptr, &commands);
        controller.start();
        const QString id = controller.currentId();
        QVERIFY(controller.store().isBuiltin(id));
        const auto modified = [&controller, &id]() {
            for (const QVariant& entry : controller.profiles())
                if (entry.toMap().value(QStringLiteral("id")).toString() == id)
                    return entry.toMap().value(QStringLiteral("modified")).toBool();
            return false;
        };
        QVERIFY(!modified());
        const QString layout = controller.store().directory(id) + QStringLiteral("/layout.json");
        QVERIFY(QDir().mkpath(controller.store().directory(id)));
        QFile file(layout);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.write("{}") > 0);
        file.close();
        QVERIFY(!controller.store().hasUserCopy(id));
        QVERIFY(modified());
        commands.trigger(QStringLiteral("profile.reset"));
        QCOMPARE(controller.currentId(), id);
        QVERIFY(!QFile::exists(layout));
        QVERIFY(!modified());
    }
};

QTEST_MAIN(TstUiProfiles)
#include "tst_ui_profiles.moc"
