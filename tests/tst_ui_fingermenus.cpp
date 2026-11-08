#include "startup.h"

#include <actcraft/ui/commands.h>
#include <actcraft/ui/layoutmetrics.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/toolhost.h>

#include <QPointingDevice>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTabletEvent>
#include <QTemporaryDir>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <memory>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

struct Shell
{
    QTemporaryDir profiles; // profils d'essai, détruit après le moteur
    QQmlApplicationEngine engine;
    QQuickWindow* window = nullptr;
    ui::PageView* view = nullptr;
    ui::ToolHost* tools = nullptr;
    ui::Commands* commands = nullptr;
    ui::LayoutMetrics* layout = nullptr;
};

bool start(Shell& s)
{
    StartupOptions options;
    options.profileRoot = s.profiles.path();
    s.window = loadMainWindow(s.engine, options);
    if (!s.window || !QTest::qWaitForWindowExposed(s.window)) {
        return false;
    }
    s.view = s.window->findChild<ui::PageView*>(QStringLiteral("pageView"));
    s.tools = s.window->findChild<ui::ToolHost*>(QStringLiteral("appTools"));
    s.commands = s.window->findChild<ui::Commands*>(QStringLiteral("commands"));
    s.layout = s.window->findChild<ui::LayoutMetrics*>(QStringLiteral("appLayout"));
    if (!(s.view && s.tools && s.commands && s.layout)) {
        return false;
    }
    s.window->resize(1280, 800);
    return QTest::qWaitFor([&] { return s.window->size() == QSize(1280, 800); }, 5000) &&
           (QTest::qWait(150), true);
}

// Parcours des éléments visuels : les délégués d'un Repeater n'ont pas la palette pour parent d'objet.
QQuickItem* findVisual(QQuickItem* from, const QString& name)
{
    if (from->objectName() == name) {
        return from;
    }
    for (QQuickItem* child : from->childItems()) {
        if (QQuickItem* found = findVisual(child, name)) {
            return found;
        }
    }
    return nullptr;
}

QQuickItem* item(Shell& s, const char* name)
{
    return findVisual(s.window->contentItem(), QString::fromLatin1(name));
}

QObject* object(Shell& s, const char* name)
{
    return s.window->findChild<QObject*>(QString::fromLatin1(name));
}

QPoint sceneCenter(QQuickItem* it)
{
    return it->mapToScene(QPointF(it->width() / 2, it->height() / 2)).toPoint();
}

} // namespace

class TstUiFingermenus : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void init() { QSettings().clear(); }
    void longPressOnObjectOpensObjectMenu();
    void longPressOnEmptyPageOpensPageMenu();
    void noMenuWithInkTool();
    void paletteLongPressShowsLabelOnly();
    void menuRowsFollowTargetSize();
    void submenuOpensOnTap();
    void imposedSelectorOpensTheMenu();
    void imposedPenOpensNoMenu();
    void secondUserHoldOpensNoMenu();

private:
    // Un trait tracé au stylet, puis sélectionné : rend le centre de son cadre (point de page).
    QPointF drawAndSelect(Shell& s)
    {
        s.tools->activate(QStringLiteral("pen"));
        const auto send = [&](QEvent::Type type, QPointF page, qreal pressure, Qt::MouseButton button,
                              Qt::MouseButtons buttons) {
            const QPointF scene = s.view->pageToScene(page);
            QTabletEvent event(type, m_stylus.get(), scene, s.window->mapToGlobal(scene), pressure, 0, 0, 0,
                               0, 0, Qt::NoModifier, button, buttons);
            QCoreApplication::sendEvent(s.window, &event);
        };
        send(QEvent::TabletPress, QPointF(400, 300), 0.7, Qt::LeftButton, Qt::LeftButton);
        for (int i = 1; i <= 20; ++i) {
            send(QEvent::TabletMove, QPointF(400 + 10.0 * i, 300), 0.7, Qt::NoButton, Qt::LeftButton);
        }
        send(QEvent::TabletRelease, QPointF(600, 300), 0.0, Qt::LeftButton, Qt::NoButton);
        s.tools->activate(QStringLiteral("select"));
        s.tools->selectAll();
        const QVariantMap frame = s.tools->selectionFrame();
        const QVariantList corners = frame.value(QStringLiteral("corners")).toList();
        if (corners.size() < 4) {
            return {};
        }
        QPointF sum;
        for (const QVariant& c : corners) {
            const QVariantMap m = c.toMap();
            sum += (m.isEmpty() ? c.toPointF() : QPointF(m.value("x").toDouble(), m.value("y").toDouble())) /
                   4.0;
        }
        return sum;
    }

    static void seedTouchUi()
    {
        QSettings seed;
        seed.setValue(QStringLiteral("workstation/touch-ui"), true);
    }

    // Appui long tel que la vue l'émet : point, utilisateur du contact et son outil effectif (par défaut, le
    // premier et son outil courant).
    static void hold(Shell& s, QPointF page, int user = 0, const QString& tool = QString())
    {
        emit s.view->touchHeld(page, s.view->pageToScene(page), user,
                               tool.isEmpty() ? s.tools->currentTool() : tool);
    }

    // Doigt posé sur `page` et tenu jusqu'au signal de l'appui long ; rend l'outil effectif qu'il porte.
    static QString holdFinger(Shell& s, QPointingDevice* screen, QPointF page)
    {
        QSignalSpy held(s.view, &ui::PageView::touchHeld);
        QTest::touchEvent(s.window, screen).press(0, s.view->pageToScene(page).toPoint(), s.window);
        if (!QTest::qWaitFor([&] { return held.count() > 0; }, 3000)) {
            return QStringLiteral("(aucun appui long)");
        }
        return held.at(0).at(3).toString();
    }

    QTemporaryDir m_settings;
    std::unique_ptr<QPointingDevice> m_stylus;
};

// Réglages d'essai : aucun réglage ni dossier du poste touché.
void TstUiFingermenus::initTestCase()
{
    QVERIFY(m_settings.isValid());
    QStandardPaths::setTestModeEnabled(true);
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings.path());
    QSettings probe;
    QCOMPARE(probe.format(), QSettings::IniFormat);
    QVERIFY(probe.fileName().startsWith(m_settings.path()));
    m_stylus = std::make_unique<QPointingDevice>(
        QStringLiteral("tst-fingermenus-stylus"), 5252, QInputDevice::DeviceType::Stylus,
        QPointingDevice::PointerType::Pen,
        QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 3);
}

void TstUiFingermenus::longPressOnObjectOpensObjectMenu()
{
    Shell s;
    QVERIFY(start(s));
    const QPointF center = drawAndSelect(s);
    QVERIFY(!center.isNull());
    QObject* fingerMenu = object(s, "fingerMenu");
    QObject* objectMenu = object(s, "objectMenu");
    QVERIFY(fingerMenu && objectMenu);
    QVERIFY(!fingerMenu->property("visible").toBool());
    hold(s, center);
    QTRY_VERIFY(objectMenu->property("opened").toBool());
    QVERIFY(fingerMenu->property("visible").toBool());
    QCOMPARE(fingerMenu->property("kind").toString(), QStringLiteral("object"));
}

void TstUiFingermenus::longPressOnEmptyPageOpensPageMenu()
{
    Shell s;
    QVERIFY(start(s));
    QVERIFY(!drawAndSelect(s).isNull());
    QObject* fingerMenu = object(s, "fingerMenu");
    QObject* pageMenu = object(s, "fingerPageMenu");
    QVERIFY(fingerMenu && pageMenu);
    hold(s, QPointF(900, 700)); // loin du cadre de la sélection
    QTRY_VERIFY(pageMenu->property("opened").toBool());
    QVERIFY(fingerMenu->property("visible").toBool());
    QCOMPARE(fingerMenu->property("kind").toString(), QStringLiteral("page"));
    QVERIFY(!object(s, "objectMenu")->property("opened").toBool());
}

void TstUiFingermenus::noMenuWithInkTool()
{
    Shell s;
    QVERIFY(start(s));
    const QPointF center = drawAndSelect(s);
    QVERIFY(!center.isNull());
    s.tools->activate(QStringLiteral("pen"));
    QCOMPARE(s.tools->currentTool(), QStringLiteral("pen"));
    QObject* fingerMenu = object(s, "fingerMenu");
    QVERIFY(fingerMenu && object(s, "objectMenu") && object(s, "fingerPageMenu"));
    hold(s, center);
    hold(s, QPointF(900, 700));
    QTest::qWait(200);
    QVERIFY(!fingerMenu->property("visible").toBool());
    QVERIFY(!object(s, "objectMenu")->property("opened").toBool());
    QVERIFY(!object(s, "fingerPageMenu")->property("opened").toBool());
}

void TstUiFingermenus::paletteLongPressShowsLabelOnly()
{
    Shell s;
    QVERIFY(start(s));
    s.tools->activate(QStringLiteral("select"));
    QQuickItem* pen = item(s, "button-tool.pen");
    QTRY_VERIFY((pen = item(s, "button-tool.pen")) && pen->isVisible());
    QQuickItem* label = item(s, "paletteLabel");
    QVERIFY(label);
    QVERIFY(!label->isVisible());
    QSignalSpy triggers(s.commands, &ui::Commands::triggered);
    QPointingDevice* screen = QTest::createTouchDevice();
    const QPoint at = sceneCenter(pen);
    QTest::touchEvent(s.window, screen).press(0, at, s.window);
    QTest::qWait(700);
    QTest::touchEvent(s.window, screen).release(0, at, s.window);
    QTRY_VERIFY(label->isVisible());
    QCOMPARE(triggers.count(), 0);
    QCOMPARE(s.tools->currentTool(), QStringLiteral("select"));
    QTRY_VERIFY_WITH_TIMEOUT(!label->isVisible(), 4000); // bulle de 1,5 s
}

void TstUiFingermenus::menuRowsFollowTargetSize()
{
    seedTouchUi();
    Shell s;
    QVERIFY(start(s));
    QTRY_VERIFY(s.layout->touchUi());
    QObject* pageMenu = object(s, "fingerPageMenu");
    QVERIFY(pageMenu);
    s.tools->activate(QStringLiteral("select"));
    hold(s, QPointF(900, 700));
    QTRY_VERIFY(pageMenu->property("opened").toBool());
    QCOMPARE(pageMenu->property("rowHeight").toDouble(), s.layout->targetSize());
    const int count = pageMenu->property("count").toInt();
    QVERIFY(count > 0);
    int rows = 0;
    for (int i = 0; i < count; ++i) {
        QQuickItem* row = nullptr;
        QVERIFY(QMetaObject::invokeMethod(pageMenu, "itemAt", Q_RETURN_ARG(QQuickItem*, row), Q_ARG(int, i)));
        QVERIFY(row);
        if (row->objectName() == QLatin1String("menuSeparator")) {
            continue;
        }
        ++rows;
        QVERIFY2(row->height() >= s.layout->targetSize(),
                 qPrintable(QStringLiteral("%1 : %2").arg(row->objectName()).arg(row->height())));
    }
    QVERIFY(rows > 0);
}

void TstUiFingermenus::submenuOpensOnTap()
{
    seedTouchUi();
    Shell s;
    QVERIFY(start(s));
    QTRY_VERIFY(s.layout->touchUi());
    const QPointF center = drawAndSelect(s);
    QVERIFY(!center.isNull());
    QObject* objectMenu = object(s, "objectMenu");
    QVERIFY(objectMenu);
    hold(s, center);
    QTRY_VERIFY(objectMenu->property("opened").toBool());
    QQuickItem* entry = nullptr;
    QObject* sub = nullptr;
    const int count = objectMenu->property("count").toInt();
    for (int i = 0; i < count && !sub; ++i) {
        QQuickItem* row = nullptr;
        QVERIFY(
            QMetaObject::invokeMethod(objectMenu, "itemAt", Q_RETURN_ARG(QQuickItem*, row), Q_ARG(int, i)));
        if (row && row->property("subMenu").value<QObject*>()) {
            entry = row;
            sub = row->property("subMenu").value<QObject*>();
        }
    }
    QVERIFY(entry && sub);
    // Pas de survol au doigt : le pointeur posé dessus n'ouvre rien.
    QTest::mouseMove(s.window, sceneCenter(entry));
    QTest::qWait(900);
    QVERIFY(!sub->property("opened").toBool());
    // Un appui l'ouvre.
    QTest::mouseClick(s.window, Qt::LeftButton, Qt::NoModifier, sceneCenter(entry));
    QTRY_VERIFY(sub->property("opened").toBool());
}

void TstUiFingermenus::imposedSelectorOpensTheMenu()
{
    // L11-C13 (a), D11-7 : mode du toucher « sélection », stylo courant. Le doigt tient le sélecteur imposé :
    // son appui long sur l'objet ouvre le menu de l'objet, sans changer l'outil courant. Décidé sur l'outil
    // courant (le stylo), aucun menu ne s'ouvrirait.
    Shell s;
    QVERIFY(start(s));
    const QPointF center = drawAndSelect(s);
    QVERIFY(!center.isNull());
    s.tools->activate(QStringLiteral("pen"));
    QVERIFY(s.commands->trigger(QStringLiteral("touch.mode"), QStringLiteral("select")));
    QObject* fingerMenu = object(s, "fingerMenu");
    QObject* objectMenu = object(s, "objectMenu");
    QVERIFY(fingerMenu && objectMenu);
    QPointingDevice* screen = QTest::createTouchDevice();
    QCOMPARE(holdFinger(s, screen, center), QStringLiteral("select"));
    QTRY_VERIFY_WITH_TIMEOUT(objectMenu->property("opened").toBool(), 2000);
    QCOMPARE(fingerMenu->property("kind").toString(), QStringLiteral("object"));
    QTest::touchEvent(s.window, screen).release(0, s.view->pageToScene(center).toPoint(), s.window);
    QCOMPARE(s.tools->currentTool(), QStringLiteral("pen"));
}

void TstUiFingermenus::imposedPenOpensNoMenu()
{
    // L11-C13 (b) : mode du toucher « stylo », sélecteur courant. Le doigt écrit : son appui long ne montre
    // aucun menu (il en montrerait un, décidé sur l'outil courant).
    Shell s;
    QVERIFY(start(s));
    const QPointF center = drawAndSelect(s);
    QVERIFY(!center.isNull());
    QCOMPARE(s.tools->currentTool(), QStringLiteral("select"));
    QVERIFY(s.commands->trigger(QStringLiteral("touch.mode"), QStringLiteral("pen")));
    QObject* fingerMenu = object(s, "fingerMenu");
    QVERIFY(fingerMenu && object(s, "objectMenu") && object(s, "fingerPageMenu"));
    QPointingDevice* screen = QTest::createTouchDevice();
    QCOMPARE(holdFinger(s, screen, center), QStringLiteral("pen"));
    QTest::qWait(200);
    QVERIFY(!fingerMenu->property("visible").toBool());
    QVERIFY(!object(s, "objectMenu")->property("opened").toBool());
    QVERIFY(!object(s, "fingerPageMenu")->property("opened").toBool());
    QTest::touchEvent(s.window, screen).release(0, s.view->pageToScene(center).toPoint(), s.window);
    QCOMPARE(s.tools->currentTool(), QStringLiteral("select"));
}

void TstUiFingermenus::secondUserHoldOpensNoMenu()
{
    // L11-C13 (c), F-303 : mode à deux, premier au sélecteur sans rien de sélectionné. L'appui long du second
    // n'ouvre aucun menu, quel que soit son outil (les entrées à boîte du menu partiraient au nom du
    // premier), et ne touche pas la sélection du premier. Décidé sur l'outil et la sélection du premier, il
    // ouvrirait le menu de l'objet et y sélectionnerait le trait.
    Shell s;
    QVERIFY(start(s));
    const QPointF center = drawAndSelect(s);
    QVERIFY(!center.isNull());
    QVERIFY(s.commands->trigger(QStringLiteral("users.dual")));
    QVERIFY(s.commands->trigger(QStringLiteral("tool.pen"), QVariant(), 1));
    s.tools->selectObjects({});
    QCOMPARE(s.tools->currentTool(), QStringLiteral("select"));
    QTRY_VERIFY(!s.tools->selectionFrame().value(QStringLiteral("visible")).toBool());
    QObject* fingerMenu = object(s, "fingerMenu");
    QVERIFY(fingerMenu && object(s, "objectMenu") && object(s, "fingerPageMenu"));
    hold(s, center, 1, QStringLiteral("pen"));
    hold(s, QPointF(900, 700), 1, QStringLiteral("pen"));
    hold(s, center, 1, QStringLiteral("select"));
    QTest::qWait(200);
    QVERIFY(!fingerMenu->property("visible").toBool());
    QVERIFY(!object(s, "objectMenu")->property("opened").toBool());
    QVERIFY(!object(s, "fingerPageMenu")->property("opened").toBool());
    QVERIFY(!s.tools->selectionFrame().value(QStringLiteral("visible")).toBool());
}

QTEST_MAIN(TstUiFingermenus)
#include "tst_ui_fingermenus.moc"
