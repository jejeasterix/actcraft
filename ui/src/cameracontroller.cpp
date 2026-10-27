#include "cameracontroller.h"

#include "presentationdocument.h"

#include <actcraft/core/model.h>

#include <QBuffer>
#include <QByteArray>
#include <QClipboard>
#include <QGuiApplication>
#include <QIODevice>
#include <QLineF>
#include <QMetaObject>
#include <QMutex>
#include <QMutexLocker>
#include <QPixmap>
#include <QPolygonF>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickImageProvider>
#include <QScreen>
#include <QSize>
#include <QSizeF>
#include <QtQml/qqml.h>

#include <array>
#include <utility>

namespace actcraft::ui {

namespace {

// Délai avant la photographie : le menu (ou la roue) qui a lancé la commande quitte l'écran.
constexpr int kSettleMs = 200;
// Poignée d'angle d'une zone : un appui à moins de cette distance d'un coin la redimensionne.
constexpr qreal kHandlePixels = 16;
// Zone tracée plus petite (clic sans glisser) : la zone d'avant reste.
constexpr qreal kMinimumSide = 4;

QString providerId()
{
    return QStringLiteral("actcraft-camera");
}

// Image figée montrée par la fenêtre de capture (Image { source: frozenSource }) : une copie partagée, lue
// sur le fil de chargement des images.
class FrozenScreenImages final : public QQuickImageProvider
{
public:
    FrozenScreenImages() : QQuickImageProvider(QQuickImageProvider::Image) {}

    void setImage(const QImage& image)
    {
        const QMutexLocker lock(&m_mutex);
        m_image = image;
    }

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override
    {
        Q_UNUSED(id);
        QImage image;
        {
            const QMutexLocker lock(&m_mutex);
            image = m_image;
        }
        if (size) {
            *size = image.size();
        }
        if (!image.isNull() && requestedSize.isValid() && requestedSize != image.size()) {
            image = image.scaled(requestedSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }
        return image;
    }

private:
    QMutex m_mutex;
    QImage m_image;
};

// Fournisseur du moteur qui a créé `owner` (créé au besoin) ; nul hors QML.
FrozenScreenImages* frozenImagesOf(QObject* owner, bool create)
{
    QQmlEngine* engine = qmlEngine(owner);
    if (!engine) {
        return nullptr;
    }
    auto* images = dynamic_cast<FrozenScreenImages*>(engine->imageProvider(providerId()));
    if (!images && create) {
        images = new FrozenScreenImages;
        engine->addImageProvider(providerId(), images); // le moteur le possède
    }
    return images;
}

} // namespace

CameraController::CameraController(QObject* parent) : QObject(parent)
{
    setCaptureSource(nullptr);
    setWindowSource(nullptr);
    m_settle.setSingleShot(true);
    connect(&m_settle, &QTimer::timeout, this, &CameraController::beginCapture);
}

CameraController::~CameraController()
{
    delete m_window.data();
}

DocumentController* CameraController::documentController() const noexcept
{
    return m_document.data();
}

void CameraController::setDocumentController(DocumentController* document)
{
    if (m_document == document) {
        return;
    }
    m_document = document;
    emit documentControllerChanged();
}

PageView* CameraController::pageView() const noexcept
{
    return m_view.data();
}

void CameraController::setPageView(PageView* view)
{
    if (m_view == view) {
        return;
    }
    m_view = view;
    emit pageViewChanged();
}

QQuickWindow* CameraController::mainWindow() const noexcept
{
    return m_main.data();
}

void CameraController::setMainWindow(QQuickWindow* window)
{
    if (m_main == window) {
        return;
    }
    m_main = window;
    emit mainWindowChanged();
}

QQmlComponent* CameraController::windowComponent() const noexcept
{
    return m_component.data();
}

void CameraController::setWindowComponent(QQmlComponent* component)
{
    if (m_component == component) {
        return;
    }
    m_component = component;
    emit windowComponentChanged();
}

QQuickWindow* CameraController::window() const noexcept
{
    return m_window.data();
}

int CameraController::mode() const noexcept
{
    return m_mode;
}

QRectF CameraController::selection() const noexcept
{
    return m_selection;
}

void CameraController::setSelection(const QRectF& selection)
{
    if (m_selection == selection) {
        return;
    }
    m_selection = selection;
    if (m_mode == AreaMode && selection.width() >= kMinimumSide && selection.height() >= kMinimumSide) {
        m_lastArea = selection; // P11
    }
    emit selectionChanged();
    emit outlineChanged();
}

QVariantList CameraController::outline() const
{
    QPolygonF shape;
    switch (m_mode) {
    case AreaMode:
    case WindowMode:
        if (!m_selection.isEmpty()) {
            shape = QPolygonF(m_selection); // fermé : cinq points
        }
        break;
    case PolygonMode:
        shape = m_polygon.polygon();
        if (m_polygon.closed() && !shape.isEmpty()) {
            shape << shape.first();
        }
        break;
    case FreehandMode:
        shape = m_freehand.polygon();
        break;
    default:
        break;
    }
    QVariantList points;
    points.reserve(shape.size());
    for (const QPointF& point : std::as_const(shape)) {
        points.push_back(point);
    }
    return points;
}

bool CameraController::isReady() const noexcept
{
    switch (m_mode) {
    case AreaMode:
    case WindowMode:
        return m_selection.width() >= 1 && m_selection.height() >= 1;
    case PolygonMode:
        return m_polygon.closed();
    case FreehandMode:
        return m_freehand.closed();
    case ScreenMode:
        return !m_frozen.isNull();
    default:
        return false;
    }
}

QString CameraController::frozenSource() const
{
    if (m_frozen.isNull()) {
        return {};
    }
    return QStringLiteral("image://%1/frozen-%2").arg(providerId()).arg(m_serial);
}

QString CameraController::message() const
{
    return m_message;
}

QImage CameraController::frozenImage() const
{
    return m_frozen;
}

void CameraController::setCaptureSource(CaptureSource source)
{
    if (source) {
        m_capture = std::move(source);
        return;
    }
    m_capture = [this] {
        // L'écran qui porte la fenêtre principale.
        QScreen* screen = m_main && m_main->screen() ? m_main->screen() : QGuiApplication::primaryScreen();
        return screen ? screen->grabWindow(0).toImage() : QImage();
    };
}

void CameraController::setWindowSource(WindowSource source)
{
    m_windowSource = source ? std::move(source) : WindowSource(features::topLevelWindows);
}

bool CameraController::start(int mode)
{
    if (mode < AreaMode || mode > ScreenMode) {
        return false;
    }
    closeWindow(); // une sélection ouverte cède la place
    m_pending = static_cast<Mode>(mode);
    m_settle.start(kSettleMs);
    return true;
}

void CameraController::cancel()
{
    m_settle.stop();
    m_pending = NoMode;
    closeWindow();
}

bool CameraController::deliver(int destination)
{
    if (!isReady()) {
        return false;
    }
    const QImage image = cropped();
    if (image.isNull()) {
        return false;
    }
    bool done = false;
    if (destination == Clipboard) {
        if (QClipboard* clipboard = QGuiApplication::clipboard()) {
            clipboard->setImage(image);
            done = true;
        }
    } else if (destination == CurrentPage || destination == NewPage) {
        done = insertOnPage(image, destination == NewPage);
    }
    if (done) {
        closeWindow();
    }
    return done;
}

void CameraController::clearMessage()
{
    setMessage(QString());
}

void CameraController::press(QPointF point)
{
    switch (m_mode) {
    case AreaMode: {
        m_dragStart = point;
        m_dragOrigin = m_selection;
        m_drag = Drag::Draw;
        const QRectF r = m_selection;
        if (!r.isEmpty()) {
            const std::array<std::pair<QPointF, QPointF>, 4> corners{{{r.topLeft(), r.bottomRight()},
                                                                      {r.topRight(), r.bottomLeft()},
                                                                      {r.bottomLeft(), r.topRight()},
                                                                      {r.bottomRight(), r.topLeft()}}};
            for (const auto& [corner, opposite] : corners) {
                if (QLineF(point, corner).length() <= kHandlePixels) {
                    m_drag = Drag::Resize;
                    m_anchor = opposite;
                    break;
                }
            }
            if (m_drag == Drag::Draw && r.contains(point)) {
                m_drag = Drag::Move;
            }
        }
        break;
    }
    case PolygonMode:
        if (m_polygon.closed()) {
            m_polygon.clear(); // un clic après la fermeture commence une autre forme
        }
        m_polygon.click(point);
        emit outlineChanged();
        break;
    case FreehandMode:
        m_freehand.press(point);
        emit outlineChanged();
        break;
    case WindowMode:
        setSelection(windowAt(point));
        break;
    default:
        break;
    }
}

void CameraController::move(QPointF point)
{
    switch (m_mode) {
    case AreaMode:
        if (m_drag == Drag::Draw) {
            setSelection(clamped(features::normalizedRect(m_dragStart, point)));
        } else if (m_drag == Drag::Resize) {
            setSelection(clamped(features::normalizedRect(m_anchor, point)));
        } else if (m_drag == Drag::Move) {
            const QRectF b = bounds();
            QRectF moved = m_dragOrigin.translated(point - m_dragStart);
            moved.moveLeft(qBound(b.left(), moved.left(), qMax(b.left(), b.right() - moved.width())));
            moved.moveTop(qBound(b.top(), moved.top(), qMax(b.top(), b.bottom() - moved.height())));
            setSelection(moved);
        }
        break;
    case FreehandMode:
        m_freehand.move(point);
        emit outlineChanged();
        break;
    case WindowMode:
        if (m_drag == Drag::None) {
            setSelection(windowAt(point)); // doigt glissé d'une fenêtre à l'autre
        }
        break;
    default:
        break;
    }
}

void CameraController::release(QPointF point)
{
    switch (m_mode) {
    case AreaMode:
        if (m_drag != Drag::None) {
            move(point);
            if (m_drag == Drag::Draw &&
                (m_selection.width() < kMinimumSide || m_selection.height() < kMinimumSide)) {
                setSelection(m_dragOrigin);
            }
        }
        m_drag = Drag::None;
        break;
    case FreehandMode:
        m_freehand.move(point);
        m_freehand.release();
        emit outlineChanged();
        break;
    default:
        break;
    }
}

void CameraController::hover(QPointF point)
{
    if (m_mode == WindowMode) {
        setSelection(windowAt(point)); // la fenêtre sous le pointeur est encadrée
    }
}

void CameraController::beginCapture()
{
    const Mode mode = std::exchange(m_pending, NoMode);
    if (mode == NoMode) {
        return;
    }
    QScreen* screen = m_main && m_main->screen() ? m_main->screen() : QGuiApplication::primaryScreen();
    m_screen = screen ? screen->geometry() : QRect();
    const QImage image = m_capture ? m_capture() : QImage();
    if (image.isNull() || m_screen.isEmpty()) {
        setMessage(
            tr("La caméra n'a reçu aucune image de l'écran. Vérifiez que l'enregistrement de l'écran est "
               "autorisé pour ActCraft, puis recommencez."));
        return;
    }
    clearMessage();
    setFrozen(image);
    if (mode == ScreenMode) {
        // Écran entier : capture immédiate, posée sur la page affichée.
        insertOnPage(m_frozen, false);
        setFrozen(QImage());
        return;
    }
    // Fenêtres du bureau, relevées avec l'image (avant notre fenêtre de sélection), en coordonnées de la
    // fenêtre de capture.
    m_windows.clear();
    if (mode == WindowMode && m_windowSource) {
        const QRectF b = bounds();
        for (const QRect& window : m_windowSource()) {
            const QRectF local = QRectF(window).translated(-QPointF(m_screen.topLeft())) & b;
            if (!local.isEmpty()) {
                m_windows.push_back(local);
            }
        }
    }
    m_polygon.clear();
    m_freehand.clear();
    m_drag = Drag::None;
    m_selection = QRectF();
    setMode(mode);
    if (mode == AreaMode) {
        const QRectF kept = clamped(m_lastArea);
        setSelection(kept.width() >= kMinimumSide && kept.height() >= kMinimumSide ? kept : defaultArea());
    }
    if (!openWindow()) {
        closeWindow();
    }
}

bool CameraController::openWindow()
{
    if (!m_component || !m_component->isReady()) {
        return false;
    }
    QQmlContext* context = m_component->creationContext();
    if (!context) {
        context = qmlContext(this);
    }
    // Sans parent : un Window enfant d'un élément prendrait la fenêtre principale pour parent transitoire
    // (réduit avec elle).
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
    window->setFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    m_component->completeCreate();
    QScreen* screen = m_main && m_main->screen() ? m_main->screen() : QGuiApplication::primaryScreen();
    if (screen) {
        window->setScreen(screen);
    }
    window->setGeometry(m_screen);
    // Fermée par le système (Alt+F4…) : la capture s'arrête, hors du traitement de l'événement (closeWindow
    // se déconnecte avant de la cacher).
    const QPointer<QQuickWindow> guard(window);
    connect(window, &QWindow::visibleChanged, this, [this, guard](bool visible) {
        if (!visible) {
            static_cast<void>(QMetaObject::invokeMethod(
                this,
                [this, guard] {
                    if (guard && m_window == guard) {
                        cancel();
                    }
                },
                Qt::QueuedConnection));
        }
    });
    m_window = window;
    emit windowChanged();
    window->show();
    window->requestActivate(); // Échap
    return true;
}

void CameraController::closeWindow()
{
    if (m_window) {
        QQuickWindow* window = m_window.data();
        m_window = nullptr;
        window->disconnect(this);
        window->hide();
        window->deleteLater();
        emit windowChanged();
    }
    m_drag = Drag::None;
    m_polygon.clear();
    m_freehand.clear();
    m_windows.clear();
    setMode(NoMode);
    setFrozen(QImage()); // l'image d'un écran entier ne reste pas en mémoire
}

void CameraController::setMode(Mode mode)
{
    if (m_mode == mode) {
        return;
    }
    m_mode = mode;
    emit modeChanged();
    emit outlineChanged();
}

void CameraController::setMessage(const QString& message)
{
    if (m_message == message) {
        return;
    }
    m_message = message;
    emit messageChanged();
}

void CameraController::setFrozen(const QImage& image)
{
    if (m_frozen.isNull() && image.isNull()) {
        return;
    }
    m_frozen = image;
    ++m_serial;
    if (FrozenScreenImages* images = frozenImagesOf(this, !image.isNull())) {
        images->setImage(image);
    }
    emit frozenSourceChanged();
}

QRectF CameraController::bounds() const
{
    return QRectF(QPointF(0, 0), QSizeF(m_screen.size()));
}

QRectF CameraController::clamped(const QRectF& rect) const
{
    return rect.normalized() & bounds();
}

QRectF CameraController::defaultArea() const
{
    const QRectF b = bounds();
    return QRectF(b.width() / 4, b.height() / 4, b.width() / 2, b.height() / 2);
}

QRectF CameraController::windowAt(QPointF point) const
{
    for (const QRectF& window : m_windows) {
        if (window.contains(point)) {
            return window; // la plus haute d'abord
        }
    }
    return bounds(); // bureau, ou aucune liste des fenêtres (hors de Windows) : l'écran entier
}

QImage CameraController::cropped() const
{
    if (m_frozen.isNull() || m_screen.isEmpty()) {
        return {};
    }
    const qreal scale = static_cast<qreal>(m_frozen.width()) / m_screen.width();
    switch (m_mode) {
    case AreaMode:
    case WindowMode: {
        const QRectF inImage(m_selection.x() * scale, m_selection.y() * scale, m_selection.width() * scale,
                             m_selection.height() * scale);
        const QRect box = inImage.toAlignedRect() & m_frozen.rect();
        return box.isEmpty() ? QImage() : m_frozen.copy(box);
    }
    case PolygonMode:
    case FreehandMode: {
        QPolygonF shape = m_mode == PolygonMode ? m_polygon.polygon() : m_freehand.polygon();
        for (QPointF& point : shape) {
            point *= scale;
        }
        return features::cropToPolygon(m_frozen, shape);
    }
    case ScreenMode:
        return m_frozen;
    default:
        return {};
    }
}

bool CameraController::insertOnPage(const QImage& image, bool newPage)
{
    if (!m_document || image.isNull() || m_frozen.isNull() || m_screen.isEmpty()) {
        return false;
    }
    QByteArray png;
    QBuffer buffer(&png);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG")) {
        return false;
    }
    // À sa taille en pixels logiques de l'écran photographié : à 100 %, l'image a sur la page la taille
    // qu'elle avait à l'écran ; centrée dans la vue (au centre de la page pour une page neuve).
    const qreal scale = static_cast<qreal>(m_frozen.width()) / m_screen.width();
    const QSizeF size(image.width() / scale, image.height() / scale);
    const QSizeF pageSize = m_document->pageSize();
    QPointF center(pageSize.width() / 2, pageSize.height() / 2);
    if (!newPage && m_view && m_view->width() > 0 && m_view->height() > 0) {
        center = m_view->sceneToPage(m_view->mapToScene(QPointF(m_view->width() / 2, m_view->height() / 2)));
    }
    core::ImageObject object;
    object.bounds = core::RectF{center.x() - size.width() / 2, center.y() - size.height() / 2, size.width(),
                                size.height()};
    object.sourceWidth = image.width();
    object.sourceHeight = image.height();
    DocumentInsertions insertions(*m_document);
    return insertions.insertImage(std::move(png), "png", object, newPage) != 0;
}

} // namespace actcraft::ui
