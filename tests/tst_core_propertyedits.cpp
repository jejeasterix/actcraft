#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/pagecache.h>
#include <actcraft/core/propertyedits.h>
#include <actcraft/core/undo.h>

#include <QStringList>
#include <QTest>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace actcraft::core;

namespace {

// Cache espion : l'ordre exact des appels que applyToCache lui envoie (comme tst_core_replace).
struct SpyCache final : PageCache
{
    QStringList calls;
    void insert(ObjectId id) override { calls << QStringLiteral("insert %1").arg(id); }
    void remove(ObjectId id, const RectF&) override { calls << QStringLiteral("remove %1").arg(id); }
    void invalidate(ObjectId id, const CacheArea&) override
    {
        calls << QStringLiteral("invalidate %1").arg(id);
    }
};

PageObject shapeWithOutline(double x, double outlineWidth)
{
    Shape s;
    s.bounds = {x, 100, 40, 40};
    s.outlineWidth = outlineWidth;
    PageObject o = s;
    return o;
}

PageObject stroke()
{
    Stroke s;
    s.points = {{400, 10, 0.5f}, {420, 30, 0.5f}};
    PageObject o = s;
    return o;
}

// Prédicat de presentationTouchesNoCache (L0-S3) : des Change (sinon l'étape n'entre pas dans la pile de
// DocumentController), tous d'un genre qui n'atteint pas le cache, aucun appel au cache. Vide : respecté.
QString cacheTouches(const Changes& changes)
{
    QStringList faults;
    if (changes.empty()) {
        faults << QStringLiteral("aucun Change");
    }
    for (const Change& change : changes) {
        if (change.kind != Change::Kind::PageSettingsChanged) {
            faults << QStringLiteral("genre %1 sur l'objet %2").arg(int(change.kind)).arg(change.object);
        }
    }
    SpyCache spy;
    if (applyToCache(changes, 0, spy) != std::optional<std::size_t>{0}) {
        faults << QStringLiteral("applyToCache ne rend pas 0");
    }
    faults << spy.calls;
    return faults.join(u"; ");
}

bool hasKind(const Changes& changes, Change::Kind kind, ObjectId object)
{
    return std::any_of(changes.begin(), changes.end(),
                       [&](const Change& c) { return c.kind == kind && c.object == object && c.page == 0; });
}

} // namespace

class TstCorePropertyedits : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<Document> m_document;
    UndoStack m_stack;
    ObjectId m_a = 0; // forme, contour de 2
    ObjectId m_b = 0; // forme, contour de 5
    ObjectId m_s = 0; // trait, jamais sélectionné

    Page& page() { return m_document->page(0); }

    ObjectId add(PageObject object)
    {
        auto command = std::make_unique<AddObjectCommand>(0, std::move(object));
        const AddObjectCommand* raw = command.get();
        m_stack.push(*m_document, std::move(command));
        return raw->id();
    }

    const PageObject* objectWith(ObjectId id)
    {
        for (const PageObject& o : page().objects) {
            if (o.common.id == id) {
                return &o;
            }
        }
        return nullptr;
    }

    std::optional<PropertyValue> valueOf(ObjectId id, ObjectProperty property)
    {
        const PageObject* object = objectWith(id);
        return object != nullptr ? readProperty(*object, property) : std::nullopt;
    }

    double numberOf(ObjectId id, ObjectProperty property)
    {
        const std::optional<PropertyValue> value = valueOf(id, property);
        if (!value || !std::holds_alternative<double>(*value)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        return std::get<double>(*value);
    }

    Changes push(std::vector<ObjectId> ids, ObjectProperty property, PropertyValue value)
    {
        return m_stack.push(*m_document, std::make_unique<SetObjectPropertiesCommand>(0, std::move(ids), property,
                                                                                      std::move(value)));
    }

    Changes pushPage(PageProperty property, PropertyValue value)
    {
        return m_stack.push(*m_document, std::make_unique<SetPagePropertiesCommand>(0, property, std::move(value)));
    }

private slots:
    void init()
    {
        m_document = std::make_unique<Document>(); // une page de 1024 × 768
        m_stack.clear();
        m_a = add(shapeWithOutline(100, 2));
        m_b = add(shapeWithOutline(200, 5));
        m_s = add(stroke());
        m_stack.clear();
    }

    // D6-6 : une sélection de N objets, une seule étape (pas une commande par objet).
    void oneStepForManyObjects()
    {
        const std::optional<PropertyValue> strokeBefore = valueOf(m_s, ObjectProperty::OutlineWidth);
        const std::size_t before = m_stack.count();
        const Changes changes = push({m_a, m_b}, ObjectProperty::OutlineWidth, 12.0);
        QCOMPARE(m_stack.count(), before + 1);
        QCOMPARE(numberOf(m_a, ObjectProperty::OutlineWidth), 12.0);
        QCOMPARE(numberOf(m_b, ObjectProperty::OutlineWidth), 12.0);
        QVERIFY(valueOf(m_s, ObjectProperty::OutlineWidth) == strokeBefore);
        // Le contour se redessine : un ObjectChanged par objet modifié.
        QCOMPARE(changes.size(), std::size_t{2});
        QVERIFY(hasKind(changes, Change::Kind::ObjectChanged, m_a));
        QVERIFY(hasKind(changes, Change::Kind::ObjectChanged, m_b));
    }

    // Chaque objet retrouve sa propre valeur, pas celle du premier.
    void undoRestoresEachOldValue()
    {
        push({m_a, m_b}, ObjectProperty::OutlineWidth, 12.0);
        const Changes undone = m_stack.undo(*m_document);
        QCOMPARE(numberOf(m_a, ObjectProperty::OutlineWidth), 2.0);
        QCOMPARE(numberOf(m_b, ObjectProperty::OutlineWidth), 5.0);
        QVERIFY(hasKind(undone, Change::Kind::ObjectChanged, m_a));
        QVERIFY(hasKind(undone, Change::Kind::ObjectChanged, m_b));
        m_stack.redo(*m_document);
        QCOMPARE(numberOf(m_a, ObjectProperty::OutlineWidth), 12.0);
        QCOMPARE(numberOf(m_b, ObjectProperty::OutlineWidth), 12.0);
    }

    // Amendement 1 : la translucidité est déléguée (SetTranslucencyCommand) ; le glissé se prouve sur
    // l'épaisseur du contour, autre réglette du navigateur.
    void sliderDragMergesIntoOneStep()
    {
        const std::size_t before = m_stack.count();
        // Premier pas : 2, la valeur de m_a, qu'il laisse tel quel ; l'étape doit quand même le lui rendre.
        for (const double width : {2.0, 10.0, 20.0, 30.0}) {
            push({m_a, m_b}, ObjectProperty::OutlineWidth, width);
        }
        QCOMPARE(m_stack.count(), before + 1);
        QCOMPARE(numberOf(m_a, ObjectProperty::OutlineWidth), 30.0);
        QCOMPARE(numberOf(m_b, ObjectProperty::OutlineWidth), 30.0);
        // L'étape garde l'état d'avant le glissé (pas celui du pas précédent, 20).
        m_stack.undo(*m_document);
        QCOMPARE(numberOf(m_a, ObjectProperty::OutlineWidth), 2.0);
        QCOMPARE(numberOf(m_b, ObjectProperty::OutlineWidth), 5.0);
        QVERIFY(!m_stack.canUndo());
        m_stack.redo(*m_document);
        QCOMPARE(numberOf(m_a, ObjectProperty::OutlineWidth), 30.0);
        // Réglette refermée (D10) : le pas suivant ouvre une étape.
        m_stack.closeMerge();
        push({m_a, m_b}, ObjectProperty::OutlineWidth, 40.0);
        QCOMPARE(m_stack.count(), before + 2);
    }

    // Une autre propriété, ou d'autres objets : pas de fusion.
    void differentPropertyDoesNotMerge()
    {
        const std::size_t before = m_stack.count();
        push({m_a, m_b}, ObjectProperty::OutlineWidth, 7.0);
        push({m_a}, ObjectProperty::OutlineWidth, 9.0);
        QCOMPARE(m_stack.count(), before + 2);
        push({m_a}, ObjectProperty::OutlineColor, std::uint32_t{0xFF00FF00u});
        QCOMPARE(m_stack.count(), before + 3);
        QVERIFY(valueOf(m_a, ObjectProperty::OutlineColor) == PropertyValue{std::uint32_t{0xFF00FF00u}});
        m_stack.undo(*m_document);
        QVERIFY(valueOf(m_a, ObjectProperty::OutlineColor) == PropertyValue{std::uint32_t{0xFF000000u}});
        QCOMPARE(numberOf(m_a, ObjectProperty::OutlineWidth), 9.0);
        m_stack.undo(*m_document);
        QCOMPARE(numberOf(m_a, ObjectProperty::OutlineWidth), 7.0);
        QCOMPARE(numberOf(m_b, ObjectProperty::OutlineWidth), 7.0);
        m_stack.undo(*m_document);
        QCOMPARE(numberOf(m_a, ObjectProperty::OutlineWidth), 2.0);
        QCOMPARE(numberOf(m_b, ObjectProperty::OutlineWidth), 5.0);
    }

    // Nom, mots-clés, légende et son style : une étape, aucun pixel du cache (ni ObjectChanged, ni PageChanged).
    void nameOnlyChangeSparesTheCache()
    {
        LabelStyle style;
        style.pointSize = 18;
        style.color = 0xFF336699u;
        const std::vector<std::pair<ObjectProperty, PropertyValue>> cases = {
            {ObjectProperty::Name, std::string("Soleil")},
            {ObjectProperty::Keywords, std::vector<std::string>{"astre", "jour"}},
            {ObjectProperty::Label, std::string("Le soleil")},
            {ObjectProperty::LabelStyle, style},
        };
        for (const auto& [property, value] : cases) {
            const auto label = QStringLiteral("propriété %1").arg(int(property));
            QVERIFY2(!propertyTouchesTheCache(property), qPrintable(label));
            const std::optional<PropertyValue> old = valueOf(m_a, property);
            const std::size_t before = m_stack.count();
            const Changes changes = push({m_a, m_b}, property, value);
            QCOMPARE(m_stack.count(), before + 1);
            QVERIFY2(valueOf(m_a, property) == value, qPrintable(label));
            QVERIFY2(valueOf(m_b, property) == value, qPrintable(label));
            const QString touched = cacheTouches(changes);
            QVERIFY2(touched.isEmpty(), qPrintable(label + u" : " + touched));
            const Changes undone = m_stack.undo(*m_document);
            QVERIFY2(valueOf(m_a, property) == old, qPrintable(label));
            const QString touchedByUndo = cacheTouches(undone);
            QVERIFY2(touchedByUndo.isEmpty(), qPrintable(label + u" (Annuler) : " + touchedByUndo));
            m_stack.redo(*m_document);
        }
        QVERIFY(propertyTouchesTheCache(ObjectProperty::OutlineWidth));
        QVERIFY(propertyTouchesTheCache(ObjectProperty::FillColor));
        QVERIFY(propertyTouchesTheCache(ObjectProperty::Left));
    }

    // Amendement 1, point 15 : lisibles ici, écrites par les commandes des lots 3 et 5.
    void layerVisibleLockedTranslucencyAreDelegated()
    {
        const std::vector<ObjectProperty> delegated = {ObjectProperty::Layer, ObjectProperty::Visible,
                                                       ObjectProperty::Locked, ObjectProperty::Translucency};
        for (int i = 0; i <= int(ObjectProperty::Mirrored); ++i) {
            const auto property = static_cast<ObjectProperty>(i);
            const bool expected = std::find(delegated.begin(), delegated.end(), property) != delegated.end();
            QVERIFY2(isDelegated(property) == expected, qPrintable(QStringLiteral("propriété %1").arg(i)));
        }
        QVERIFY(valueOf(m_a, ObjectProperty::Layer) == PropertyValue{Layer::Middle});
        QVERIFY(valueOf(m_a, ObjectProperty::Visible) == PropertyValue{true});
        QVERIFY(valueOf(m_a, ObjectProperty::Locked) == PropertyValue{false});
        QVERIFY(valueOf(m_a, ObjectProperty::Translucency) == PropertyValue{0.0});

        const std::uint64_t original = fingerprint(page());
        const std::vector<std::pair<ObjectProperty, PropertyValue>> writes = {
            {ObjectProperty::Layer, Layer::Top},
            {ObjectProperty::Visible, false},
            {ObjectProperty::Locked, true},
            {ObjectProperty::Translucency, 128.0},
        };
        for (const auto& [property, value] : writes) {
            SetObjectPropertiesCommand command(0, {m_a, m_b}, property, value);
            Changes changes;
            command.redo(*m_document, changes);
            QVERIFY(changes.empty());
            QCOMPARE(fingerprint(page()), original);
        }
        QVERIFY(valueOf(m_a, ObjectProperty::Layer) == PropertyValue{Layer::Middle});
        QVERIFY(valueOf(m_a, ObjectProperty::Visible) == PropertyValue{true});
        QVERIFY(valueOf(m_a, ObjectProperty::Locked) == PropertyValue{false});
        QVERIFY(valueOf(m_a, ObjectProperty::Translucency) == PropertyValue{0.0});
    }

    // Position, taille, angle, miroir : par la transformation, lus sur la boîte transformée (boundsOnPage).
    void geometryGoesThroughTheTransform()
    {
        const RectF box = boundsOnPage(*objectWith(m_a));
        push({m_a}, ObjectProperty::Left, 150.0);
        QCOMPARE(numberOf(m_a, ObjectProperty::Left), 150.0);
        QCOMPARE(numberOf(m_a, ObjectProperty::Top), box.y);
        push({m_a}, ObjectProperty::Top, 60.0);
        QCOMPARE(numberOf(m_a, ObjectProperty::Top), 60.0);
        push({m_a}, ObjectProperty::Width, box.width * 2);
        QCOMPARE(numberOf(m_a, ObjectProperty::Width), box.width * 2);
        QCOMPARE(numberOf(m_a, ObjectProperty::Left), 150.0);
        QCOMPARE(numberOf(m_a, ObjectProperty::Height), box.height);
        push({m_a}, ObjectProperty::Angle, 90.0);
        QCOMPARE(numberOf(m_a, ObjectProperty::Angle), 90.0);
        QVERIFY(valueOf(m_a, ObjectProperty::Mirrored) == PropertyValue{false});
        push({m_a}, ObjectProperty::Mirrored, true);
        QVERIFY(valueOf(m_a, ObjectProperty::Mirrored) == PropertyValue{true});
        QCOMPARE(m_stack.count(), std::size_t{5});
        while (m_stack.canUndo()) {
            m_stack.undo(*m_document);
        }
        QVERIFY(objectWith(m_a)->common.transform.isIdentity());
        QCOMPARE(numberOf(m_a, ObjectProperty::Left), box.x);
    }

    // Largeur de page : une étape (pas fusionnés compris), PageChanged ; la hauteur ne fusionne pas avec elle.
    void pageWidthChangeIsOneStep()
    {
        QCOMPARE(page().width, 1024.0);
        const std::size_t before = m_stack.count();
        pushPage(PageProperty::Width, 1100.0);
        const Changes changes = pushPage(PageProperty::Width, 1280.0);
        QCOMPARE(m_stack.count(), before + 1);
        QCOMPARE(page().width, 1280.0);
        QVERIFY(hasKind(changes, Change::Kind::PageChanged, 0));
        QVERIFY(readPageProperty(page(), PageProperty::Width) == PropertyValue{1280.0});
        const Changes undone = m_stack.undo(*m_document);
        QCOMPARE(page().width, 1024.0);
        QVERIFY(hasKind(undone, Change::Kind::PageChanged, 0));
        m_stack.redo(*m_document);
        pushPage(PageProperty::Height, 900.0);
        QCOMPARE(m_stack.count(), before + 2);
        QCOMPARE(page().height, 900.0);
        QCOMPARE(page().width, 1280.0);
    }

    // Nom, description et mots-clés de la page : sans cache.
    void pageTextSparesTheCache()
    {
        const std::vector<std::pair<PageProperty, PropertyValue>> cases = {
            {PageProperty::Name, std::string("Les planètes")},
            {PageProperty::Description, std::string("Système solaire, cycle 3")},
            {PageProperty::Keywords, std::vector<std::string>{"astronomie"}},
        };
        for (const auto& [property, value] : cases) {
            const auto label = QStringLiteral("propriété de page %1").arg(int(property));
            const PropertyValue old = readPageProperty(page(), property);
            const Changes changes = pushPage(property, value);
            QVERIFY2(readPageProperty(page(), property) == value, qPrintable(label));
            const QString touched = cacheTouches(changes);
            QVERIFY2(touched.isEmpty(), qPrintable(label + u" : " + touched));
            const Changes undone = m_stack.undo(*m_document);
            QVERIFY2(readPageProperty(page(), property) == old, qPrintable(label));
            const QString touchedByUndo = cacheTouches(undone);
            QVERIFY2(touchedByUndo.isEmpty(), qPrintable(label + u" (Annuler) : " + touchedByUndo));
        }
        QCOMPARE(page().name, std::string());
        QCOMPARE(page().description, std::string());
    }
};

QTEST_MAIN(TstCorePropertyedits)
#include "tst_core_propertyedits.moc"
