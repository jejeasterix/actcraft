#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pagethumbnails.h>
#include <actcraft/ui/pageview.h>

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

// Trait large en travers de la page : le centre de la miniature est sombre.
core::Stroke bigStroke()
{
    core::Stroke stroke;
    stroke.points = {core::StrokePoint{100, 100, 0.5f}, core::StrokePoint{900, 700, 0.5f}};
    stroke.baseWidth = 30;
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

core::Change pageChange(core::Change::Kind kind, std::size_t page)
{
    core::Change change{};
    change.kind = kind;
    change.page = page;
    change.object = 0;
    change.area = core::RectF{};
    return change;
}

// Déplacement d'une page en tête, avec les changements que L4-T15 fixe pour MovePagesCommand (un PageRemoved
// à l'indice du moment, puis un PageInserted à l'indice d'arrivée). MovePagesCommand est encore un talon dans
// cette base (L4-T15 est de la même vague) : le contrat suivi par les miniatures est celui des changements.
class MovePageToFront final : public core::UndoCommand
{
public:
    explicit MovePageToFront(std::size_t from) : m_from(from) {}
    void redo(core::Document& document, core::Changes& changes) override
    {
        core::Page page = document.takePage(m_from);
        changes.push_back(pageChange(core::Change::Kind::PageRemoved, m_from));
        document.insertPage(0, std::move(page));
        changes.push_back(pageChange(core::Change::Kind::PageInserted, 0));
    }
    void undo(core::Document& document, core::Changes& changes) override
    {
        core::Page page = document.takePage(0);
        changes.push_back(pageChange(core::Change::Kind::PageRemoved, 0));
        document.insertPage(m_from, std::move(page));
        changes.push_back(pageChange(core::Change::Kind::PageInserted, m_from));
    }

private:
    std::size_t m_from;
};

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
    void rendersAfterSettling()
    {
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        PageThumbnails thumbs;
        thumbs.setDocumentController(&controller);
        QTRY_COMPARE_WITH_TIMEOUT(thumbs.renderCount(), 1, 5000); // première image de la page vierge
        QVERIFY(!darkAtCenter(thumbs.image(0)));
        controller.commitStroke(bigStroke());
        QTest::qWait(PageThumbnails::kSettleMs / 2);
        QCOMPARE(thumbs.renderCount(), 1); // pas encore
        QCOMPARE(thumbs.pendingCount(), 1);
        QTRY_COMPARE_WITH_TIMEOUT(thumbs.renderCount(), 2, 5000);
        const QImage image = thumbs.image(0);
        QCOMPARE(image.width(), PageThumbnails::kDefaultWidth);
        QCOMPARE(image.height(), PageThumbnails::kDefaultWidth * 768 / 1024);
        QVERIFY(qGray(image.pixel(image.width() / 2, image.height() / 2)) < 128);
        QCOMPARE(thumbs.pendingCount(), 0);
    }

    // Dix traits à 20 ms d'écart : un seul rendu de plus (regroupé), pas dix.
    void oneRenderForManyEdits()
    {
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        PageThumbnails thumbs;
        thumbs.setDocumentController(&controller);
        QTRY_COMPARE_WITH_TIMEOUT(thumbs.renderCount(), 1, 5000);
        for (int i = 0; i < 10; ++i) {
            controller.commitStroke(smallStroke(i));
            QTest::qWait(20);
        }
        QTRY_COMPARE_WITH_TIMEOUT(thumbs.renderCount(), 2, 5000);
        QTest::qWait(2 * PageThumbnails::kSettleMs);
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
        QVERIFY(thumbs.lastCopyMs() >= 0);
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

    // Défaut 11 : un déplacement retire puis insère, le nombre de pages ne change pas ; toutes les lignes
    // sont rendues de nouveau, la miniature suit la page.
    void moveRerendersShiftedRows()
    {
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        controller.nextPage();
        controller.commitStroke(bigStroke()); // page 2 (indice 1) sombre
        controller.nextPage();
        PageThumbnails thumbs;
        thumbs.setDocumentController(&controller);
        QTRY_COMPARE_WITH_TIMEOUT(thumbs.renderCount(), 3, 5000);
        QVERIFY(!darkAtCenter(thumbs.image(0)));
        QVERIFY(darkAtCenter(thumbs.image(1)));
        const int before = thumbs.renderCount();
        QSignalSpy reset(&thumbs, &QAbstractItemModel::modelReset);
        controller.execute(std::make_unique<MovePageToFront>(1));
        QCOMPARE(controller.pageCount(), 3);
        QCOMPARE(thumbs.rowCount(), 3);
        QCOMPARE(reset.count(), 1);
        QTest::qWait(2 * PageThumbnails::kSettleMs);
        QTRY_COMPARE_WITH_TIMEOUT(thumbs.renderCount(), before + 3, 5000);
        QVERIFY(darkAtCenter(thumbs.image(0))); // l'ancienne page 2
        QVERIFY(!darkAtCenter(thumbs.image(1)));
        QVERIFY(!darkAtCenter(thumbs.image(2)));
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
};

QTEST_MAIN(TstUiPagethumbnails)
#include "tst_ui_pagethumbnails.moc"
