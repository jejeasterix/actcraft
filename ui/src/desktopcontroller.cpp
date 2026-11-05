#include "desktopcontroller.h"

#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>
#include <actcraft/features/desktopwheel.h>
#include <actcraft/ui/pageview.h>

#include <QBuffer>
#include <QByteArray>
#include <QGuiApplication>
#include <QIODevice>
#include <QMetaObject>
#include <QPixmap>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QScreen>
#include <QSizeF>
#include <QSurfaceFormat>
#include <QTimer>
#include <QUrl>
#include <QVariantMap>
#include <QWindow>
#include <QtQml/qqml.h>

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace actcraft::ui {

namespace {

constexpr core::Argb kTransparent = 0x00000000u;

// Page vierge du paperboard du bureau : fond transparent, le bureau se voit à travers (P13).
core::Page transparentPage(double width, double height)
{
    core::Page page;
    page.width = width;
    page.height = height;
    page.background.fill = core::solidFill(kTransparent);
    return page;
}

core::Change pageChange(core::Change::Kind kind, std::size_t page)
{
    return core::Change{kind, page, core::ObjectId{0}, core::RectF{}};
}

// P13 : une page donnée, insérée à `index`, en une étape.
class InsertPageCommand final : public core::UndoCommand
{
public:
    InsertPageCommand(std::size_t index, core::Page page) : m_index(index), m_page(std::move(page)) {}

    void redo(core::Document& document, core::Changes& changes) override
    {
        if (m_index > document.pageCount()) {
            return;
        }
        document.insertPage(m_index, m_page);
        changes.push_back(pageChange(core::Change::Kind::PageInserted, m_index));
    }

    void undo(core::Document& document, core::Changes& changes) override
    {
        if (m_index >= document.pageCount()) {
            return;
        }
        m_page = document.takePage(m_index);
        changes.push_back(pageChange(core::Change::Kind::PageRemoved, m_index));
    }

private:
    std::size_t m_index;
    core::Page m_page;
};

// Fond « capture du bureau » (F-071) : la ressource PNG et l'image de fond de la page, en une étape. Annuler
// rend le fond d'avant et retire la ressource si cette commande l'avait ajoutée.
class DesktopCaptureCommand final : public core::UndoCommand
{
public:
    DesktopCaptureCommand(std::size_t page, QByteArray png) : m_page(page), m_png(std::move(png)) {}

    void redo(core::Document& document, core::Changes& changes) override
    {
        if (m_page >= document.pageCount()) {
            return;
        }
        const std::size_t resourcesBefore = document.resources().size();
        m_key = document.addResource(m_png, "png");
        m_added = document.resources().size() > resourcesBefore;
        core::Background& background = document.page(m_page).background;
        m_before = background;
        background.image = m_key;
        changes.push_back(pageChange(core::Change::Kind::PageChanged, m_page));
    }

    void undo(core::Document& document, core::Changes& changes) override
    {
        if (m_page >= document.pageCount()) {
            return;
        }
        document.page(m_page).background = m_before;
        if (m_added) {
            document.removeResource(m_key);
            m_added = false;
        }
        changes.push_back(pageChange(core::Change::Kind::PageChanged, m_page));
    }

private:
    std::size_t m_page;
    QByteArray m_png;
    std::string m_key;
    bool m_added = false;
    core::Background m_before;
};

// m6 : toutes les pages à une taille donnée (celle de l'écran), en une étape ; annuler rend les tailles
// d'avant.
class ResizePagesCommand final : public core::UndoCommand
{
public:
    ResizePagesCommand(double width, double height) : m_width(width), m_height(height) {}

    void redo(core::Document& document, core::Changes& changes) override
    {
        m_before.clear();
        for (std::size_t i = 0; i < document.pageCount(); ++i) {
            core::Page& page = document.page(i);
            m_before.emplace_back(page.width, page.height);
            page.width = m_width;
            page.height = m_height;
            changes.push_back(pageChange(core::Change::Kind::PageChanged, i));
        }
    }

    void undo(core::Document& document, core::Changes& changes) override
    {
        for (std::size_t i = 0; i < document.pageCount() && i < m_before.size(); ++i) {
            core::Page& page = document.page(i);
            page.width = m_before[i].first;
            page.height = m_before[i].second;
            changes.push_back(pageChange(core::Change::Kind::PageChanged, i));
        }
    }

private:
    double m_width;
    double m_height;
    std::vector<std::pair<double, double>> m_before;
};

// Une fenêtre masquée quitte l'écran à l'image suivante du compositeur, après l'éventuelle transition de
// Windows : la saisie de l'écran attend ce délai (captureBackground vient de masquer nos fenêtres), par une
// minuterie, jamais par un sommeil du fil principal (M4).
constexpr int kHiddenWindowsSettleMs = 250;

// Outils de la roue qui s'ouvrent dans la fenêtre principale : panneaux de l'horloge, des dés et de la
// calculatrice, message défilant et son dialogue. ÉCART À L'ORIGINAL (hypothèse H-M8, à revoir à la porte de
// clôture) : l'original montre ces outils sur le bureau, sa fenêtre restant cachée. Ici, ce sont des éléments
// de la fenêtre principale, invisibles tant qu'elle est réduite par la roue : l'entrée la rend d'abord
// (desktop.restore), puis lance l'outil. Les montrer sur le bureau demanderait une fenêtre propre à chacun.
bool opensInTheMainWindow(const QString& command)
{
    for (const char* id : {"tool.ticker", "tool.clock", "tool.calculator", "tool.dice"}) {
        if (command == QLatin1String(id)) {
            return true;
        }
    }
    return false;
}

QByteArray pngOf(const QImage& image)
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG")) {
        return {};
    }
    return bytes;
}

QImage grabPrimaryScreen()
{
    QScreen* screen = QGuiApplication::primaryScreen();
    return screen ? screen->grabWindow(0).toImage() : QImage();
}

QSizeF primaryScreenSize()
{
    const QScreen* screen = QGuiApplication::primaryScreen();
    return screen ? QSizeF(screen->geometry().size()) : QSizeF(1024, 768);
}

} // namespace

DesktopController::DesktopController(QObject* parent)
    : QObject(parent), m_desktop(new DocumentController(this)), m_capture(grabPrimaryScreen)
{
    m_desktop->setObjectName(QStringLiteral("desktopDocument"));
    const QSizeF size = primaryScreenSize();
    m_desktop->replaceDocument(
        core::Document(std::vector<core::Page>{transparentPage(size.width(), size.height())}), QUrl(),
        tr("Annotation du bureau"));
    m_desktop->addChangeObserver(
        [this](const core::Changes& changes, DocumentController::ChangeOrigin) { followPages(changes); });
    connect(m_desktop, &DocumentController::pagesChanged, this, &DesktopController::updateLive);
    // Comme Main.qml pour le document ouvert : page suivante ou précédente pendant un trait (doigt sur la
    // barre, télécommande), le trait se termine et reste sur sa page.
    connect(m_desktop, &DocumentController::pageAboutToChange, this, &DesktopController::finishStroke);
    connect(m_desktop, &DocumentController::documentReplaced, this, [this] {
        m_livePages.clear();
        updateLive();
    });
}

DesktopController::~DesktopController()
{
    // La fenêtre montre le paperboard (enfant de ce contrôleur) : elle part la première.
    delete m_window.data();
}

bool DesktopController::isAnnotating() const noexcept
{
    return m_annotating;
}

void DesktopController::setAnnotating(bool annotating)
{
    if (m_annotating == annotating) {
        return;
    }
    if (annotating) {
        if (!openWindow()) {
            return;
        }
    } else {
        closeWindow();
    }
    m_annotating = annotating;
    if (annotating) {
        keepMainInFront(); // après le show de la fenêtre d'annotation : la principale la dépasse
    } else {
        releaseMainFront();
    }
    emit annotatingChanged();
}

bool DesktopController::passThrough() const noexcept
{
    return m_passThrough;
}

void DesktopController::setPassThrough(bool passThrough)
{
    if (m_passThrough == passThrough) {
        return;
    }
    m_passThrough = passThrough;
    if (m_window) {
        m_window->setFlags(windowFlags());
    }
    emit passThroughChanged();
}

void DesktopController::followTool(const QString& toolId)
{
    if (m_annotating) {
        setPassThrough(toolId == QStringLiteral("select"));
    }
}

DocumentController* DesktopController::desktopDocument() const noexcept
{
    return m_desktop;
}

DocumentController* DesktopController::openDocument() const noexcept
{
    return m_open.data();
}

void DesktopController::setOpenDocument(DocumentController* document)
{
    if (m_open == document) {
        return;
    }
    m_open = document;
    emit openDocumentChanged();
}

QQmlComponent* DesktopController::windowComponent() const noexcept
{
    return m_component.data();
}

void DesktopController::setWindowComponent(QQmlComponent* component)
{
    if (m_component == component) {
        return;
    }
    m_component = component;
    emit windowComponentChanged();
}

QQuickWindow* DesktopController::window() const noexcept
{
    return m_window.data();
}

bool DesktopController::liveDesktopBackground() const noexcept
{
    return m_live;
}

QQuickWindow* DesktopController::mainWindow() const noexcept
{
    return m_main.data();
}

void DesktopController::setMainWindow(QQuickWindow* window)
{
    if (m_main == window) {
        return;
    }
    if (m_main) {
        m_main->disconnect(this);
    }
    m_main = window;
    if (m_main) {
        connect(m_main, &QWindow::visibilityChanged, this, &DesktopController::onMainVisibilityChanged);
    }
    emit mainWindowChanged();
}

bool DesktopController::toolsShown() const noexcept
{
    return m_toolsShown;
}

void DesktopController::setToolsShown(bool shown)
{
    if (m_toolsShown == shown) {
        return;
    }
    m_toolsShown = shown;
    emit toolsShownChanged();
}

void DesktopController::showTools()
{
    if (!m_main) {
        return;
    }
    if (!m_toolsShown) {
        // M5 : état à rendre au retour. Pendant l'annotation, la palette (état normal) ; si l'annotation se
        // ferme roue ouverte, releaseMainFront y met l'état d'avant l'annotation.
        const QWindow::Visibility visibility = m_main->visibility();
        const bool full = visibility == QWindow::FullScreen || visibility == QWindow::Maximized;
        m_wheelReturn = !m_annotating && full ? visibility : QWindow::Windowed;
    }
    setToolsShown(true);
    m_main->showMinimized();
}

void DesktopController::restore()
{
    const QWindow::Visibility back = m_toolsShown && !m_annotating ? m_wheelReturn : QWindow::Windowed;
    m_wheelReturn = QWindow::Windowed;
    setToolsShown(false);
    if (m_main) {
        if (back == QWindow::FullScreen) {
            m_main->showFullScreen();
        } else if (back == QWindow::Maximized) {
            m_main->showMaximized();
        } else {
            m_main->showNormal();
        }
        m_main->requestActivate();
    }
    keepMainInFront();
}

void DesktopController::onMainVisibilityChanged()
{
    // La fenêtre principale revient (autrement que par restore) : la roue n'a plus lieu d'être. Une saisie
    // d'écran la masque et la rend le temps d'une capture : ce n'est pas un retour.
    if (!m_toolsShown || m_hidingForCapture || !m_main) {
        return;
    }
    const QWindow::Visibility visibility = m_main->visibility();
    if (visibility == QWindow::Windowed || visibility == QWindow::Maximized ||
        visibility == QWindow::FullScreen) {
        m_wheelReturn = QWindow::Windowed;
        setToolsShown(false);
        keepMainInFront();
    }
}

void DesktopController::keepMainInFront()
{
    // Fenêtre principale réduite (roue ouverte) : la lever la restaurerait.
    if (!m_annotating || !m_main || m_toolsShown) {
        return;
    }
    if (!m_mainState.saved) {
        // Première fois de cette annotation : l'état d'avant est gardé, puis la fenêtre est réduite à
        // l'emprise de la palette (le paperboard disparaît, seule la palette reste), toujours devant.
        const QRect target = paletteRect(); // avant tout changement d'état : coordonnées d'aujourd'hui
        m_mainState.saved = true;
        m_mainState.visibility = m_main->visibility();
        m_mainState.flags = m_main->flags();
        if (m_mainState.visibility == QWindow::FullScreen || m_mainState.visibility == QWindow::Maximized) {
            m_main->showNormal(); // on part de l'état normal ; sa géométrie est celle à rendre
        }
        m_mainState.geometry = m_main->geometry();
        m_main->setFlag(Qt::WindowStaysOnTopHint, true);
        if (target.isValid()) {
            m_main->setGeometry(target);
        }
    }
    m_main->raise();
}

QRect DesktopController::paletteRect() const
{
    const auto* palette = m_main ? m_main->findChild<QQuickItem*>(QStringLiteral("toolPalette")) : nullptr;
    if (!palette || palette->width() <= 0 || palette->height() <= 0) {
        return {};
    }
    // La barre du haut garde sa place au-dessus de la palette (même mise en page dans la fenêtre réduite) :
    // la palette reste exactement où elle est à l'écran.
    const auto* topBar = m_main->findChild<QQuickItem*>(QStringLiteral("topBar"));
    const int extra = topBar ? static_cast<int>(topBar->height()) : 0;
    const QPoint inWindow = palette->mapToScene(QPointF(0, 0)).toPoint();
    const QPoint global = m_main->mapToGlobal(inWindow);
    return QRect(global.x(), global.y() - extra, static_cast<int>(palette->width()),
                 static_cast<int>(palette->height()) + extra);
}

void DesktopController::releaseMainFront()
{
    if (!m_mainState.saved) {
        return;
    }
    const MainState state = m_mainState;
    m_mainState = MainState{};
    if (!m_main) {
        return;
    }
    m_main->setFlags(state.flags);
    m_main->setGeometry(state.geometry);
    if (m_toolsShown) {
        // Fenêtre réduite par la roue : elle ne revient pas d'elle-même ; restore() rendra l'état d'avant
        // l'annotation (M5).
        m_wheelReturn = state.visibility;
        return;
    }
    if (state.visibility == QWindow::FullScreen) {
        m_main->showFullScreen();
    } else if (state.visibility == QWindow::Maximized) {
        m_main->showMaximized();
    }
}

QVariantList DesktopController::wheelEntries(double radius, double ringRadius) const
{
    // Entrées placées sur le cercle de rayon r ; leur anneau secondaire, sur le cercle extérieur (rayon
    // ringR), dont les propres anneaux restent sur ce même cercle.
    const auto toVariant = [](const std::vector<features::WheelEntry>& entries, double r, double ringR,
                              auto&& self) -> QVariantList {
        QVariantList list;
        const std::vector<QPointF> positions = features::wheelPositions(static_cast<int>(entries.size()), r);
        for (std::size_t i = 0; i < entries.size(); ++i) {
            QVariantMap entry;
            entry.insert(QStringLiteral("command"), entries[i].command);
            entry.insert(QStringLiteral("label"), entries[i].label);
            entry.insert(QStringLiteral("x"), positions[i].x());
            entry.insert(QStringLiteral("y"), positions[i].y());
            entry.insert(QStringLiteral("restoresMain"), opensInTheMainWindow(entries[i].command));
            entry.insert(QStringLiteral("ring"), self(entries[i].ring, ringR, ringR, self));
            list.push_back(entry);
        }
        return list;
    };
    return toVariant(features::defaultWheel(), radius, ringRadius, toVariant);
}

void DesktopController::setCaptureSource(CaptureSource source)
{
    m_capture = source ? std::move(source) : CaptureSource(grabPrimaryScreen);
}

void DesktopController::toggle()
{
    setAnnotating(!m_annotating);
}

void DesktopController::nextPage()
{
    finishStroke(); // avant la page insérée : le trait la précède dans l'historique
    const core::Document& document = m_desktop->document();
    const auto current = static_cast<std::size_t>(m_desktop->currentIndex());
    if (current >= document.pageCount()) {
        return;
    }
    if (current + 1 < document.pageCount()) {
        m_desktop->nextPage(); // une page suit déjà : on y va
        return;
    }
    // Dernière page : jamais DocumentController::nextPage seul (F-125, page au fond par défaut, opaque).
    const core::Page& shown = document.page(current);
    m_desktop->execute(
        std::make_unique<InsertPageCommand>(current + 1, transparentPage(shown.width, shown.height)));
    m_desktop->nextPage();
}

void DesktopController::pageBack()
{
    m_desktop->pageBack();
}

bool DesktopController::captureBackground()
{
    DocumentController* target = activeDocument();
    if (!target || !m_capture || m_hidingForCapture) {
        return false;
    }
    m_captureTarget = target;
    m_capturePage = static_cast<std::size_t>(target->currentIndex()); // la page affichée à la demande
    hideOurWindows();
    QTimer::singleShot(kHiddenWindowsSettleMs, this, &DesktopController::finishCapture);
    return true;
}

void DesktopController::finishCapture()
{
    const QImage image = m_capture ? m_capture() : QImage();
    showOurWindows();
    const QPointer<DocumentController> target = m_captureTarget;
    m_captureTarget = nullptr;
    QByteArray png = image.isNull() ? QByteArray() : pngOf(image);
    const bool done = target && !png.isEmpty();
    if (done) {
        target->execute(std::make_unique<DesktopCaptureCommand>(m_capturePage, std::move(png)));
    }
    emit backgroundCaptured(done);
}

void DesktopController::toggleLiveDesktopBackground()
{
    if (!m_annotating) {
        setAnnotating(true);
    }
    const auto page = static_cast<std::size_t>(m_desktop->currentIndex());
    const auto found = std::find(m_livePages.begin(), m_livePages.end(), page);
    if (found == m_livePages.end()) {
        m_livePages.push_back(page);
    } else {
        m_livePages.erase(found);
    }
    updateLive();
}

bool DesktopController::openWindow()
{
    if (m_window) {
        return true;
    }
    if (!m_component || !m_component->isReady()) {
        return false;
    }
    fitPagesToScreen(); // résolution changée depuis la dernière ouverture
    QQmlContext* context = m_component->creationContext();
    if (!context) {
        context = qmlContext(this);
    }
    QObject* object = m_component->beginCreate(context);
    auto* window = qobject_cast<QQuickWindow*>(object);
    if (!window) {
        if (object) {
            m_component->completeCreate();
            delete object;
        }
        return false;
    }
    QQmlEngine::setObjectOwnership(window, QQmlEngine::CppOwnership);
    // Avant la création native (completeCreate, show) : une surface sans canal alpha resterait opaque.
    QSurfaceFormat format = window->requestedFormat();
    format.setAlphaBufferSize(8);
    window->setFormat(format);
    window->setColor(Qt::transparent);
    window->setFlags(windowFlags());
    m_component->completeCreate();
    if (QScreen* screen = QGuiApplication::primaryScreen()) {
        window->setScreen(screen);
        window->setGeometry(screen->geometry());
    }
    // Fermée par le système (Alt+F4…) : l'annotation s'arrête, hors du traitement de l'événement.
    // (closeWindow se déconnecte avant de la cacher ; masquée pour une capture, elle reste ouverte.)
    connect(window, &QWindow::visibleChanged, this, [this](bool visible) {
        if (!visible && !m_hidingForCapture) {
            static_cast<void>(
                QMetaObject::invokeMethod(this, [this] { setAnnotating(false); }, Qt::QueuedConnection));
        }
    });
    m_window = window;
    emit windowChanged();
    window->show();
    return true;
}

void DesktopController::closeWindow()
{
    if (!m_window) {
        return;
    }
    finishStroke(); // la vue part avec la fenêtre : le trait en cours est versé d'abord
    QQuickWindow* window = m_window.data();
    m_window = nullptr;
    emit windowChanged();
    window->disconnect(this);
    m_desktop->setFixedPage(nullptr); // la page fixe part avec la fenêtre
    window->hide();
    window->deleteLater();
    setPassThrough(false);
}

void DesktopController::finishStroke()
{
    if (!m_window) {
        return;
    }
    if (auto* view = m_window->findChild<PageView*>(QStringLiteral("pageView"))) {
        view->finishStroke();
    }
}

void DesktopController::fitPagesToScreen()
{
    const QSizeF size = primaryScreenSize();
    const core::Document& document = m_desktop->document();
    for (std::size_t i = 0; i < document.pageCount(); ++i) {
        const core::Page& page = document.page(i);
        if (page.width != size.width() || page.height != size.height()) {
            m_desktop->execute(std::make_unique<ResizePagesCommand>(size.width(), size.height()));
            return;
        }
    }
}

void DesktopController::hideOurWindows()
{
    m_hidden.clear();
    m_focusBeforeCapture = QGuiApplication::focusWindow();
    m_hidingForCapture = true;
    const QWindowList windows = QGuiApplication::topLevelWindows();
    for (QWindow* window : windows) {
        if (window->isVisible()) {
            m_hidden.push_back({window, window->windowStates()});
            window->hide();
        }
    }
}

void DesktopController::showOurWindows()
{
    for (const HiddenWindow& entry : m_hidden) {
        if (!entry.window) {
            continue;
        }
        entry.window->setVisible(true);
        if (entry.window->windowStates() != entry.states) {
            entry.window->setWindowStates(entry.states);
        }
    }
    m_hidden.clear();
    if (m_focusBeforeCapture) {
        m_focusBeforeCapture->requestActivate();
    }
    m_focusBeforeCapture = nullptr;
    m_hidingForCapture = false;
}

Qt::WindowFlags DesktopController::windowFlags() const noexcept
{
    Qt::WindowFlags flags = Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint;
    if (m_passThrough) {
        flags |= Qt::WindowTransparentForInput;
    }
    return flags;
}

DocumentController* DesktopController::activeDocument() const noexcept
{
    return m_annotating ? m_desktop : m_open.data();
}

void DesktopController::followPages(const core::Changes& changes)
{
    for (const core::Change& change : changes) {
        if (change.kind == core::Change::Kind::PageInserted) {
            for (std::size_t& page : m_livePages) {
                if (page >= change.page) {
                    ++page;
                }
            }
        } else if (change.kind == core::Change::Kind::PageRemoved) {
            std::erase(m_livePages, change.page);
            for (std::size_t& page : m_livePages) {
                if (page > change.page) {
                    --page;
                }
            }
        }
    }
}

void DesktopController::updateLive()
{
    const auto page = static_cast<std::size_t>(m_desktop->currentIndex());
    const bool live = std::find(m_livePages.begin(), m_livePages.end(), page) != m_livePages.end();
    if (live == m_live) {
        return;
    }
    m_live = live;
    emit liveDesktopBackgroundChanged();
}

} // namespace actcraft::ui
