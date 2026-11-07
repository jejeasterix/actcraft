#pragma once

#include <actcraft/ui/commands.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/preferences.h>

#include <QObject>
#include <QPointer>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Réglage du mode du toucher (F-930) : « ignore », « pen », « select » ou « full » (défaut, décision 25). Le
// mode est un réglage du poste (workstation/touch-mode). La commande touch.mode le change ; la politique du
// routeur de la vue est posée par PageView::setRoutingPolicy (le contrôleur ne touche ni au routeur ni aux
// échantillons). Un changement demandé pendant un contact attend le relâché.
class TouchModeController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(actcraft::ui::Commands* commands READ commands WRITE setCommands NOTIFY commandsChanged)
    Q_PROPERTY(actcraft::ui::Preferences* preferences READ preferences WRITE setPreferences NOTIFY
                   preferencesChanged)
    Q_PROPERTY(actcraft::ui::PageView* view READ view WRITE setView NOTIFY viewChanged)
    Q_PROPERTY(QString mode READ mode NOTIFY modeChanged)

public:
    explicit TouchModeController(QObject* parent = nullptr);
    ~TouchModeController() override;

    Commands* commands() const noexcept;
    void setCommands(Commands* commands);
    Preferences* preferences() const noexcept;
    void setPreferences(Preferences* preferences);
    PageView* view() const noexcept;
    void setView(PageView* view);
    QString mode() const;

    // Vrai si l'identifiant est connu (changement fait, ou différé jusqu'au relâché) ; faux sinon, rien ne
    // change.
    Q_INVOKABLE bool setMode(const QString& id);

signals:
    void commandsChanged();
    void preferencesChanged();
    void viewChanged();
    void modeChanged();

private:
    void apply(const QString& wanted);
    void applyPolicy();
    void applyPending();

    QPointer<Commands> m_commands;
    QPointer<Preferences> m_preferences;
    QPointer<PageView> m_view;
    QString m_mode;    // identifiant en vigueur
    QString m_pending; // changement demandé pendant un contact, vide sinon
};

} // namespace actcraft::ui
