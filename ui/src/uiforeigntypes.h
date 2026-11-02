#pragma once

#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/gridoverlayitem.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/render/liftlayeritem.h>
#include <actcraft/render/pendinglayeritem.h>

#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// render/ n'est pas un module QML : ses éléments y sont exposés ici.
struct FixedPageForeign
{
    Q_GADGET
    QML_FOREIGN(actcraft::render::FixedPageItem)
    QML_NAMED_ELEMENT(FixedPage)
};

struct InkLayerForeign
{
    Q_GADGET
    QML_FOREIGN(actcraft::render::InkItem)
    QML_NAMED_ELEMENT(InkLayer)
};

// Lot 10 : couche d'attente, au-dessus du cache (PageArea.qml).
struct PendingLayerForeign
{
    Q_GADGET
    QML_FOREIGN(actcraft::render::PendingLayerItem)
    QML_NAMED_ELEMENT(PendingLayer)
};

// Lot 3 : couche des objets soulevés, au-dessus de la page fixe (socle, L0-S6).
struct LiftLayerForeign
{
    Q_GADGET
    QML_FOREIGN(actcraft::render::LiftLayerItem)
    QML_NAMED_ELEMENT(LiftLayer)
};

// Lot 4 : grille dessinée au-dessus des objets (render/ n'est pas un module QML).
struct GridOverlayForeign
{
    Q_GADGET
    QML_FOREIGN(actcraft::render::GridOverlayItem)
    QML_NAMED_ELEMENT(GridOverlay)
};

} // namespace actcraft::ui
