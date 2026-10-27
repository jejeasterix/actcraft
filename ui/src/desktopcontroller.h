#pragma once

#include <actcraft/ui/documentcontroller.h>

#include <QImage>
#include <QObject>
#include <QPointer>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QRect>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <cstddef>
#include <functional>
#include <vector>

namespace actcraft::ui {

// Annotation du bureau (F-070) et fonds de bureau (F-071), L7-T30.
//
// Le contrôleur POSSÈDE le paperboard du bureau (`desktopDocument`) : un DocumentController à part, créé une
// fois et gardé d'une activation à l'autre (P14), jamais le document ouvert (`openDocument`, celui que
// Presentation.qml passe à la racine) ; ses pages sont transparentes (P13). Il crée lui-même la fenêtre
// d'annotation depuis le Component de la racine : beginCreate, format à canal alpha (explicite, demandé par
// 7b ; setColor(Qt::transparent) le demande aussi) et couleur transparente posés sur la QQuickWindow avant sa
// création native, completeCreate, puis show. À chaque ouverture, les pages prennent la taille de l'écran. Un
// trait en cours est versé sur sa page avant tout changement de page et avant la fermeture (comme Main.qml).
// Fenêtre fermée et détruite à la désactivation ; le paperboard reste.
//
// Outils de bureau (F-080), L7-T31 : `showTools` réduit la fenêtre principale (`mainWindow`) et ouvre la roue
// (`toolsShown`, fenêtre de DesktopToolsWheel.qml) ; `restore` rend la fenêtre principale et ferme la roue,
// qui se ferme aussi si la fenêtre est restaurée autrement. Pendant l'annotation, la fenêtre principale est
// réduite à l'emprise de la palette (état normal, barre du haut au-dessus) et gardée devant (drapeau
// « toujours devant ») : le paperboard disparaît, la palette reste. À la fermeture, visibilité, géométrie et
// drapeaux d'avant sont rendus. Les entrées de la roue viennent de features::defaultWheel(), chacune une
// commande nommée.
//
// Fonds de bureau : la capture (image figée, ressource PNG du document, une étape) vise la page affichée du
// paperboard actif (celui du bureau pendant l'annotation, sinon le document ouvert) ; nos fenêtres visibles
// sont masquées le temps de la saisie puis rendues dans leur état (la capture ne montre rien d'ActCraft) ; la
// superposition
// (bureau vivant) est un état de la vue, porté par UNE page du paperboard du bureau (P22), jamais écrit dans
// le document : enregistrer écrit le fond d'origine.
class DesktopController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool annotating READ isAnnotating WRITE setAnnotating NOTIFY annotatingChanged)
    Q_PROPERTY(bool passThrough READ passThrough WRITE setPassThrough NOTIFY passThroughChanged)
    Q_PROPERTY(actcraft::ui::DocumentController* desktopDocument READ desktopDocument CONSTANT)
    Q_PROPERTY(actcraft::ui::DocumentController* openDocument READ openDocument WRITE setOpenDocument NOTIFY
                   openDocumentChanged)
    Q_PROPERTY(QQmlComponent* windowComponent READ windowComponent WRITE setWindowComponent NOTIFY
                   windowComponentChanged)
    Q_PROPERTY(QQuickWindow* window READ window NOTIFY windowChanged)
    Q_PROPERTY(bool liveDesktopBackground READ liveDesktopBackground NOTIFY liveDesktopBackgroundChanged)
    Q_PROPERTY(QQuickWindow* mainWindow READ mainWindow WRITE setMainWindow NOTIFY mainWindowChanged)
    Q_PROPERTY(bool toolsShown READ toolsShown NOTIFY toolsShownChanged)

public:
    // Image du bureau ; par défaut, l'écran principal (QScreen::grabWindow). Les tests en injectent une.
    using CaptureSource = std::function<QImage()>;

    explicit DesktopController(QObject* parent = nullptr);
    ~DesktopController() override; // détruit la fenêtre d'annotation avant le paperboard qu'elle montre

    bool isAnnotating() const noexcept;
    void setAnnotating(bool annotating); // ouvre ou ferme la fenêtre ; le paperboard reste
    bool passThrough() const noexcept;
    void setPassThrough(bool passThrough); // Qt::WindowTransparentForInput sur la fenêtre d'annotation
    DocumentController* desktopDocument() const noexcept;
    DocumentController* openDocument() const noexcept;
    void setOpenDocument(DocumentController* document);
    QQmlComponent* windowComponent() const noexcept;
    void setWindowComponent(QQmlComponent* component);
    QQuickWindow* window() const noexcept; // nul hors annotation
    // Vrai si la page affichée du paperboard du bureau porte la superposition (P22).
    bool liveDesktopBackground() const noexcept;

    QQuickWindow* mainWindow() const noexcept;
    void setMainWindow(QQuickWindow* window);
    bool toolsShown() const noexcept;

    void setCaptureSource(CaptureSource source);

    Q_INVOKABLE void toggle();
    // P13 : insère après la page affichée une page transparente vierge de sa taille (une étape) et l'affiche.
    Q_INVOKABLE void nextPage();
    Q_INVOKABLE void pageBack();
    // Fond « capture du bureau » de la page affichée du paperboard actif ; faux si aucune image.
    Q_INVOKABLE bool captureBackground();
    // Superposition du bureau sur la page affichée du paperboard du bureau (ouvre l'annotation au besoin).
    Q_INVOKABLE void toggleLiveDesktopBackground();

    // F-080 : réduit la fenêtre principale et ouvre la roue ; sans fenêtre principale, rien.
    Q_INVOKABLE void showTools();
    // Rend la fenêtre principale et ferme la roue.
    Q_INVOKABLE void restore();
    // Entrées de la roue, prêtes pour QML : {command, label, x, y, ring: [...]}, positions relatives au
    // centre, sens horaire depuis le haut ; les entrées d'un anneau secondaire sont sur le cercle extérieur.
    Q_INVOKABLE QVariantList wheelEntries(double radius, double ringRadius) const;

signals:
    void mainWindowChanged();
    void toolsShownChanged();
    void annotatingChanged();
    void passThroughChanged();
    void openDocumentChanged();
    void windowComponentChanged();
    void windowChanged();
    void liveDesktopBackgroundChanged();

private:
    bool openWindow(); // faux sans Component prêt ou s'il ne rend pas une QQuickWindow
    void closeWindow();
    void finishStroke();     // trait en cours de la vue de la fenêtre, versé sur sa page
    void fitPagesToScreen(); // toutes les pages à la taille de l'écran principal, en une étape, si besoin
    QImage grabWithoutOurWindows();
    Qt::WindowFlags windowFlags() const noexcept;
    DocumentController* activeDocument() const noexcept;
    void
    followPages(const core::Changes& changes); // indices des pages superposées après un ajout ou un retrait
    void updateLive();
    void keepMainInFront();    // palette principale devant l'annotation
    void releaseMainFront();   // rend la fenêtre principale dans l'état d'avant l'annotation
    QRect paletteRect() const; // emprise de la palette à l'écran (avec la barre du haut), nulle sans palette
    void setToolsShown(bool shown);
    void onMainVisibilityChanged();

    DocumentController* m_desktop = nullptr; // enfant : vit autant que le contrôleur
    QPointer<DocumentController> m_open;
    QPointer<QQmlComponent> m_component;
    QPointer<QQuickWindow> m_window;
    bool m_annotating = false;
    bool m_passThrough = false;
    bool m_live = false;
    QPointer<QQuickWindow> m_main;
    bool m_toolsShown = false;
    // Fenêtre principale avant l'annotation : visibilité, géométrie normale et drapeaux, rendus à la
    // fermeture.
    struct MainState
    {
        bool saved = false;
        QWindow::Visibility visibility = QWindow::Windowed;
        QRect geometry;
        Qt::WindowFlags flags;
    };
    MainState m_mainState;
    bool m_hidingForCapture = false;      // fenêtre masquée pour la saisie : ce n'est pas une fermeture
    std::vector<std::size_t> m_livePages; // pages du paperboard du bureau qui portent la superposition
    CaptureSource m_capture;
};

} // namespace actcraft::ui
