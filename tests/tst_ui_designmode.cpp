#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/core/objectedits.h>
#include <actcraft/core/shapegeometry.h>
#include <actcraft/core/undo.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/pagepainter.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/designmode.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pagethumbnails.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/profilecontroller.h>
#include <actcraft/ui/toolhost.h>

#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <cmath>
#include <memory>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

constexpr const char* kSettingNames[] = {"editLocked",      "showHidden",  "highlightActions",
                                         "disableActions",  "disableRestrictions",
                                         "disableBlockers", "disableSnap", "disableDesktop",
                                         "showTips"};
constexpr const char* kLot8Effects[] = {"actionsHighlighted", "actionsDisabled", "restrictionsDisabled",
                                        "blockersDisabled"};

// Rectangle rouge plein, masqué (drapeau du document).
core::PageObject hiddenRedRectangle(const core::RectF& box)
{
    core::ShapeStyle style;
    style.fill = 0xFFFF0000u;
    style.outline = 0xFFFF0000u;
    core::PageObject object = core::makeShapeObject("rectangle", box, style);
    object.common.visible = false;
    return object;
}

// Teinte rouge de l'objet (opaque ou à 40 %) ; fond blanc et grille grise ne le sont pas.
bool tinted(const QColor& c)
{
    return c.red() - c.green() > 40;
}

QColor centerOf(const QImage& image)
{
    return image.pixelColor(image.width() / 2, image.height() / 2);
}

// Fenêtre principale hors écran (Main.qml), page vierge : comme tst_ui_lot5tools.
struct App
{
    QQmlApplicationEngine engine;
    QQuickWindow* window = nullptr;
    ui::PageView* view = nullptr;
    ui::DocumentController* document = nullptr;
    ui::ToolHost* tools = nullptr;
    ui::Commands* commands = nullptr;
    ui::DesignMode* design = nullptr;
    ui::ProfileController* profile = nullptr;

    bool trigger(const char* id) { return commands->trigger(QString::fromLatin1(id)); }
    QPoint scene(QPointF page) const { return view->pageToScene(page).toPoint(); }
    const core::Page& page() const
    {
        return document->document().page(static_cast<std::size_t>(document->currentIndex()));
    }
    const core::PageObject& object(core::ObjectId id) const
    {
        return page().objects.at(core::indexOf(page(), id));
    }
    core::ObjectId add(core::PageObject object)
    {
        document->execute(std::make_unique<core::AddObjectCommand>(0, std::move(object)));
        return page().objects.back().common.id;
    }
    void mouseDrag(QPointF from, QPointF to, int steps = 10)
    {
        QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, scene(from));
        for (int i = 1; i <= steps; ++i) {
            QTest::mouseMove(window, scene(from + (to - from) * i / steps));
        }
        QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, scene(to));
    }
    QColor screenAt(QPointF page) const { return window->grabWindow().pixelColor(scene(page)); }
};

} // namespace

class TstUiDesignmode : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    std::unique_ptr<App> m_app;

    bool start(App& app)
    {
        app.engine.setInitialProperties(
            {{QStringLiteral("cacheEnabled"), true},
             {QStringLiteral("generatedPage"), false},
             {QStringLiteral("profileRoot"), m_dir.filePath(QStringLiteral("profiles"))}});
        app.engine.loadFromModule("ActCraft.Ui", "Main");
        if (app.engine.rootObjects().isEmpty()) {
            return false;
        }
        app.window = qobject_cast<QQuickWindow*>(app.engine.rootObjects().constFirst());
        if (!app.window) {
            return false;
        }
        app.view = app.window->findChild<ui::PageView*>(QStringLiteral("pageView"));
        app.document = app.window->findChild<ui::DocumentController*>(QStringLiteral("documentController"));
        app.tools = app.window->findChild<ui::ToolHost*>(QStringLiteral("appTools"));
        app.commands = app.window->findChild<ui::Commands*>(QStringLiteral("commands"));
        app.design = app.window->findChild<ui::DesignMode*>(QStringLiteral("appDesign"));
        app.profile = app.window->findChild<ui::ProfileController*>(QStringLiteral("appProfile"));
        if (!app.view || !app.document || !app.tools || !app.commands || !app.design || !app.profile) {
            return false;
        }
        app.tools->setSettingsFile(m_dir.filePath(QStringLiteral("outils.ini"))); // jamais le profil réel
        if (!QTest::qWaitForWindowExposed(app.window)) {
            return false;
        }
        app.window->requestActivate();
        if (!QTest::qWaitForWindowActive(app.window)) {
            return false;
        }
        return QTest::qWaitFor([&] { return app.view->cacheReady(); }, 20000);
    }

    // Page neuve affichée, cache prêt, mode éteint.
    bool freshPage()
    {
        App& app = *m_app;
        if (app.design->active() && !app.trigger("mode.design")) {
            return false;
        }
        app.document->newDocument(1024, 768);
        return QTest::qWaitFor([&] { return app.view->cacheReady(); }, 20000);
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
        m_app = std::make_unique<App>();
        QVERIFY(start(*m_app));
    }

    void cleanupTestCase() { m_app.reset(); }

    // `mode.design` (F2, palette) est cochable, bascule `active` et suit sa coche.
    void toggleIsCheckable()
    {
        App& app = *m_app;
        QVERIFY(freshPage());
        const auto* spec = app.commands->registry().find(QStringLiteral("mode.design"));
        QVERIFY(spec != nullptr);
        QVERIFY(spec->checkable);
        QVERIFY(!app.design->active());
        QVERIFY(!app.commands->isChecked(QStringLiteral("mode.design")));
        QVERIFY(app.trigger("mode.design"));
        QVERIFY(app.design->active());
        QVERIFY(app.commands->isChecked(QStringLiteral("mode.design")));
        QVERIFY(app.trigger("mode.design"));
        QVERIFY(!app.design->active());
        QVERIFY(!app.commands->isChecked(QStringLiteral("mode.design")));
    }

    // Neuf réglages vrais par défaut (F-560, D11 de L6-T0), gardés dans la section `design-mode` du profil.
    void nineSettingsWithDefaults()
    {
        ui::DesignMode fresh;
        for (const char* name : kSettingNames) {
            QVERIFY2(fresh.metaObject()->indexOfProperty(name) >= 0, name);
            QVERIFY2(fresh.property(name).toBool(), name);
        }
        App& app = *m_app;
        app.design->setShowTips(false);
        QCOMPARE(app.profile->setting(QStringLiteral("design-mode"), QStringLiteral("showTips"), true).toBool(),
                 false);
        ui::DesignMode reread;
        reread.setProfile(app.profile);
        QVERIFY(!reread.showTips());
        QVERIFY(reread.editLocked());
        app.design->setShowTips(true);
        QVERIFY(app.profile->setting(QStringLiteral("design-mode"), QStringLiteral("showTips"), false).toBool());
    }

    // Verrouillé : le glisser de l'outil de sélection est sans effet ; mode actif, il le déplace en une étape et
    // le drapeau `locked` du document reste posé.
    void lockedObjectMovesWhileDesigning()
    {
        App& app = *m_app;
        QVERIFY(freshPage());
        core::ShapeStyle style;
        core::PageObject locked = core::makeShapeObject("rectangle", {100, 100, 200, 200}, style);
        locked.common.locked = true;
        const core::ObjectId id = app.add(std::move(locked));
        QTRY_VERIFY(app.view->cacheReady());
        QVERIFY(app.trigger("tool.select"));
        app.tools->selectObjects({id});
        const double left = core::boundsOnPage(app.object(id)).x;

        app.mouseDrag(QPointF(200, 200), QPointF(320, 260));
        QCOMPARE(core::boundsOnPage(app.object(id)).x, left);

        QVERIFY(app.trigger("mode.design"));
        QVERIFY(app.tools->lockIgnored());
        app.tools->selectObjects({id});
        app.mouseDrag(QPointF(200, 200), QPointF(320, 260));
        QVERIFY(core::boundsOnPage(app.object(id)).x - left > 50);
        QVERIFY(app.object(id).common.locked);

        app.document->undo(); // une seule étape
        QVERIFY(std::abs(core::boundsOnPage(app.object(id)).x - left) < 0.01);
        QVERIFY(app.object(id).common.locked);
        QVERIFY(app.trigger("mode.design"));
        QVERIFY(!app.tools->lockIgnored());
    }

    // Peintre partagé : avec l'option, le centre d'un objet masqué est teinté (translucide) ; sans, c'est le
    // fond. Vue de page : le mode pose le drapeau de la seule vue, une reconstruction par bascule.
    void hiddenObjectsDrawnTranslucent()
    {
        core::Page page;
        page.width = 400;
        page.height = 300;
        page.grid.visible = false;
        page.objects.push_back(hiddenRedRectangle({150, 100, 100, 100}));
        render::PaintOptions shown;
        shown.showHiddenTranslucent = true;
        const QColor with = centerOf(render::renderPage(page, {}, 1.0, shown));
        const QColor without = centerOf(render::renderPage(page, {}, 1.0, render::PaintOptions{}));
        QVERIFY(tinted(with));
        QVERIFY2(with.green() > 120, "translucide, pas opaque");
        QVERIFY(!tinted(without));

        App& app = *m_app;
        QVERIFY(freshPage());
        app.add(hiddenRedRectangle({400, 300, 200, 200}));
        QTRY_VERIFY(app.view->cacheReady());
        auto* fixed = app.document->fixedPage();
        QVERIFY(fixed != nullptr);
        QVERIFY(!tinted(app.screenAt({500, 400})));
        const int rebuilds = fixed->rebuildCount();
        const int refreshes = app.view->cacheRefreshCount();

        QVERIFY(app.trigger("mode.design"));
        QVERIFY(fixed->showHiddenTranslucent());
        QTRY_VERIFY(app.view->cacheRefreshCount() > refreshes && app.view->cacheReady());
        const QColor onScreen = app.screenAt({500, 400});
        QVERIFY(tinted(onScreen));
        QVERIFY2(onScreen.green() > 120, "translucide, pas opaque");
        QCOMPARE(fixed->rebuildCount(), rebuilds + 1); // une reconstruction, un redessin du cache
        QCOMPARE(app.view->cacheRefreshCount(), refreshes + 1);

        QVERIFY(app.trigger("mode.design"));
        QVERIFY(!fixed->showHiddenTranslucent());
        QTRY_VERIFY(app.view->cacheRefreshCount() > refreshes + 1 && app.view->cacheReady());
        QVERIFY(!tinted(app.screenAt({500, 400})));
        QCOMPARE(fixed->rebuildCount(), rebuilds + 2);
        QCOMPARE(app.view->cacheRefreshCount(), refreshes + 2);
    }

    // A18 (amendement 5) : propriété publiée pour le lot 7, vraie quand le mode est actif, sans effet branché.
    void transitionsSuspended()
    {
        ui::DesignMode mode;
        QSignalSpy effects(&mode, &ui::DesignMode::effectsChanged);
        QVERIFY(!mode.transitionsSuspended());
        QVERIFY(!mode.property("transitionsSuspended").toBool());
        mode.setActive(true);
        QVERIFY(mode.transitionsSuspended());
        QVERIFY(mode.property("transitionsSuspended").toBool());
        QCOMPARE(effects.count(), 1);
        for (const char* name : kSettingNames) {
            QVERIFY(mode.setProperty(name, false));
        }
        QVERIFY(mode.transitionsSuspended()); // aucun des neuf réglages ne la coupe
        mode.setActive(false);
        QVERIFY(!mode.transitionsSuspended());
    }

    // Les quatre effets d'actions et de restrictions existent pour le lot 8 et suivent `active`.
    void settingsPublishedForLot8()
    {
        ui::DesignMode mode;
        for (const char* name : kLot8Effects) {
            const int index = mode.metaObject()->indexOfProperty(name);
            QVERIFY2(index >= 0, name);
            QVERIFY2(mode.metaObject()->property(index).hasNotifySignal(), name);
            QVERIFY2(!mode.property(name).toBool(), name);
        }
        mode.setActive(true);
        for (const char* name : kLot8Effects) {
            QVERIFY2(mode.property(name).toBool(), name);
        }
        mode.setDisableActions(false);
        QVERIFY(!mode.actionsDisabled());
        QVERIFY(mode.actionsHighlighted());
        mode.setActive(false);
        for (const char* name : kLot8Effects) {
            QVERIFY2(!mode.property(name).toBool(), name);
        }
    }

    // Amendement 1 : bascule pendant un contact gardée, appliquée au relâché avec son seul redessin.
    void toggleDeferredDuringContact()
    {
        ui::DesignMode mode;
        QSignalSpy effects(&mode, &ui::DesignMode::effectsChanged);
        mode.setContactActive(true);
        mode.toggle();
        QVERIFY(!mode.active());
        QCOMPARE(effects.count(), 0);
        mode.setContactActive(false);
        QVERIFY(mode.active());
        QCOMPARE(effects.count(), 1);

        mode.setContactActive(true);
        mode.toggle();
        mode.toggle(); // aller et retour pendant le même contact : rien au relâché
        mode.setContactActive(false);
        QVERIFY(mode.active());
        QCOMPARE(effects.count(), 1);
    }

    // Discriminant du patron écarté (amendements 1 et 5) : le mode ne bascule pas `visible` dans le document
    // et ne pose aucune option globale du peintre. Mode actif, la miniature (L4-T17) d'une page à objet masqué
    // n'est pas teintée au centre de l'objet, et l'empreinte du document ne bouge pas à la bascule.
    void thumbnailsNeverShowHidden()
    {
        QVERIFY(!render::PaintOptions{}.showHiddenTranslucent);
        App& app = *m_app;
        QVERIFY(freshPage());
        app.add(hiddenRedRectangle({412, 284, 200, 200})); // centré sur la page de 1024 × 768
        const QString before = app.document->fingerprintText();

        QVERIFY(app.trigger("mode.design"));
        QVERIFY(app.design->hiddenShown());
        QCOMPARE(app.document->fingerprintText(), before);

        ui::PageThumbnails thumbnails;
        thumbnails.setDocumentController(app.document);
        thumbnails.refreshAll();
        QTRY_VERIFY_WITH_TIMEOUT(!thumbnails.image(0).isNull(), 10000);
        QVERIFY(!tinted(centerOf(thumbnails.image(0))));

        const QImage png = QImage::fromData(render::thumbnailPng(app.page(), {}), "PNG");
        QVERIFY(!tinted(centerOf(png)));

        QVERIFY(app.trigger("mode.design"));
        QCOMPARE(app.document->fingerprintText(), before);
    }
};

QTEST_MAIN(TstUiDesignmode)
#include "tst_ui_designmode.moc"
