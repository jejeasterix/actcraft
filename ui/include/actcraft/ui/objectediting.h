#pragma once
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/toolhost.h>
#include <actcraft/ui/webframeitem.h>

#include <QObject>
#include <QPointF>
#include <QQmlEngine>

namespace actcraft::ui {

class Insertion;
class PagesController;

class ObjectEditing : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(DocumentController* document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(ToolHost* tools READ tools WRITE setTools NOTIFY toolsChanged)
    Q_PROPERTY(
        actcraft::ui::WebFrameItems* webFrames READ webFrames WRITE setWebFrames NOTIFY webFramesChanged)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionStateChanged)
    Q_PROPERTY(bool canPaste READ canPaste NOTIFY clipboardChanged)
    Q_PROPERTY(bool selectionLocked READ selectionLocked NOTIFY selectionStateChanged)
    Q_PROPERTY(bool selectionHidden READ selectionHidden NOTIFY selectionStateChanged)
    Q_PROPERTY(bool selectionDragCopy READ selectionDragCopy NOTIFY selectionStateChanged)
    Q_PROPERTY(bool selectionIsGroup READ selectionIsGroup NOTIFY selectionStateChanged)
    Q_PROPERTY(bool selectionIsAnnotation READ selectionIsAnnotation NOTIFY selectionStateChanged)
public:
    explicit ObjectEditing(QObject* parent = nullptr);
    DocumentController* document() const;
    void setDocument(DocumentController* document);
    ToolHost* tools() const;
    void setTools(ToolHost* tools);
    WebFrameItems* webFrames() const;
    void setWebFrames(WebFrameItems* frames);
    bool hasSelection() const;
    bool canPaste() const;
    bool selectionLocked() const;
    bool selectionHidden() const;
    bool selectionDragCopy() const;
    bool selectionIsGroup() const;
    bool selectionIsAnnotation() const;

    Q_INVOKABLE void cut();
    Q_INVOKABLE void copy();
    Q_INVOKABLE void paste();
    Q_INVOKABLE void pasteAt(QPointF pagePoint);
    Q_INVOKABLE void pasteOnto();
    Q_INVOKABLE void duplicate();
    Q_INVOKABLE void toggleGroup();
    Q_INVOKABLE void
    arrange(const QString& how); // front, back, forward, backward, layer-top, layer-middle, layer-bottom
    Q_INVOKABLE void
    align(const QString& how); // left, center-x, right, top, center-y, bottom, width, height, angle, all
    Q_INVOKABLE void
    transform(const QString& how); // original-size, fit-*, mirror-x|y, flip-x|y, origin-flip-x|y
    Q_INVOKABLE void toggleFlag(const QString& flag); // lock, hide, drag-copy

signals:
    void documentChanged();
    void toolsChanged();
    void selectionStateChanged();
    void clipboardChanged();
    void insertionFailed(const QString& message);
    void webFramesChanged();

private:
    DocumentController* m_document = nullptr;
    ToolHost* m_tools = nullptr;
    Insertion* m_insertion = nullptr;   // collage d'image et de texte venus d'ailleurs
    PagesController* m_pages = nullptr; // sans sélection d'objet, les commandes agissent sur la page (D3)
    bool m_pageClipboard = false;       // le dernier copier ou couper portait sur une page
    bool m_writing = false;             // on écrit nous-mêmes dans le presse-papiers
};

} // namespace actcraft::ui
