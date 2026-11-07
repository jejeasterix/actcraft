#pragma once

#include <actcraft/input/routing.h>
#include <actcraft/render/liftlayeritem.h> // types complets exigés par moc pour les propriétés
#include <actcraft/tools/userstate.h>
#include <actcraft/ui/actionrunner.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>

#include <QColor>
#include <QObject>
#include <QPointer>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include <memory>
#include <vector>

namespace actcraft::tools {
class SelectTool;
class Tool;
class UserState;
} // namespace actcraft::tools

namespace actcraft::ui {

struct ToolAdapters;    // tooladapters.h
class ActionGate;       // toolhost.cpp : l'exécuteur d'actions devant l'outil de sélection (L8-T9)
class DesignModePolicy; // toolhost.cpp : réglages de DesignMode vus par les outils (L8-T16, D8-9)

// Outils de l'utilisateur (DA6, DA-A1) : registre, état d'utilisateur, adaptateurs des interfaces d'outil,
// commandes du lot 3, mémoire par outil (F-136, QSettings).
class ToolHost : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Tools)
    Q_PROPERTY(actcraft::ui::Commands* commands READ commands WRITE setCommands NOTIFY wiringChanged)
    Q_PROPERTY(actcraft::ui::DocumentController* documentController READ documentController WRITE
                   setDocumentController NOTIFY wiringChanged)
    Q_PROPERTY(actcraft::ui::PageView* view READ view WRITE setView NOTIFY wiringChanged)
    Q_PROPERTY(
        actcraft::render::LiftLayerItem* liftLayer READ liftLayer WRITE setLiftLayer NOTIFY wiringChanged)
    Q_PROPERTY(QString currentTool READ currentTool NOTIFY currentToolChanged)
    Q_PROPERTY(QColor color READ color NOTIFY settingsChanged)
    Q_PROPERTY(qreal width READ width NOTIFY settingsChanged)
    Q_PROPERTY(int selectionCount READ selectionCount NOTIFY selectionChanged)
    Q_PROPERTY(bool handlesVisible READ handlesVisible NOTIFY selectionChanged)
    Q_PROPERTY(QVariantMap selectionFrame READ selectionFrame NOTIFY selectionChanged)
    Q_PROPERTY(QRectF trashArea READ trashArea WRITE setTrashArea NOTIFY trashAreaChanged) // scène
    Q_PROPERTY(bool trashHover READ trashHover NOTIFY trashHoverChanged)
    // Vide : réglages de l'application ; tests : chemin d'un fichier .ini.
    Q_PROPERTY(QString settingsFile READ settingsFile WRITE setSettingsFile NOTIFY settingsFileChanged)
    Q_PROPERTY(QString shapePreset READ shapePreset NOTIFY settingsChanged)
    Q_PROPERTY(bool shapeFilled READ shapeFilled NOTIFY settingsChanged)
    Q_PROPERTY(bool shapeFromCenter READ shapeFromCenter NOTIFY settingsChanged)
    Q_PROPERTY(QStringList shapePresets READ shapePresets CONSTANT) // ordre de la palette
    // Nuancier de la palette des formes (L4-C7) : remplissage courant, et défaut de l'outil (1re case).
    Q_PROPERTY(QColor shapeFillColor READ shapeFillColor NOTIFY settingsChanged)
    Q_PROPERTY(QColor shapeDefaultFillColor READ shapeDefaultFillColor CONSTANT)
    // L5-C18 (M3, M18) : mémoire du connecteur, relue par son panneau.
    Q_PROPERTY(int connectorStyle READ connectorStyle NOTIFY settingsChanged)
    Q_PROPERTY(int connectorCapA READ connectorCapA NOTIFY settingsChanged)
    Q_PROPERTY(int connectorCapB READ connectorCapB NOTIFY settingsChanged)
    // Mode conception (F-560, L6-T22) : rendus aux outils par leur contexte ; Main.qml les lie à DesignMode.
    Q_PROPERTY(bool lockIgnored READ lockIgnored WRITE setLockIgnored NOTIFY designFlagsChanged)
    Q_PROPERTY(bool snapDisabled READ snapDisabled WRITE setSnapDisabled NOTIFY designFlagsChanged)
    // Lot 8 (L8-T9) : exécuteur d'actions posé par setActionRunner (surcouche des actions : cadre, curseur).
    Q_PROPERTY(actcraft::ui::ActionRunner* actionRunner READ actionRunner NOTIFY actionRunnerChanged)
    // Lot 8 (L8-T16) : le DesignMode de l'application ; ses effets (restrictions, objets bloquants, actions)
    // sont lus par l'outil de sélection à chaque geste (tools::InteractionPolicy). nullptr : tout actif.
    Q_PROPERTY(QObject* designMode READ designMode WRITE setDesignMode NOTIFY designFlagsChanged)
    // Lot 11 (L11-T7, D11-2) : mode à deux utilisateurs ; DualMode l'écrit (L11-T9).
    Q_PROPERTY(bool dualUsers READ dualUsers WRITE setDualUsers NOTIFY dualUsersChanged)

public:
    explicit ToolHost(QObject* parent = nullptr);
    ~ToolHost() override;

    Commands* commands() const noexcept;
    void setCommands(Commands* commands);
    DocumentController* documentController() const noexcept;
    void setDocumentController(DocumentController* controller);
    PageView* view() const noexcept;
    void setView(PageView* view);
    render::LiftLayerItem* liftLayer() const noexcept;
    void setLiftLayer(render::LiftLayerItem* layer);
    QString currentTool() const;
    QColor color() const;
    qreal width() const;
    int selectionCount() const;
    int connectorStyle() const noexcept;
    int connectorCapA() const noexcept;
    int connectorCapB() const noexcept;
    bool handlesVisible() const;
    QVariantMap selectionFrame() const;
    QRectF trashArea() const noexcept;
    void setTrashArea(const QRectF& area);
    bool trashHover() const noexcept;
    QString settingsFile() const;
    void setSettingsFile(const QString& path);

    // « select », « pen », « highlighter », « eraser » : outil courant de l'utilisateur 1, commande tool.<id>
    // cochée, réglages de l'outil rendus à la vue (couleur et épaisseur de l'encre).
    Q_INVOKABLE void activate(const QString& toolId);
    // L5-T17 : « pen-shape » avec le tracé choisi (identifiant de pen.shape ; « free » : retour au trait
    // libre) ; seulement sous le stylo, le marqueur ou ce même outil, sinon sans effet (F-221). Autre outil :
    // activate(id).
    Q_INVOKABLE void activate(const QString& toolId, const QVariant& argument);
    // Outil courant s'il a sa couleur (encre ; formes : le contour, L4-C7), sinon le dernier outil d'encre
    // (F-134).
    Q_INVOKABLE void setColor(const QColor& color);
    Q_INVOKABLE void setWidth(qreal width);
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void clearSelection();
    // H5-15 : active l'outil Sélection et sélectionne ces objets de la page affichée (ceux qui existent).
    void selectObjects(const std::vector<core::ObjectId>& ids);
    // Lot 8 (L8-T9) : sous l'outil de sélection, l'appui va d'abord à l'exécuteur, les déplacements tant
    // qu'il les prend, le relâché aussi ; sinon l'outil reçoit l'appui retenu puis les suivants (aucun
    // échantillon perdu). nullptr : l'outil reçoit tout.
    Q_INVOKABLE void setActionRunner(actcraft::ui::ActionRunner* runner);
    ActionRunner* actionRunner() const noexcept;
    Q_INVOKABLE void deleteSelection();
    Q_INVOKABLE void enlargeSelection();
    Q_INVOKABLE void reduceSelection();
    Q_INVOKABLE void setSelectionTranslucency(int value);
    Q_INVOKABLE void cancelGesture();
    Q_INVOKABLE void setConnectorStyle(int style);         // L5-T17
    Q_INVOKABLE void setConnectorCaps(int capA, int capB); // L5-C18 : rangs 0 aucune, 1 rayon, 2 flèche
    // H5-33 (L5-C3) : le choix d'un style au panneau (connector.style) règle aussi les extrémités selon ses
    // flèches ; setConnectorStyle seul ne les touche pas.
    Q_INVOKABLE void chooseConnectorStyle(int style);
    // L5-C3 : vignette d'un style du panneau, tracé de core dans une boîte width × height : « points »,
    // « outwardA », « outwardB », « arrowStart », « arrowEnd », « dashed ».
    Q_INVOKABLE QVariantMap connectorStyleSketch(int style, qreal width, qreal height) const;

    // Outil courant de l'utilisateur ; nullptr tant que le contrôleur, la vue et la couche ne sont pas
    // branchés. Pendant un appui sur un objet d'action sous la sélection (L8-T9) : la sélection précédée de
    // l'exécuteur.
    tools::Tool* activeTool(input::UserId user = input::kFirstUser) const;
    // L11-C1 (D11-4) : outil imposé à un contact de doigt (modes pen et select), sans changer l'outil
    // courant. Select : l'outil Sélectionner (créé à la première demande). Pen et Current : nullptr ; la vue
    // garde son stylo imposé, réglé à chaque appui sur les réglages « pen » de l'utilisateur (l'hôte n'a
    // qu'un outil par identifiant, partagé avec l'outil courant).
    tools::Tool* imposedTool(input::UserId user, input::FingerTool tool) const;
    // État de l'utilisateur. Mode à deux utilisateurs allumé : tout id autre que kFirstUser rend le second
    // état (identifiant kSecondUser) ; éteint : l'utilisateur 0, comme le socle (D-S10).
    tools::UserState& user(input::UserId id = input::kFirstUser);
    bool dualUsers() const noexcept;
    // Allumé, le second UserState est créé à la première fois (un état neuf commence au stylo, H11-16) puis
    // gardé : éteindre puis rallumer rend le même état, avec son outil et ses réglages.
    Q_INVOKABLE void setDualUsers(bool on);
    bool lockIgnored() const noexcept;
    void setLockIgnored(bool ignored); // republie la sélection (poignées d'un verrouillé)
    bool snapDisabled() const noexcept;
    void setSnapDisabled(bool disabled);
    QObject* designMode() const noexcept;
    void setDesignMode(QObject* designMode);

    // Lot 4 (outil des formes), L4-T11.
    QString shapePreset() const;
    bool shapeFilled() const;
    bool shapeFromCenter() const;
    QStringList shapePresets() const;
    QColor shapeFillColor() const;
    QColor shapeDefaultFillColor() const;
    Q_INVOKABLE void chooseShape(const QString& preset); // active aussi l'outil « shape »
    Q_INVOKABLE void setShapeFilled(bool filled);
    Q_INVOKABLE void setShapeFillColor(const QColor& color);
    Q_INVOKABLE void setShapeFromCenter(bool fromCenter);
    // Contour normalisé du préréglage dans un carré 1 × 1, pour la palette et le curseur : [{x, y}, …] fermé.
    Q_INVOKABLE QVariantList shapeOutline(const QString& preset) const;

signals:
    void wiringChanged();
    void currentToolChanged();
    void settingsChanged();
    void selectionChanged();
    void trashAreaChanged();
    void trashHoverChanged();
    void settingsFileChanged();
    void designFlagsChanged();
    void actionRunnerChanged();
    // L8-T16 : un dépôt accepté par un conteneur (F-577) ; `chime` : le son de confirmation est à jouer
    // (L8-T25).
    void containerAccepted(quint64 container, bool chime);
    void dualUsersChanged();

private:
    // Adaptateurs et outils ; nullptr tant que contrôleur, vue (et son encre) et couche ne sont pas branchés.
    ToolAdapters* adapters() const;
    void discardAdapters() const; // jette les adaptateurs, l'outil occupé abandonnant d'abord son geste
    tools::Tool* toolFor(const QString& id) const;      // créé à la première demande
    tools::Tool* existingTool(const QString& id) const; // jamais créé
    tools::SelectTool* selectTool() const;              // l'outil Sélectionner, créé à la première demande
    tools::InkSettings settingsOf(const QString& id) const; // réglages de l'utilisateur, ou ceux par défaut
    // Réglages rangés chez l'utilisateur, écrits dans le fichier et rendus à la vue.
    void storeSettings(const QString& id, const tools::InkSettings& settings);
    void ensureSettings(); // QSettings → UserState (F-136), une fois par fichier
    void writeSettings(const QString& toolId) const;
    void applyToView(); // couleur (alpha du marqueur) et épaisseur rendues à PageView
    void syncToolCommands();
    void showSelection(const QVariantMap& frame, bool handlesVisible);
    void setTrashHoverState(bool hover);
    void refreshSelection();
    void relayModifiers(int key, int modifiers);
    // L5-T17 : réglages gardés par outil (couleur du pot, style du connecteur, tracé choisi) rendus à
    // l'outil.
    void configureTool(tools::Tool* tool) const;
    void writeValue(const QString& key, const QVariant& value) const;
    QString shownInkTool() const;
    QString shownWidthTool() const;
    // L6-P1 (P11 de L6-T0) : sous Sélectionner, les textes sélectionnés prennent la couleur en une étape
    // d'annulation ; outil, sélection et couleur de l'encre restent. Vrai si la sélection contient au moins
    // un texte : la couleur lui revient, même si tous l'ont déjà (alors aucune étape, document inchangé).
    // Faux sans texte sélectionné : la couleur va à l'encre (L3-T0).
    bool recolorSelectedTexts(const QColor& color);
    bool eventFilter(QObject* watched, QEvent* event) override; // Maj et Ctrl (L3-T8)
    void armGate(const QEvent* event); // L8-T9 : appui sur un objet d'action sous la sélection

    QPointer<Commands> m_commands;
    QPointer<DocumentController> m_controller;
    QPointer<PageView> m_view;
    QPointer<render::LiftLayerItem> m_liftLayer;
    QRectF m_trashArea;
    bool m_trashHover = false;
    QString m_settingsFile;
    std::unique_ptr<tools::UserState> m_user;
    std::unique_ptr<tools::UserState> m_secondUser; // L11-T7 : créé au premier allumage du mode
    bool m_dualUsers = false;
    mutable std::unique_ptr<ToolAdapters> m_adapters;
    QPointer<ActionRunner> m_actionRunner;
    mutable std::unique_ptr<ActionGate> m_gate; // refait quand l'outil de sélection change
    bool m_gateArmed = false;    // geste en cours armé par armGate : activeTool() rend la porte
    bool m_lockIgnored = false;  // mode conception, recopié dans le contexte des adaptateurs
    bool m_snapDisabled = false; // idem
    std::unique_ptr<DesignModePolicy> m_policy; // posée sur l'outil de sélection à sa création
    QString m_lastInk = QStringLiteral("pen");  // dernier outil d'encre (L3-T0)
    QVariantMap m_selectionFrame;
    bool m_handlesVisible = false;
    bool m_settingsLoaded = false;
    bool m_filtering = false;
    int m_connectorStyle = 0; // L5-T17 : style du connecteur (rang du panneau)
    int m_connectorCapA = 0;  // L5-C18 : extrémités A et B (rang de core::ConnectorCap)
    int m_connectorCapB = 0;
    QString m_penShape; // L5-T17 : tracé choisi de la modification de stylo
    int m_observer = 0;
};

} // namespace actcraft::ui
