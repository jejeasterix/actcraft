#include "isolatedtext.h"

#include <actcraft/core/notesedits.h>
#include <actcraft/ui/notescontroller.h>

#include <QQuickTextDocument>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>

#include <memory>

namespace actcraft::ui {

namespace {

// Bloquant 5 : les notes viennent du fichier ouvert ; elles se lisent dans le document isolé, qui ne charge
// rien.
bool visibleText(const QString& html)
{
    const std::unique_ptr<QTextDocument> text = isolatedTextDocument({});
    text->setHtml(html);
    return !text->toPlainText().trimmed().isEmpty();
}

// Texte donné à l'éditeur QML : sa TextArea riche chargerait elle-même feuilles et images. Il reçoit le HTML
// assaini (texte et mise en forme gardés), sans aucune image, data: raster comprises (essai 2 : la TextArea
// décode les data: sur leur contenu) ; la page garde les notes reçues tant que personne n'écrit.
QString shownHtml(const QString& notes)
{
    return notes.isEmpty() ? QString() : safeTextHtml(notes, {}, DataImages::Drop);
}

} // namespace

NotesController::NotesController(QObject* parent) : QObject(parent) {}

DocumentController* NotesController::document() const noexcept
{
    return m_document.data();
}

void NotesController::setDocument(DocumentController* document)
{
    if (document == m_document) {
        return;
    }
    if (m_document) {
        disconnect(m_document, nullptr, this, nullptr);
    }
    m_document = document;
    if (m_document) {
        connect(m_document, &DocumentController::pagesChanged, this, &NotesController::refresh);
        connect(m_document, &DocumentController::documentChanged, this, &NotesController::refresh);
        connect(m_document, &DocumentController::pageContentChanged, this, [this](int page) {
            if (m_document && page == m_document->currentIndex()) {
                refresh();
            }
        });
    }
    updateEditable();
    emit wiringChanged();
    refresh();
}

Commands* NotesController::commands() const noexcept
{
    return m_commands.data();
}

void NotesController::setCommands(Commands* commands)
{
    if (commands == m_commands) {
        return;
    }
    if (m_commands) {
        disconnect(m_commands, nullptr, this, nullptr);
    }
    m_commands = commands;
    if (m_commands) {
        connect(m_commands, &Commands::stateChanged, this, &NotesController::updateEditable);
    }
    updateEditable();
    emit wiringChanged();
}

QString NotesController::html() const
{
    return m_html;
}

void NotesController::setHtml(const QString& html)
{
    if (m_loading || html == m_html || !m_editable || !m_document) {
        return;
    }
    // Texte de l'éditeur lui-même : il le montre déjà, il ne se réassainit pas (le curseur ne bouge pas).
    m_html = html;
    m_notes = html;
    m_hasNotes = visibleText(html);
    m_document->execute(
        std::make_unique<core::SetPageNotesCommand>(m_document->currentIndex(), html.toStdString()));
    emit htmlChanged();
}

bool NotesController::hasNotes() const noexcept
{
    return m_hasNotes;
}

bool NotesController::isEditable() const noexcept
{
    return m_editable;
}

bool NotesController::contactActive() const noexcept
{
    return m_contactActive;
}

void NotesController::setContactActive(bool active)
{
    if (active == m_contactActive) {
        return;
    }
    m_contactActive = active;
    emit contactActiveChanged();
    if (!active && m_dirty) {
        refresh();
    }
}

void NotesController::refresh()
{
    if (m_contactActive) {
        m_dirty = true;
        return;
    }
    m_dirty = false;
    QString notes;
    if (m_document && m_document->pageCount() > 0) {
        notes = QString::fromStdString(
            m_document->document().page(static_cast<std::size_t>(m_document->currentIndex())).notes);
    }
    if (notes == m_notes) {
        return;
    }
    m_loading = true;
    m_notes = notes;
    m_html = shownHtml(notes);
    m_hasNotes = visibleText(notes);
    emit htmlChanged();
    m_loading = false;
}

void NotesController::updateEditable()
{
    const bool editable = m_document && m_commands && !m_commands->isLocked();
    if (editable == m_editable) {
        return;
    }
    m_editable = editable;
    emit editableChanged();
}

void NotesController::toggleStyle(QQuickTextDocument* editor, int start, int end, const QString& style)
{
    if (editor == nullptr || editor->textDocument() == nullptr || !m_editable) {
        return;
    }
    QTextCursor cursor(editor->textDocument());
    cursor.setPosition(qMin(start, end));
    cursor.setPosition(qMax(start, end), QTextCursor::KeepAnchor);
    const QTextCharFormat current = cursor.charFormat();
    QTextCharFormat wanted;
    if (style == QLatin1String("bold")) {
        wanted.setFontWeight(current.fontWeight() > QFont::Normal ? QFont::Normal : QFont::Bold);
    } else if (style == QLatin1String("italic")) {
        wanted.setFontItalic(!current.fontItalic());
    } else if (style == QLatin1String("underline")) {
        wanted.setFontUnderline(!current.fontUnderline());
    } else {
        return;
    }
    cursor.mergeCharFormat(wanted);
}

} // namespace actcraft::ui
