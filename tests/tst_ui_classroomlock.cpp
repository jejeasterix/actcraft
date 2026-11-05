#include <actcraft/ui/commands.h>

#include <QtTest>

using actcraft::ui::Commands;

class TstUiClassroomlock : public QObject
{
    Q_OBJECT
private slots:
    void disabledDuringLockStaysDisabled();
};

// Amendement 1 : Commands::setEnabled compare à l'état propre (isOwnEnabled), pas à isEnabled, qui voit à
// travers le verrou. Sous l'ancienne comparaison, désactiver une commande verrouillable pendant le verrou
// n'écrit rien (isEnabled est déjà faux) et elle revient active au déverrouillage.
void TstUiClassroomlock::disabledDuringLockStaysDisabled()
{
    Commands commands;
    QVERIFY(commands.registry().find(QStringLiteral("edit.undo")));
    QVERIFY(commands.registry().find(QStringLiteral("edit.undo"))->lockable);
    QVERIFY(commands.isEnabled(QStringLiteral("edit.undo")));

    commands.setLocked(true);
    QVERIFY(!commands.isEnabled(QStringLiteral("edit.undo")));
    commands.setEnabled(QStringLiteral("edit.undo"), false); // historique vidé pendant le verrou
    commands.setLocked(false);

    QVERIFY(!commands.isEnabled(QStringLiteral("edit.undo")));
    QVERIFY(!commands.registry().isOwnEnabled(QStringLiteral("edit.undo")));
}

QTEST_MAIN(TstUiClassroomlock)
#include "tst_ui_classroomlock.moc"
