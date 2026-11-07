// Talon L11 : rempli par L11-T21.
#pragma once

#include <actcraft/ui/preferences.h>

#include <QObject>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Talon L11 : rempli par L11-T21. Coquille : propriétés posées par la liaison de Main.qml ; la tâche ajoute
// le comportement.
class UpdateChecker : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(actcraft::ui::Preferences* preferences READ preferences WRITE setPreferences NOTIFY
                   preferencesChanged)

public:
    explicit UpdateChecker(QObject* parent = nullptr);

    Preferences* preferences() const noexcept;
    void setPreferences(Preferences* preferences);

signals:
    void preferencesChanged();

private:
    QPointer<Preferences> m_preferences;
};

} // namespace actcraft::ui
