#pragma once

#include <actcraft/features/camera.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>

#include <QImage>
#include <QObject>
#include <QPointF>
#include <QPointer>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QRect>
#include <QRectF>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <functional>
#include <vector>

namespace actcraft::ui {

// Caméra (F-275), L7-T28 : cinq captures (zone, point par point, main levée, fenêtre, écran entier) et trois
// destinations (page affichée, nouvelle page, presse-papiers ; les ressources arrivent au lot 6, H7-3).
//
// Au déclenchement, après un court délai (le menu qui a lancé la commande quitte l'écran), l'écran qui porte
// la fenêtre principale est photographié. Image vide (Wayland, macOS sans autorisation d'enregistrer
// l'écran) : un message en français, aucune fenêtre. Sinon (sauf l'écran entier, posé tout de suite sur la
// page), le contrôleur crée LUI-MÊME la fenêtre de sélection depuis le Component de la racine, comme
// DesktopController (décision 4 bis) : beginCreate sans parent (avec un parent, la fenêtre prendrait la
// principale pour parent transitoire et se réduirait avec elle), sans cadre, toujours devant, sur l'écran
// photographié, à sa taille. La palette est un élément de CETTE fenêtre : jamais une couverture de la vue
// principale. Échap ferme la fenêtre. Les gestes (appui, glisser, relâché, survol) arrivent en coordonnées de
// la fenêtre, donc en pixels logiques de l'écran ; l'image figée est en pixels physiques.
//
// Destinations « page » : l'image découpée devient une ressource PNG du document et un objet image, à sa
// taille en pixels logiques, centré dans la vue, en une étape (DocumentInsertions::insertImage, comme la
// capture du spot). La zone d'une capture partielle est gardée pour la suivante (P11).
class CameraController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(actcraft::ui::DocumentController* documentController READ documentController WRITE
                   setDocumentController NOTIFY documentControllerChanged)
    Q_PROPERTY(actcraft::ui::PageView* pageView READ pageView WRITE setPageView NOTIFY pageViewChanged)
    Q_PROPERTY(QQuickWindow* mainWindow READ mainWindow WRITE setMainWindow NOTIFY mainWindowChanged)
    Q_PROPERTY(QQmlComponent* windowComponent READ windowComponent WRITE setWindowComponent NOTIFY
                   windowComponentChanged)
    Q_PROPERTY(QQuickWindow* window READ window NOTIFY windowChanged)
    Q_PROPERTY(int mode READ mode NOTIFY modeChanged)
    Q_PROPERTY(QRectF selection READ selection WRITE setSelection NOTIFY selectionChanged)
    Q_PROPERTY(QVariantList outline READ outline NOTIFY outlineChanged)
    Q_PROPERTY(bool ready READ isReady NOTIFY outlineChanged)
    Q_PROPERTY(QString frozenSource READ frozenSource NOTIFY frozenSourceChanged)
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)

public:
    enum Mode
    {
        NoMode,
        AreaMode,
        PolygonMode,
        FreehandMode,
        WindowMode,
        ScreenMode
    };
    Q_ENUM(Mode)

    enum Destination
    {
        CurrentPage,
        NewPage,
        Clipboard
    };
    Q_ENUM(Destination)

    // Image de l'écran ; par défaut, l'écran de la fenêtre principale (QScreen::grabWindow). Les tests en
    // injectent une (aucune capture réelle dans les tests automatiques).
    using CaptureSource = std::function<QImage()>;
    // Fenêtres du bureau en pixels logiques globaux ; par défaut, features::topLevelWindows().
    using WindowSource = std::function<std::vector<QRect>()>;

    explicit CameraController(QObject* parent = nullptr);
    ~CameraController() override;

    DocumentController* documentController() const noexcept;
    void setDocumentController(DocumentController* document);
    PageView* pageView() const noexcept;
    void setPageView(PageView* view);
    QQuickWindow* mainWindow() const noexcept;
    void setMainWindow(QQuickWindow* window);
    QQmlComponent* windowComponent() const noexcept;
    void setWindowComponent(QQmlComponent* component);
    QQuickWindow* window() const noexcept; // nul hors sélection

    int mode() const noexcept;
    QRectF selection() const noexcept; // zone ou fenêtre choisie, en coordonnées de la fenêtre de capture
    void setSelection(const QRectF& selection);
    QVariantList outline() const; // contour dessiné (points de la fenêtre de capture)
    bool isReady() const noexcept; // une forme est choisie : une destination peut la recevoir
    QString frozenSource() const;  // image figée, par le fournisseur d'images du moteur
    QString message() const;
    QImage frozenImage() const;

    void setCaptureSource(CaptureSource source);
    void setWindowSource(WindowSource source);

    // Commande tool.camera-* : photographie l'écran après le délai, puis ouvre la sélection (ou pose l'écran
    // entier sur la page). Faux si le mode est inconnu.
    Q_INVOKABLE bool start(int mode);
    Q_INVOKABLE void cancel();
    // Pose la forme choisie à la destination, puis ferme la fenêtre ; faux si rien n'est posé.
    Q_INVOKABLE bool deliver(int destination);
    Q_INVOKABLE void clearMessage();

    // Gestes de la surface de sélection, en coordonnées de la fenêtre de capture.
    Q_INVOKABLE void press(QPointF point);
    Q_INVOKABLE void move(QPointF point);
    Q_INVOKABLE void release(QPointF point);
    Q_INVOKABLE void hover(QPointF point);

signals:
    void documentControllerChanged();
    void pageViewChanged();
    void mainWindowChanged();
    void windowComponentChanged();
    void windowChanged();
    void modeChanged();
    void selectionChanged();
    void outlineChanged();
    void frozenSourceChanged();
    void messageChanged();

private:
    enum class Drag
    {
        None,
        Draw,
        Move,
        Resize
    };

    void beginCapture();
    bool openWindow();
    void closeWindow();
    void setMode(Mode mode);
    void setMessage(const QString& message);
    void setFrozen(const QImage& image);
    QRectF bounds() const;
    QRectF clamped(const QRectF& rect) const;
    QRectF defaultArea() const;
    QRectF windowAt(QPointF point) const;
    QImage cropped() const;
    bool insertOnPage(const QImage& image, bool newPage);

    QPointer<DocumentController> m_document;
    QPointer<PageView> m_view;
    QPointer<QQuickWindow> m_main;
    QPointer<QQmlComponent> m_component;
    QPointer<QQuickWindow> m_window;
    CaptureSource m_capture;
    WindowSource m_windowSource;
    QTimer m_settle;

    Mode m_mode = NoMode;
    Mode m_pending = NoMode;
    QImage m_frozen;
    int m_serial = 0;
    QRect m_screen;              // écran photographié, pixels logiques globaux
    std::vector<QRectF> m_windows; // fenêtres du bureau, coordonnées de la fenêtre de capture
    QRectF m_selection;
    QRectF m_lastArea; // P11 : zone gardée d'une capture à l'autre
    features::PolygonPicker m_polygon;
    features::FreehandPicker m_freehand;
    Drag m_drag = Drag::None;
    QPointF m_dragStart;
    QRectF m_dragOrigin;
    QPointF m_anchor; // coin opposé pendant un redimensionnement
    QString m_message;
};

} // namespace actcraft::ui
