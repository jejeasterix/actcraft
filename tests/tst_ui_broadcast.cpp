// Diffusion protégée II (L8-T32, S18) : la porte du registre refuse les commandes des catégories interdites par
// le document ouvert, comme le verrou de classe (D6-7), sans jamais écrire l'état propre des commandes.
#include <actcraft/actions/commandregistry.h>
#include <actcraft/core/model.h>
#include <actcraft/ui/broadcastgate.h>

#include <QtTest>

using namespace actcraft;

namespace {

// Déclarations réelles (actions/data/commands.json) : les drapeaux lockable et restrictable sont ceux livrés.
void loadDefaults(actions::CommandRegistry& registry)
{
    QString error;
    QVERIFY2(registry.loadDeclarations(actions::defaultCommandsJson(), error), qPrintable(error));
}

core::BroadcastRules copyAndPrint()
{
    core::BroadcastRules rules;
    rules.noCopy = true;
    rules.noPrint = true;
    return rules;
}

} // namespace

class TstUiBroadcast : public QObject
{
    Q_OBJECT
private slots:
    void restrictedCommandsGreyed();
    void enableCannotBypassTheRestriction();
};

// Copie et impression interdites : edit.copy et file.print grisées ; page.copy et page.cut aussi (H8-8), bien
// qu'elles ne soient pas déclarées restreignables ; la navigation reste ouverte.
void TstUiBroadcast::restrictedCommandsGreyed()
{
    actions::CommandRegistry registry;
    loadDefaults(registry);
    QVERIFY(registry.find(QStringLiteral("page.copy")) != nullptr);
    QVERIFY2(!registry.find(QStringLiteral("page.copy"))->restrictable,
             "H8-8 : page.copy n'est pas restreignable ; la diffusion la refuse quand même");
    QVERIFY(!registry.find(QStringLiteral("page.cut"))->restrictable);

    registry.setRestricted(ui::BroadcastGate::restrictedIds(registry, copyAndPrint()));

    for (const char* id : {"edit.copy", "edit.cut", "file.print", "page.copy", "page.cut"}) {
        QVERIFY2(registry.isRestricted(QString::fromLatin1(id)), id);
        QVERIFY2(!registry.isEnabled(QString::fromLatin1(id)), id);
        QVERIFY2(registry.isOwnEnabled(QString::fromLatin1(id)), id); // l'état propre n'est pas touché
    }
    for (const char* id : {"page.next", "file.save", "edit.paste"}) {
        QVERIFY2(!registry.isRestricted(QString::fromLatin1(id)), id);
        QVERIFY2(registry.isEnabled(QString::fromLatin1(id)), id);
    }
}

// Un contrôleur (sélection, mode conception) qui rouvre edit.copy par setEnabled ne lève pas la restriction ; à
// la levée des règles, l'état propre posé par ce contrôleur revient tel quel, ni forcé à vrai ni à faux.
void TstUiBroadcast::enableCannotBypassTheRestriction()
{
    actions::CommandRegistry registry;
    loadDefaults(registry);
    const QString copy = QStringLiteral("edit.copy");
    int calls = 0;
    registry.setHandler(copy, [&calls](const QVariant&) { ++calls; });

    registry.setEnabled(copy, false); // rien de sélectionné
    registry.setRestricted(ui::BroadcastGate::restrictedIds(registry, copyAndPrint()));
    registry.setEnabled(copy, true); // la sélection change : le contrôleur rouvre la commande
    QVERIFY(!registry.isEnabled(copy));
    QVERIFY(registry.isOwnEnabled(copy));
    QVERIFY(!registry.trigger(copy, {}, actions::currentPlatform()));
    QCOMPARE(calls, 0);

    registry.setEnabled(copy, false); // sélection vidée pendant la diffusion
    registry.setRestricted({});
    QVERIFY(!registry.isRestricted(copy));
    QVERIFY2(!registry.isEnabled(copy), "la levée rend l'état propre du contrôleur, pas un « tout ouvert »");
    registry.setEnabled(copy, true);
    QVERIFY(registry.isEnabled(copy));
    QVERIFY(registry.trigger(copy, {}, actions::currentPlatform()));
    QCOMPARE(calls, 1);
}

QTEST_MAIN(TstUiBroadcast)
#include "tst_ui_broadcast.moc"
