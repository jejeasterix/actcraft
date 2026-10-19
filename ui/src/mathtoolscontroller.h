#pragma once

#include <actcraft/features/mathtools.h>
#include <actcraft/features/strokeguide.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/toolhost.h>

#include <QObject>
#include <QPointF>
#include <QPointer>
#include <QSizeF>
#include <QString>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include <memory>
#include <vector>

namespace actcraft::ui {

// Outils de maths ouverts dans la fenêtre (F-280 ; équerre, rapporteur, compas, origine : L7-T10 à L7-T13).
// Une aide, pas un contenu : rien n'est écrit dans le document. Leurs bords guident l'encre par le filtre
// d'échantillons de la vue (PageView::setSampleFilter), appliqué avant le Pipeline, relâchés synthétisés
// compris (à partir du dernier point brut) : le stylo reçoit des échantillons déjà posés sur le bord et verse
// son trait comme tout trait ; ni le stylo ni la vue ne savent qu'une règle existe. Par fenêtre, pas par page
// : les outils restent en changeant de page. Coordonnées de page.
class MathToolsController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(actcraft::ui::PageView* view READ view WRITE setView NOTIFY viewChanged)
    Q_PROPERTY(actcraft::ui::ToolHost* toolHost READ toolHost WRITE setToolHost NOTIFY toolHostChanged)
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

public:
    static constexpr double kSnapScreenPixels = 16;

    explicit MathToolsController(QObject* parent = nullptr);
    ~MathToolsController() override;

    PageView* view() const noexcept;
    void setView(PageView* view);
    ToolHost* toolHost() const noexcept;
    void setToolHost(ToolHost* host);
    int count() const noexcept;
    int revision() const noexcept;
    static int defaultOpacity() noexcept;
    qreal guideBand() const noexcept;
    static qreal minimumLength() noexcept;
    static qreal maximumLength() noexcept;

    // Règle au centre de la page (longueur 600, largeur 80, angle 0 ; une suivante décalée de 24) ; son
    // index.
    Q_INVOKABLE int openRuler();
    // Équerre au centre de la page (côté 300, angle 0 : côtés de l'angle droit vers +x et +y) ; son index.
    Q_INVOKABLE int openSetSquare();
    // Ferme l'outil : les traits déjà versés restent (ils sont au document, l'outil n'en possède aucun).
    Q_INVOKABLE void close(int index);
    // Règle : centre, longueur, largeur, angle. Équerre : `center` est le sommet de l'angle droit, `length`
    // le côté, `thickness` est ignorée.
    Q_INVOKABLE void setFrame(int index, QPointF center, qreal length, qreal thickness, qreal angle);
    // Unités des deux bords : « cm », « mm », « inch », « none » ; gardées pour la règle suivante
    // (QSettings).
    Q_INVOKABLE void setUnits(int index, const QString& top, const QString& bottom);
    Q_INVOKABLE void setShowAngle(int index, bool on);
    // kind (« ruler » ou « setSquare »), command, centerX, centerY, length, thickness, angle, displayAngle,
    // topUnit, bottomUnit, showAngle ; vide hors des bornes. Équerre : centre = sommet de l'angle droit,
    // length = thickness = côté.
    Q_INVOKABLE QVariantMap toolAt(int index) const;
    // Point d'accroche sous `pagePoint` (cercle du survol) : {snapped, x, y} ; aucun changement d'état.
    Q_INVOKABLE QVariantMap snapAt(QPointF pagePoint) const;

signals:
    void viewChanged();
    void toolHostChanged();
    void countChanged();
    void revisionChanged();
    void guideBandChanged();
    // Le délégué de l'index est à créer (kind comme dans toolAt()) ou à retirer : la couche tient un modèle
    // par outil, sans recréer les délégués des autres.
    void toolOpened(int index, const QString& kind);
    void toolClosed(int index);

private:
    struct Entry
    {
        QString command; // identifiant de la commande qui l'ouvre (UserState::setOpenMathTools)
        std::unique_ptr<features::Ruler> ruler; // exactement l'un des deux
        std::unique_ptr<features::SetSquare> square;
        QPointF vertex;   // équerre : sommet de l'angle droit
        double side = 0;  // équerre : côté
        double angle = 0; // équerre : degrés
        bool showAngle = true;
        features::StrokeGuide* guide() const noexcept;
        const char* settingsGroup() const noexcept;
    };

    bool valid(int index) const noexcept;
    QSizeF pageSize() const;
    int add(Entry entry, const QString& kind);
    void updateFilter();
    // Les guides ne valent que pour le stylo (7a : « avec le stylo, un appui… accroche le trait ») ; la
    // sélection, le marqueur et la gomme passent tels quels (marqueur et gomme : L7-T34).
    bool guidesTool(input::UserId user) const;
    input::Sample guide(const input::Sample& sample);
    void syncTolerance();
    void storeOpenTools();
    void touch();

    QPointer<PageView> m_view;
    QPointer<ToolHost> m_toolHost;
    QMetaObject::Connection m_zoomConnection;
    bool m_filterInstalled = false;
    std::vector<Entry> m_tools;
    features::GuideSet m_guides;
    int m_revision = 0;
};

} // namespace actcraft::ui
