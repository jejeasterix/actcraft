#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/objectedits.h>
#include <actcraft/tools/erasertool.h>
#include <actcraft/tools/toolcontext.h>
#include <actcraft/tools/userstate.h>
#include <actcraft/ui/documentcontroller.h>

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

#include <algorithm>
#include <cstddef>
#include <cstdint>
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

std::size_t countOf(const Calls& calls, char kind)
{
    return static_cast<std::size_t>(
        std::count_if(calls.begin(), calls.end(), [kind](const auto& call) { return call.first == kind; }));
}

// tools::DocumentEditor sur le vrai contrôleur, comme ui::DocumentEditorAdapter (privé au module) ; compte
// les identifiants demandés à lift().
class ControllerEditor final : public tools::DocumentEditor
{
public:
    explicit ControllerEditor(ui::DocumentController& controller) : m_controller(controller) {}
    std::size_t liftedIds = 0;
    const core::Page& page() const override { return m_controller.document().page(pageIndex()); }
    std::size_t pageIndex() const override { return static_cast<std::size_t>(m_controller.currentIndex()); }
    void execute(std::unique_ptr<core::UndoCommand> command) override
    {
        m_controller.execute(std::move(command));
    }
    std::vector<core::PageObject> lift(const std::vector<core::ObjectId>& ids) override
    {
        liftedIds += ids.size();
        return m_controller.liftObjects(ids);
    }
    void drop(std::unique_ptr<core::UndoCommand> command) override
    {
        m_controller.dropLifted(std::move(command));
    }
    bool isLifting() const override { return m_controller.isLifting(); }

private:
    ui::DocumentController& m_controller;
};

class CountingLift final : public tools::LiftSurface
{
public:
    std::size_t lastShown = 0;
    std::size_t maxShown = 0;
    void show(std::vector<core::PageObject> objects) override
    {
        lastShown = objects.size();
        maxShown = std::max(maxShown, lastShown);
    }
    void setTransform(const core::Transform&) override {}
    void clear() override { lastShown = 0; }
};

class NullInk final : public tools::InkSurface
{
public:
    void beginStroke(core::Argb, double) override {}
    void addPoint(const core::StrokePoint&, std::uint64_t) override {}
    void clear() override {}
};

class NullStrokes final : public tools::DocumentSink
{
public:
    void commitStroke(core::Stroke) override {}
};

class NullSelection final : public tools::SelectionSurface
{
public:
    void show(const tools::SelectionView&) override {}
};

class NullView final : public tools::ViewInfo
{
public:
    double zoom() const override { return 1; }
    bool overTrash(core::PointF) const override { return false; }
    void setTrashHover(bool) override {}
    QStringList objectBarCommands() const override { return {}; }
};

class NullCommands final : public tools::CommandSink
{
public:
    bool trigger(const QString&, const QVariant&) override { return false; }
};

// Modèle du cache mené par l'observateur, comme la page fixe : retrait et insertion par IDENTIFIANT (Change
// ne porte aucun rang), l'inséré prenant sa place dans l'ordre de la page. `faults` compte un retrait
// d'absent ou une insertion de présent ; inStep() dit si le cache montre exactement la page, dans son ordre.
struct CacheModel
{
    const ui::DocumentController* controller = nullptr;
    std::vector<core::ObjectId> ids;
    int faults = 0;
    std::vector<core::ObjectId> pageIds() const
    {
        std::vector<core::ObjectId> out;
        for (const core::PageObject& object : controller->document().page(0).objects) {
            out.push_back(object.common.id);
        }
        return out;
    }
    void attach(ui::DocumentController& c)
    {
        controller = &c;
        ids = pageIds();
        c.addChangeObserver([this](const core::Changes& changes, Origin) {
            for (const core::Change& change : changes) {
                const auto at = std::find(ids.begin(), ids.end(), change.object);
                if (change.kind == core::Change::Kind::ObjectRemoved) {
                    if (at == ids.end()) {
                        ++faults;
                    } else {
                        ids.erase(at);
                    }
                } else if (change.kind == core::Change::Kind::ObjectInserted) {
                    const std::vector<core::ObjectId> page = pageIds();
                    const auto rank = std::find(page.begin(), page.end(), change.object);
                    if (at != ids.end() || rank == page.end()) {
                        ++faults;
                        continue;
                    }
                    // devant le premier objet du cache qui le suit dans la page
                    auto before = ids.begin();
                    while (before != ids.end() && std::find(page.begin(), rank, *before) != rank) {
                        ++before;
                    }
                    ids.insert(before, change.object);
                }
            }
        });
    }
    bool inStep() const { return faults == 0 && ids == pageIds(); }
};

input::Sample sample(input::Phase phase, double x, double y)
{
    input::Sample s;
    s.x = x;
    s.y = y;
    s.phase = phase;
    s.device = input::DeviceKind::Mouse;
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

    // L3-C5 : un second soulèvement pendant le même soulèvement AJOUTE aux objets soulevés (contrat de
    // tools::DocumentEditor::lift) ; il ne remet les premiers ni dans la page, ni dans le cache.
    void secondLiftAddsToTheFirst()
    {
        ui::DocumentController controller;
        controller.commitStroke(line(100));
        controller.commitStroke(line(200));
        controller.commitStroke(line(300));
        const core::ObjectId a = controller.document().page(0).objects[0].common.id;
        const core::ObjectId b = controller.document().page(0).objects[1].common.id;
        const core::ObjectId c = controller.document().page(0).objects[2].common.id;
        CacheSpy spy;
        spy.attach(controller);
        controller.liftObjects({c});
        QCOMPARE(controller.liftObjects({a}).size(), std::size_t{1});
        QVERIFY(controller.isLifting());
        QCOMPARE(controller.shownObjectCount(), 1);       // seul b reste dans la page
        QCOMPARE(spy.calls, (Calls{{'r', c}, {'r', a}})); // aucun retour au cache entre les deux
        controller.dropLifted(nullptr);
        QVERIFY(!controller.isLifting());
        const std::vector<core::PageObject>& after = controller.document().page(0).objects;
        QCOMPARE(after.size(), std::size_t{3});
        QCOMPARE(after[0].common.id, a); // rangs d'origine
        QCOMPARE(after[1].common.id, b);
        QCOMPARE(after[2].common.id, c);
    }

    // L3-C5 : la gomme sur le vrai contrôleur. Chaque trait touché est soulevé une fois, quel que soit le
    // nombre d'allers-retours : traits soulevés, objets montrés et changements du cache sont bornés par les
    // traits touchés, pas par l'historique du geste (au banc, avant correctif : 792 traits soulevés pour 24).
    void eraserLiftsEachStrokeOnceWhateverThePasses()
    {
        for (const int passes : {2, 6}) {
            ui::DocumentController controller;
            CacheSpy spy;
            ControllerEditor editor{controller};
            CountingLift lift;
            NullInk ink;
            NullStrokes strokes;
            NullSelection selection;
            NullView view;
            NullCommands commands;
            tools::UserState user;
            tools::ToolContext context{editor, ink, strokes, lift, selection, view, commands, user};
            tools::EraserTool eraser{context};
            // Deux traits horizontaux (y 200 et 300) ; passages verticaux qui les croisent, espacés de 2 r +
            // 10 : chacun coupe chaque trait une fois, à part des autres ; entre deux passages, un pas
            // horizontal loin des traits.
            const double r = eraser.radius();
            QVERIFY(r > 0);
            const double spacing = 2 * r + 10;
            const double top = 200 - 2 * r - 20;
            const double bottom = 300 + 2 * r + 20;
            const double end = 100 + 4 * r + passes * spacing;
            for (const double at : {200.0, 300.0}) {
                core::Stroke stroke;
                stroke.points = {{100, at, 0.5f}, {end, at, 0.5f}};
                controller.commitStroke(stroke);
            }
            spy.attach(controller);
            double x = 100 + 2 * r;
            double y = top;
            eraser.onSample(sample(input::Phase::Press, x, y));
            for (int pass = 0; pass < passes; ++pass) {
                const double from = y;
                const double to = pass % 2 == 0 ? bottom : top;
                for (int i = 1; i <= 20; ++i) {
                    eraser.onSample(sample(input::Phase::Move, x, from + (to - from) * i / 20));
                }
                y = to;
                x += spacing;
                eraser.onSample(sample(input::Phase::Move, x, y));
            }
            const std::size_t pieces = static_cast<std::size_t>(2 * (passes + 1));
            QCOMPARE(editor.liftedIds, std::size_t{2});
            QCOMPARE(countOf(spy.calls, 'i'), std::size_t{0}); // aucun trait remis au cache pendant le geste
            QCOMPARE(countOf(spy.calls, 'r'), std::size_t{2});
            QCOMPARE(lift.lastShown, pieces);
            QCOMPARE(lift.maxShown, pieces);
            eraser.onSample(sample(input::Phase::Release, x, y));
            QCOMPARE(controller.shownObjectCount(), static_cast<int>(pieces)); // sans morceau en double
        }
    }

    // L3-C5 : rangs mélangés. Les rangs de la page entière que liftObjects reprend ne vont jamais au cache
    // (Change n'a qu'un identifiant) : après un second soulèvement d'objets placés avant, entre et après les
    // premiers, puis au dépôt, le cache montre exactement la page, dans son ordre.
    void mixedRanksKeepTheCacheInStep()
    {
        ui::DocumentController controller;
        for (int i = 0; i < 7; ++i) {
            controller.commitStroke(line(100 + 50 * i));
        }
        const std::vector<core::PageObject> original = controller.document().page(0).objects;
        const auto id = [&original](int rank) { return original[static_cast<std::size_t>(rank)].common.id; };
        CacheModel cache;
        cache.attach(controller);
        controller.liftObjects({id(2), id(4)});
        QVERIFY(cache.inStep());
        QCOMPARE(controller.liftObjects({id(6), id(0), id(3)}).size(), std::size_t{3}); // après, avant, entre
        QVERIFY(cache.inStep());
        QCOMPARE(cache.ids, (std::vector<core::ObjectId>{id(1), id(5)}));
        controller.dropLifted(nullptr);
        QVERIFY(cache.inStep());
        QCOMPARE(cache.pageIds().size(), original.size());
        for (int rank = 0; rank < 7; ++rank) {
            QCOMPARE(cache.ids[static_cast<std::size_t>(rank)], id(rank)); // rangs d'origine
        }
    }

    // L3-C5 : la gomme lève des traits à des rangs mélangés (d'abord le 4, puis le 2 et le 0 avant lui, puis
    // le 6 après), des traits non touchés entre eux. Le cache suit la page à chaque échantillon, au relâché
    // (ReplaceObjectsCommand) et à l'abandon du geste (Échap : tout revient à sa place).
    void eraserAtMixedRanksKeepsTheCacheInStep()
    {
        for (const bool cancel : {false, true}) {
            ui::DocumentController controller;
            for (int i = 0; i < 7; ++i) {
                // rangs pairs à gauche (x 100 à 300, sous la gomme), impairs à droite (x 500 à 700, jamais
                // touchés)
                const double left = i % 2 == 0 ? 100 : 500;
                core::Stroke stroke;
                stroke.points = {{left, 100.0 + 50 * i, 0.5f}, {left + 200, 100.0 + 50 * i, 0.5f}};
                controller.commitStroke(stroke);
            }
            const std::vector<core::PageObject> original = controller.document().page(0).objects;
            CacheModel cache;
            cache.attach(controller);
            ControllerEditor editor{controller};
            CountingLift lift;
            NullInk ink;
            NullStrokes strokes;
            NullSelection selection;
            NullView view;
            NullCommands commands;
            tools::UserState user;
            tools::ToolContext context{editor, ink, strokes, lift, selection, view, commands, user};
            tools::EraserTool eraser{context};
            const double r = eraser.radius();
            QVERIFY(r > 0 && r < 50); // les traits de gauche sont à 100 px l'un de l'autre
            // trait 4 (y 300) d'abord, puis montée jusqu'au 0 (y 100), puis descente jusqu'au 6 (y 400)
            const auto path = [](double from, double to) {
                std::vector<double> ys;
                for (int i = 1; i <= 40; ++i) {
                    ys.push_back(from + (to - from) * i / 40);
                }
                return ys;
            };
            eraser.onSample(sample(input::Phase::Press, 200, 300));
            QVERIFY(cache.inStep());
            for (const auto& [from, to] : {std::pair{300.0, 60.0}, std::pair{60.0, 440.0}}) {
                for (const double y : path(from, to)) {
                    eraser.onSample(sample(input::Phase::Move, 200, y));
                    QVERIFY(cache.inStep());
                }
            }
            QCOMPARE(editor.liftedIds, std::size_t{4});
            QCOMPARE(cache.ids, (std::vector<core::ObjectId>{original[1].common.id, original[3].common.id,
                                                             original[5].common.id}));
            if (cancel) {
                eraser.cancel();
            } else {
                eraser.onSample(sample(input::Phase::Release, 200, 440));
            }
            QVERIFY(!controller.isLifting());
            QVERIFY(cache.inStep());
            const std::vector<core::PageObject>& after = controller.document().page(0).objects;
            if (cancel) {
                QCOMPARE(after.size(), original.size());
                for (std::size_t rank = 0; rank < original.size(); ++rank) {
                    QCOMPARE(after[rank].common.id, original[rank].common.id);
                }
            } else {
                QCOMPARE(after.size(),
                         std::size_t{3 + 2 * 4}); // trois intacts, deux morceaux par trait coupé
                QVERIFY(core::indexOf(controller.document().page(0), original[0].common.id) == after.size());
            }
        }
    }
};

QTEST_MAIN(TstUiLift)
#include "tst_ui_lift.moc"
