#include "equationcontroller.h"

#include "presentationdocument.h"

#include <actcraft/render/equationpaint.h>

#include <QPainter>
#include <QPicture>
#include <QRect>
#include <QVariantMap>

#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>

namespace actcraft::ui {

namespace {

constexpr double kPointSize = 24; // nouvelles équations : la taille par défaut de core::Equation
constexpr double kMargin = 2;     // autour de l'encre mesurée, pixels de page
constexpr QSizeF kFallbackSize(200, 60);

} // namespace

EquationController::EquationController(QObject* parent) : QObject(parent)
{
    m_editorSize = naturalSize(m_editor.editorMathml(), kPointSize);
}

QString EquationController::mathml() const
{
    return QString::fromStdString(m_editor.mathml());
}

QString EquationController::editorMathml() const
{
    return QString::fromStdString(m_editor.editorMathml());
}

bool EquationController::hasPlaceholder() const
{
    return m_editor.hasPlaceholder();
}

QSizeF EquationController::editorSize() const
{
    return m_editorSize;
}

QString EquationController::message() const
{
    return m_message;
}

QVariantList EquationController::palettes()
{
    static const QVariantList list = [] {
        QVariantList out;
        const auto& templates = features::EquationEditor::templates();
        for (const features::EquationPalette& palette : features::EquationEditor::palettes()) {
            QVariantList entries;
            for (const QString& id : palette.entries) {
                const auto found =
                    std::find_if(templates.begin(), templates.end(),
                                 [&](const features::EquationTemplate& t) { return t.id == id; });
                if (found == templates.end()) {
                    continue;
                }
                entries.push_back(
                    QVariantMap{{QStringLiteral("id"), id},
                                {QStringLiteral("label"), found->label},
                                {QStringLiteral("mathml"),
                                 QString::fromStdString(features::EquationEditor::templateMathml(id))}});
            }
            out.push_back(QVariantMap{{QStringLiteral("id"), palette.id},
                                      {QStringLiteral("label"), palette.label},
                                      {QStringLiteral("entries"), entries}});
        }
        return out;
    }();
    return list;
}

DocumentController* EquationController::documentController() const noexcept
{
    return m_document.data();
}

void EquationController::setDocumentController(DocumentController* controller)
{
    if (m_document == controller) {
        return;
    }
    m_document = controller;
    emit documentControllerChanged();
}

// Le dessin de la page enregistré sans bornes (core::Equation::bounds vide : échelle 1, coin haut gauche du
// dessin en (0, 0)) ; l'encre enregistrée, marge comprise, donne la boîte.
QSizeF EquationController::naturalSize(const std::string& mathml, double pointSize)
{
    core::Equation probe;
    probe.mathml = mathml;
    probe.pointSize = pointSize;
    probe.bounds = core::RectF{0, 0, 0, 0};
    QPicture picture;
    QPainter painter(&picture);
    render::paintEquation(painter, probe, render::EquationPaintMode::Editor);
    painter.end();
    const QRect ink = picture.boundingRect();
    if (!ink.isValid() || ink.right() <= 0 || ink.bottom() <= 0) {
        return {};
    }
    return QSizeF(ink.right() + 1 + kMargin, ink.bottom() + 1 + kMargin);
}

void EquationController::open()
{
    m_editor.clear();
    edited();
}

bool EquationController::insertTemplate(const QString& id)
{
    if (!m_editor.insertTemplate(id)) {
        return false;
    }
    edited();
    return true;
}

void EquationController::type(const QString& text)
{
    m_editor.type(text);
    edited();
}

void EquationController::backspace()
{
    m_editor.backspace();
    edited();
}

void EquationController::moveLeft()
{
    m_editor.moveLeft();
    edited();
}

void EquationController::moveRight()
{
    m_editor.moveRight();
    edited();
}

void EquationController::moveUp()
{
    m_editor.moveUp();
    edited();
}

void EquationController::moveDown()
{
    m_editor.moveDown();
    edited();
}

bool EquationController::accept(QPointF pageCentre)
{
    // Ce qui entre dans le document : jamais editorMathml (marques de l'aperçu), toujours documentMathml.
    const std::optional<std::string> written = m_editor.documentMathml();
    if (!written) {
        setMessage(tr("Une case est encore vide : remplissez-la ou effacez-la avant d'insérer l'équation."));
        return false;
    }
    if (m_document.isNull()) {
        return false;
    }
    DocumentInsertions insertions(*m_document);
    const core::Page* page = insertions.shownPage();
    if (!page) {
        return false;
    }
    QPointF centre = pageCentre;
    if (!std::isfinite(centre.x()) || !std::isfinite(centre.y())) {
        centre = QPointF(page->width / 2, page->height / 2);
    }
    QSizeF size = naturalSize(*written, kPointSize);
    if (size.isEmpty()) {
        size = kFallbackSize;
    }
    core::Equation equation;
    equation.bounds = core::RectF{centre.x() - size.width() / 2, centre.y() - size.height() / 2, size.width(),
                                  size.height()};
    equation.mathml = *written;
    equation.pointSize = kPointSize;
    core::PageObject object = std::move(equation);
    object.common.layer = core::Layer::Middle;
    if (insertions.insertObject(std::move(object)) == 0) {
        return false;
    }
    setMessage({});
    return true;
}

void EquationController::edited()
{
    m_editorSize = naturalSize(m_editor.editorMathml(), kPointSize);
    setMessage({});
    emit equationChanged();
}

void EquationController::setMessage(const QString& message)
{
    if (message == m_message) {
        return;
    }
    m_message = message;
    emit messageChanged();
}

} // namespace actcraft::ui
