#pragma once

#include <QRandomGenerator>
#include <QString>

#include <string>
#include <vector>

namespace actcraft::features {

struct TransitionEffect
{
    QString id;    // id anglais à nous
    QString label; // label français à nous
};
// « none » d'abord, « random » ensuite, puis les seize mouvements de L7-T0.
const std::vector<TransitionEffect>& transitionCatalogue();

struct TransitionSettings
{
    QString global = QStringLiteral("none");
    int durationTenths = 15;
};
enum class NavigationKind
{
    Next,
    Previous,
    Jump
};
struct TransitionBlockers
{
    bool curtainOrSpotlight = false;
    bool desktopAnnotation = false;
    bool designMode = false;
};
QString effectForLeaving(const TransitionSettings& settings, const std::string& leavingPageEffect,
                         NavigationKind navigation, TransitionBlockers blockers);
int clampDurationTenths(int tenths) noexcept;

// « random » : un effet du catalogue tiré à chaque changement, hors « none » et « random » ; tout autre
// identifiant est rendu tel quel. Appelé par le lecteur (phase 2) sur le résultat d'effectForLeaving.
QString pickEffect(const QString& id, QRandomGenerator& generator);

} // namespace actcraft::features
