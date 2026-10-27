#include "equationcontroller.h"

#include "presentationdocument.h"

#include <actcraft/core/equation.h>
#include <actcraft/render/equationpaint.h>

#include <QPainter>
#include <QPicture>
#include <QRect>
#include <QString>
#include <QVariantMap>

#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>

namespace actcraft::ui {

namespace {

constexpr double kEquationPoints = 24; // nouvelles équations : la taille par défaut de core::Equation
constexpr double kMeasureZoom = 100;   // QPicture borne son dessin au pixel entier : 0,01 px de page
constexpr QSizeF kFallbackSize(200, 60);

// Ajoute actcraft-cursor aux classes de la racine : en mode Editor, son surlignage couvre la boîte entière de
// la mise en page.
void markWhole(core::EquationNode& root)
{
    for (auto& [name, value] : root.attributes) {
        if (name == "class") {
            value += value.empty() ? "actcraft-cursor" : " actcraft-cursor";
            return;
        }
    }
    root.attributes.emplace_back("class", "actcraft-cursor");
}

} // namespace

EquationController::EquationController(QObject* parent) : QObject(parent)
{
    m_editorSize = naturalSize(m_editor.editorMathml(), kEquationPoints);
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

// Cadre du dessin de la page : la boîte de la mise en page et l'encre (render/src/equationlayout.cpp,
// layoutDrawing), que paintEquation met à l'échelle dans les bornes de l'équation. Des bornes de la taille du
// cadre la font dessiner à l'échelle 1 ; l'encre seule ne suffit pas (un chiffre a une boîte plus haute que
// son encre, un jambage déborde de la sienne). L'API publique ne donne pas le cadre : la racine marquée
// actcraft-cursor fait peindre sa boîte en mode Editor (surlignage du curseur), et boîte et encre,
// enregistrées sans bornes (échelle 1, coin haut gauche du cadre en (0, 0)), donnent le cadre.
QSizeF EquationController::naturalSize(const std::string& mathml, double pointSize)
{
    QString error;
    std::optional<core::EquationNode> root = core::parseMathml(mathml, error);
    if (!root) {
        return {};
    }
    markWhole(*root);
    core::Equation probe;
    probe.mathml = core::toMathml(*root);
    probe.pointSize = pointSize;
    probe.bounds = core::RectF{0, 0, 0, 0};
    QPicture picture;
    QPainter painter(&picture);
    painter.scale(kMeasureZoom, kMeasureZoom);
    render::paintEquation(painter, probe, render::EquationPaintMode::Editor);
    painter.end();
    const QRect frame = picture.boundingRect();
    if (!frame.isValid() || frame.right() <= 0 || frame.bottom() <= 0) {
        return {};
    }
    return QSizeF((frame.right() + 1) / kMeasureZoom, (frame.bottom() + 1) / kMeasureZoom);
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
        setMessage(tr("Aucune page n'est montrée : l'équation n'a pas été posée."));
        return false;
    }
    QPointF centre = pageCentre;
    if (!std::isfinite(centre.x()) || !std::isfinite(centre.y())) {
        centre = QPointF(page->width / 2, page->height / 2);
    }
    QSizeF size = naturalSize(*written, kEquationPoints);
    if (size.isEmpty()) {
        size = kFallbackSize;
    }
    core::Equation equation;
    equation.bounds = core::RectF{centre.x() - size.width() / 2, centre.y() - size.height() / 2, size.width(),
                                  size.height()};
    equation.mathml = *written;
    equation.pointSize = kEquationPoints;
    core::PageObject object = std::move(equation);
    object.common.layer = core::Layer::Middle;
    if (insertions.insertObject(std::move(object)) == 0) {
        setMessage(tr("L'équation n'a pas pu être posée sur la page."));
        return false;
    }
    setMessage({});
    return true;
}

void EquationController::edited()
{
    m_editorSize = naturalSize(m_editor.editorMathml(), kEquationPoints);
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
