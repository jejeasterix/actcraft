#pragma once
#include <actcraft/actions/objectactions.h>
#include <actcraft/core/model.h>
#include <actcraft/ui/commands.h> // types complets exigés par moc pour les propriétés
#include <actcraft/ui/linklauncher.h>
#include <actcraft/ui/pagehistory.h>

#include <QObject>
#include <QPointF>
#include <QPointer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <cstddef>
#include <optional>

namespace actcraft::ui {
class DesignMode;
class DocumentController;
class ToolHost;

// Exécuteur des actions d'objet (lot 8, D8-2, D8-3). Il reçoit de ToolHost les appuis et relâchés de l'outil
// de sélection. Un clic (relâché à moins de 4 px de l'appui, sans glisser) sur un objet qui porte des actions
// les exécute dans l'ordre, au relâché, une fois la garde de contact retombée : jamais à l'appui, jamais
// pendant un contact. Un glisser est rendu à l'outil ; le clic droit (menu contextuel) et le mode conception
// (« désactiver les actions ») ne sont jamais pris. Chemins : catégorie `command` et `page` par
// Commands::trigger (verrou et diffusion protégée compris) ; `document` par LinkLauncher::request ; `object`
// par core::objectActionCommand puis DocumentController::execute.
class ActionRunner : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(actcraft::ui::Commands* commands READ commands WRITE setCommands NOTIFY commandsChanged)
    Q_PROPERTY(bool contactActive READ contactActive WRITE setContactActive NOTIFY contactActiveChanged)
    Q_PROPERTY(QObject* document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(QObject* designMode READ designMode WRITE setDesignMode NOTIFY designModeChanged)
    Q_PROPERTY(actcraft::ui::LinkLauncher* links READ links WRITE setLinks NOTIFY linksChanged)
    Q_PROPERTY(bool hovering READ hovering NOTIFY hoveringChanged) // curseur d'action (F-890)
    // Emprises (coordonnées de page) des objets d'action de la page affichée : cadre du mode conception.
    Q_PROPERTY(QVariantList actionFrames READ actionFrames NOTIFY actionFramesChanged)

public:
    explicit ActionRunner(QObject* parent = nullptr);
    ~ActionRunner() override;

    // Appelés par ToolHost quand l'outil courant est la sélection ; rendent vrai si l'échantillon est pris.
    bool press(QPointF pagePoint, Qt::MouseButton button);
    bool move(QPointF pagePoint);        // au-delà de 4 px : rend faux et laisse le glisser à l'outil
    bool release(QPointF pagePoint);     // exécute, une fois contactActive retombé (file d'attente d'un tour)
    void cancel();                       // geste abandonné (changement d'outil, de page) : rien n'est exécuté
    bool takes(QPointF pagePoint) const; // press le prendrait (bouton gauche), sans rien retenir
    Q_INVOKABLE void hover(QPointF pagePoint);
    Q_INVOKABLE void historyBack();    // page.history-back
    Q_INVOKABLE void historyForward(); // page.history-forward

    Commands* commands() const noexcept;
    void setCommands(Commands* commands); // pose les gestionnaires natifs de l'historique des pages
    bool contactActive() const noexcept;
    void setContactActive(bool active);
    QObject* document() const noexcept;
    void setDocument(QObject* document); // DocumentController
    QObject* designMode() const noexcept;
    void setDesignMode(QObject* designMode); // DesignMode
    LinkLauncher* links() const noexcept;
    void setLinks(LinkLauncher* links);
    // Hôte des outils : sélection de la cible des actions copier, couper, aligner, grouper (C++ seulement,
    // posé par ToolHost::setActionRunner).
    void setTools(ToolHost* tools);
    bool hovering() const noexcept;
    QVariantList actionFrames() const;

signals:
    void actionRefused(const QString& reason); // verrou, diffusion protégée, action non prise en charge
    void commandsChanged();
    void contactActiveChanged();
    void documentChanged();
    void designModeChanged();
    void linksChanged();
    void hoveringChanged();
    void actionFramesChanged();

private:
    struct Pending
    {
        std::size_t page = 0;
        core::ObjectId object = 0;
    };
    DocumentController* controller() const;
    bool actionsDisabled() const;
    qreal viewZoom() const; // zoom de la vue de l'hôte des outils, 1 sans vue (seuil et tolérance)
    std::optional<core::ObjectId> actionObjectAt(QPointF pagePoint) const;
    void schedule(); // un tour de boucle, puis runPending si le contact est retombé
    void runPending();
    void run(const Pending& pending);
    void runOne(const core::Action& action, const Pending& pending);
    void runCommand(const QString& id, const QVariant& argument);
    void runDocument(const actions::ObjectActionInfo& info, const core::Action& action);
    void runOnTarget(const QString& command, core::ObjectId target);
    void refuse(const QString& reason);
    void pageShown();
    void resetHistory();
    void setHovering(bool hovering);
    void bindHistoryHandlers(Commands* commands, bool bind);

    QPointer<Commands> m_commands;
    QPointer<DocumentController> m_controller;
    QPointer<DesignMode> m_design;
    QPointer<LinkLauncher> m_links;
    QPointer<ToolHost> m_tools;
    actions::ObjectActionCatalog m_catalog;
    PageHistory m_history;
    bool m_contactActive = false;
    bool m_hovering = false;
    bool m_scheduled = false;
    // Appui pris : objet d'action sous l'appui, point de l'appui ; vide hors geste ou après un glisser.
    std::optional<Pending> m_pressed;
    QPointF m_pressPoint;
    std::optional<Pending> m_pending; // relâché reçu, exécution en attente de la retombée du contact
};
} // namespace actcraft::ui
