#include "startup.h"

#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/keyboardinset.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/textediting.h>

#include <QPointingDevice>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSettings>
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
    ui::DocumentController* document = nullptr;
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
    s.document = s.window->findChild<ui::DocumentController*>(QStringLiteral("documentController"));
    return s.view && s.document;
}

bool resizeTo(Shell& s, QSize size)
{
    s.window->resize(size);
    if (!QTest::qWaitFor([&] { return s.window->size() == size; }, 5000)) {
        return false;
    }
    QTest::qWait(100); // la mise en page se refait au tour de rendu suivant
    return true;
}

} // namespace

class TstUiOrientation : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void shiftZeroWithoutKeyboard();
    void shiftKeepsTheCaretAbove();
    void shiftResetsWhenHidden();
    void rotationKeepsTheStroke();
    void rotationDuringStrokeKeepsIt();

private:
    void send(Shell& s, QEvent::Type type, QPointF page, qreal pressure, Qt::MouseButton button,
              Qt::MouseButtons buttons)
    {
        const QPointF scene = s.view->pageToScene(page);
        QTabletEvent event(type, m_stylus.get(), scene, s.window->mapToGlobal(scene), pressure, 0, 0, 0, 0, 0,
                           Qt::NoModifier, button, buttons);
        QCoreApplication::sendEvent(s.window, &event);
    }

    // Trait horizontal de (300, y) à (500, y), moitié avant la rotation éventuelle, moitié après.
    void stroke(Shell& s, qreal y, const QSize* rotateTo)
    {
        send(s, QEvent::TabletPress, QPointF(300, y), 0.7, Qt::LeftButton, Qt::LeftButton);
        for (int i = 1; i <= 100; ++i) {
            if (rotateTo && i == 50) {
                QVERIFY(resizeTo(s, *rotateTo));
            }
            send(s, QEvent::TabletMove, QPointF(300 + 2.0 * i, y), 0.7, Qt::NoButton, Qt::LeftButton);
        }
        send(s, QEvent::TabletRelease, QPointF(500, y), 0.0, Qt::LeftButton, Qt::NoButton);
    }

    QTemporaryDir m_settings;
    std::unique_ptr<QPointingDevice> m_stylus;
};

// Réglages d'essai : aucun réglage ni dossier du poste touché.
void TstUiOrientation::initTestCase()
{
    QVERIFY(m_settings.isValid());
    QStandardPaths::setTestModeEnabled(true);
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings.path());
    QSettings probe;
    QCOMPARE(probe.format(), QSettings::IniFormat);
    QVERIFY(probe.fileName().startsWith(m_settings.path()));
    m_stylus = std::make_unique<QPointingDevice>(
        QStringLiteral("tst-orientation-stylus"), 5151, QInputDevice::DeviceType::Stylus,
        QPointingDevice::PointerType::Pen,
        QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 3);
}

void TstUiOrientation::shiftZeroWithoutKeyboard()
{
    ui::KeyboardInset inset;
    inset.setCaretRect(QRectF(100, 560, 20, 40));
    QCOMPARE(inset.shift(), 0.0); // pas de clavier : rien à dégager
    inset.setKeyboardRect(QRectF(0, 500, 800, 300));
    QVERIFY(inset.shift() > 0);
    inset.setKeyboardRect(QRectF(0, 700, 800, 300)); // clavier plus bas que le curseur et sa marge
    QCOMPARE(inset.shift(), 0.0);
}

void TstUiOrientation::shiftKeepsTheCaretAbove()
{
    ui::KeyboardInset inset;
    QCOMPARE(inset.margin(), 16.0);
    inset.setKeyboardRect(QRectF(0, 500, 800, 300));
    inset.setCaretRect(QRectF(100, 560, 20, 40)); // bas à 600
    QCOMPARE(inset.shift(), 116.0);               // 600 + 16 - 500

    // De bout en bout : la zone de la page applique le défilement pendant l'édition d'un texte.
    Shell s;
    QVERIFY(start(s));
    auto* area = s.window->findChild<QObject*>(QStringLiteral("pageArea"));
    auto* live = s.window->findChild<ui::KeyboardInset*>(QStringLiteral("keyboardInset"));
    auto* text = s.window->findChild<ui::TextEditing*>(QStringLiteral("appText"));
    QVERIFY(area && live && text);
    QVERIFY(text->beginAt(QPointF(300, 600), false));
    const QSizeF size = s.window->size();
    live->setKeyboardRect(QRectF(0, size.height() - 300, size.width(), 300));
    QVERIFY(live->shift() > 0);
    QCOMPARE(area->property("appliedShift").toDouble(), live->shift());
    // Le curseur appliqué reste au-dessus du clavier, marge comprise.
    QVERIFY(live->caretRect().bottom() - live->shift() + live->margin() <= live->keyboardRect().top() + 0.5);
}

void TstUiOrientation::shiftResetsWhenHidden()
{
    Shell s;
    QVERIFY(start(s));
    auto* area = s.window->findChild<QObject*>(QStringLiteral("pageArea"));
    auto* live = s.window->findChild<ui::KeyboardInset*>(QStringLiteral("keyboardInset"));
    auto* text = s.window->findChild<ui::TextEditing*>(QStringLiteral("appText"));
    QVERIFY(area && live && text);
    QVERIFY(text->beginAt(QPointF(300, 600), false));
    const QSizeF size = s.window->size();
    live->setKeyboardRect(QRectF(0, size.height() - 300, size.width(), 300));
    QVERIFY(area->property("appliedShift").toDouble() > 0);
    live->setKeyboardRect(QRectF()); // clavier masqué
    QCOMPARE(live->shift(), 0.0);
    QCOMPARE(area->property("appliedShift").toDouble(), 0.0);
    // Clavier remonté puis texte fermé : la vue revient à 0.
    live->setKeyboardRect(QRectF(0, size.height() - 300, size.width(), 300));
    QVERIFY(area->property("appliedShift").toDouble() > 0);
    text->cancel();
    QCOMPARE(area->property("appliedShift").toDouble(), 0.0);
}

void TstUiOrientation::rotationKeepsTheStroke()
{
    Shell s;
    QVERIFY(start(s));
    QVERIFY(resizeTo(s, QSize(1280, 800)));
    const int before = s.document->shownObjectCount();
    stroke(s, 300, nullptr);
    QVERIFY(QTest::qWaitFor([&] { return s.document->shownObjectCount() == before + 1; }, 5000));
    QVERIFY(resizeTo(s, QSize(800, 1280)));
    QCOMPARE(s.document->shownObjectCount(), before + 1);
    const QPointF scene = s.view->pageToScene(QPointF(400, 300));
    QVERIFY(
        QRectF(QPointF(0, 0), QSizeF(s.window->size())).contains(scene)); // le trait reste dans la fenêtre
    const QColor c =
        s.window->grabWindow().pixelColor((scene * s.window->effectiveDevicePixelRatio()).toPoint());
    QVERIFY2(c.lightness() < 160, qPrintable(c.name()));
    // Retour en paysage : toujours là.
    QVERIFY(resizeTo(s, QSize(1280, 800)));
    QCOMPARE(s.document->shownObjectCount(), before + 1);
}

void TstUiOrientation::rotationDuringStrokeKeepsIt()
{
    Shell s;
    QVERIFY(start(s));
    QVERIFY(resizeTo(s, QSize(1280, 800)));
    const int before = s.document->shownObjectCount();
    const QSize portrait(800, 1280);
    stroke(s, 300, &portrait); // la rotation tombe au milieu du trait
    QCOMPARE(s.window->size(), portrait);
    QVERIFY(QTest::qWaitFor([&] { return s.document->shownObjectCount() == before + 1; }, 5000));
    QVERIFY(!s.view->contactActive()); // le contact en cours est terminé, pas laissé ouvert
}

QTEST_MAIN(TstUiOrientation)
#include "tst_ui_orientation.moc"
