// Talon L11 : rempli par L11-T9.
#pragma once

#include <actcraft/ui/commands.h>
#include <actcraft/ui/pageview.h>

#include <QObject>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Talon L11 : rempli par L11-T9. Coquille : propriétés posées par la liaison de Main.qml ; la tâche ajoute le
// comportement.
class DualMode : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(actcraft::ui::Commands* commands READ commands WRITE setCommands NOTIFY commandsChanged)
    Q_PROPERTY(actcraft::ui::PageView* view READ view WRITE setView NOTIFY viewChanged)
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)

public:
    explicit DualMode(QObject* parent = nullptr);

    Commands* commands() const noexcept;
    void setCommands(Commands* commands);
    PageView* view() const noexcept;
    void setView(PageView* view);
    bool active() const noexcept;

signals:
    void commandsChanged();
    void viewChanged();
    void activeChanged();

private:
    QPointer<Commands> m_commands;
    QPointer<PageView> m_view;
    bool m_active = false;
};

} // namespace actcraft::ui
