// L8-T25 : médias sur la page. La vidéo joue dans une surcouche de `pageOverlays`, au-dessus du cache, jamais
// dans la page fixe (qui ne peint que l'espace réservé) : les deux premières fonctions distinguent ce chemin
// de celui qui peindrait les images de la vidéo par la page fixe (rafraîchissement du cache à chaque image).
// Données écrites par nous : vidéo H.264 de 3 s (64 × 48, 10 images/s, damier qui glisse, AVAssetWriter),
// sons WAV et GIF de deux images produits ici.
#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/media/player.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/linklauncher.h>
#include <actcraft/ui/mediaitems.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/toolhost.h>

#include <QDataStream>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QtTest>

#include <cmath>
#include <memory>

using namespace actcraft;
using actcraft::media::Player;
using actcraft::ui::MediaItems;

namespace {

const char kClipBase64[] =
    "AAAAHGZ0eXBtcDQyAAAAAWlzb21tcDQxbXA0MgAAAAFtZGF0AAAAAAAADDgAAAA2BgUtR1ZK3FxMQz+U78URPNFDqAEAAAMAAQMA"
    "AAMAAQIAAF3ACwAAAwAAAwAAAwFADAORPwGAAAAAjiW4IAO/8P/C0UAAz4vxAADANEAALgJPg4CvEAAMASIAAXB58HAS8QAAwBIg"
    "ABcHnwcBLxAADAEiAAFweeCQUGOAAIAccAAQHb7777776glN09PT0wSV1111111BLT09PTBJXXXXXXXEYoxABAB4gEACzre1ta66"
    "6666666euuuuuuuuuuunrrrrrrrrrrwAAABDIeEEDPgMChz4zB64gABELEAAjYzB64gABELEAAjYzB64gABELEAAjRHxOD7zvnfO"
    "8R8/nfO+d4j/QYUX/qov/VRfwAAAAEUh4ggK+AwqHF7GYONxAACAkQABFxmDi+IAAQCRAAEfGYOJ8QAAgEiAAI8R8Tg43nfO+d4j"
    "5/O+d87xH+g0ov/VRf+qi/gAAABHIeMMCPgE+odpcZg5uIAAVBYgABANjMHJcQAAqCxAACATGYOS4gABUFiAAEAkR8TilzvnfO8R"
    "8/nfO+d4j/QcUX/qov/VRfwAAABQIeQQG+AvoJxyqqqqrz4zBwfxAACQJEAAIh8Zg4L8QAAkGiAAEQuMwcF+IAASDRAACIWI+JxQ"
    "XO+d87xHz+d8753iP9B5eGKELEf6qL/1UX8AAABJIeUUE+AwKHaxmDhTiAAFwLEAAKBMZg4W4gABcHiAAFATGYOFuIAAXB4gABQE"
    "iPjcHDP2d8753iPn8753zvEf6BAov/VRf+qi/gAAAE0h5hgvgMCCcdWta9jMHBXiAAGAaIAAXB8Zg4L8QAAwBIgABcHxmDgvxAAD"
    "AEiAAFweI+JwcGed8753iPn8753zvEf6BEov/VRf+qi/gAAAAFQh5xx834f+FhwoD4o+IAAZNEAALDYzBwLcQAAyDxAACwNjMHAk"
    "4gABkCxAACwNjMHAk4gABkCxAACwNEfPvO+d87xHz+d8753iP9AkUX/qov/VRfwAAABKIeggfYECAlaHPjcHAS8QAAwBIgABcHxm"
    "DgJeIAAYAkQAAuD4zBwEvEAAMASIAAXB4j59Z3zvneI+fzvnfO8R/oEii/9VF/6qL+AAAABeIekkfN+H/hYcKD4o+IAAZJEAALGz"
    "f//C0U+KPiAAGQLEAALA2b//4WinxR8QAAyBYgABYGzf//C0U+KPiAAGQLEAALA0R8/nfO+d4j5/O+d87xH+gSKL/1UX/qov4AAA"
    "ABoGBRVHVkrcXExDP5TvxRE80UOoAwAAAwABgAAAAI8luBAH/w/8LDhQB8X4gABhogABck+DgK8QAAwBIgABcHnwcBLxAADAEiAA"
    "FwefBwEvEAAMASIAAXB/D//BECQV8cAA3HAAEBO+++++++oJTdPT09MEldddddddQS09PT0wSV111111xGKADEAEAeIBAHnW9ra1"
    "1111111109dddddddddddPXXXXXXXXXXgAAAAFAh4QR+AwIJhyqqqq3xmDgW4gABkDxAACwNjMHAk4gABkCxAACwNjMHAk4gABkC"
    "xAACwNEfN+H/h+/nfO+d4j5/O+d87xH+gSKL/1UX/qov4AAAAFch4gh834f+FhwoPi/EAAMaIAAXTGYOAl4gABgCRAAC4PjMHAS8"
    "QAAwBIgABcHxmDgJeIAAYAkQAAuDxHz5/O+d87xHz+d8753iP5kCRXmiP51UX/qov4AAAABSIeMMfN+H/hYc/FHxAADMiAAFtjMH"
    "Ak4gABkHiAAFgbGYOBJxAADIFiAAFgbGYOBJxAADIFiAAFgaI+fzvnfO8R8/nfO+d4j/QJFF/6qL/1UX8AAAAFwh5BB8bHA4CLxw"
    "ADCIAAXmbD//C0UfFPiAAGAJEAALg+b//4Wij4p8QAAwBIgABcHzf//C0UfFPiAAGAJEAALg8R8+8753zvEfP53zvneI/mQJFeaI"
    "/nVRf+qi/gAAAFkh5RQ/N+H/gjHCj44AB1HAANM3//wtFPij4gAB0BYgABoDYzBwCTiAAHQFiAAGgNmw//wtFPij4gAB0BYgABoD"
    "RHz+d8753iPn8753zvEf6BMov/VRf+qi/gAAAFgh5hgXxscDgArxAABAAsQAA7cZg4AIvEAAEAAAkQAA6AuMwcAEXiAACAAASIAA"
    "dAXGYOACLxAABAAAJEAAOgLEfP53zvneI+fzvnfO8R/oFCi/9VF/6qL+AAAAXSHnHB/GxwoAFiAACAL4gAAgALjMHAAjcQAAQBQB"
    "YgAAgAAExmDgARuIAAIAoAsQAAQAACYzBwAI3EAAEAUAWIAAIAABIj5/O+d87xHz+d8753iP9AqUX/qov/VRfwAAAF4h6CAfxscD"
    "gAtxAABAEWIAAIAL4zBwAT8QAAQBADRAABABALjMHABF4gAAgCACRAABABALjMHABF4gAAgCACRAABABALEfP53zvneI+fzvnfO8"
    "R/oFSi/9VF/6qL+AAAAAYCHpJB/GxwOAEbiAACAK8QAAQAC4zBwAicQAAQBQBYgAAgAANjMHACJxAABAFAFiAACAAATGYOAETiAA"
    "CAKALEAAEAAAkR8/i39i39i39EfP53zvneI/0CpRf+qi/9VF/AAAABoGBRVHVkrcXExDP5TvxRE80UOoAwAAAwABgAAAAKIluCAH"
    "/8P/Cw5eKHxAABAEPEAAEAEX//+FooPih8QAAQBABIgAAgAgF///haKD4ofEAAEAQASIAAIAIBf//4Wig+KHxAABAEAEiAACACAX"
    "//+CIFQo+OAATjgACATfffffffUFdPT09MFVdddddddQV09PT0wVV111111xGKAATiACA8QCA9ra2tdddddddddddddddddddddd"
    "dddddddddeAAAABiIeEEH834f+FhwoB8UfEAAEAUWIAAIAA+MwcALcQAAQBQBYgAAgAANjMHACJxAABAFAFiAACAAATGYOAETiAA"
    "CAKALEAAEAAAkR8+XzvnfO8R8/nfO+d4j/QKlF/6qL/1UX8AAABsIeIIH834f+Fhwp8UPiAACAIPEAAEAEPm//+Foo+KfEAAEAQA"
    "0QAAQAQC5v//haKPinxAABAEAEiAACACAXN//8LRR8U+IAAIAgAkQAAQAQCxHz+dz+dz+dz8R8/nfO+d4j/QKlF/6qL/1UX8AAAA"
    "WiHjDB/gQIJhyqqqqqf43BwAk4gAAgCgCxAABAAAbGYOAE3EAAEAUAWIAAIAABMZg4AROIAAIAoAsQAAQAACRHxOKAFzvnfO8R8/"
    "nfO+d4j/QKlF/6qL/1UX8AAAAG0h5BAfzfh/4WHCgfFD4gAAgCAsQAAQAQfN//8LRR8U+IAAIAgAkQAAQAQC5v//haKPinxAABAE"
    "AEiAACACAXN//8LRR8U+IAAIAgAkQAAQAQCxHz+dz+dz+dz8R8/nfO+d4j/QKlF/6qL/1UX8AAAAWiHlFB/gQIJhyqqqqrx3G4OA"
    "EnEAAEAUAWIAAIAADYzBwAm4gAAgCgCxAABAAAJjMHACJxAABAFAFiAACAAASI+JxQAud8753iPn8753zvEf6BUov/VRf+qi/gAA"
    "AG4h5hgfzfh/4WHCgB8UPiAACAIBYgAAgAgLm//+FooPih8QAAQBABIgAAgAgHzf//C0UHxQ+IAAIAgAkQAAQAQC5v//haKD4ofE"
    "AAEAQASIAAIAIBYj4vvnfO+d4j5/O+d87xH+gVKL/1UX/qov4AAAAG4h5xwfzfh/4WHCgAFvFB8QAAQBQPEAAEAAAub//4Wih8UH"
    "xAABAFAFiAACAAA2b//4Wih8UHxAABAFAFiAACAAATN//8LRQ+KD4gAAgCgCxAABAAAJEfP53zvneI+fzvnfO8R/oFSi/9VF/6qL"
    "+AAAAG8h6CAfzfh/4WHCgAEPFD4gAAgCALEAAEAEA+b//4Wig+KHxAABAEANEAAEAEAub//4Wig+KHxAABAEAEiAACACAXN//8LR"
    "QfFD4gAAgCACRAABABALEfF/s753zvEfP53zvneI/0CpRf+qi/9VF/AAAABvIekkH834f+FhwoABrxQfEAAEAUAsQAAQAABc3//w"
    "tFD4oPiAACAKALEAAEAABs3//wtFD4oPiAACAKALEAAEAAAmb//4Wih8UHxAABAFAFiAACAAASI+L753zvneI+fzvnfO8R/oFSi/"
    "9VF/6qL+AAADW21vb3YAAABsbXZoZAAAAADnE8yJ5xPMiQAAAlgAAAcIAAEAAAEAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAA"
    "AQAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAIAAALndHJhawAAAFx0a2hkAAAAAecTzInnE8yJ"
    "AAAAAQAAAAAAAAcIAAAAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAQAAAAABAAAAAMAAAAAAA"
    "JGVkdHMAAAAcZWxzdAAAAAAAAAABAAAHCAAAAAAAAQAAAAACX21kaWEAAAAgbWRoZAAAAADnE8yJ5xPMiQAAAlgAAAcIVcQAAAAA"
    "ADFoZGxyAAAAAAAAAAB2aWRlAAAAAAAAAAAAAAAAQ29yZSBNZWRpYSBWaWRlbwAAAAIGbWluZgAAABR2bWhkAAAAAQAAAAAAAAAA"
    "AAAAJGRpbmYAAAAcZHJlZgAAAAAAAAABAAAADHVybCAAAAABAAABxnN0YmwAAACcc3RzZAAAAAAAAAABAAAAjGF2YzEAAAAAAAAA"
    "AQAAAAAAAAAAAAAAAAAAAAAAQAAwAEgAAABIAAAAAAAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAY//8AAAAi"
    "YXZjQwFCAAv/4QALJ0IAC6tBhvAgwygBAAQozjyAAAAACmZpZWwBAAAAAApjaHJtAAAAAAAYc3R0cwAAAAAAAAABAAAAHgAAADwA"
    "AAAcc3RzcwAAAAAAAAADAAAAAQAAAAsAAAAVAAAAKnNkdHAAAAAAIBAQEBAQEBAQECAQEBAQEBAQEBAgEBAQEBAQEBAQAAAAHHN0"
    "c2MAAAAAAAAAAQAAAAEAAAAKAAAAAQAAAIxzdHN6AAAAAAAAAAAAAAAeAAAAzAAAAEcAAABJAAAASwAAAFQAAABNAAAAUQAAAFgA"
    "AABOAAAAYgAAALEAAABUAAAAWwAAAFYAAABgAAAAXQAAAFwAAABhAAAAYgAAAGQAAADEAAAAZgAAAHAAAABeAAAAcQAAAF4AAABy"
    "AAAAcgAAAHMAAABzAAAAHHN0Y28AAAAAAAAAAwAAACwAAAPNAAAHww==";

QByteArray clipMp4()
{
    return QByteArray::fromBase64(QByteArray(kClipBase64));
}

// Son de `ms` millisecondes à 440 Hz, mono 16 bits.
QByteArray toneWav(int ms)
{
    constexpr int rate = 22050;
    const int samples = rate * ms / 1000;
    QByteArray pcm;
    for (int i = 0; i < samples; ++i) {
        const auto v =
            static_cast<qint16>(std::lround(8000.0 * std::sin(2.0 * 3.14159265358979 * 440.0 * i / rate)));
        pcm.append(static_cast<char>(v & 0xFF));
        pcm.append(static_cast<char>((v >> 8) & 0xFF));
    }
    QByteArray wav;
    QDataStream out(&wav, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::LittleEndian);
    out.writeRawData("RIFF", 4);
    out << static_cast<quint32>(36 + pcm.size());
    out.writeRawData("WAVEfmt ", 8);
    out << quint32(16) << quint16(1) << quint16(1) << quint32(rate) << quint32(rate * 2) << quint16(2)
        << quint16(16);
    out.writeRawData("data", 4);
    out << static_cast<quint32>(pcm.size());
    out.writeRawData(pcm.constData(), static_cast<int>(pcm.size()));
    return wav;
}

// GIF89a de 16 × 16 en boucle : image 1 rouge, image 2 bleue, 400 ms chacune. LZW sans croissance de table :
// un code d'effacement tous les deux pixels (codes de 3 bits).
QByteArray twoFrameGif()
{
    QByteArray g("GIF89a");
    auto u16 = [&g](int v) {
        g.append(static_cast<char>(v & 0xFF));
        g.append(static_cast<char>((v >> 8) & 0xFF));
    };
    u16(16);
    u16(16);
    g.append(static_cast<char>(0x91)); // table globale de 4 couleurs
    g.append('\0');
    g.append('\0');
    const unsigned char palette[] = {0xFF, 0, 0, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0, 0, 0};
    g.append(reinterpret_cast<const char*>(palette), sizeof palette);
    const unsigned char loop[] = {0x21, 0xFF, 0x0B, 'N', 'E',  'T',  'S', 'C', 'A', 'P',
                                  'E',  '2',  '.',  '0', 0x03, 0x01, 0,   0,   0};
    g.append(reinterpret_cast<const char*>(loop), sizeof loop);
    for (int colour = 0; colour < 2; ++colour) {
        const unsigned char control[] = {0x21, 0xF9, 0x04, 0x04, 40, 0, 0, 0};
        g.append(reinterpret_cast<const char*>(control), sizeof control);
        g.append(static_cast<char>(0x2C));
        u16(0);
        u16(0);
        u16(16);
        u16(16);
        g.append('\0');
        g.append(static_cast<char>(2)); // taille minimale des codes
        QByteArray bytes;
        quint32 buffer = 0;
        int bits = 0;
        auto put = [&](int code) {
            buffer |= static_cast<quint32>(code) << bits;
            bits += 3;
            while (bits >= 8) {
                bytes.append(static_cast<char>(buffer & 0xFF));
                buffer >>= 8;
                bits -= 8;
            }
        };
        for (int i = 0; i < 256; i += 2) {
            put(4);
            put(colour);
            put(colour);
        }
        put(5);
        if (bits > 0)
            bytes.append(static_cast<char>(buffer & 0xFF));
        for (qsizetype at = 0; at < bytes.size(); at += 255) {
            const qsizetype n = std::min<qsizetype>(255, bytes.size() - at);
            g.append(static_cast<char>(n));
            g.append(bytes.mid(at, n));
        }
        g.append('\0');
    }
    g.append(static_cast<char>(0x3B));
    return g;
}

core::PageObject mediaObject(core::ObjectId id, const std::string& key, double x, double y, double w,
                             double h)
{
    core::Media media;
    media.bounds.x = x;
    media.bounds.y = y;
    media.bounds.width = w;
    media.bounds.height = h;
    media.media = key;
    core::PageObject object = std::move(media);
    object.common.id = id;
    object.common.layer = core::Layer::Middle;
    return object;
}

core::Media& mediaOf(core::Document& d, std::size_t i)
{
    return std::get<core::Media>(static_cast<core::ObjectVariant&>(d.page(0).objects[i]));
}

bool isDescendantOf(const QQuickItem* item, const QQuickItem* ancestor)
{
    for (auto* p = item ? item->parentItem() : nullptr; p; p = p->parentItem())
        if (p == ancestor)
            return true;
    return false;
}

// Zone de page réelle hors écran (patron de tst_ui_actionrunner) ; la couche des médias reçoit son modèle et
// sa vue comme dans Main.qml.
struct Scene
{
    QQmlEngine engine;
    std::unique_ptr<QObject> root;
    QQuickWindow* window = nullptr;
    ui::PageView* view = nullptr;
    render::FixedPageItem* fixed = nullptr;
    QQuickItem* overlays = nullptr;

    bool mount(ui::DocumentController& controller, MediaItems& items)
    {
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nimport QtQuick.Window\nimport ActCraft.Ui\n"
                          "Window { width: 800; height: 600; visible: true\n"
                          "  PageArea { objectName: \"area\"; anchors.fill: parent } }",
                          QUrl(QStringLiteral("qrc:/tst_ui_mediaitems/Window.qml")));
        root.reset(component.create());
        if (!root) {
            qWarning() << component.errors();
            return false;
        }
        window = qobject_cast<QQuickWindow*>(root.get());
        QQuickItem* area = window ? window->findChild<QQuickItem*>(QStringLiteral("area")) : nullptr;
        if (!area)
            return false;
        area->setProperty("documentController", QVariant::fromValue(&controller));
        view = window->findChild<ui::PageView*>(QStringLiteral("pageView"));
        fixed = window->findChild<render::FixedPageItem*>(QStringLiteral("fixedPage"));
        overlays = window->findChild<QQuickItem*>(QStringLiteral("pageOverlays"));
        auto* layer = window->findChild<QQuickItem*>(QStringLiteral("mediaLayer"));
        if (!view || !fixed || !overlays || !layer)
            return false;
        controller.setFixedPage(fixed);
        layer->setProperty("view", QVariant::fromValue(static_cast<QObject*>(view)));
        layer->setProperty("items", QVariant::fromValue(static_cast<QObject*>(&items)));
        return QTest::qWaitForWindowExposed(window) &&
               QTest::qWaitFor([&] { return fixed->isReady() && view->cacheReady(); }, 10000);
    }
    QQuickItem* item(const QString& name) const { return window->findChild<QQuickItem*>(name); }
};

} // namespace

class TstUiMediaitems : public QObject
{
    Q_OBJECT
private slots:
    void init();
    void cleanup();
    void videoIsAboveTheCache();
    void videoNeverEntersTheCache();
    void controlsHiddenOption();
    void loopOption();
    void playOnOpenOnlyInternal();
    void gifAnimates();
    void chimePlaysOnAcceptedDrop();
    void soundControllerToggles();

private:
    void open(core::Document document)
    {
        m_controller->replaceDocument(std::move(document), QUrl(), QStringLiteral("medias"));
        m_items->setDocument(m_controller.get());
    }
    std::unique_ptr<ui::DocumentController> m_controller;
    std::unique_ptr<ui::LinkLauncher> m_links;
    std::unique_ptr<MediaItems> m_items;
    std::unique_ptr<Scene> m_scene;
};

void TstUiMediaitems::init()
{
    m_controller = std::make_unique<ui::DocumentController>();
    m_links = std::make_unique<ui::LinkLauncher>();
    m_links->setOpener([](const QUrl&) { return false; });
    m_items = std::make_unique<MediaItems>();
    m_items->setLinks(m_links.get());
    m_scene = std::make_unique<Scene>();
}

void TstUiMediaitems::cleanup()
{
    m_scene.reset();
    m_items.reset();
    m_links.reset();
    m_controller.reset();
}

// L'élément vidéo est un descendant de `pageOverlays`, jamais de la page fixe, et couvre le cadre du média
// placé par `view.pageToScene`.
void TstUiMediaitems::videoIsAboveTheCache()
{
    core::Document d(1024, 768);
    const std::string key = d.addResource(clipMp4(), "mp4");
    d.page(0).objects.push_back(mediaObject(11, key, 100, 120, 320, 240));
    open(std::move(d));
    QVERIFY(m_scene->mount(*m_controller, *m_items));
    QTRY_COMPARE(m_items->rowCount(), 1);
    QQuickItem* video = nullptr;
    QTRY_VERIFY((video = m_scene->item(QStringLiteral("mediaVideo-11"))) != nullptr);
    QVERIFY(video->inherits("QQuickVideoOutput"));
    QVERIFY2(isDescendantOf(video, m_scene->overlays), "la vidéo doit être une surcouche de pageOverlays");
    QVERIFY2(!isDescendantOf(video, m_scene->fixed),
             "la vidéo ne doit jamais être dans la page fixe (cache)");
    const QRectF scene = video->mapRectToScene(QRectF(0, 0, video->width(), video->height()));
    QTRY_COMPARE(video->mapRectToScene(QRectF(0, 0, video->width(), video->height())).topLeft(),
                 m_scene->view->pageToScene(QPointF(100, 120)));
    QCOMPARE(video->mapRectToScene(QRectF(0, 0, video->width(), video->height())).bottomRight(),
             m_scene->view->pageToScene(QPointF(420, 360)));
    Q_UNUSED(scene);
}

// 2 s de lecture réelle (des images arrivent au VideoOutput) : aucune demande de redessin du cache.
void TstUiMediaitems::videoNeverEntersTheCache()
{
    core::Document d(1024, 768);
    const std::string key = d.addResource(clipMp4(), "mp4");
    d.page(0).objects.push_back(mediaObject(11, key, 100, 120, 320, 240));
    open(std::move(d));
    QVERIFY(m_scene->mount(*m_controller, *m_items));
    QQuickItem* video = nullptr;
    QTRY_VERIFY((video = m_scene->item(QStringLiteral("mediaVideo-11"))) != nullptr);
    auto* sink = video->property("videoSink").value<QObject*>();
    QVERIFY(sink);
    QSignalSpy frames(sink, SIGNAL(videoFrameChanged(QVideoFrame)));
    QSignalSpy refresh(m_scene->view, &ui::PageView::cacheRefreshRequested);
    QSignalSpy updated(m_scene->view, &ui::PageView::cacheUpdated);
    QSignalSpy content(m_scene->fixed, &render::FixedPageItem::contentChanged);
    QVERIFY(m_items->play(11));
    QTest::qWait(2000);
    // Lecture réelle : sans images, l'espion ne prouverait rien.
    QVERIFY2(frames.count() >= 2, qPrintable(QStringLiteral("%1 image(s) reçue(s)").arg(frames.count())));
    QCOMPARE(refresh.count(), 0);
    QCOMPARE(updated.count(), 0);
    QCOMPARE(content.count(), 0);
}

void TstUiMediaitems::controlsHiddenOption()
{
    core::Document d(1024, 768);
    const std::string key = d.addResource(clipMp4(), "mp4");
    d.page(0).objects.push_back(mediaObject(11, key, 50, 50, 320, 240));
    d.page(0).objects.push_back(mediaObject(12, key, 450, 50, 320, 240));
    mediaOf(d, 1).controlsHidden = true;
    open(std::move(d));
    QVERIFY(m_scene->mount(*m_controller, *m_items));
    QQuickItem* shown = nullptr;
    QQuickItem* hidden = nullptr;
    QTRY_VERIFY((shown = m_scene->item(QStringLiteral("mediaControls-11"))) != nullptr);
    QTRY_VERIFY((hidden = m_scene->item(QStringLiteral("mediaControls-12"))) != nullptr);
    QVERIFY(shown->isVisible());
    QVERIFY(!hidden->isVisible());
}

// Fin de lecture : un son en boucle reprend (sa position repart en arrière), l'autre s'arrête.
void TstUiMediaitems::loopOption()
{
    core::Document d(1024, 768);
    const std::string key = d.addResource(toneWav(400), "wav");
    d.page(0).objects.push_back(mediaObject(21, key, 50, 50, 64, 64));
    d.page(0).objects.push_back(mediaObject(22, key, 200, 50, 64, 64));
    mediaOf(d, 0).loop = true;
    open(std::move(d));
    QTRY_COMPARE(m_items->rowCount(), 2);
    Player* player = m_items->player();
    qint64 last = -1;
    bool wrapped = false;
    connect(player, &Player::positionChanged, this, [&](int track, qint64 position) {
        if (track == Player::First) {
            wrapped = wrapped || (last > 200 && position < last);
            last = position;
        }
    });
    QSignalSpy finished(player, &Player::finished);
    QVERIFY(m_items->play(21));
    QTRY_VERIFY_WITH_TIMEOUT(wrapped, 5000);
    QVERIFY(player->isPlaying(Player::First));
    QCOMPARE(finished.count(), 0);
    QVERIFY(m_items->play(22));
    QTRY_VERIFY_WITH_TIMEOUT(!player->isPlaying(Player::First), 5000);
}

// « Lecture auto » démarre le lecteur intégré à l'affichage de la page ; jamais LinkLauncher. Sans l'option,
// rien ne joue.
void TstUiMediaitems::playOnOpenOnlyInternal()
{
    QSignalSpy asked(m_links.get(), &ui::LinkLauncher::confirmationNeeded);
    QSignalSpy opened(m_links.get(), &ui::LinkLauncher::opened);
    {
        core::Document d(1024, 768);
        const std::string sound = d.addResource(toneWav(3000), "wav");
        const std::string clip = d.addResource(clipMp4(), "mp4");
        d.page(0).objects.push_back(mediaObject(31, sound, 50, 50, 64, 64));
        d.page(0).objects.push_back(mediaObject(32, clip, 200, 50, 320, 240));
        open(std::move(d));
        QVERIFY(m_scene->mount(*m_controller, *m_items));
        QTRY_COMPARE(m_items->rowCount(), 2);
        QTest::qWait(300);
        QVERIFY(!m_items->player()->isPlaying(Player::First));
        QQuickItem* frame = m_scene->item(QStringLiteral("mediaFrame-32"));
        QVERIFY(frame);
        QVERIFY(!frame->property("playing").toBool());
    }
    core::Document d(1024, 768);
    const std::string sound = d.addResource(toneWav(3000), "wav");
    const std::string clip = d.addResource(clipMp4(), "mp4");
    d.page(0).objects.push_back(mediaObject(31, sound, 50, 50, 64, 64));
    d.page(0).objects.push_back(mediaObject(32, clip, 200, 50, 320, 240));
    mediaOf(d, 0).playOnOpen = true;
    mediaOf(d, 1).playOnOpen = true;
    open(std::move(d));
    QTRY_VERIFY_WITH_TIMEOUT(m_items->player()->isPlaying(Player::First), 5000);
    QTRY_VERIFY_WITH_TIMEOUT(m_scene->item(QStringLiteral("mediaFrame-32")) &&
                                 m_scene->item(QStringLiteral("mediaFrame-32"))->property("playing").toBool(),
                             5000);
    QCOMPARE(asked.count(), 0);
    QCOMPARE(opened.count(), 0);
}

// Deux images différentes à 600 ms d'écart (V57) : l'animation est dans la surcouche, pas dans le cache.
void TstUiMediaitems::gifAnimates()
{
    core::Document d(1024, 768);
    const std::string key = d.addResource(twoFrameGif(), "gif");
    core::ImageObject image;
    image.bounds.x = 100;
    image.bounds.y = 100;
    image.bounds.width = 160;
    image.bounds.height = 160;
    image.sourceWidth = 16;
    image.sourceHeight = 16;
    image.resource = key;
    core::PageObject object = std::move(image);
    object.common.id = 41;
    object.common.layer = core::Layer::Middle;
    d.page(0).objects.push_back(std::move(object));
    open(std::move(d));
    QVERIFY(m_scene->mount(*m_controller, *m_items));
    QQuickItem* gif = nullptr;
    QTRY_VERIFY((gif = m_scene->item(QStringLiteral("mediaGif-41"))) != nullptr);
    QTRY_COMPARE(gif->property("frameCount").toInt(), 2);
    QSignalSpy changed(gif, SIGNAL(currentFrameChanged()));
    QVERIFY(changed.wait(3000));
    const QPoint at = m_scene->view->pageToScene(QPointF(180, 180)).toPoint();
    const int first = gif->property("currentFrame").toInt();
    const QColor before = m_scene->window->grabWindow().pixelColor(at);
    QTest::qWait(600);
    const int second = gif->property("currentFrame").toInt();
    const QColor after = m_scene->window->grabWindow().pixelColor(at);
    QVERIFY2(first != second, "l'image du GIF doit avoir changé");
    QVERIFY2(before != after, qPrintable(before.name() + QLatin1String(" puis ") + after.name()));
}

// containerAccepted avec `chime` : piste 2 lancée ; sans `chime`, rien.
void TstUiMediaitems::chimePlaysOnAcceptedDrop()
{
    core::Document d(1024, 768);
    const std::string key = d.addResource(toneWav(1500), "wav");
    core::ImageObject box;
    box.bounds.width = 200;
    box.bounds.height = 200;
    core::PageObject object = std::move(box);
    object.common.id = 51;
    object.common.container.chime = true;
    object.common.container.chimeSound = key;
    d.page(0).objects.push_back(std::move(object));
    open(std::move(d));
    ui::ToolHost tools;
    m_items->setTools(&tools);
    emit tools.containerAccepted(51, false);
    QTest::qWait(200);
    QVERIFY(!m_items->player()->isPlaying(Player::Second));
    emit tools.containerAccepted(51, true);
    QTRY_VERIFY_WITH_TIMEOUT(m_items->player()->isPlaying(Player::Second), 5000);
    QVERIFY(!m_items->player()->isPlaying(Player::First));
    m_items->setTools(nullptr);
}

// media.sound-controller montre puis masque le lecteur de sons.
void TstUiMediaitems::soundControllerToggles()
{
    ui::Commands commands;
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick\nimport ActCraft.Ui\nSoundController {}",
                      QUrl(QStringLiteral("qrc:/tst_ui_mediaitems/Sound.qml")));
    std::unique_ptr<QObject> controller(component.createWithInitialProperties(
        {{QStringLiteral("commands"), QVariant::fromValue(&commands)},
         {QStringLiteral("items"), QVariant::fromValue(static_cast<QObject*>(m_items.get()))}}));
    QVERIFY2(controller, qPrintable(component.errorString()));
    QCOMPARE(controller->property("visible").toBool(), false);
    QVERIFY(commands.trigger(QStringLiteral("media.sound-controller")));
    QCOMPARE(controller->property("visible").toBool(), true);
    QVERIFY(commands.trigger(QStringLiteral("media.sound-controller")));
    QCOMPARE(controller->property("visible").toBool(), false);
}

// Sous macOS, le moteur darwin de Qt Multimedia livre par la file principale de GCD : répartiteur Core
// Foundation obligatoire (patron de tst_media_player).
int main(int argc, char* argv[])
{
#if defined(Q_OS_DARWIN)
    if (!qEnvironmentVariableIsSet("ACTCRAFT_TEST_NO_CF"))
        qputenv("QT_EVENT_DISPATCHER_CORE_FOUNDATION", "1");
#endif
    QGuiApplication app(argc, argv);
    TstUiMediaitems test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}
#include "tst_ui_mediaitems.moc"
