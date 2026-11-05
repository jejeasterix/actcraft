#include "equationcontroller.h"

#include "presentationdocument.h"

#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/equation.h>
#include <actcraft/core/hittest.h>
#include <actcraft/core/undo.h>
#include <actcraft/render/equationpaint.h>

#include <QPainter>
#include <QPicture>
#include <QRect>
#include <QString>
#include <QVariantMap>

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <utility>

namespace actcraft::ui {

namespace {

constexpr double kEquationPoints = 24; // nouvelles équations : la taille par défaut de core::Equation
constexpr double kMeasureZoom = 100;   // QPicture borne son dessin au pixel entier : 0,01 px de page
constexpr QSizeF kFallbackSize(200, 60);

// Remplace un objet de la page sous son identifiant, au même rang (core::ReplaceObjectsCommand tire des
// identifiants neufs et ignore celui de l'ajout : elle ne convient pas à la réédition). Changes : un
// ObjectChanged, zone = emprise avant ∪ emprise après. Objet absent : rien ne change.
class EquationRewrite final : public core::UndoCommand
{
public:
    EquationRewrite(std::size_t page, core::PageObject after) : m_page(page), m_after(std::move(after)) {}
    void redo(core::Document& document, core::Changes& changes) override
    {
        core::Page& page = document.page(m_page);
        if (!m_applied) {
            m_index = core::indexOf(page, m_after.common.id);
            if (m_index >= page.objects.size()) {
                return;
            }
            m_before = page.objects[m_index];
            m_applied = true;
        }
        page.objects[m_index] = m_after;
        changes.push_back(
            core::Change{core::Change::Kind::ObjectChanged, m_page, m_after.common.id,
                         core::unitedRect(core::boundsOnPage(m_before), core::boundsOnPage(m_after))});
    }
    void undo(core::Document& document, core::Changes& changes) override
    {
        core::Page& page = document.page(m_page);
        page.objects[m_index] = m_before;
        changes.push_back(
            core::Change{core::Change::Kind::ObjectChanged, m_page, m_before.common.id,
                         core::unitedRect(core::boundsOnPage(m_before), core::boundsOnPage(m_after))});
    }

private:
    std::size_t m_page;
    core::PageObject m_after;
    core::PageObject m_before;
    std::size_t m_index = 0;
    bool m_applied = false;
};

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

quint64 EquationController::editingId() const noexcept
{
    return m_editId;
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
    core::addClassToken(*root, core::kCursorClass);
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
    m_editId = 0;
    m_fontPoints = kEquationPoints;
    m_baseline.reset();
    edited();
}

quint64 EquationController::equationAt(QPointF pagePoint, double zoom) const
{
    if (m_document.isNull() || zoom <= 0 || m_document->currentIndex() < 0) {
        return 0;
    }
    const core::Page& page =
        m_document->document().page(static_cast<std::size_t>(m_document->currentIndex()));
    const std::optional<core::ObjectId> hit =
        core::objectAt(page, core::PointF{pagePoint.x(), pagePoint.y()}, core::kHitToleranceScreen / zoom);
    if (!hit) {
        return 0;
    }
    const std::size_t at = core::indexOf(page, *hit);
    return at < page.objects.size() && std::get_if<core::Equation>(&core::body(page.objects[at])) ? *hit : 0;
}

bool EquationController::edit(quint64 id)
{
    if (m_document.isNull() || id == 0) {
        return false;
    }
    DocumentInsertions insertions(*m_document);
    const core::Page* page = insertions.shownPage();
    if (!page) {
        return false;
    }
    const std::size_t at = core::indexOf(*page, id);
    if (at >= page->objects.size()) {
        return false;
    }
    const auto* equation = std::get_if<core::Equation>(&core::body(page->objects[at]));
    if (!equation) {
        return false;
    }
    QString error;
    if (!m_editor.load(equation->mathml, error)) {
        return false;
    }
    // Le curseur d'une équation lue est sur sa première case : la saisie irait après le premier jeton. En
    // réédition il va à la dernière case (le modèle d'édition n'a pas de position avant le premier jeton).
    for (int guard = 0; guard < 10000; ++guard) {
        const std::vector<int> before = m_editor.cursorPath();
        m_editor.moveRight();
        if (m_editor.cursorPath() == before) {
            break;
        }
    }
    m_editId = id;
    m_fontPoints = equation->pointSize > 0 ? equation->pointSize : kEquationPoints;
    m_baseline = m_editor.documentMathml();
    edited();
    return true;
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
    if (m_editId != 0) {
        return acceptEdit(*written);
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

bool EquationController::acceptEdit(const std::string& written)
{
    DocumentInsertions insertions(*m_document);
    const core::Page* page = insertions.shownPage();
    const std::size_t at = page ? core::indexOf(*page, m_editId) : 0;
    if (!page || at >= page->objects.size() || !std::get_if<core::Equation>(&core::body(page->objects[at]))) {
        setMessage(tr("L'équation n'est plus sur la page affichée : rien n'a été modifié."));
        return false;
    }
    if (m_baseline && *m_baseline == written) {
        setMessage({}); // rien de modifié : le document garde les octets d'origine
        return true;
    }
    core::PageObject object = page->objects[at];
    auto& equation = std::get<core::Equation>(core::body(object));
    const QSizeF size = naturalSize(written, equation.pointSize > 0 ? equation.pointSize : kEquationPoints);
    if (!size.isEmpty()) {
        const double cx = equation.bounds.x + equation.bounds.width / 2;
        const double cy = equation.bounds.y + equation.bounds.height / 2;
        equation.bounds =
            core::RectF{cx - size.width() / 2, cy - size.height() / 2, size.width(), size.height()};
    }
    equation.mathml = written;
    m_document->execute(std::make_unique<EquationRewrite>(insertions.shownIndex(), std::move(object)));
    setMessage({});
    return true;
}

void EquationController::edited()
{
    m_editorSize = naturalSize(m_editor.editorMathml(), m_fontPoints);
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
