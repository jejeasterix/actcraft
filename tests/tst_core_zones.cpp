// Zones de page (F-603, D11) : une page compte des cadres (0 à 40 en largeur, 0 à 53 en hauteur) d'une taille
// en pixels ; réglées en une étape, ignorées par le cache (dessinées en surcouche), hors de l'empreinte tant
// qu'elles sont nulles (L4-T27).
#include <actcraft/core/document.h>
#include <actcraft/core/gridsnap.h>
#include <actcraft/core/pagecache.h>
#include <actcraft/core/pagesettings.h>
#include <actcraft/core/pagezones.h>
#include <actcraft/core/undo.h>

#include <QtTest>

#include <cstdint>
#include <limits>

using namespace actcraft::core;

class TstCoreZones : public QObject
{
    Q_OBJECT

private slots:
    void zonesKeepOldFingerprints()
    {
        Page page;
        const auto before = fingerprint(page);
        page.zones.horizontal = 2;
        QVERIFY(fingerprint(page) != before);
        page.zones.horizontal = 0;
        QCOMPARE(fingerprint(page), before);
        page.zones.vertical = 2;
        QVERIFY(fingerprint(page) != before);
        page.zones.vertical = 0;
        QCOMPARE(fingerprint(page), before);
        page.zones.frameWidth = 400; // amendement 4 : la taille d'un cadre entre dans l'empreinte
        QVERIFY(fingerprint(page) != before);
        page.zones.frameWidth = 0;
        QCOMPARE(fingerprint(page), before);
        page.zones.frameHeight = 400;
        QVERIFY(fingerprint(page) != before);
        page.zones.frameHeight = 0;
        QCOMPARE(fingerprint(page), before);
        // Chaque champ compte, à sa place : 2 cadres en largeur ne valent pas 2 en hauteur, 2 en hauteur ne
        // valent pas 3, une hauteur de cadre de 500 ne vaut pas 600.
        const auto withZones = [](PageZones zones) {
            Page p;
            p.zones = zones;
            return fingerprint(p);
        };
        QVERIFY(withZones({.horizontal = 2}) != withZones({.vertical = 2}));
        QVERIFY(withZones({.vertical = 2}) != withZones({.vertical = 3}));
        QVERIFY(withZones({.frameWidth = 500}) != withZones({.frameHeight = 500}));
        QVERIFY(withZones({.frameHeight = 500}) != withZones({.frameHeight = 600}));
        // Valeur figée, recoupée hors C++ (Python) : l'état FNV-1a de fingerprint(Page{})
        // (0xB59B2DE2A9B3C823, tst_core_model) prolongé de l'octet 0xA6, de 2 et 3 (int32 petit-boutistes)
        // puis de 500 et 600 (IEEE 754 petit-boutistes).
        QCOMPARE(withZones({.horizontal = 2, .vertical = 3, .frameWidth = 500, .frameHeight = 600}),
                 std::uint64_t{0x501B62CF9F23665Bull});
    }
    void extendKeepsTheFrameSize() // amendement 4 (D11) : l'allongement ne touche pas les cadres
    {
        Document d;
        d.page(0).zones = PageZones{.horizontal = 2, .vertical = 3, .frameWidth = 500, .frameHeight = 600};
        UndoStack stack;
        stack.push(d, std::make_unique<ExtendPageCommand>(0));
        QCOMPARE(d.page(0).zones.frameWidth, 500.0);
        QCOMPARE(d.page(0).zones.frameHeight, 600.0);
        QCOMPARE(d.page(0).zones.vertical, 3);
    }
    // Une étape : annuler rend les quatre champs d'avant, rétablir les quatre d'après.
    void setZonesIsOneStep()
    {
        const auto same = [](const PageZones& a, const PageZones& b) {
            return a.horizontal == b.horizontal && a.vertical == b.vertical && a.frameWidth == b.frameWidth &&
                   a.frameHeight == b.frameHeight;
        };
        const PageZones before{.horizontal = 2, .vertical = 3, .frameWidth = 500, .frameHeight = 600};
        const PageZones after{.horizontal = 4, .vertical = 1, .frameWidth = 700, .frameHeight = 800};
        Document d;
        d.page(0).zones = before;
        UndoStack stack;
        stack.push(d, std::make_unique<SetPageZonesCommand>(0, after));
        QVERIFY(same(d.page(0).zones, after));
        QCOMPARE(stack.count(), std::size_t{1});
        stack.undo(d);
        QVERIFY(same(d.page(0).zones, before));
        QVERIFY(!stack.canUndo());
        stack.redo(d);
        QVERIFY(same(d.page(0).zones, after));
    }
    void zonesChangeIsIgnoredByTheCache()
    {
        struct Spy final : PageCache
        {
            int calls = 0;
            void insert(ObjectId) override { ++calls; }
            void remove(ObjectId, const RectF&) override { ++calls; }
            void invalidate(ObjectId, const CacheArea&) override { ++calls; }
        } spy;
        Document d;
        UndoStack stack;
        const Changes c = stack.push(d, std::make_unique<SetPageZonesCommand>(0, PageZones{.vertical = 3}));
        QCOMPARE(c.size(), std::size_t{1});
        applyToCache(c, 0, spy);
        QCOMPARE(spy.calls, 0);
    }
    void zonesAreBounded() // amendement 4 (D11) : 0 à 40 en largeur, 0 à 53 en hauteur
    {
        const PageZones z = clampZones({.horizontal = -1, .vertical = 60});
        QCOMPARE(z.horizontal, 0);
        QCOMPARE(z.vertical, 53);
        const PageZones w = clampZones({.horizontal = 50, .vertical = 2});
        QCOMPARE(w.horizontal, 40);
        QCOMPARE(w.vertical, 2);
        const PageZones s =
            clampZones({.frameWidth = -5, .frameHeight = std::numeric_limits<double>::quiet_NaN()});
        QCOMPARE(s.frameWidth, 0.0); // amendement 4 : taille négative ou non finie ramenée à 0
        QCOMPARE(s.frameHeight, 0.0);
        const PageZones m = clampZones({.frameWidth = 400, .frameHeight = 300});
        QCOMPARE(m.frameWidth, 500.0); // amendement 5 (H4-20) : un cadre fait au moins 500 de côté
        QCOMPARE(m.frameHeight, 500.0);
        QCOMPARE(clampZones({.frameWidth = 0, .frameHeight = 700}).frameWidth, 0.0); // 0 : non donnée, gardée
        QCOMPARE(clampZones({.frameWidth = 0, .frameHeight = 700}).frameHeight, 700.0);
    }
    // Amendement 5 (L4-O2, H4-21) : les séparations n'attirent pas un objet déplacé et n'ajoutent aucun point
    // d'accrochage ; la grille part de l'origine de la page, pas de celle de chaque cadre (supposé). Passe
    // par snapDelta, qui reçoit la page entière (zones comprises), comme l'outil de sélection (L4-T9). Cadres
    // de 500 : séparations en x = 500 et y = 500, qui ne sont pas des multiples de 32.
    void separatorsDoNotAttract()
    {
        Page page;
        page.width = 1000;
        page.height = 1500;
        page.zones = PageZones{.horizontal = 2, .vertical = 3, .frameWidth = 500, .frameHeight = 500};
        page.grid.snapEnabled = true;
        page.grid.levels.push_back(GridLevel{}); // pas de 32, départ en (0, 0)
        PageObject shape{Shape{}};
        shape.common.id = 1;
        std::get<Shape>(body(shape)).bounds = RectF{497, 463, 40, 40}; // coin bas gauche en (497, 503)
        page.objects.push_back(shape);
        const PointF anchor = snapAnchor(page.objects.front());
        QCOMPARE(anchor.x, 497.0);
        QCOMPARE(anchor.y, 503.0);
        // Ancrage ramené sur (512, 512) : +15, +9. Vers la séparation (500, 500), ou une grille repartie du
        // cadre, ce serait +3, -3.
        const PointF delta = snapDelta(page, {ObjectId{1}}, PointF{0, 0});
        QCOMPARE(delta.x, 15.0);
        QCOMPARE(delta.y, 9.0);
    }
};

QTEST_GUILESS_MAIN(TstCoreZones)
#include "tst_core_zones.moc"
