#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/hittest.h>
#include <actcraft/core/objectedits.h>
#include <actcraft/core/undo.h>

#include <QTest>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

using namespace actcraft::core;

namespace {

Document twoStrokes(ObjectId& a, ObjectId& b)
{
    Document doc;
    Stroke s;
    s.points = {{100, 100, 0.5f}, {300, 100, 0.5f}};
    Changes ignored;
    auto annotation = [](Stroke st) {
        PageObject o{std::move(st)};
        o.common.layer = Layer::Top; // annotation (F-230)
        return o;
    };
    AddObjectCommand first(0, annotation(s));
    first.redo(doc, ignored);
    a = first.id();
    s.points = {{100, 200, 0.5f}, {300, 200, 0.5f}};
    AddObjectCommand second(0, annotation(s));
    second.redo(doc, ignored);
    b = second.id();
    return doc;
}

int count(const Changes& changes, Change::Kind kind, ObjectId id)
{
    int n = 0;
    for (const Change& c : changes) {
        n += (c.kind == kind && c.object == id) ? 1 : 0;
    }
    return n;
}

// Ajoute un objet sur la couche `layer` de la page 0 ; rend son identifiant.
template <typename T> ObjectId addTo(Document& doc, T body, Layer layer, bool locked = false)
{
    PageObject o{std::move(body)};
    o.common.layer = layer;
    o.common.locked = locked;
    Changes ignored;
    AddObjectCommand add(0, std::move(o));
    add.redo(doc, ignored);
    return add.id();
}

Stroke strokeAt(double y)
{
    Stroke s;
    s.points = {{100, y, 0.5f}, {300, y, 0.5f}};
    return s;
}

Shape shapeAt(double x)
{
    Shape s;
    s.bounds = {x, 400, 40, 40};
    return s;
}

ImageObject imageAt(double x)
{
    ImageObject im;
    im.bounds = {x, 500, 60, 40};
    return im;
}

std::vector<ObjectId> idsOf(const Page& page)
{
    std::vector<ObjectId> ids;
    for (const PageObject& o : page.objects) {
        ids.push_back(o.common.id);
    }
    return ids;
}

// Page attendue : celle d'avant, réduite aux objets `kept` (fond, grille et le reste inchangés).
Page keepingOnly(Page page, const std::vector<ObjectId>& kept)
{
    std::erase_if(page.objects, [&kept](const PageObject& o) {
        return std::find(kept.begin(), kept.end(), o.common.id) == kept.end();
    });
    return page;
}

void dressPage(Page& page)
{
    page.background.fill = solidFill(0xFF336699u);
    page.background.image = "background-key";
    page.grid.visible = true;
    page.grid.snapEnabled = true;
    page.grid.levels.emplace_back();
}

bool isOnePageChange(const Changes& c, std::size_t page)
{
    return c.size() == 1 && c[0].kind == Change::Kind::PageChanged && c[0].page == page && c[0].object == 0;
}

} // namespace

class TstCoreObjectedits : public QObject
{
    Q_OBJECT

private slots:
    void transformUndoRestores()
    {
        ObjectId a = 0, b = 0;
        Document doc = twoStrokes(a, b);
        const std::uint64_t before = fingerprint(doc.page(0));
        TransformObjectsCommand move(0, {a, b}, translation(10, 20));
        Changes c;
        move.redo(doc, c);
        QCOMPARE(c.size(), std::size_t{2});
        QCOMPARE(doc.page(0).objects[0].common.transform.dx, 10.0);
        c.clear();
        move.undo(doc, c);
        QCOMPARE(fingerprint(doc.page(0)), before);
    }

    // Zone d'un ObjectChanged : emprise avant ∪ emprise après (le cache efface l'ancienne place).
    void transformAreaJoinsBeforeAndAfter()
    {
        ObjectId a = 0, b = 0;
        Document doc = twoStrokes(a, b);
        const RectF old = boundsOnPage(doc.page(0).objects[0]);
        TransformObjectsCommand move(0, {a}, translation(500, 0));
        Changes c;
        move.redo(doc, c);
        const RectF now = boundsOnPage(doc.page(0).objects[0]);
        QCOMPARE(c.size(), std::size_t{1});
        QCOMPARE(c[0].kind, Change::Kind::ObjectChanged);
        QCOMPARE(c[0].object, a);
        QCOMPARE(c[0].area.x, old.x);
        QVERIFY(qAbs(c[0].area.x + c[0].area.width - (now.x + now.width)) < 1e-9);
        QVERIFY(now.x > old.x + old.width); // les deux emprises sont disjointes : la réunion les couvre
    }

    void removeRestoresAtSameDepth()
    {
        ObjectId a = 0, b = 0;
        Document doc = twoStrokes(a, b);
        RemoveObjectsCommand remove(0, {a});
        Changes c;
        remove.redo(doc, c);
        QCOMPARE(doc.page(0).objects.size(), std::size_t{1});
        c.clear();
        remove.undo(doc, c);
        QCOMPARE(doc.page(0).objects[0].common.id, a);
        QCOMPARE(count(c, Change::Kind::ObjectInserted, a), 1);
    }

    void clearAnnotationsKeepsShapes()
    {
        ObjectId a = 0, b = 0;
        Document doc = twoStrokes(a, b);
        Shape shape;
        shape.bounds = {10, 10, 50, 50};
        Changes c;
        AddObjectCommand add(0, PageObject{shape});
        add.redo(doc, c);
        c.clear();
        ClearPageCommand clear(0, ClearKind::Annotations);
        clear.redo(doc, c);
        QCOMPARE(doc.page(0).objects.size(), std::size_t{1});
        QVERIFY(std::holds_alternative<Shape>(body(doc.page(0).objects[0])));
        QCOMPARE(c.size(), std::size_t{1});
        QCOMPARE(c[0].kind, Change::Kind::PageChanged);
        c.clear();
        clear.undo(doc, c);
        QCOMPARE(doc.page(0).objects.size(), std::size_t{3});
    }

    // P9 et ancien dépôt (document.py, filtre des verrouillés commun aux trois effacements) : une annotation
    // verrouillée reste, comme sous la gomme (D4).
    void clearAnnotationsSparesLocked()
    {
        Document doc;
        const ObjectId loose = addTo(doc, strokeAt(100), Layer::Top);
        const ObjectId locked = addTo(doc, strokeAt(200), Layer::Top, true);
        ClearPageCommand clear(0, ClearKind::Annotations);
        Changes c;
        clear.redo(doc, c);
        QVERIFY(isOnePageChange(c, 0));
        QVERIFY(idsOf(doc.page(0)) == std::vector<ObjectId>{locked});
        QCOMPARE(indexOf(doc.page(0), loose), doc.page(0).objects.size());
    }

    // D8 : « Objets » retire tout ce qui n'est pas une annotation, sur toutes les couches, celle du fond
    // comprise, sauf les verrouillés ; fond et grille restent.
    void clearObjectsSweepsEveryLayerButLocked()
    {
        Document doc;
        const ObjectId backImage = addTo(doc, imageAt(10), Layer::Background);
        const ObjectId lowStroke =
            addTo(doc, strokeAt(300), Layer::Bottom); // hors du dessus : pas une annotation
        const ObjectId middleShape = addTo(doc, shapeAt(10), Layer::Middle);
        const ObjectId lockedShape = addTo(doc, shapeAt(60), Layer::Middle, true);
        const ObjectId ink = addTo(doc, strokeAt(100), Layer::Top);
        const ObjectId topShape = addTo(doc, shapeAt(110), Layer::Top);
        dressPage(doc.page(0));
        const std::uint64_t before = fingerprint(doc.page(0));
        const Page expected = keepingOnly(doc.page(0), {lockedShape, ink});

        UndoStack stack;
        const Changes c = stack.push(doc, std::make_unique<ClearPageCommand>(0, ClearKind::Objects));
        QVERIFY(isOnePageChange(c, 0));
        QVERIFY(idsOf(doc.page(0)) == (std::vector<ObjectId>{lockedShape, ink}));
        for (const ObjectId gone : {backImage, lowStroke, middleShape, topShape}) {
            QCOMPARE(indexOf(doc.page(0), gone), doc.page(0).objects.size());
        }
        QCOMPARE(fingerprint(doc.page(0)), fingerprint(expected)); // fond et grille gardés
        QCOMPARE(stack.count(), std::size_t{1});
        stack.undo(doc);
        QCOMPARE(fingerprint(doc.page(0)), before);
    }

    // D8 : « Page » retire les annotations et les objets non verrouillés ; fond, grille et verrouillés
    // restent.
    void clearPageKeepsBackgroundGridAndLocked()
    {
        Document doc;
        addTo(doc, imageAt(10), Layer::Background);
        addTo(doc, shapeAt(10), Layer::Middle);
        const ObjectId lockedShape = addTo(doc, shapeAt(60), Layer::Middle, true);
        addTo(doc, strokeAt(100), Layer::Top);
        const ObjectId lockedInk = addTo(doc, strokeAt(200), Layer::Top, true);
        dressPage(doc.page(0));
        const std::uint64_t before = fingerprint(doc.page(0));
        const Page expected = keepingOnly(doc.page(0), {lockedShape, lockedInk});

        UndoStack stack;
        const Changes c = stack.push(doc, std::make_unique<ClearPageCommand>(0, ClearKind::Page));
        QVERIFY(isOnePageChange(c, 0));
        QVERIFY(idsOf(doc.page(0)) == (std::vector<ObjectId>{lockedShape, lockedInk}));
        QCOMPARE(fingerprint(doc.page(0)), fingerprint(expected));
        QCOMPARE(stack.count(), std::size_t{1});
        stack.undo(doc);
        QCOMPARE(fingerprint(doc.page(0)), before);
    }

    // D8 : « Grille » est une étape annulable ; les lignes tracées, les traits et le fond restent.
    void clearGridIsOneUndoableStep()
    {
        ObjectId a = 0, b = 0;
        Document doc = twoStrokes(a, b);
        Line ruled;
        ruled.points = {{0, 400}, {1024, 400}};
        addTo(doc, ruled, Layer::Middle);
        dressPage(doc.page(0));
        const std::uint64_t before = fingerprint(doc.page(0));
        Page expected = doc.page(0);
        expected.grid = Grid{};

        UndoStack stack;
        const Changes c = stack.push(doc, std::make_unique<ClearPageCommand>(0, ClearKind::Grid));
        QVERIFY(isOnePageChange(c, 0));
        QCOMPARE(fingerprint(doc.page(0)), fingerprint(expected));
        QCOMPARE(doc.page(0).objects.size(), std::size_t{3});
        QCOMPARE(stack.count(), std::size_t{1});
        QVERIFY(stack.canUndo());
        const Changes back = stack.undo(doc);
        QVERIFY(isOnePageChange(back, 0));
        QCOMPARE(fingerprint(doc.page(0)), before);
        stack.redo(doc);
        QCOMPARE(fingerprint(doc.page(0)), fingerprint(expected));
    }

    // D8 : « Fond » remet le fond seul ; les objets de la couche du fond restent.
    void clearBackgroundKeepsBackgroundLayerObjects()
    {
        Document doc;
        const ObjectId backImage = addTo(doc, imageAt(10), Layer::Background);
        const ObjectId backShape = addTo(doc, shapeAt(10), Layer::Background);
        dressPage(doc.page(0));
        const std::uint64_t before = fingerprint(doc.page(0));
        Page expected = doc.page(0);
        expected.background = Background{};

        UndoStack stack;
        const Changes c = stack.push(doc, std::make_unique<ClearPageCommand>(0, ClearKind::Background));
        QVERIFY(isOnePageChange(c, 0));
        QVERIFY(idsOf(doc.page(0)) == (std::vector<ObjectId>{backImage, backShape}));
        QCOMPARE(fingerprint(doc.page(0)), fingerprint(expected)); // grille gardée aussi
        stack.undo(doc);
        QCOMPARE(fingerprint(doc.page(0)), before);
    }

    void clearNothingChangesNothing()
    {
        Document doc;
        ClearPageCommand clear(0, ClearKind::Annotations);
        Changes c;
        clear.redo(doc, c);
        QVERIFY(c.empty());
    }

    // Une commande sans effet ne rend aucun changement (le contrôleur ne l'empile pas).
    void commandsWithoutEffectChangeNothing()
    {
        ObjectId a = 0, b = 0;
        Document doc = twoStrokes(a, b);
        const std::uint64_t before = fingerprint(doc.page(0));
        const ObjectId absent = 987654;
        Changes c;
        TransformObjectsCommand moveAbsent(0, {absent}, translation(5, 5));
        moveAbsent.redo(doc, c);
        TransformObjectsCommand identity(0, {a}, Transform{});
        identity.redo(doc, c);
        RemoveObjectsCommand removeAbsent(0, {absent});
        removeAbsent.redo(doc, c);
        SetTranslucencyCommand sameValue(0, {a, b}, std::uint8_t{0});
        sameValue.redo(doc, c);
        ClearPageCommand grid(0, ClearKind::Grid);
        grid.redo(doc, c);
        ClearPageCommand background(0, ClearKind::Background);
        background.redo(doc, c);
        ClearPageCommand objects(0, ClearKind::Objects); // rien que des annotations
        objects.redo(doc, c);
        ReplacePageCommand samePage(0, doc.page(0));
        samePage.redo(doc, c);
        QVERIFY(c.empty());
        QCOMPARE(fingerprint(doc.page(0)), before);
    }

    void revertPageIsOneStep()
    {
        ObjectId a = 0, b = 0;
        Document doc = twoStrokes(a, b);
        Page saved; // page vierge 1024 × 768 : un paperboard jamais enregistré (F-131)
        const std::uint64_t before = fingerprint(doc.page(0));
        UndoStack stack;
        stack.push(doc, std::make_unique<ReplacePageCommand>(0, saved));
        QVERIFY(doc.page(0).objects.empty());
        QCOMPARE(stack.count(), std::size_t{1});
        stack.undo(doc);
        QCOMPARE(fingerprint(doc.page(0)), before);
    }

    // D9 : Rétablir la page est une étape de plus sur la pile, pas un retour au point enregistré : la pile
    // n'est pas remontée, rien n'est à rétablir, et le document reste modifié alors que la page est celle
    // de l'enregistrement.
    void revertPageLeavesTheDocumentModified()
    {
        Document doc;
        UndoStack stack;
        PageObject first{strokeAt(100)};
        first.common.layer = Layer::Top;
        stack.push(doc, std::make_unique<AddObjectCommand>(0, first));
        stack.setClean(); // enregistré avec un trait
        const Page saved = doc.page(0);
        PageObject second{strokeAt(200)};
        second.common.layer = Layer::Top;
        stack.push(doc, std::make_unique<AddObjectCommand>(0, second));
        stack.push(doc, std::make_unique<ClearPageCommand>(0, ClearKind::Annotations));
        const std::uint64_t beforeRevert = fingerprint(doc.page(0));

        const Changes c = stack.push(doc, std::make_unique<ReplacePageCommand>(0, saved));
        QVERIFY(isOnePageChange(c, 0));
        QCOMPARE(fingerprint(doc.page(0)), fingerprint(saved));
        QCOMPARE(stack.count(), std::size_t{4});
        QCOMPARE(stack.index(), std::size_t{4});
        QVERIFY(!stack.isClean());
        QVERIFY(!stack.canRedo());

        stack.undo(doc); // une seule étape défait le rétablissement
        QCOMPARE(fingerprint(doc.page(0)), beforeRevert);
        QCOMPARE(stack.index(), std::size_t{3});
        stack.redo(doc);
        QCOMPARE(fingerprint(doc.page(0)), fingerprint(saved));
        QVERIFY(!stack.isClean());
    }

    // La page devient `saved` en entier (taille, fond, grille, objets) ; le prochain identifiant reste
    // au-dessus des siens.
    void revertPageTakesSizeBackgroundAndGrid()
    {
        ObjectId a = 0, b = 0;
        Document doc = twoStrokes(a, b);
        Page saved;
        saved.width = 800;
        saved.height = 600;
        dressPage(saved);
        PageObject shape{shapeAt(10)};
        shape.common.id = doc.nextId() + 5;
        saved.objects.push_back(shape);

        ReplacePageCommand revert(0, saved);
        Changes c;
        revert.redo(doc, c);
        QVERIFY(isOnePageChange(c, 0));
        QCOMPARE(fingerprint(doc.page(0)), fingerprint(saved));
        QVERIFY(doc.nextId() > shape.common.id);
        QVERIFY(hasUniqueIds(doc));
    }

    void translucencyDragMerges()
    {
        ObjectId a = 0, b = 0;
        Document doc = twoStrokes(a, b);
        UndoStack stack;
        stack.push(doc,
                   std::make_unique<SetTranslucencyCommand>(0, std::vector<ObjectId>{a}, std::uint8_t{50}));
        stack.push(doc,
                   std::make_unique<SetTranslucencyCommand>(0, std::vector<ObjectId>{a}, std::uint8_t{120}));
        QCOMPARE(stack.count(), std::size_t{1});
        QCOMPARE(doc.page(0).objects[0].common.translucency, std::uint8_t{120});
        stack.undo(doc);
        QCOMPARE(doc.page(0).objects[0].common.translucency, std::uint8_t{0});
    }

    void translucencyOtherObjectsDoNotMerge()
    {
        ObjectId a = 0, b = 0;
        Document doc = twoStrokes(a, b);
        UndoStack stack;
        stack.push(doc,
                   std::make_unique<SetTranslucencyCommand>(0, std::vector<ObjectId>{a}, std::uint8_t{50}));
        stack.push(doc,
                   std::make_unique<SetTranslucencyCommand>(0, std::vector<ObjectId>{b}, std::uint8_t{60}));
        stack.push(doc,
                   std::make_unique<TransformObjectsCommand>(0, std::vector<ObjectId>{b}, translation(1, 0)));
        QCOMPARE(stack.count(), std::size_t{3});
        stack.undo(doc);
        stack.undo(doc);
        QCOMPARE(doc.page(0).objects[0].common.translucency, std::uint8_t{50});
        QCOMPARE(doc.page(0).objects[1].common.translucency, std::uint8_t{0});
    }

    void pivotsAreTheSelectionCenter()
    {
        ObjectId a = 0, b = 0;
        Document doc = twoStrokes(a, b);
        const PointF c = selectionCenter(doc.page(0), {a, b});
        QCOMPARE(c.x, 200.0);
        QVERIFY(qAbs(c.y - 150.0) < 3.0); // emprises élargies de la demi-épaisseur
        const Transform t = scalingAbout({100, 100}, 2, 2);
        QCOMPARE(t.map({100, 100}).x, 100.0);
        QCOMPARE(t.map({110, 100}).x, 120.0);
        const Transform r = rotationAbout({100, 100}, 90);
        QCOMPARE(r.map({110, 100}).x, 100.0);
        QCOMPARE(r.map({110, 100}).y, 110.0);
        const PointF none = selectionCenter(doc.page(0), {987654});
        QCOMPARE(none.x, 0.0);
        QCOMPARE(none.y, 0.0);
    }
};

QTEST_GUILESS_MAIN(TstCoreObjectedits)
#include "tst_core_objectedits.moc"
