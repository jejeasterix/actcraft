#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/objectedits.h>
#include <actcraft/ui/documentcontroller.h>

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

using namespace actcraft;

namespace {

using Origin = ui::DocumentController::ChangeOrigin;
using Calls = std::vector<std::pair<char, core::ObjectId>>;

// Observateur unique du socle (L0-S5) : reçoit exactement les changements passés au cache, et leur origine.
struct CacheSpy
{
    Calls calls; // 'i' inséré, 'r' retiré, 'v' changé
    std::vector<Origin> origins;
    void attach(ui::DocumentController& controller)
    {
        controller.addChangeObserver([this](const core::Changes& changes, Origin origin) {
            origins.push_back(origin);
            for (const core::Change& c : changes) {
                switch (c.kind) {
                case core::Change::Kind::ObjectInserted:
                    calls.emplace_back('i', c.object);
                    break;
                case core::Change::Kind::ObjectRemoved:
                    calls.emplace_back('r', c.object);
                    break;
                case core::Change::Kind::ObjectChanged:
                case core::Change::Kind::PageChanged:
                    calls.emplace_back('v', c.object);
                    break;
                case core::Change::Kind::PageInserted:
                case core::Change::Kind::PageRemoved:
                case core::Change::Kind::PageSettingsChanged:
                    break; // sans effet de cache (C4062)
                }
            }
        });
    }
};

core::Stroke line(double y)
{
    core::Stroke s;
    s.points = {{100, y, 0.5f}, {300, y, 0.5f}};
    return s;
}

} // namespace

// Soulever et déposer (DA11, L3-T12) : le contrôleur reste la seule voie vers le cache.
class TstUiLift : public QObject
{
    Q_OBJECT

private slots:
    void liftRemovesThroughTheCacheInterface()
    {
        ui::DocumentController controller;
        CacheSpy spy;
        spy.attach(controller);
        controller.commitStroke(line(100));
        const core::ObjectId id = controller.document().page(0).objects.back().common.id;
        spy.calls.clear();
        QSignalSpy about(&controller, &ui::DocumentController::editAboutToApply);
        QSignalSpy lifting(&controller, &ui::DocumentController::liftingChanged);
        QSignalSpy undoSpy(&controller, &ui::DocumentController::undoChanged);
        const auto copies = controller.liftObjects({id});
        QCOMPARE(copies.size(), std::size_t{1});
        QCOMPARE(copies[0].common.id, id);
        QCOMPARE(about.count(), 0); // B1 : soulever n'annonce rien, sinon l'outil qui soulève s'annulerait
        QCOMPARE(lifting.count(), 1);
        QCOMPARE(undoSpy.count(), 0); // hors pile
        QVERIFY(controller.isLifting());
        QCOMPARE(controller.shownObjectCount(), 0);
        QVERIFY(spy.calls == (Calls{{'r', id}})); // retiré du cache
        QCOMPARE(static_cast<int>(spy.origins.back()), static_cast<int>(Origin::Lift));
    }

    // Le cache n'a plus l'objet soulevé : au dépôt, il le reçoit inséré (core::changesAfterLift), jamais
    // l'invalidation brute de la commande (un objet qu'il n'a plus).
    void dropInsertsLiftedObjects()
    {
        ui::DocumentController controller;
        CacheSpy spy;
        spy.attach(controller);
        controller.commitStroke(line(100));
        const core::ObjectId id = controller.document().page(0).objects.back().common.id;
        controller.liftObjects({id});
        spy.calls.clear();
        QSignalSpy lifting(&controller, &ui::DocumentController::liftingChanged);
        controller.dropLifted(std::make_unique<core::TransformObjectsCommand>(
            0, std::vector<core::ObjectId>{id}, core::translation(40, 0)));
        QVERIFY(spy.calls == (Calls{{'i', id}})); // inséré, pas invalidé
        QCOMPARE(static_cast<int>(spy.origins.back()), static_cast<int>(Origin::Drop));
        QCOMPARE(lifting.count(), 1);
        QVERIFY(!controller.isLifting());
        QCOMPARE(controller.document().page(0).objects.back().common.transform.dx, 40.0);
        controller.undo(); // une étape
        QCOMPARE(controller.document().page(0).objects.back().common.transform.dx, 0.0);
        QCOMPARE(controller.shownObjectCount(), 1);
    }

    void dropWithoutCommandAddsNoStep()
    {
        ui::DocumentController controller;
        controller.commitStroke(line(100));
        const core::ObjectId id = controller.document().page(0).objects.back().common.id;
        const QString before = controller.fingerprintText();
        controller.liftObjects({id});
        QSignalSpy undoSpy(&controller, &ui::DocumentController::undoChanged);
        controller.dropLifted(nullptr);
        QCOMPARE(controller.fingerprintText(), before);
        QCOMPARE(undoSpy.count(), 0); // aucune étape
        QVERIFY(!controller.isLifting());
        controller.undo(); // annule le trait, pas un geste
        QCOMPARE(controller.shownObjectCount(), 0);
    }

    void undoWhileLiftingCancelsFirst()
    {
        ui::DocumentController controller;
        controller.commitStroke(line(100));
        controller.commitStroke(line(200));
        const core::ObjectId first = controller.document().page(0).objects.front().common.id;
        controller.liftObjects({first});
        controller.undo(); // dépose sans étape, puis annule le second trait
        QVERIFY(!controller.isLifting());
        QCOMPARE(controller.shownObjectCount(), 1);
        QCOMPARE(controller.document().page(0).objects.front().common.id, first);
    }

    // M2 : le changement de page est annoncé pendant que les objets sont encore soulevés (l'outil abandonne
    // son geste), puis le contrôleur dépose sans étape ce qui reste soulevé.
    void pageChangeWhileLiftingDropsFirst()
    {
        ui::DocumentController controller;
        controller.commitStroke(line(100));
        const core::ObjectId id = controller.document().page(0).objects.back().common.id;
        controller.liftObjects({id});
        bool liftingWhenAnnounced = false;
        connect(&controller, &ui::DocumentController::pageAboutToChange, &controller,
                [&] { liftingWhenAnnounced = controller.isLifting(); });
        QSignalSpy about(&controller, &ui::DocumentController::pageAboutToChange);
        controller.nextPage(); // annonce, puis dépose sans étape ce qui reste soulevé
        QCOMPARE(about.count(), 1);
        QVERIFY(liftingWhenAnnounced);
        QVERIFY(!controller.isLifting());
        QCOMPARE(controller.document().page(0).objects.size(), std::size_t{1});
        QCOMPARE(controller.currentIndex(), 1);
    }

    // Enregistrer n'écrit jamais un document dont des objets sont soulevés.
    void saveWhileLiftingDropsFirst()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        ui::DocumentController controller;
        controller.commitStroke(line(100));
        const core::ObjectId id = controller.document().page(0).objects.back().common.id;
        controller.liftObjects({id});
        bool liftingWhenAnnounced = false;
        connect(&controller, &ui::DocumentController::editAboutToApply, &controller,
                [&] { liftingWhenAnnounced = controller.isLifting(); });
        QSignalSpy about(&controller, &ui::DocumentController::editAboutToApply);
        QSignalSpy saved(&controller, &ui::DocumentController::saveFinished);
        controller.saveToUrl(QUrl::fromLocalFile(dir.filePath(QStringLiteral("a.actcraft"))));
        QCOMPARE(about.count(), 1); // l'outil abandonne son geste avant l'écriture
        QVERIFY(liftingWhenAnnounced);
        QVERIFY(!controller.isLifting());
        QVERIFY(saved.wait(10000));
        QCOMPARE(saved.first().first().toBool(), true);
        QVERIFY(!controller.isModified());
        // rouvert : l'objet est dans le fichier
        QSignalSpy loaded(&controller, &ui::DocumentController::loadFinished);
        controller.openUrl(QUrl::fromLocalFile(dir.filePath(QStringLiteral("a.actcraft"))));
        QVERIFY(loaded.wait(10000));
        QCOMPARE(loaded.first().first().toBool(), true);
        QCOMPARE(controller.shownObjectCount(), 1);
    }

    // Document remplacé pendant un soulèvement (replaceDocument direct, comme le banc) : rien de l'ancien
    // document n'entre dans le nouveau ; « Nouveau » annonce l'oubli.
    void replacedDocumentForgetsTheLift()
    {
        ui::DocumentController controller;
        controller.commitStroke(line(100));
        const core::ObjectId id = controller.document().page(0).objects.back().common.id;
        controller.liftObjects({id});
        QVERIFY(controller.isLifting());
        controller.replaceDocument(core::Document(800, 600), QUrl(), QString());
        QVERIFY(!controller.isLifting());
        controller.dropLifted(nullptr);
        QCOMPARE(controller.shownObjectCount(), 0);
        QVERIFY(!controller.canUndo());
        QSignalSpy lifting(&controller, &ui::DocumentController::liftingChanged);
        controller.commitStroke(line(100));
        const core::ObjectId fresh = controller.document().page(0).objects.back().common.id;
        controller.liftObjects({fresh});
        controller.newDocument(800, 600);
        QVERIFY(!controller.isLifting());
        QCOMPARE(lifting.count(), 2); // soulevé, puis oublié
        QCOMPARE(controller.shownObjectCount(), 0);
    }
};

QTEST_MAIN(TstUiLift)
#include "tst_ui_lift.moc"
