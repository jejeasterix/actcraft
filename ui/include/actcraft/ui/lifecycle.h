// Talon L11 : rempli par L11-T13.
#pragma once

#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>

#include <QObject>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Talon L11 : rempli par L11-T13. Coquille : propriétés posées par la liaison de Main.qml ; la tâche ajoute
// le comportement.
class LifecycleKeeper : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(
        actcraft::ui::DocumentController* document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(actcraft::ui::PageView* view READ view WRITE setView NOTIFY viewChanged)

public:
    explicit LifecycleKeeper(QObject* parent = nullptr);

    DocumentController* document() const noexcept;
    void setDocument(DocumentController* document);
    PageView* view() const noexcept;
    void setView(PageView* view);

signals:
    void documentChanged();
    void viewChanged();

private:
    QPointer<DocumentController> m_document;
    QPointer<PageView> m_view;
};

} // namespace actcraft::ui
