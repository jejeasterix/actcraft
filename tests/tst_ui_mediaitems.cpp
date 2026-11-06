// L8-T25 : médias sur la page. La vidéo joue dans une surcouche de `pageOverlays`, au-dessus du cache, jamais
// dans la page fixe (qui ne peint que l'espace réservé) : les deux premières fonctions distinguent ce chemin de
// celui qui peindrait les images de la vidéo par la page fixe (rafraîchissement du cache à chaque image).
#include <actcraft/media/player.h>
#include <actcraft/ui/mediaitems.h>

#include <QQuickItem>
#include <QQuickView>
#include <QSignalSpy>
#include <QtTest>

using actcraft::media::Player;
using actcraft::ui::MediaItems;

namespace {

bool isDescendantOf(const QQuickItem* item, const QQuickItem* ancestor)
{
    for (auto* p = item ? item->parentItem() : nullptr; p; p = p->parentItem())
        if (p == ancestor)
            return true;
    return false;
}

} // namespace

class TstUiMediaitems : public QObject
{
    Q_OBJECT
private slots:
    void videoIsAboveTheCache();
    void videoNeverEntersTheCache();
    void controlsHiddenOption();
    void loopOption();
    void playOnOpenOnlyInternal();
    void gifAnimates();
    void chimePlaysOnAcceptedDrop();
    void soundControllerToggles();
};

// L'élément vidéo est un descendant de `pageOverlays`, jamais de la page fixe, et couvre le cadre du média
// placé par `view.pageToScene`.
void TstUiMediaitems::videoIsAboveTheCache()
{
    // Scène : PageArea (page fixe + pageOverlays) et MediaItems sur un document qui porte une vidéo.
    QQuickItem* root = nullptr;     // à poser : PageArea chargée hors écran
    MediaItems* items = nullptr;    // à poser
    const quint64 videoId = 0;      // à poser
    const QRectF frameOnPage;       // à poser : bounds du média
    QVERIFY(root && items);
    auto* overlays = root->findChild<QQuickItem*>(QStringLiteral("pageOverlays"));
    auto* fixed = root->findChild<QQuickItem*>(QStringLiteral("fixedPage"));
    auto* video = root->findChild<QQuickItem*>(QStringLiteral("mediaVideo-%1").arg(videoId));
    QVERIFY(overlays && fixed && video);
    QVERIFY(video->inherits("QQuickVideoOutput"));
    QVERIFY2(isDescendantOf(video, overlays), "la vidéo doit être une surcouche de pageOverlays");
    QVERIFY2(!isDescendantOf(video, fixed), "la vidéo ne doit jamais être dans la page fixe (cache)");
    auto* view = root->findChild<QQuickItem*>(QStringLiteral("pageView"));
    QVERIFY(view);
    QPointF a, b;
    QMetaObject::invokeMethod(view, "pageToScene", Q_RETURN_ARG(QPointF, a), Q_ARG(QPointF, frameOnPage.topLeft()));
    QMetaObject::invokeMethod(view, "pageToScene", Q_RETURN_ARG(QPointF, b),
                              Q_ARG(QPointF, frameOnPage.bottomRight()));
    const QRectF scene = video->mapRectToScene(QRectF(0, 0, video->width(), video->height()));
    QCOMPARE(scene.topLeft(), a);
    QCOMPARE(scene.bottomRight(), b);
}

// 2 s de lecture réelle (des images arrivent au VideoOutput) : aucune demande de redessin du cache.
void TstUiMediaitems::videoNeverEntersTheCache()
{
    QQuickItem* root = nullptr;  // à poser : même scène
    MediaItems* items = nullptr; // à poser
    const quint64 videoId = 0;   // à poser
    QVERIFY(root && items);
    auto* view = root->findChild<QQuickItem*>(QStringLiteral("pageView"));
    auto* fixed = root->findChild<QQuickItem*>(QStringLiteral("fixedPage"));
    auto* video = root->findChild<QQuickItem*>(QStringLiteral("mediaVideo-%1").arg(videoId));
    QVERIFY(view && fixed && video);
    auto* sink = video->property("videoSink").value<QObject*>();
    QVERIFY(sink);
    QSignalSpy frames(sink, SIGNAL(videoFrameChanged(QVideoFrame)));
    QSignalSpy refresh(view, SIGNAL(cacheRefreshRequested()));
    QSignalSpy content(fixed, SIGNAL(contentChanged()));
    QVERIFY(items->play(videoId));
    QTest::qWait(2000);
    // Lecture réelle : sans images, l'espion ne prouverait rien.
    QVERIFY2(frames.count() >= 2, qPrintable(QStringLiteral("%1 image(s) reçue(s)").arg(frames.count())));
    QCOMPARE(refresh.count(), 0);
    QCOMPARE(content.count(), 0);
}

void TstUiMediaitems::controlsHiddenOption()
{
    QFAIL("à écrire");
}

void TstUiMediaitems::loopOption()
{
    QFAIL("à écrire");
}

void TstUiMediaitems::playOnOpenOnlyInternal()
{
    QFAIL("à écrire");
}

void TstUiMediaitems::gifAnimates()
{
    QFAIL("à écrire");
}

void TstUiMediaitems::chimePlaysOnAcceptedDrop()
{
    QFAIL("à écrire");
}

void TstUiMediaitems::soundControllerToggles()
{
    QFAIL("à écrire");
}

QTEST_MAIN(TstUiMediaitems)
#include "tst_ui_mediaitems.moc"
