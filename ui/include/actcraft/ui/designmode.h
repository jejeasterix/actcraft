#pragma once

#include <actcraft/ui/commands.h>
#include <actcraft/ui/profilecontroller.h>

#include <QObject>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

#include <array>
#include <cstddef>
#include <optional>

namespace actcraft::ui {

// Mode conception (F-560, D6-12) : bascule `mode.design` (F2, bouton du groupe `commandes` de la palette) et
// neuf réglages, tous vrais par défaut, gardés dans la section `design-mode` du profil. Le mode ne modifie
// aucun objet (verrouillé et masqué restent des drapeaux du document) : il publie des effets, chacun vrai
// quand le mode est actif et son réglage vrai. Branchés au lot 6 : verrou ignoré au glisser et aux poignées
// (lockIgnored, contexte d'outil par ToolHost), objets masqués translucides (hiddenShown, drapeau de la seule
// vue de page), accrochage coupé (snapDisabled). Publiés pour les lots 7 et 8 : transitionsSuspended (A18) et
// les effets d'actions, de restrictions, de bureau et d'astuces. Miniatures et export n'en lisent rien. Une
// bascule pendant un contact (contactActive) est gardée et appliquée au relâché, avec son seul redessin.
class DesignMode : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(bool contactActive READ contactActive WRITE setContactActive NOTIFY contactActiveChanged)
    Q_PROPERTY(actcraft::ui::Commands* commands READ commands WRITE setCommands NOTIFY wiringChanged)
    Q_PROPERTY(actcraft::ui::ProfileController* profile READ profile WRITE setProfile NOTIFY wiringChanged)
    // Les neuf réglages (F-560).
    Q_PROPERTY(bool editLocked READ editLocked WRITE setEditLocked NOTIFY settingsChanged)
    Q_PROPERTY(bool showHidden READ showHidden WRITE setShowHidden NOTIFY settingsChanged)
    Q_PROPERTY(bool highlightActions READ highlightActions WRITE setHighlightActions NOTIFY settingsChanged)
    Q_PROPERTY(bool disableActions READ disableActions WRITE setDisableActions NOTIFY settingsChanged)
    Q_PROPERTY(
        bool disableRestrictions READ disableRestrictions WRITE setDisableRestrictions NOTIFY settingsChanged)
    Q_PROPERTY(bool disableBlockers READ disableBlockers WRITE setDisableBlockers NOTIFY settingsChanged)
    Q_PROPERTY(bool disableSnap READ disableSnap WRITE setDisableSnap NOTIFY settingsChanged)
    Q_PROPERTY(bool disableDesktop READ disableDesktop WRITE setDisableDesktop NOTIFY settingsChanged)
    Q_PROPERTY(bool showTips READ showTips WRITE setShowTips NOTIFY settingsChanged)
    // Effets : mode actif et réglage vrai.
    Q_PROPERTY(bool lockIgnored READ lockIgnored NOTIFY effectsChanged)
    Q_PROPERTY(bool hiddenShown READ hiddenShown NOTIFY effectsChanged)
    Q_PROPERTY(bool actionsHighlighted READ actionsHighlighted NOTIFY effectsChanged)
    Q_PROPERTY(bool actionsDisabled READ actionsDisabled NOTIFY effectsChanged)
    Q_PROPERTY(bool restrictionsDisabled READ restrictionsDisabled NOTIFY effectsChanged)
    Q_PROPERTY(bool blockersDisabled READ blockersDisabled NOTIFY effectsChanged)
    Q_PROPERTY(bool snapDisabled READ snapDisabled NOTIFY effectsChanged)
    Q_PROPERTY(bool desktopDisabled READ desktopDisabled NOTIFY effectsChanged)
    Q_PROPERTY(bool tipsShown READ tipsShown NOTIFY effectsChanged)
    // Effets de changement de page du lot 7 suspendus (A18, amendement 5) : vrai quand le mode est actif ;
    // aucun effet branché ici, le lot 7 lit cette propriété.
    Q_PROPERTY(bool transitionsSuspended READ transitionsSuspended NOTIFY effectsChanged)

public:
    enum class Setting : std::size_t
    {
        EditLocked,
        ShowHidden,
        HighlightActions,
        DisableActions,
        DisableRestrictions,
        DisableBlockers,
        DisableSnap,
        DisableDesktop,
        ShowTips,
    };
    static constexpr std::size_t kSettingCount = 9;

    explicit DesignMode(QObject* parent = nullptr);
    ~DesignMode() override;

    bool active() const noexcept;
    void setActive(bool active); // pendant un contact : gardé, appliqué au relâché
    Q_INVOKABLE void toggle();   // gestionnaire de `mode.design`
    bool contactActive() const noexcept;
    void setContactActive(bool active);
    Commands* commands() const noexcept;
    void setCommands(Commands* commands); // pose le gestionnaire natif de `mode.design` et sa coche
    ProfileController* profile() const noexcept;
    void setProfile(ProfileController* profile); // relit la section `design-mode` à chaque profil

    bool setting(Setting which) const noexcept;
    void setSetting(Setting which, bool value); // écrit aussi dans le profil
    bool editLocked() const noexcept { return setting(Setting::EditLocked); }
    void setEditLocked(bool value) { setSetting(Setting::EditLocked, value); }
    bool showHidden() const noexcept { return setting(Setting::ShowHidden); }
    void setShowHidden(bool value) { setSetting(Setting::ShowHidden, value); }
    bool highlightActions() const noexcept { return setting(Setting::HighlightActions); }
    void setHighlightActions(bool value) { setSetting(Setting::HighlightActions, value); }
    bool disableActions() const noexcept { return setting(Setting::DisableActions); }
    void setDisableActions(bool value) { setSetting(Setting::DisableActions, value); }
    bool disableRestrictions() const noexcept { return setting(Setting::DisableRestrictions); }
    void setDisableRestrictions(bool value) { setSetting(Setting::DisableRestrictions, value); }
    bool disableBlockers() const noexcept { return setting(Setting::DisableBlockers); }
    void setDisableBlockers(bool value) { setSetting(Setting::DisableBlockers, value); }
    bool disableSnap() const noexcept { return setting(Setting::DisableSnap); }
    void setDisableSnap(bool value) { setSetting(Setting::DisableSnap, value); }
    bool disableDesktop() const noexcept { return setting(Setting::DisableDesktop); }
    void setDisableDesktop(bool value) { setSetting(Setting::DisableDesktop, value); }
    bool showTips() const noexcept { return setting(Setting::ShowTips); }
    void setShowTips(bool value) { setSetting(Setting::ShowTips, value); }

    bool effect(Setting which) const noexcept { return m_active && setting(which); }
    bool lockIgnored() const noexcept { return effect(Setting::EditLocked); }
    bool hiddenShown() const noexcept { return effect(Setting::ShowHidden); }
    bool actionsHighlighted() const noexcept { return effect(Setting::HighlightActions); }
    bool actionsDisabled() const noexcept { return effect(Setting::DisableActions); }
    bool restrictionsDisabled() const noexcept { return effect(Setting::DisableRestrictions); }
    bool blockersDisabled() const noexcept { return effect(Setting::DisableBlockers); }
    bool snapDisabled() const noexcept { return effect(Setting::DisableSnap); }
    bool desktopDisabled() const noexcept { return effect(Setting::DisableDesktop); }
    bool tipsShown() const noexcept { return effect(Setting::ShowTips); }
    bool transitionsSuspended() const noexcept { return m_active; }

signals:
    void activeChanged();
    void contactActiveChanged();
    void wiringChanged();
    void settingsChanged();
    void effectsChanged(); // une fois par bascule ou par réglage qui change un effet

private:
    using Settings = std::array<bool, kSettingCount>;
    unsigned effectMask() const noexcept;
    void apply(bool active);
    void loadSettings(); // section `design-mode` du profil ; vrai par défaut
    void changeSettings(const Settings& values);
    void bindCommands();
    void unbindCommands();

    Settings m_settings{};
    bool m_active = false;
    bool m_contactActive = false;
    std::optional<bool> m_requested; // bascule demandée pendant un contact
    QPointer<Commands> m_commands;
    QPointer<ProfileController> m_profile;
};

} // namespace actcraft::ui
