#include "equationpreviewitem.h"

#include <actcraft/core/model.h>
#include <actcraft/render/equationpaint.h>

#include <QPainter>

namespace actcraft::ui {

EquationPreview::EquationPreview(QQuickItem* parent) : QQuickPaintedItem(parent)
{
    setAntialiasing(true);
}

QString EquationPreview::mathml() const
{
    return m_mathml;
}

void EquationPreview::setMathml(const QString& mathml)
{
    if (mathml == m_mathml) {
        return;
    }
    m_mathml = mathml;
    m_utf8 = mathml.toStdString();
    update();
    emit mathmlChanged();
}

void EquationPreview::paint(QPainter* painter)
{
    if (!painter || !(width() > 0 && height() > 0)) {
        return;
    }
    core::Equation equation;
    equation.bounds = core::RectF{0, 0, width(), height()};
    equation.mathml = m_utf8;
    render::paintEquation(*painter, equation, render::EquationPaintMode::Editor);
}

} // namespace actcraft::ui
