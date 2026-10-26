#include "desktopcontroller.h"

#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>

#include <QBuffer>
#include <QByteArray>
#include <QGuiApplication>
#include <QIODevice>
#include <QMetaObject>
#include <QPixmap>
#include <QQmlContext>
#include <QQmlEngine>
#include <QScreen>
#include <QSizeF>
#include <QSurfaceFormat>
#include <QUrl>
#include <QtQml/qqml.h>

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

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
    if (!target || !m_capture) {
        return false;
    }
    const QImage image = m_capture();
    if (image.isNull()) {
        return false;
    }
    QByteArray png = pngOf(image);
    if (png.isEmpty()) {
        return false;
    }
    target->execute(std::make_unique<DesktopCaptureCommand>(
        static_cast<std::size_t>(target->currentIndex()), std::move(png)));
    return true;
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
    // (closeWindow se déconnecte avant de la cacher.)
    connect(window, &QWindow::visibleChanged, this, [this](bool visible) {
        if (!visible) {
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
    QQuickWindow* window = m_window.data();
    m_window = nullptr;
    emit windowChanged();
    window->disconnect(this);
    m_desktop->setFixedPage(nullptr); // la page fixe part avec la fenêtre
    window->hide();
    window->deleteLater();
    setPassThrough(false);
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
