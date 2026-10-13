#pragma once

#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>

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

} // namespace actcraft::ui
