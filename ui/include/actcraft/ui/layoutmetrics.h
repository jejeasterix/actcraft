// Talon L11 : rempli par L11-T10.
#pragma once

#include <actcraft/ui/preferences.h>

#include <QObject>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Talon L11 : rempli par L11-T10. Coquille : propriétés posées par la liaison de Main.qml ; la tâche ajoute
// le comportement.
class LayoutMetrics : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(actcraft::ui::Preferences* preferences READ preferences WRITE setPreferences NOTIFY
                   preferencesChanged)
    Q_PROPERTY(qreal windowWidth READ windowWidth WRITE setWindowWidth NOTIFY windowWidthChanged)
    Q_PROPERTY(qreal windowHeight READ windowHeight WRITE setWindowHeight NOTIFY windowHeightChanged)

public:
    explicit LayoutMetrics(QObject* parent = nullptr);

    Preferences* preferences() const noexcept;
    void setPreferences(Preferences* preferences);
    qreal windowWidth() const noexcept;
    void setWindowWidth(qreal windowWidth);
    qreal windowHeight() const noexcept;
    void setWindowHeight(qreal windowHeight);

signals:
    void preferencesChanged();
    void windowWidthChanged();
    void windowHeightChanged();

private:
    QPointer<Preferences> m_preferences;
    qreal m_windowWidth = 0;
    qreal m_windowHeight = 0;
};

} // namespace actcraft::ui
