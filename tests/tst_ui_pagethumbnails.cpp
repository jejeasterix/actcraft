#include <actcraft/core/model.h>
#include <actcraft/core/pageedits.h>
#include <actcraft/core/undo.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pagethumbnails.h>
#include <actcraft/ui/pageview.h>

#include <QElapsedTimer>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTest>
#include <QThread>

#include <cstddef>
#include <memory>
#include <utility>

using namespace actcraft;
using ui::PageThumbnails;

namespace {

// Trait large en travers de la page, par son centre : le centre de la miniature est sombre. (Le trait
// diagonal de l'esquisse du plan, de (100, 100) à (900, 700), passe à 20 px du centre : il ne le couvre pas.)
core::Stroke bigStroke()
{
    core::Stroke stroke;
    stroke.points = {core::StrokePoint{100, 384, 0.5f}, core::StrokePoint{924, 384, 0.5f}};
    stroke.baseWidth = 60;
    return stroke;
}

// Petit trait dans un coin : la page change, son centre reste blanc.
core::Stroke smallStroke(int i)
{
    core::Stroke stroke;
    const double x = 20.0 + 4.0 * i;
    stroke.points = {core::StrokePoint{x, 20, 0.5f}, core::StrokePoint{x + 2, 24, 0.5f}};
    stroke.baseWidth = 2;
    return stroke;
}

bool darkAtCenter(const QImage& image)
{
    return !image.isNull() && qGray(image.pixel(image.width() / 2, image.height() / 2)) < 128;
}

// Trois pages, la deuxième (indice 1) sombre, la troisième affichée.
void threePagesSecondDark(ui::DocumentController& controller)
{
    controller.newDocument(1024, 768);
    controller.nextPage();
    controller.commitStroke(bigStroke());
    controller.nextPage();
}

// Vue réelle hors écran (montage de tst_ui_inputpath, sans couverture ni couche d'attente) : zoom 1, sans
// défilement, point de fenêtre = point de page.
struct ViewFixture
{
    QQuickWindow window;
    ui::PageView* view = nullptr;
    render::FixedPageItem* fixed = nullptr;
    std::unique_ptr<ui::DocumentController> controller; // détruit avant la fenêtre et la page fixe
};

bool openView(ViewFixture& f)
{
    f.window.resize(800, 600);
    f.view = new ui::PageView;
    f.view->setParentItem(f.window.contentItem());
    f.view->setSize(QSizeF(800, 600));
    auto* content = new QQuickItem;
    content->setSize(QSizeF(1024, 768));
    content->setParentItem(f.view);
    f.fixed = new render::FixedPageItem;
    f.fixed->setSize(content->size());
    f.fixed->setParentItem(content);
    auto* ink = new render::InkItem;
    ink->setSize(content->size());
    ink->setParentItem(content);
    f.view->setContent(content);
    f.view->setFixedPage(f.fixed);
    f.view->setInk(ink);
    f.fixed->setPage(core::Page{});
    f.window.show();
    if (!QTest::qWaitForWindowExposed(&f.window) ||
        !QTest::qWaitFor([&] { return f.fixed->isReady(); }, 5000)) {
        return false;
    }
    f.controller = std::make_unique<ui::DocumentController>();
    f.controller->setFixedPage(f.fixed);
    f.view->setDocumentController(f.controller.get());
    f.view->setZoom(1.0);
    f.view->setPan(QPointF(0, 0));
    return true;
}

} // namespace

class TstUiPagethumbnails : public QObject
{
    Q_OBJECT

private slots:
    // Le patron écarté (miniature rendue à chaque pageContentChanged) rendrait tout de suite : rouge ici.
    // Q12 (L4-C10) : délai long pendant l'attente courte, qu'un poste chargé ne peut pas faire échoir.
    void rendersAfterSettling()
    {
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        PageThumbnails thumbs;
        thumbs.setDocumentController(&controller);
        QTRY_COMPARE_WITH_TIMEOUT(thumbs.renderCount(), 1, 5000); // première image de la page vierge
        QVERIFY(!darkAtCenter(thumbs.image(0)));
        QCOMPARE(thumbs.settleDelay(), PageThumbnails::kSettleMs);
        thumbs.setSettleDelay(2000);
        controller.commitStroke(bigStroke());
        QCOMPARE(thumbs.launchCount(), 1); // rien lancé au versement
        QTest::qWait(100);
        QCOMPARE(thumbs.renderCount(), 1); // pas encore
        QCOMPARE(thumbs.launchCount(), 1);
        QCOMPARE(thumbs.pendingCount(), 1);
        QTRY_COMPARE_WITH_TIMEOUT(thumbs.renderCount(), 2, 10000);
        const QImage image = thumbs.image(0);
        QCOMPARE(image.width(), PageThumbnails::kDefaultWidth);
        QCOMPARE(image.height(), PageThumbnails::kDefaultWidth * 768 / 1024);
        QVERIFY(qGray(image.pixel(image.width() / 2, image.height() / 2)) < 128);
        QCOMPARE(thumbs.pendingCount(), 0);
    }

    // M-I (L4-C10) : quinze traits espacés d'un dixième du délai, soit une fois et demie le délai. Attente de
    // calme : rien n'est lancé pendant les traits, puis un seul rendu. Une limitation de débit (minuteur non
    // relancé à chaque marque) lancerait un rendu au bout du délai, au milieu des traits : rouge ici.
    void oneRenderForManyEdits()
    {
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        PageThumbnails thumbs;
        thumbs.setDocumentController(&controller);
        QTRY_COMPARE_WITH_TIMEOUT(thumbs.renderCount(), 1, 5000);
        constexpr int settle = 1000;
        thumbs.setSettleDelay(settle);
        QElapsedTimer clock;
        clock.start();
        for (int i = 0; i < 15; ++i) {
            controller.commitStroke(smallStroke(i));
            QTest::qWait(settle / 10);
            QCOMPARE(thumbs.launchCount(), 1); // aucun rendu tant que les traits se suivent
        }
        QVERIFY(clock.elapsed() > settle); // la rafale a duré plus que le délai
        QTRY_COMPARE_WITH_TIMEOUT(thumbs.renderCount(), 2, 10000);
        QTest::qWait(2 * settle);
        QCOMPARE(thumbs.renderCount(), 2);
        QCOMPARE(thumbs.launchCount(), 2);
    }

    // Appui sur la vue : contactActive vrai, aucun rendu lancé tant qu'il dure ; relâché : un rendu.
    void postponedDuringContact()
    {
        ViewFixture f;
        QVERIFY(openView(f));
        PageThumbnails thumbs; // détruit avant la vue et le contrôleur
        thumbs.setDocumentController(f.controller.get());
        thumbs.setView(f.view);
        QTRY_COMPARE_WITH_TIMEOUT(thumbs.renderCount(), 1, 5000);
        QSignalSpy contact(f.view, &ui::PageView::contactActiveChanged);
        QVERIFY(!f.view->contactActive());
        QTest::mousePress(&f.window, Qt::LeftButton, Qt::NoModifier, QPoint(300, 300));
        QVERIFY(f.view->contactActive());
        QCOMPARE(contact.count(), 1);
        const int launches = thumbs.launchCount();
        f.controller->commitStroke(bigStroke());
        QTest::qWait(2 * PageThumbnails::kSettleMs);
        QCOMPARE(thumbs.renderCount(), 1);
        QCOMPARE(thumbs.launchCount(), launches); // amendement 3 (m4) : rien lancé pendant le contact
        QCOMPARE(thumbs.pendingCount(), 1);
        QTest::mouseRelease(&f.window, Qt::LeftButton, Qt::NoModifier, QPoint(300, 300));
        QVERIFY(!f.view->contactActive());
        QCOMPARE(contact.count(), 2);
        QTRY_COMPARE_WITH_TIMEOUT(thumbs.renderCount(), 2, 5000);
        QVERIFY(darkAtCenter(thumbs.image(0)));
        QTest::qWait(2 * PageThumbnails::kSettleMs);
        QCOMPARE(thumbs.renderCount(), 2);
    }

    // Le rendu tourne sur un fil du réservoir, jamais sur le fil principal ; la copie seule y est faite.
    void renderRunsOffTheMainThread()
    {
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        PageThumbnails thumbs;
        thumbs.setDocumentController(&controller);
        QTRY_COMPARE_WITH_TIMEOUT(thumbs.renderCount(), 1, 5000);
        QVERIFY(thumbs.lastRenderThread() != nullptr);
        QVERIFY(thumbs.lastRenderThread() != QThread::currentThread());
        // Page de 300 objets (banc) : coût de la copie sur le fil principal relevé (mesure, preset debug).
        controller.loadGeneratedFirstPage();
        QTRY_COMPARE_WITH_TIMEOUT(thumbs.renderCount(), 2, 10000);
        QVERIFY(thumbs.lastRenderThread() != QThread::currentThread());
        // Q13 (L4-C10) : une seule page et un seul lancement depuis le premier, donc la copie relevée est celle
        // de la page générée ; une copie non relevée laisserait 0.
        QCOMPARE(controller.pageCount(), 1);
        QCOMPARE(thumbs.launchCount(), 2);
        QVERIFY(controller.document().page(0).objects.size() >= 300);
        QVERIFY(thumbs.lastCopyMs() > 0);
        qInfo("MESURE copie d'une page de 300 objets sur le fil principal : %.3f ms", thumbs.lastCopyMs());
    }

    // Une ligne par page, numérotée de 1 à N ; la page affichée est marquée.
    void rowsFollowPages()
    {
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        PageThumbnails thumbs;
        thumbs.setDocumentController(&controller);
        QCOMPARE(thumbs.rowCount(), 1);
        controller.nextPage();
        controller.nextPage();
        QCOMPARE(thumbs.rowCount(), 3);
        for (int row = 0; row < 3; ++row) {
            const QModelIndex index = thumbs.index(row);
            QCOMPARE(thumbs.data(index, PageThumbnails::PageNumberRole).toInt(), row + 1);
            QCOMPARE(thumbs.data(index, PageThumbnails::CurrentRole).toBool(), row == 2);
            QCOMPARE(thumbs.data(index, PageThumbnails::AspectRole).toDouble(), 768.0 / 1024.0);
        }
        QVERIFY(thumbs.roleNames().values().contains("pageNumber"));
        QTRY_COMPARE_WITH_TIMEOUT(thumbs.renderCount(), 3, 5000);
        controller.pageBack();
        QCOMPARE(thumbs.data(thumbs.index(1), PageThumbnails::CurrentRole).toBool(), true);
        QCOMPARE(thumbs.data(thumbs.index(2), PageThumbnails::CurrentRole).toBool(), false);
    }

    // Défaut 11 : un déplacement (MovePagesCommand : un PageRemoved puis un PageInserted par page) retire puis
    // insère, le nombre de pages ne change pas ; les lignes sont refaites.
    // M-H (L4-C10) : seule la ligne d'arrivée est rendue de nouveau. Q11 : avant tout rendu, les images gardées
    // ont déjà suivi leur page. Q4a2-m6 : l'attente fixe suit le QTRY et précède le contrôle « pas de rendu en
    // trop ».
    void moveRendersOnlyArrivedRow()
    {
        ui::DocumentController controller;
        threePagesSecondDark(controller);
        PageThumbnails thumbs;
        thumbs.setDocumentController(&controller);
        QTRY_COMPARE_WITH_TIMEOUT(thumbs.renderCount(), 3, 5000);
        QTRY_COMPARE_WITH_TIMEOUT(thumbs.pendingCount(), 0, 5000);
        QVERIFY(!darkAtCenter(thumbs.image(0)));
        QVERIFY(darkAtCenter(thumbs.image(1)));
        QVERIFY(!darkAtCenter(thumbs.image(2)));
        const int renders = thumbs.renderCount();
        const int launches = thumbs.launchCount();
        QSignalSpy reset(&thumbs, &QAbstractItemModel::modelReset);
        controller.execute(std::make_unique<core::MovePagesCommand>(std::vector<std::size_t>{2}, 0)); // [C, A, B]
        QCOMPARE(controller.pageCount(), 3);
        QCOMPARE(thumbs.rowCount(), 3);
        QCOMPARE(reset.count(), 1);
        QCOMPARE(thumbs.launchCount(), launches); // rien de rendu encore
        QVERIFY(thumbs.image(0).isNull());        // ligne d'arrivée : à rendre
        QVERIFY(!thumbs.image(1).isNull());       // l'image de A a suivi A
        QVERIFY(!darkAtCenter(thumbs.image(1)));
        QVERIFY(darkAtCenter(thumbs.image(2))); // l'image de B (sombre) a suivi B
        QCOMPARE(thumbs.pendingCount(), 1);
        QTRY_COMPARE_WITH_TIMEOUT(thumbs.renderCount(), renders + 1, 5000);
        QTest::qWait(2 * PageThumbnails::kSettleMs);
        QCOMPARE(thumbs.renderCount(), renders + 1);
        QCOMPARE(thumbs.launchCount(), launches + 1);
        QVERIFY(!thumbs.image(0).isNull());
        QVERIFY(!darkAtCenter(thumbs.image(0)));
        QVERIFY(!darkAtCenter(thumbs.image(1)));
        QVERIFY(darkAtCenter(thumbs.image(2)));
    }

    // Rendu lancé, puis document remplacé avant son retour : son résultat est jeté.
    void staleRenderIsDropped()
    {
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        controller.commitStroke(bigStroke()); // page 1 de l'ancien document : sombre
        PageThumbnails thumbs;
        bool replaced = false;
        bool darkSeen = false;
        // Branché en direct : le remplacement a lieu au lancement même, avant tout retour (livré en file).
        connect(&thumbs, &PageThumbnails::renderingChanged, &thumbs, [&] {
            if (!replaced && thumbs.launchCount() == 1) {
                replaced = true;
                controller.newDocument(1024, 768);
            }
            darkSeen = darkSeen || darkAtCenter(thumbs.image(0));
        });
        thumbs.setDocumentController(&controller);
        QTRY_VERIFY_WITH_TIMEOUT(replaced, 5000);
        QTRY_COMPARE_WITH_TIMEOUT(thumbs.renderCount(), 1, 5000);
        QTest::qWait(PageThumbnails::kSettleMs);
        QVERIFY(!darkSeen);
        QVERIFY(!thumbs.image(0).isNull());
        QVERIFY(!darkAtCenter(thumbs.image(0)));
        QCOMPARE(thumbs.renderCount(), 1);
        QCOMPARE(thumbs.launchCount(), 2);
    }

    // M-G (L4-C10) : objet soulevé (texte en édition, objets déplacés) : la page affichée n'a plus l'objet, sa
    // miniature garde l'image d'avant et n'est rendue qu'après le dépôt, comme Enregistrer attend le dépôt
    // (documentcontroller.h, liftObjects). La page est marquée juste avant le soulèvement : une marque en
    // attente ne doit pas non plus partir pendant le soulèvement.
    void liftedObjectStaysInThumbnail()
    {
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        controller.commitStroke(bigStroke());
        PageThumbnails thumbs;
        thumbs.setDocumentController(&controller);
        QTRY_COMPARE_WITH_TIMEOUT(thumbs.renderCount(), 1, 5000);
        QVERIFY(darkAtCenter(thumbs.image(0)));
        const auto id = controller.document().page(0).objects.front().common.id;
        controller.commitStroke(smallStroke(0));
        const int launches = thumbs.launchCount();
        QCOMPARE(controller.liftObjects({id}).size(), std::size_t{1});
        QVERIFY(controller.isLifting());
        QTest::qWait(3 * PageThumbnails::kSettleMs);
        QCOMPARE(thumbs.launchCount(), launches); // rien rendu pendant le soulèvement
        QVERIFY(darkAtCenter(thumbs.image(0)));   // l'objet soulevé reste dans la miniature
        controller.dropLifted(nullptr);
        QVERIFY(!controller.isLifting());
        QTRY_COMPARE_WITH_TIMEOUT(thumbs.launchCount(), launches + 1, 5000);
        QTRY_COMPARE_WITH_TIMEOUT(thumbs.pendingCount(), 0, 5000);
        QVERIFY(darkAtCenter(thumbs.image(0)));
        QTest::qWait(2 * PageThumbnails::kSettleMs);
        QCOMPARE(thumbs.launchCount(), launches + 1);
    }

    // M-H (L4-C10) : une page ajoutée est la seule rendue ; les autres gardent leur image sans nouveau rendu.
    void insertRendersOnlyNewPage()
    {
        ui::DocumentController controller;
        threePagesSecondDark(controller);
        PageThumbnails thumbs;
        thumbs.setDocumentController(&controller);
        QTRY_COMPARE_WITH_TIMEOUT(thumbs.renderCount(), 3, 5000);
        QTRY_COMPARE_WITH_TIMEOUT(thumbs.pendingCount(), 0, 5000);
        const int launches = thumbs.launchCount();
        controller.nextPage(); // quatrième page ajoutée en fin, affichée
        QCOMPARE(thumbs.rowCount(), 4);
        QCOMPARE(thumbs.pendingCount(), 1);
        QVERIFY(darkAtCenter(thumbs.image(1))); // gardée
        QTRY_COMPARE_WITH_TIMEOUT(thumbs.launchCount(), launches + 1, 5000);
        QTRY_COMPARE_WITH_TIMEOUT(thumbs.pendingCount(), 0, 5000);
        QTest::qWait(2 * PageThumbnails::kSettleMs);
        QCOMPARE(thumbs.launchCount(), launches + 1);
        QVERIFY(!thumbs.image(3).isNull());
        QVERIFY(darkAtCenter(thumbs.image(1)));
    }

    // M-H (L4-C10) : toutes les pages à rendre (document ouvert) : la page affichée part la première.
    void shownPageRendersFirst()
    {
        ui::DocumentController controller;
        threePagesSecondDark(controller);
        QCOMPARE(controller.currentIndex(), 2);
        PageThumbnails thumbs;
        QSignalSpy changed(&thumbs, &QAbstractItemModel::dataChanged);
        thumbs.setDocumentController(&controller);
        QCOMPARE(thumbs.pendingCount(), 3);
        QTRY_COMPARE_WITH_TIMEOUT(thumbs.renderCount(), 3, 5000);
        int first = -1;
        for (int i = 0; i < changed.count(); ++i) {
            const QList<QVariant> arguments = changed.at(i);
            const auto roles = arguments.at(2).value<QList<int>>();
            if (roles.contains(PageThumbnails::RevisionRole)) {
                first = arguments.at(0).value<QModelIndex>().row();
                break;
            }
        }
        QCOMPARE(first, 2);
    }
};

QTEST_MAIN(TstUiPagethumbnails)
#include "tst_ui_pagethumbnails.moc"
