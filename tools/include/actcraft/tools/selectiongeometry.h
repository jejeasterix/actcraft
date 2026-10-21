#pragma once

#include <actcraft/core/model.h>
#include <actcraft/tools/toolcontext.h>

#include <QString>
#include <QStringList>

#include <vector>

namespace actcraft::tools {

inline constexpr double kHandleRadiusScreen = 6.0; // poignée ronde (F-520), pixels d'écran
inline constexpr double kBarButtonScreen = 24.0;   // bouton carré de la barre (F-521)
inline constexpr double kBarGapScreen = 8.0;       // écart cadre → barre
inline constexpr double kEnlargeStep = 1.1;        // F-523

// Cadre (emprise réunie ; un seul objet : son cadre transformé), 8 poignées (ordre de HandleKind), barre
// au-dessus du cadre, replacée sous lui si elle sortirait du haut de la page (F-521). Coordonnées de page.
SelectionView layoutSelection(const core::Page& page, const std::vector<core::ObjectId>& ids, double zoom,
                              const QStringList& barCommands);

enum class SelectionHitKind : std::uint8_t
{
    None,
    Inside,
    Handle,
    BarButton,
};
struct SelectionHit
{
    SelectionHitKind kind = SelectionHitKind::None;
    HandleKind handle = HandleKind::TopLeft;
    QString command;
};
// Ordre d'essai : bouton de barre, poignée, intérieur du cadre.
SelectionHit hitSelection(const SelectionView& view, core::PointF point, double zoom);

} // namespace actcraft::tools
