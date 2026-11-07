// Talon L11 : rempli par L11-T6.
#pragma once

#include <actcraft/ui/commands.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/preferences.h>

#include <QObject>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Talon L11 : rempli par L11-T6. Coquille : propriétés posées par la liaison de Main.qml ; la tâche ajoute le
// comportement.
class TouchModeController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(actcraft::ui::Commands* commands READ commands WRITE setCommands NOTIFY commandsChanged)
    Q_PROPERTY(actcraft::ui::Preferences* preferences READ preferences WRITE setPreferences NOTIFY
                   preferencesChanged)
    Q_PROPERTY(actcraft::ui::PageView* view READ view WRITE setView NOTIFY viewChanged)

public:
    explicit TouchModeController(QObject* parent = nullptr);

    Commands* commands() const noexcept;
    void setCommands(Commands* commands);
    Preferences* preferences() const noexcept;
    void setPreferences(Preferences* preferences);
    PageView* view() const noexcept;
    void setView(PageView* view);

signals:
    void commandsChanged();
    void preferencesChanged();
    void viewChanged();

private:
    QPointer<Commands> m_commands;
    QPointer<Preferences> m_preferences;
    QPointer<PageView> m_view;
};

} // namespace actcraft::ui
