#include <actcraft/features/transitions.h>

#include <algorithm>

namespace actcraft::features {

const std::vector<TransitionEffect>& transitionCatalogue()
{
    // Liste de L7-T0 (docs/verifications/2026-10-18-lot7-principes.md, N2-A6) : 16 mouvements retenus (17
    // relevés, l'effet à nom de marque écarté), plus « aucun » et « au hasard » ; identifiants et noms à
    // nous. « none » toujours en premier, « random » ensuite : 18 entrées.
    static const std::vector<TransitionEffect> catalogue{
        {QStringLiteral("none"), QStringLiteral("Sans effet")},
        {QStringLiteral("random"), QStringLiteral("Au hasard")},
        {QStringLiteral("dissolve"), QStringLiteral("Fondu")},
        {QStringLiteral("corner-peel"), QStringLiteral("Coin soulevé")},
        {QStringLiteral("box-turn"), QStringLiteral("Boîte qui tourne")},
        {QStringLiteral("card-flip"), QStringLiteral("Carte retournée")},
        {QStringLiteral("tiles-spin"), QStringLiteral("Carreaux")},
        {QStringLiteral("fold-half"), QStringLiteral("Pliage")},
        {QStringLiteral("slide-right"), QStringLiteral("Glissement")},
        {QStringLiteral("strips-drop"), QStringLiteral("Bandes")},
        {QStringLiteral("corner-swing"), QStringLiteral("Pivot d'angle")},
        {QStringLiteral("spiral-out"), QStringLiteral("Tourbillon")},
        {QStringLiteral("turn-reveal"), QStringLiteral("Dévoilement tournant")},
        {QStringLiteral("drapes"), QStringLiteral("Tentures")},
        {QStringLiteral("star-iris"), QStringLiteral("Étoile")},
        {QStringLiteral("shards"), QStringLiteral("Éclats")},
        {QStringLiteral("sweep"), QStringLiteral("Balayage")},
        {QStringLiteral("funnel"), QStringLiteral("Entonnoir")},
    };
    return catalogue;
}

QString effectForLeaving(const TransitionSettings& settings, const std::string& leavingPageEffect,
                         NavigationKind navigation, TransitionBlockers blockers)
{
    const QString none = QStringLiteral("none");
    if (navigation == NavigationKind::Jump || blockers.curtainOrSpotlight || blockers.desktopAnnotation ||
        blockers.designMode)
        return none;
    // L'effet de la page QUITTÉE prime sur l'effet global (F-640, confirmé par L7-T0).
    const QString id =
        leavingPageEffect.empty() ? settings.global : QString::fromStdString(leavingPageEffect);
    const auto& list = transitionCatalogue();
    const bool known =
        std::any_of(list.begin(), list.end(), [&id](const TransitionEffect& e) { return e.id == id; });
    return known ? id : none;
}

QString pickEffect(const QString& id, QRandomGenerator& generator)
{
    if (id != QStringLiteral("random"))
        return id;
    const auto& list = transitionCatalogue(); // [0] none, [1] random, puis les 16 mouvements
    const auto index = 2 + static_cast<std::size_t>(generator.bounded(static_cast<quint32>(list.size() - 2)));
    return list[index].id;
}

int clampDurationTenths(int tenths) noexcept
{
    return std::clamp(tenths, 5, 30); // 0,5 à 3 s (F-640, confirmé par L7-T0)
}

} // namespace actcraft::features
