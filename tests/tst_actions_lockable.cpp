#include <actcraft/actions/commandregistry.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSet>
#include <QtTest>

using namespace actcraft::actions;

// Porte du verrou de classe (D6-7, L6-T3) : le verrou est un état à part, combiné dans isEnabled() et
// trigger(). Il n'écrit jamais l'état propre posé par setEnabled() : un contrôleur qui réactive une commande
// pendant le verrou ne la libère pas, et le déverrouillage rend l'état propre tel qu'il a été posé.
class TstActionsLockable : public QObject
{
    Q_OBJECT

    static QByteArray two()
    {
        return R"({"format":"actcraft-commands/1","commands":[
            {"id":"a.b","label":"A","lockable":true,"restrictable":true},
            {"id":"c.d","label":"C","lockable":false,"restrictable":false}]})";
    }

    static QSet<QString> idsWhere(const QString& attribute, bool value)
    {
        QSet<QString> out;
        const QJsonArray all =
            QJsonDocument::fromJson(defaultCommandsJson()).object().value(QStringLiteral("commands")).toArray();
        for (const QJsonValue& v : all) {
            const QJsonObject o = v.toObject();
            if (o.value(attribute).toBool(!value) == value) {
                out.insert(o.value(QStringLiteral("id")).toString());
            }
        }
        return out;
    }

private slots:
    void attributesRead()
    {
        CommandRegistry r;
        QString error;
        QVERIFY2(r.loadDeclarations(two(), error), qPrintable(error));
        QVERIFY(r.find(QStringLiteral("a.b")));
        QVERIFY(r.find(QStringLiteral("c.d")));
        QVERIFY(r.find(QStringLiteral("a.b"))->lockable);
        QVERIFY(r.find(QStringLiteral("a.b"))->restrictable);
        QVERIFY(!r.find(QStringLiteral("c.d"))->lockable);
        QVERIFY(!r.find(QStringLiteral("c.d"))->restrictable);
    }

    // Défaut sûr quand l'attribut manque : verrouillable, non restreignable. Une valeur non booléenne est
    // une erreur qui nomme la commande, et rien n'est déclaré.
    void attributesValidated()
    {
        CommandRegistry r;
        QString error;
        QVERIFY2(r.loadDeclarations(R"({"format":"actcraft-commands/1","commands":[{"id":"e.f","label":"E"}]})",
                                    error),
                 qPrintable(error));
        QVERIFY(r.find(QStringLiteral("e.f"))->lockable);
        QVERIFY(!r.find(QStringLiteral("e.f"))->restrictable);

        CommandRegistry bad;
        QVERIFY(!bad.loadDeclarations(R"({"format":"actcraft-commands/1","commands":[
            {"id":"a.b","label":"A"},{"id":"g.h","label":"G","lockable":"yes"}]})",
                                      error));
        QCOMPARE(error, QStringLiteral("commande 2 (g.h) : lockable n'est pas un booléen"));
        QVERIFY(bad.ids().isEmpty());

        QVERIFY(!bad.loadDeclarations(R"({"format":"actcraft-commands/1","commands":[
            {"id":"i.j","label":"I","restrictable":1}]})",
                                      error));
        QCOMPARE(error, QStringLiteral("commande 1 (i.j) : restrictable n'est pas un booléen"));
        QVERIFY(bad.ids().isEmpty());
    }

    void lockGreysOnlyLockable()
    {
        CommandRegistry r;
        QString error;
        QVERIFY(r.loadDeclarations(two(), error));
        QVERIFY(!r.isLocked());
        r.setLocked(true);
        QVERIFY(r.isLocked());
        QVERIFY(!r.isEnabled(QStringLiteral("a.b")));
        QVERIFY(r.isEnabled(QStringLiteral("c.d")));
        r.setLocked(false);
        QVERIFY(!r.isLocked());
        QVERIFY(r.isEnabled(QStringLiteral("a.b")));
        QVERIFY(r.isEnabled(QStringLiteral("c.d")));
    }

    // trigger() passe par la même porte qu'isEnabled() : une commande verrouillée n'appelle rien, même
    // réactivée pendant le verrou ; une commande libre se déclenche.
    void lockedTriggerCallsNothing()
    {
        CommandRegistry r;
        QString error;
        QVERIFY(r.loadDeclarations(two(), error));
        int lockedCalls = 0;
        int freeCalls = 0;
        r.setHandler(QStringLiteral("a.b"), [&](const QVariant&) { ++lockedCalls; });
        r.setHandler(QStringLiteral("c.d"), [&](const QVariant&) { ++freeCalls; });
        r.setLocked(true);
        QVERIFY(!r.trigger(QStringLiteral("a.b"), QVariant(), currentPlatform()));
        r.setEnabled(QStringLiteral("a.b"), true);
        QVERIFY(!r.trigger(QStringLiteral("a.b"), QVariant(), currentPlatform()));
        QCOMPARE(lockedCalls, 0);
        QVERIFY(r.trigger(QStringLiteral("c.d"), QVariant(), currentPlatform()));
        QCOMPARE(freeCalls, 1);
        r.setLocked(false);
        QVERIFY(r.trigger(QStringLiteral("a.b"), QVariant(), currentPlatform()));
        QCOMPARE(lockedCalls, 1);
    }

    // Le patron écarté (griser par setEnabled(false)) serait défait ici : le premier contrôleur qui
    // réactive une commande la libérerait.
    void enableCannotBypassTheLock()
    {
        CommandRegistry r;
        QString error;
        QVERIFY(r.loadDeclarations(two(), error));
        r.setLocked(true);
        r.setEnabled(QStringLiteral("a.b"), true);
        QVERIFY(!r.isEnabled(QStringLiteral("a.b")));
        r.setEnabled(QStringLiteral("a.b"), false);
        r.setLocked(false);
        QVERIFY(!r.isEnabled(QStringLiteral("a.b"))); // l'état propre reste celui posé
    }

    void everyCommandDeclaresBoth()
    {
        const QJsonArray all =
            QJsonDocument::fromJson(defaultCommandsJson()).object().value(QStringLiteral("commands")).toArray();
        QVERIFY(!all.isEmpty());
        for (const QJsonValue& v : all) {
            const QJsonObject o = v.toObject();
            const QString id = o.value(QStringLiteral("id")).toString();
            QVERIFY2(o.value(QStringLiteral("lockable")).isBool(), qPrintable(id));
            QVERIFY2(o.value(QStringLiteral("restrictable")).isBool(), qPrintable(id));
        }
    }

    // Amendement 4 (D3) : exactement ces commandes restent libres sous le verrou ; toutes les autres sont
    // verrouillables. Restreignables : copie, enregistrement, impression, export et caméra (tool.camera-*).
    void fileAndQuitStayFree()
    {
        CommandRegistry r;
        QString error;
        QVERIFY2(r.loadDeclarations(defaultCommandsJson(), error), qPrintable(error));
        for (const char* id : {"file.quit", "classroom.lock", "help.contents", "page.next", "page.previous",
                               "view.fullscreen", "view.fit", "view.zoom"}) {
            const CommandInfo* info = r.find(QString::fromLatin1(id));
            QVERIFY2(info, id);
            QVERIFY2(!info->lockable, id);
        }
        for (const char* id : {"file.open", "file.save", "profile.edit", "palette.float", "mode.design"}) {
            const CommandInfo* info = r.find(QString::fromLatin1(id));
            QVERIFY2(info, id);
            QVERIFY2(info->lockable, id);
        }

        const QSet<QString> free{QStringLiteral("file.quit"),       QStringLiteral("classroom.lock"),
                                 QStringLiteral("classroom.password"), QStringLiteral("help.contents"),
                                 QStringLiteral("help.website"),    QStringLiteral("help.about"),
                                 QStringLiteral("page.next"),       QStringLiteral("page.previous"),
                                 QStringLiteral("view.fullscreen"), QStringLiteral("view.fit"),
                                 QStringLiteral("view.zoom")};
        QCOMPARE(idsWhere(QStringLiteral("lockable"), false), free);

        const QSet<QString> restrictable{
            QStringLiteral("edit.copy"),          QStringLiteral("edit.cut"),
            QStringLiteral("edit.duplicate"),     QStringLiteral("file.save"),
            QStringLiteral("file.save-as"),       QStringLiteral("file.print"),
            QStringLiteral("export.page-image"),  QStringLiteral("export.pdf"),
            QStringLiteral("tool.camera-area"),   QStringLiteral("tool.camera-polygon"),
            QStringLiteral("tool.camera-freehand"), QStringLiteral("tool.camera-window"),
            QStringLiteral("tool.camera-screen")};
        QCOMPARE(idsWhere(QStringLiteral("restrictable"), true), restrictable);

        // Le verrou posé sur les données réelles : quitter reste possible, enregistrer non.
        r.setLocked(true);
        QVERIFY(r.isEnabled(QStringLiteral("file.quit")));
        QVERIFY(!r.isEnabled(QStringLiteral("file.save")));
    }

    // Amendement 1, § 1 : l'état propre (isOwnEnabled) est gardé à part du verrou, dans cet ordre.
    void ownStateSurvivesTheLock()
    {
        CommandRegistry r;
        QString error;
        QVERIFY(r.loadDeclarations(two(), error));
        const QString ab = QStringLiteral("a.b");
        QVERIFY(!r.isOwnEnabled(QStringLiteral("x.y"))); // inconnue

        // (1) verrou posé, commande désactivée par son contrôleur
        r.setLocked(true);
        r.setEnabled(ab, false);
        QVERIFY(!r.isOwnEnabled(ab));
        // (2) verrou levé : elle reste désactivée
        r.setLocked(false);
        QVERIFY(!r.isEnabled(ab));
        // (3) réactivée par son contrôleur
        r.setEnabled(ab, true);
        QVERIFY(r.isOwnEnabled(ab));
        QVERIFY(r.isEnabled(ab));
        // (4) verrou reposé : grisée, mais son état propre reste vrai
        r.setLocked(true);
        QVERIFY(!r.isEnabled(ab));
        QVERIFY(r.isOwnEnabled(ab));
    }
};

QTEST_MAIN(TstActionsLockable)
#include "tst_actions_lockable.moc"
