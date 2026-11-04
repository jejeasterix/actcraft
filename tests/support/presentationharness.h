#pragma once

// Aides des tests d'interface du lot 7 : fenêtre principale hors écran, trait injecté au stylet, au doigt ou
// à la souris. Stylet : QTabletEvent envoyé à la fenêtre (le filtre de la vue le prend avant les éléments
// QML, socle H-S1, tests/tst_pageview.cpp) ; doigt : QTest::touchEvent sur un périphérique tactile de test ;
// souris : QTest::mousePress / mouseMove / mouseRelease. API publique de Qt seulement.
#include "startup.h"

#include <actcraft/render/fixedpageitem.h>
#include <actcraft/ui/pageview.h>

#include <QMetaObject>
#include <QPointF>
#include <QPointingDevice>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QSettings>
#include <QTabletEvent>
#include <QTemporaryDir>
#include <QVariant>
#include <QtTest>

namespace presentationharness {

enum class Pointer
{
    Stylus,
    Finger,
    Mouse
};

inline void addPointerRows()
{
    QTest::addColumn<Pointer>("pointer");
    QTest::newRow("stylet") << Pointer::Stylus;
    QTest::newRow("doigt") << Pointer::Finger;
    QTest::newRow("souris") << Pointer::Mouse;
}

// Périphériques construits comme ceux de tests/tst_ui_inputpath.cpp:220-224 (socle L0-S7) ; jamais
// d'enregistrement par QWindowSystemInterface (API privée).
inline const QPointingDevice* stylusDevice()
{
    // Volontairement jamais détruit : un objet statique serait détruit après QGuiApplication (revue contre le
    // socle).
    static const QPointingDevice* device =
        new QPointingDevice(QStringLiteral("tst-ui-lot7-stylus"), 5171, QInputDevice::DeviceType::Stylus,
                            QPointingDevice::PointerType::Pen,
                            QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 3);
    return device;
}

inline QPointingDevice* touchDevice()
{
    static QPointingDevice* device = QTest::createTouchDevice();
    return device;
}

// Comme tst_ui_inputpath.cpp:141-150 : bouton changé à l'appui et au relâché, aucun au déplacement.
inline void sendTablet(QQuickWindow* window, QEvent::Type type, QPointF point)
{
    const Qt::MouseButtons buttons = type == QEvent::TabletRelease ? Qt::NoButton : Qt::LeftButton;
    const Qt::MouseButton changed = type == QEvent::TabletMove ? Qt::NoButton : Qt::LeftButton;
    QTabletEvent event(type, stylusDevice(), point, window->mapToGlobal(point), buttons ? 0.5 : 0.0, 0, 0, 0,
                       0, 0, Qt::NoModifier, changed, buttons);
    QCoreApplication::sendEvent(window, &event);
}

// Qt Quick retarde et compresse les TouchUpdate jusqu'à l'image suivante (tst_ui_inputpath.cpp:171-176) : une
// image entre deux étapes.
inline void settleTouch()
{
    QTest::qWait(50);
}

inline void strokeAt(QQuickWindow* window, Pointer pointer, QPointF from, QPointF to, int steps = 8)
{
    const auto at = [&](int i) { return from + (to - from) * (static_cast<qreal>(i) / steps); };
    switch (pointer) {
    case Pointer::Stylus:
        sendTablet(window, QEvent::TabletPress, from);
        for (int i = 1; i <= steps; ++i) {
            sendTablet(window, QEvent::TabletMove, at(i));
        }
        sendTablet(window, QEvent::TabletRelease, to);
        break;
    case Pointer::Finger:
        QTest::touchEvent(window, touchDevice()).press(0, from.toPoint());
        settleTouch();
        for (int i = 1; i <= steps; ++i) {
            QTest::touchEvent(window, touchDevice()).move(0, at(i).toPoint());
            settleTouch();
        }
        QTest::touchEvent(window, touchDevice()).release(0, to.toPoint());
        settleTouch();
        break;
    case Pointer::Mouse:
        QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, from.toPoint());
        for (int i = 1; i <= steps; ++i) {
            QTest::mouseMove(window, at(i).toPoint());
        }
        QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, to.toPoint());
        break;
    }
    QCoreApplication::processEvents();
}

// Les délégués d'un Repeater n'ont pas toujours la fenêtre pour parent QObject : on suit l'arbre des éléments
// (tests/tst_ui_shell.cpp, findItem).
inline QQuickItem* findItem(QQuickItem* root, const QString& name)
{
    if (!root) {
        return nullptr;
    }
    if (root->objectName() == name) {
        return root;
    }
    const QList<QQuickItem*> children = root->childItems();
    for (QQuickItem* child : children) {
        if (QQuickItem* found = findItem(child, name)) {
            return found;
        }
    }
    return nullptr;
}

// Fenêtre principale chargée comme l'application (app/startup.cpp, loadMainWindow) et comme tst_ui_shell
// (start, ready). Réglages des outils dans un dossier temporaire, jamais dans le vrai registre : le sien, ou
// celui que le test passe (deux fenêtres successives qui partagent leurs réglages). Tout avertissement QML
// d'un fichier du module fait échouer la fonction de test en cours, au chargement, pendant le test et au
// démontage (failOnWarning est posé avant le chargement et vaut jusqu'à la fin de la fonction).
class MainWindow
{
public:
    explicit MainWindow(StartupOptions options = {}, const QString& settingsPath = {})
    {
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                           settingsPath.isEmpty() ? m_settingsDir.path() : settingsPath);
        QTest::failOnWarning(QRegularExpression(QStringLiteral("qrc:.*\\.qml")));
        QQuickWindow* loaded = loadMainWindow(m_engine, options);
        if (!loaded || !QTest::qWaitForWindowExposed(loaded)) {
            return;
        }
        m_view = loaded->findChild<actcraft::ui::PageView*>(QStringLiteral("pageView"));
        m_fixed = loaded->findChild<actcraft::render::FixedPageItem*>(QStringLiteral("fixedPage"));
        if (waitReady()) {
            m_window = loaded;
        }
    }

    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;

    // Nul si le chargement, l'exposition ou la première page ont échoué.
    QQuickWindow* window() const noexcept { return m_window; }
    actcraft::ui::PageView* pageView() const noexcept { return m_view; }
    actcraft::render::FixedPageItem* fixedPage() const noexcept { return m_fixed; }

    // Élément nommé : par l'arbre des objets, sinon par l'arbre des éléments.
    QQuickItem* item(const QString& objectName) const
    {
        if (!m_window) {
            return nullptr;
        }
        if (auto* found = m_window->findChild<QQuickItem*>(objectName)) {
            return found;
        }
        return findItem(m_window->contentItem(), objectName);
    }

    // Objets de la page affichée, lus sur la page fixe comme les tests du socle ; -1 sans page fixe.
    int pageObjectCount() const { return m_fixed ? m_fixed->objectCount() : -1; }

    // Page fixe prête et cache de la vue à jour (tst_ui_shell, ready).
    bool waitReady() const
    {
        return m_view && m_fixed &&
               QTest::qWaitFor([this] { return m_fixed->isReady() && m_view->cacheReady(); }, 10000);
    }

    // Affirmation négative sur le versement d'un trait : vrai si la page affichée garde `expected` objets
    // pendant `ms` millisecondes, faux dès que le compte change. waitReady() n'y suffit pas : un trait sur la
    // même page ne relance pas le décodage, la page fixe reste prête (render/src/fixedpageitem.cpp:104-114).
    bool countStaysAt(int expected, int ms = 300) const
    {
        return pageObjectCount() == expected &&
               !QTest::qWaitFor([&] { return pageObjectCount() != expected; }, ms);
    }

    // Commande nommée, par Main.qml (runCommand), comme le menu ou un raccourci (tst_ui_shell, run).
    bool run(const QString& id, const QVariant& argument = QVariant()) const
    {
        if (!m_window) {
            return false;
        }
        QVariant result;
        const bool invoked =
            QMetaObject::invokeMethod(m_window, "runCommand", Q_RETURN_ARG(QVariant, result),
                                      Q_ARG(QVariant, QVariant(id)), Q_ARG(QVariant, argument));
        return invoked && result.toBool();
    }

private:
    // Déclaré avant le moteur : détruit après lui (les outils écrivent leurs réglages au démontage).
    QTemporaryDir m_settingsDir;
    QQmlApplicationEngine m_engine;
    QQuickWindow* m_window = nullptr;
    actcraft::ui::PageView* m_view = nullptr;
    actcraft::render::FixedPageItem* m_fixed = nullptr;
};

} // namespace presentationharness

Q_DECLARE_METATYPE(presentationharness::Pointer)
