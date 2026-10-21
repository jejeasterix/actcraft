#include <actcraft/ui/textediting.h>

namespace actcraft::ui {

// Talon L4-T1 : rempli par L4-T21 (édition) et L4-T25 (texte de haut en bas).
TextEditing::TextEditing(QObject* parent) : QObject(parent) {}

DocumentController* TextEditing::documentController() const noexcept
{
    return m_controller.data();
}

void TextEditing::setDocumentController(DocumentController* controller)
{
    if (m_controller == controller) {
        return;
    }
    m_controller = controller;
    emit wiringChanged();
}

PageView* TextEditing::view() const noexcept
{
    return m_view.data();
}

void TextEditing::setView(PageView* view)
{
    if (m_view == view) {
        return;
    }
    m_view = view;
    emit wiringChanged();
}

QQuickTextDocument* TextEditing::textDocument() const noexcept
{
    return m_document.data();
}

void TextEditing::setTextDocument(QQuickTextDocument* document)
{
    if (m_document == document) {
        return;
    }
    m_document = document;
    emit wiringChanged();
}

int TextEditing::selectionStart() const noexcept
{
    return m_selectionStart;
}

void TextEditing::setSelectionStart(int position)
{
    m_selectionStart = position;
}

int TextEditing::selectionEnd() const noexcept
{
    return m_selectionEnd;
}

void TextEditing::setSelectionEnd(int position)
{
    m_selectionEnd = position;
}

bool TextEditing::isActive() const noexcept
{
    return m_active;
}

bool TextEditing::isVertical() const noexcept
{
    return m_vertical;
}

QRectF TextEditing::frame() const noexcept
{
    return m_frame;
}

QString TextEditing::plainText() const
{
    return {};
}

QString TextEditing::fontFamily() const
{
    return {};
}

qreal TextEditing::fontSize() const
{
    return 0;
}

bool TextEditing::bold() const
{
    return false;
}

bool TextEditing::italic() const
{
    return false;
}

bool TextEditing::underline() const
{
    return false;
}

bool TextEditing::beginAt(QPointF, bool)
{
    static_cast<void>(m_editing);
    return false;
}

void TextEditing::commit() {}

void TextEditing::cancel() {}

void TextEditing::apply(const QString&, const QVariant&) {}

void TextEditing::setFrameWidth(qreal) {}

void TextEditing::moveFrame(QPointF) {}

} // namespace actcraft::ui
