#pragma once

#include <actcraft/ui/commands.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/toolhost.h>  // types complets exigés par moc pour les propriétés
#include <actcraft/ui/touchmode.h> // idem

#include <QObject>
#include <QPointer>
#include <QRectF>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Mode à deux utilisateurs (F-110, F-303, D11-2 ; L11-T9). La commande users.dual le bascule (gestionnaire
// C++ posé sur `commands`) ; la bascule attend la fin des contacts en cours de la vue. Allumé : deux états
// d'utilisateur (ToolHost::setDualUsers), input::TwoUserPolicy posée sur la vue, aire du second utilisateur =
// moitié gauche de la vue (côté de la deuxième palette), suivie au redimensionnement ; un changement du mode
// du toucher pendant ce temps ne la remplace pas. Éteint : la politique du mode du toucher courant
// (`touchMode.mode`, gestion complète sans contrôleur).
class DualMode : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(actcraft::ui::Commands* commands READ commands WRITE setCommands NOTIFY commandsChanged)
    Q_PROPERTY(actcraft::ui::PageView* view READ view WRITE setView NOTIFY viewChanged)
    Q_PROPERTY(actcraft::ui::ToolHost* tools READ tools WRITE setTools NOTIFY toolsChanged)
    Q_PROPERTY(actcraft::ui::TouchModeController* touchMode READ touchMode WRITE setTouchMode NOTIFY
                   touchModeChanged)
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)

public:
    explicit DualMode(QObject* parent = nullptr);
    ~DualMode() override;

    Commands* commands() const noexcept;
    void setCommands(Commands* commands);
    PageView* view() const noexcept;
    void setView(PageView* view);
    ToolHost* tools() const noexcept;
    void setTools(ToolHost* tools);
    TouchModeController* touchMode() const noexcept;
    void setTouchMode(TouchModeController* touchMode);
    bool active() const noexcept;

    // users.dual : demande l'état contraire de l'état demandé ; appliquée tout de suite hors contact.
    Q_INVOKABLE void toggle();

signals:
    void commandsChanged();
    void viewChanged();
    void toolsChanged();
    void touchModeChanged();
    void activeChanged();

private:
    void applyPending();
    void poseTwoUserPolicy();
    void restoreTouchPolicy();
    void updateArea();
    QRectF secondArea() const;

    QPointer<Commands> m_commands;
    QPointer<PageView> m_view;
    QPointer<ToolHost> m_tools;
    QPointer<TouchModeController> m_touchMode;
    bool m_active = false;
    bool m_wanted = false; // état demandé, appliqué hors contact
};

} // namespace actcraft::ui
