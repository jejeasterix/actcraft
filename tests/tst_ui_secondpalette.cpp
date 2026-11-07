// L11-T9 : mode à deux utilisateurs (F-110, F-303, N2-A7) — attribut opensDialog, commandes à boîte refusées
// au second utilisateur par Commands::trigger (et par lui seul), bascule users.dual, deuxième palette.
#include <actcraft/actions/commandregistry.h>
#include <actcraft/ui/commands.h>

#include <QJSEngine>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

using namespace actcraft;

namespace {

// Gestionnaire JavaScript qui compte ses appels et retient (argument, user), comme ceux de Main.qml.
struct HandlerProbe
{
    QJSEngine engine;
    QJSValue handler;

    HandlerProbe()
    {
        engine.globalObject().setProperty(QStringLiteral("calls"), 0);
        handler = engine.evaluate(QStringLiteral(
            "(function (argument, user) { calls = calls + 1; lastArgument = argument; lastUser = user; })"));
    }
    int calls() { return engine.globalObject().property(QStringLiteral("calls")).toInt(); }
    QJSValue lastUser() { return engine.globalObject().property(QStringLiteral("lastUser")); }
    QJSValue lastArgument() { return engine.globalObject().property(QStringLiteral("lastArgument")); }
};

} // namespace

class TstUiSecondpalette : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void opensDialogParsed();
    void everyDialogCommandMarked();
    void secondUserCannotOpenADialog();
    void firstUserStillCan();
    void toggleSetsTheTwoUserPolicy();
    void offRestoresTheTouchMode();
    void pinchZoomsOnlyWhenOff();
    void secondUserReceivesInDualMode();

private:
    QTemporaryDir m_settings;
    QTemporaryDir m_profile;
};

void TstUiSecondpalette::initTestCase()
{
    QVERIFY(m_settings.isValid());
    QVERIFY(m_profile.isValid());
    QStandardPaths::setTestModeEnabled(true);
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings.path());
    QSettings probe;
    QCOMPARE(probe.format(), QSettings::IniFormat);
    QVERIFY(probe.fileName().startsWith(m_settings.path()));
}

void TstUiSecondpalette::opensDialogParsed()
{
    ui::Commands commands;
    commands.setProfileDir(m_profile.path());
    QVERIFY2(commands.errors().isEmpty(), qPrintable(commands.errors().join(u'\n')));
    const actions::CommandInfo* open = commands.registry().find(QStringLiteral("file.open"));
    QVERIFY(open);
    QVERIFY(open->opensDialog);
    const actions::CommandInfo* color = commands.registry().find(QStringLiteral("pen.color"));
    QVERIFY(color);
    QVERIFY(!color->opensDialog); // clé absente : faux
}

void TstUiSecondpalette::everyDialogCommandMarked()
{
    // Liste relevée dans les gestionnaires QML : Dialog, Popup modal, sélecteur de fichier. Exacte dans les deux
    // sens : une commande marquée hors de la liste rougit aussi.
    const QStringList dialogs = {
        QStringLiteral("file.open"),
        QStringLiteral("help.check-updates"), // boîte en L11-T22
    };
    ui::Commands commands;
    commands.setProfileDir(m_profile.path());
    for (const QString& id : dialogs) {
        const actions::CommandInfo* info = commands.registry().find(id);
        QVERIFY2(info, qPrintable(id));
        QVERIFY2(info->opensDialog, qPrintable(id));
    }
    QStringList extra;
    for (const actions::CommandInfo& info : commands.registry().commands()) {
        if (info.opensDialog && !dialogs.contains(info.id))
            extra << info.id;
    }
    QVERIFY2(extra.isEmpty(), qPrintable(extra.join(u' ')));
}

void TstUiSecondpalette::secondUserCannotOpenADialog()
{
    ui::Commands commands;
    commands.setProfileDir(m_profile.path());
    HandlerProbe open;
    commands.setHandler(QStringLiteral("file.open"), open.handler);
    QSignalSpy refused(&commands, &ui::Commands::refusedForSecondUser);
    QSignalSpy triggered(&commands, &ui::Commands::triggered);

    // Le refus est dans trigger : le gestionnaire n'est jamais appelé, quelle que soit la voie.
    QVERIFY(!commands.trigger(QStringLiteral("file.open"), QVariantMap{}, 1));
    QCOMPARE(open.calls(), 0);
    QCOMPARE(refused.count(), 1);
    QCOMPARE(refused.at(0).at(0).toString(), QStringLiteral("file.open"));
    QCOMPARE(triggered.count(), 0);

    // Une commande sans boîte passe, et l'utilisateur atteint le gestionnaire (argument, user).
    HandlerProbe color;
    commands.setHandler(QStringLiteral("pen.color"), color.handler);
    QVERIFY(commands.trigger(QStringLiteral("pen.color"), QStringLiteral("#1E88E5"), 1));
    QCOMPARE(color.calls(), 1);
    QCOMPARE(color.lastArgument().toString(), QStringLiteral("#1E88E5"));
    QCOMPARE(color.lastUser().toInt(), 1);
    QCOMPARE(refused.count(), 1);
}

void TstUiSecondpalette::firstUserStillCan()
{
    ui::Commands commands;
    commands.setProfileDir(m_profile.path());
    HandlerProbe open;
    commands.setHandler(QStringLiteral("file.open"), open.handler);
    QSignalSpy refused(&commands, &ui::Commands::refusedForSecondUser);

    QVERIFY(commands.trigger(QStringLiteral("file.open"), QVariantMap{}, 0));
    QCOMPARE(open.calls(), 1);
    QCOMPARE(open.lastUser().toInt(), 0);
    // L'ancienne signature vaut user = 0.
    QVERIFY(commands.trigger(QStringLiteral("file.open")));
    QCOMPARE(open.calls(), 2);
    QVERIFY(open.lastUser().isNumber());
    QCOMPARE(open.lastUser().toInt(), 0);
    QCOMPARE(refused.count(), 0);
}

void TstUiSecondpalette::toggleSetsTheTwoUserPolicy()
{
    QFAIL("L11-T9 : à écrire (politique posée sur la vue par DualMode).");
}

void TstUiSecondpalette::offRestoresTheTouchMode()
{
    QFAIL("L11-T9 : à écrire (politique du mode du toucher rendue à l'extinction).");
}

void TstUiSecondpalette::pinchZoomsOnlyWhenOff()
{
    QFAIL("L11-T9 : à écrire (pincement suspendu en mode allumé).");
}

void TstUiSecondpalette::secondUserReceivesInDualMode()
{
    QFAIL("L11-T9 : à écrire (Main.qml : outil, couleur et stylet du second utilisateur).");
}

QTEST_MAIN(TstUiSecondpalette)
#include "tst_ui_secondpalette.moc"
