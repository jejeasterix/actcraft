#include <actcraft/tools/selectiongeometry.h>

// Talon L0-S : rempli par L3-T8.

namespace actcraft::tools {

SelectionView layoutSelection(const core::Page&, const std::vector<core::ObjectId>&, double,
                              const QStringList&)
{
    return {};
}

SelectionHit hitSelection(const SelectionView&, core::PointF, double)
{
    return {};
}

} // namespace actcraft::tools
