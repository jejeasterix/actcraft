#pragma once

#include <actcraft/core/model.h>
#include <actcraft/features/mathtools.h>
#include <actcraft/features/strokeguide.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/toolhost.h>

#include <QObject>
#include <QPointF>
#include <QPointer>
#include <QSizeF>
#include <QString>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace actcraft::ui {

// Outils de maths ouverts dans la fenêtre (F-280 règle, F-283 équerre, F-281 rapporteur, F-282 compas ;
// origine : L7-T13). L'outil n'est jamais écrit dans le document. Leurs bords guident l'encre par le filtre
// d'échantillons de la vue (PageView::setSampleFilter), appliqué avant le Pipeline, relâchés synthétisés
// compris (à partir du dernier point brut) : le stylo reçoit des échantillons déjà posés sur le bord et verse
// son trait comme tout trait ; ni le stylo ni la vue ne savent qu'une règle existe. Rapporteur en style
// « arc fermé » ou « arc plein » : une fois le trait guidé versé par le stylo (inchangé, l'arc seul), le
// contrôleur ajoute les deux rayons (core::Line) ou le secteur rempli (core::Shape Path) par
// DocumentInsertions::insertObject, une étape d'annulation à elle seule ; jamais par le stylo ni par le
// filtre. Compas « marquer le centre » : de même, la croix du centre (un core::Line) suit le trait guidé par
// la mine, par insertObject, sur la couche du trait. Par fenêtre, pas par page : les outils restent en
// changeant de page. Coordonnées de page. Mode à deux utilisateurs (H11-O13, L11-C23) : un outil appartient à
// qui l'a ouvert et ne guide que son trait ; mode à un utilisateur : tout outil ouvert guide le seul
// utilisateur.
class MathToolsController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(actcraft::ui::PageView* view READ view WRITE setView NOTIFY viewChanged)
    Q_PROPERTY(actcraft::ui::ToolHost* toolHost READ toolHost WRITE setToolHost NOTIFY toolHostChanged)
    // Document où le rapporteur ajoute ses rayons ou son secteur ; nul : il n'ajoute rien.
    Q_PROPERTY(actcraft::ui::DocumentController* documentController READ documentController WRITE
                   setDocumentController NOTIFY documentControllerChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    // Augmente à chaque changement de cadre ou de réglage d'un outil : les délégués relisent toolAt().
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)
    // Translucidité par défaut, sur 255 (features::kDefaultToolOpacity, relevée par L7-T0).
    Q_PROPERTY(int defaultOpacity READ defaultOpacity CONSTANT)
    // Bande d'accroche le long d'un bord, en pixels de page (16 pixels d'écran ÷ zoom).
    Q_PROPERTY(qreal guideBand READ guideBand NOTIFY guideBandChanged)
    // Bornes de la longueur d'une règle, pixels de page : le délégué les lit pour allonger sans déplacer le
    // bout fixe.
    Q_PROPERTY(qreal minimumLength READ minimumLength CONSTANT)
    Q_PROPERTY(qreal maximumLength READ maximumLength CONSTANT)
    // Bornes du rayon d'un rapporteur, pixels de page.
    Q_PROPERTY(qreal minimumRadius READ minimumRadius CONSTANT)
    Q_PROPERTY(qreal maximumRadius READ maximumRadius CONSTANT)
    // Origine XY (F-284) : marqueur montré (par fenêtre) et position de l'origine de la page affichée, relue
    // de son core::PagePresentation (hasOrigin, origin) ; défaut : centre de la page.
    Q_PROPERTY(bool originVisible READ originVisible WRITE setOriginVisible NOTIFY originVisibleChanged)
    Q_PROPERTY(QPointF origin READ origin NOTIFY originChanged)

public:
    static constexpr double kSnapScreenPixels = 16;
    // Propriété dynamique de la vue : l'origine montrée (QPointF), pivot de la poignée de rotation du lot 3
    // (ViewInfoAdapter::rotationPivot) ; absente quand l'origine est cachée.
    static constexpr const char* kRotationPivotProperty = "actcraftRotationPivot";

    explicit MathToolsController(QObject* parent = nullptr);
    ~MathToolsController() override;

    PageView* view() const noexcept;
    void setView(PageView* view);
    ToolHost* toolHost() const noexcept;
    void setToolHost(ToolHost* host);
    DocumentController* documentController() const noexcept;
    void setDocumentController(DocumentController* controller);
    int count() const noexcept;
    int revision() const noexcept;
    static int defaultOpacity() noexcept;
    qreal guideBand() const noexcept;
    static qreal minimumLength() noexcept;
    static qreal maximumLength() noexcept;
    static qreal minimumRadius() noexcept;
    static qreal maximumRadius() noexcept;
    bool originVisible() const noexcept;
    void setOriginVisible(bool visible);
    QPointF origin() const noexcept;

    // Mémorise `point` comme origine de la page affichée, en une étape
    // (DocumentInsertions::setPagePresentation : les autres réglages de la page sont gardés). Faux sans
    // document, sans page, ou si la page porte déjà cette origine.
    Q_INVOKABLE bool moveOrigin(QPointF point);

    // Les open… prennent l'utilisateur qui ouvre (le `user` du gestionnaire (argument, user) des commandes
    // math.*), à qui l'outil appartiendra ; hors du mode à deux, le seul utilisateur.
    // Règle au centre de la page (longueur 600, largeur 80, angle 0 ; une suivante décalée de 24) ; son
    // index.
    Q_INVOKABLE int openRuler(int user = input::kFirstUser);
    // Équerre au centre de la page (côté 300, angle 0 : côtés de l'angle droit vers +x et +y) ; son index.
    Q_INVOKABLE int openSetSquare(int user = input::kFirstUser);
    // Rapporteur centré sur la page (rayon 200, angle 0) ; forme et style d'arc du dernier choix (QSettings
    // mathtools/protractor ; défaut : complet, arc seul) ; son index.
    Q_INVOKABLE int openProtractor(int user = input::kFirstUser);
    // Compas : pointe à gauche du centre de la page, mine à droite (rayon 5 cm, bras de 250, angle 0) ;
    // réglages du dernier choix (QSettings mathtools/compass ; défaut : non réfléchi, angle et rayon
    // affichés, centre non marqué) ; son index.
    Q_INVOKABLE int openCompass(int user = input::kFirstUser);
    // Ferme l'outil : les traits déjà versés restent (ils sont au document, l'outil n'en possède aucun).
    Q_INVOKABLE void close(int index);
    // Règle : centre, longueur, largeur, angle. Équerre : `center` est le sommet de l'angle droit, `length`
    // le côté, `thickness` est ignorée. Rapporteur : `center` est le centre du cercle, `length` le rayon,
    // `thickness` est ignorée. Compas : `center` est la pointe, `length` le rayon (borné à 20 … 1,9 × la
    // longueur des bras), `angle` la direction de la pointe vers la mine ; `thickness` est ignorée (longueur
    // des bras : stepCompassLength).
    Q_INVOKABLE void setFrame(int index, QPointF center, qreal length, qreal thickness, qreal angle);
    // Unités des deux bords : « cm », « mm », « inch », « none » ; gardées pour la règle suivante
    // (QSettings).
    Q_INVOKABLE void setUnits(int index, const QString& top, const QString& bottom);
    Q_INVOKABLE void setShowAngle(int index, bool on);
    // Rapporteur : « full » (360°) ou « half » (180°, au-dessus du diamètre à l'angle 0) ; gardé pour le
    // suivant.
    Q_INVOKABLE void setProtractorShape(int index, const QString& shape);
    // Rapporteur : « arc » (le trait seul), « closed » (trait + deux rayons), « filled » (trait + secteur
    // rempli de la couleur du trait) ; gardé pour le suivant.
    Q_INVOKABLE void setArcStyle(int index, const QString& style);
    // Compas : dessin réfléchi (charnière de l'autre côté ; la mine ne bouge pas), rayon affiché, centre
    // marqué (une croix après chaque trait guidé par la mine) ; gardés pour le suivant.
    Q_INVOKABLE void setMirrored(int index, bool on);
    Q_INVOKABLE void setShowRadius(int index, bool on);
    Q_INVOKABLE void setMarkCentre(int index, bool on);
    // Compas : longueur des bras augmentée (step > 0, × 1,2), réduite (step < 0) ou réinitialisée (0, 250),
    // bornée à 120 … 600 ; le rayon reste sous 1,9 × la longueur.
    Q_INVOKABLE void stepCompassLength(int index, int step);
    // kind (« ruler », « setSquare » ou « protractor »), command, centerX, centerY, length, thickness, angle,
    // displayAngle, topUnit, bottomUnit, showAngle ; vide hors des bornes. Équerre : centre = sommet de
    // l'angle droit, length = thickness = côté. Rapporteur : centre du cercle, length = thickness = rayon, et
    // shape, arcStyle, swept (angle balayé par le trait guidé en cours ou le dernier, degrés entiers). Compas
    // : centre = pointe, length = rayon, thickness = longueur des bras, angle = direction de la mine, et
    // mirrored, showRadius, markCentre, radiusText (« 3,0 cm »), leadX, leadY (la mine).
    Q_INVOKABLE QVariantMap toolAt(int index) const;
    // Point d'accroche sous `pagePoint` (cercle du survol) : {snapped, x, y, tool} ; tool : index de l'outil
    // dont le guide saisirait le stylo (-1 sans accroche). Aucun changement d'état.
    Q_INVOKABLE QVariantMap snapAt(QPointF pagePoint) const;

    // Filtre posé sur la vue (PageView::setSampleFilter) : l'échantillon, posé sur le bord qui le guide le
    // cas échéant. sample.user dit à qui est le trait. Public pour les essais à deux utilisateurs.
    input::Sample guide(const input::Sample& sample);

signals:
    void viewChanged();
    void toolHostChanged();
    void documentControllerChanged();
    void countChanged();
    void revisionChanged();
    void guideBandChanged();
    void originVisibleChanged();
    void originChanged();
    // Le délégué de l'index est à créer (kind comme dans toolAt()) ou à retirer : la couche tient un modèle
    // par outil, sans recréer les délégués des autres.
    void toolOpened(int index, const QString& kind);
    void toolClosed(int index);

private:
    // Rapporteur vu par GuideSet : le guide de features, plus la marque « saisi au dernier appui » (GuideSet
    // ne dit pas quel guide tient le trait).
    class ArcGuide final : public features::StrokeGuide
    {
    public:
        features::Protractor protractor;
        bool seized = false;
        std::optional<input::Sample> project(const input::Sample& sample, double tolerance) const override;
        std::optional<input::Sample> capture(const input::Sample& sample, double tolerance) override;
        input::Sample follow(const input::Sample& sample) override;
    };

    // Compas vu par GuideSet, comme ArcGuide : le guide de features et la marque « saisi au dernier appui ».
    class CompassGuide final : public features::StrokeGuide
    {
    public:
        features::Compass compass;
        bool seized = false;
        std::optional<input::Sample> project(const input::Sample& sample, double tolerance) const override;
        std::optional<input::Sample> capture(const input::Sample& sample, double tolerance) override;
        input::Sample follow(const input::Sample& sample) override;
    };

    struct Entry
    {
        QString command; // identifiant de la commande qui l'ouvre (UserState::setOpenMathTools)
        input::UserId owner = input::kFirstUser; // qui l'a ouvert : seul son trait est guidé en mode à deux
        std::unique_ptr<features::Ruler> ruler;  // exactement l'un des quatre
        std::unique_ptr<features::SetSquare> square;
        std::unique_ptr<ArcGuide> protractor;
        std::unique_ptr<CompassGuide> compass; // pointe, rayon, angle et miroir : dans features::Compass
        QPointF vertex;                        // équerre : sommet de l'angle droit ; rapporteur : centre
        double side = 0;  // équerre : côté ; rapporteur : rayon ; compas : longueur des bras
        double angle = 0; // équerre, rapporteur : degrés
        features::Protractor::Kind shape = features::Protractor::Kind::Full;
        features::ArcStyle arcStyle = features::ArcStyle::Arc;
        // Publié à chaque degré entier : rapporteur, angle balayé ; compas, angle du bras (la mine suit le
        // stylo).
        int sweptShown = 0;
        bool showAngle = true;
        bool showRadius = true;  // compas
        bool markCentre = false; // compas
        features::StrokeGuide* guide() const noexcept;
        const char* settingsGroup() const noexcept;
    };

    // Ce qu'un outil ajoute après le trait guidé qu'il a tenu (rayons ou secteur d'un rapporteur fermé ou
    // plein, croix du centre d'un compas) : armé au relâché, complété par le trait que le stylo verse
    // aussitôt (observateur du document), inséré ensuite.
    struct PendingAddition
    {
        enum class Kind
        {
            Arc,
            Cross
        };
        Kind kind = Kind::Arc;
        QPointF center; // arc : centre du cercle ; croix : pointe du compas
        double radius = 0;
        double start = 0; // degrés de page (repère de features::arcPath)
        double sweep = 0; // avec son sens
        features::ArcStyle style = features::ArcStyle::Arc;
        core::ObjectId stroke = 0; // trait versé par le stylo : couleur et épaisseur lues à l'insertion
        std::size_t page = 0;
    };

    // Tous les outils d'un utilisateur et la seule mine de ses compas (la gomme) : un jeu de guides par
    // utilisateur en mode à deux, parce que GuideSet retient le guide saisi d'un seul trait à la fois.
    struct UserGuides
    {
        features::GuideSet all;
        features::GuideSet compass;
    };

    bool valid(int index) const noexcept;
    bool dualUsers() const noexcept;
    // L'identifiant tel que ToolHost le range : le second seulement en mode à deux (ToolHost::user).
    input::UserId resolveUser(int user) const;
    // L'outil répond au trait de `user` : le sien en mode à deux, tout outil sinon.
    bool guidesUser(const Entry& entry, input::UserId user) const noexcept;
    UserGuides& userGuides(input::UserId user) noexcept;
    QSizeF pageSize() const;
    int add(Entry entry, const QString& kind);
    void updateFilter();
    // Guides que suit l'outil courant de l'utilisateur : stylo et marqueur, tous (F-280 « avec Stylo ou
    // Marqueur ») ; gomme, la mine du compas seule (F-282 « Stylo, marqueur ou gomme près de la mine ») ;
    // sélection et autres outils, aucun (L7-T34).
    enum class GuidedBy
    {
        All,
        Compass,
        None
    };
    GuidedBy guidedBy(input::UserId user) const;
    // Rapporteur ou compas qui tient le trait de `user` (saisi à l'appui), sinon nul.
    Entry* seizedProtractor(input::UserId user) noexcept;
    Entry* seizedCompass(input::UserId user) noexcept;
    Entry* compassEntry(int index) noexcept; // compas de l'index, sinon nul
    // poursInk : l'outil verse un trait (stylo, marqueur) ; la gomme n'arme aucune croix.
    void followCompass(Entry& entry, input::Phase phase, bool poursInk);
    static void storeCompassChoice(const Entry& entry);
    static void applyProtractorFrame(Entry& entry);
    static void storeProtractorChoice(const Entry& entry);
    void watchStrokes();
    void insertAddition(const PendingAddition& pending);
    void syncTolerance();
    void storeOpenTools();
    void touch();
    // Relit l'origine de la page affichée dans le document ; appelée au changement de page, au remplacement
    // du document (documentReplaced) et après chaque modification (annuler, rétablir, trait).
    void refreshOrigin();
    void syncPivot(); // kRotationPivotProperty sur la vue, d'après originVisible et origin

    QPointer<PageView> m_view;
    QPointer<ToolHost> m_toolHost;
    QPointer<DocumentController> m_document;
    int m_strokeObserver = 0;
    std::optional<PendingAddition> m_pending;
    QMetaObject::Connection m_zoomConnection;
    bool m_filterInstalled = false;
    std::vector<Entry> m_tools;
    // Tous les outils : ce que suit le seul utilisateur (mode à un), et l'aperçu d'accroche.
    features::GuideSet m_guides;
    features::GuideSet m_compassGuides;     // la mine des compas seule : ce que suit la gomme
    std::array<UserGuides, 2> m_userGuides; // mode à deux, par utilisateur
    int m_revision = 0;
    bool m_originVisible = false;
    QPointF m_origin;
    int m_originIndex = -1; // page affichée à la dernière lecture de l'origine
    QMetaObject::Connection m_originConnections[3];
};

} // namespace actcraft::ui
