#include "keyboardcontroller.h"
#include "support/lot7registry.h"
#include "support/presentationharness.h"

#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>

#include <QEvent>
#include <QGuiApplication>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRectF>
#include <QStringList>
#include <QTemporaryDir>
#include <QUrl>
#include <QWindow>
#include <QtTest>

#include <memory>

namespace {

using actcraft::ui::Commands;
using actcraft::ui::DocumentController;
using actcraft::ui::KeyboardController;
using presentationharness::MainWindow;
using presentationharness::Pointer;

// Contrôleur porté par la racine `keyboardRoot` de Presentation.qml (QtObject : cherché par l'arbre des
// objets).
KeyboardController* keyboardOf(const MainWindow& main)
{
    if (!main.window()) {
        return nullptr;
    }
    QObject* root = main.window()->findChild<QObject*>(QStringLiteral("keyboardRoot"));
    if (!root) {
        return nullptr;
    }
    return qobject_cast<KeyboardController*>(root->property("controller").value<QObject*>());
}

QQmlEngine* engineOf(const MainWindow& main)
{
    QQuickWindow* window = main.window();
    if (QQmlEngine* engine = qmlEngine(window)) {
        return engine;
    }
    const QList<QQuickItem*> items = window->contentItem()->childItems();
    for (QQuickItem* item : items) {
        if (QQmlEngine* engine = qmlEngine(item)) {
            return engine;
        }
    }
    return nullptr;
}

// Champ de texte de test dans la fenêtre principale, active, le focus sur le champ.
std::unique_ptr<QQuickItem> focusedField(const MainWindow& main)
{
    QQmlEngine* engine = engineOf(main);
    if (!engine) {
        return {};
    }
    QQmlComponent component(engine);
    component.setData(
        "import QtQuick\nimport QtQuick.Controls.Basic\n"
        "TextField { objectName: \"keyboardTestField\"; x: 20; y: 20; width: 240; height: 40 }\n",
        QUrl());
    std::unique_ptr<QQuickItem> field(qobject_cast<QQuickItem*>(component.create()));
    if (!field) {
        return {};
    }
    field->setParentItem(main.window()->contentItem());
    main.window()->requestActivate();
    if (!QTest::qWaitForWindowActive(main.window())) {
        return {};
    }
    field->forceActiveFocus();
    if (QGuiApplication::focusObject() != field.get()) {
        return {};
    }
    return field;
}

QString textOf(const QQuickItem* field)
{
    return field->property("text").toString();
}

// Ouvre le clavier par sa commande nommée ; vrai quand sa fenêtre est exposée.
bool openKeyboard(MainWindow& main, KeyboardController* keyboard)
{
    if (!main.run(QStringLiteral("tool.keyboard"))) {
        return false;
    }
    if (!keyboard->isOpen() || !keyboard->window()) {
        return false;
    }
    return QTest::qWaitForWindowExposed(keyboard->window());
}

// Centre de la touche `id` dans sa fenêtre ; nul si la touche manque ou tombe hors de la fenêtre (hors écran,
// l'écran mesure 800 x 800 : un appui hors de la fenêtre ne prouverait rien).
QPointF keyCenter(QQuickWindow* window, const QString& id)
{
    QQuickItem* key =
        presentationharness::findItem(window->contentItem(), QStringLiteral("keyboardKey_") + id);
    if (!key || !key->isVisible() || key->width() <= 0 || key->height() <= 0) {
        return {};
    }
    const QPointF center = key->mapToScene(QPointF(key->width() / 2, key->height() / 2));
    if (!QRectF(QPointF(0, 0), QSizeF(window->size())).contains(center)) {
        return {};
    }
    return center;
}

// Compte les appuis de touche envoyés à une fenêtre, sans les arrêter.
class KeyPressCounter : public QObject
{
public:
    explicit KeyPressCounter(QObject* watched) : m_watched(watched) { watched->installEventFilter(this); }

    int count() const { return m_count; }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == m_watched && event->type() == QEvent::KeyPress) {
            ++m_count;
        }
        return false;
    }

private:
    QObject* m_watched;
    int m_count = 0;
};

// Plateforme qui donne quand même le focus au clavier (drapeau ignoré), simulée hors écran en retirant le
// drapeau : active la fenêtre du clavier et tape la touche `id` à l'instant où le clavier reçoit le focus,
// avant que le contrôleur ne le rende. Vrai si la touche est partie pendant que le clavier avait le focus.
bool typeWhileTheKeyboardHasTheFocus(KeyboardController* keyboard, const QString& id)
{
    QQuickWindow* window = keyboard->window();
    if (!window) {
        return false;
    }
    bool reached = false;
    bool focused = false;
    bool typed = false;
    const QMetaObject::Connection connection =
        QObject::connect(qGuiApp, &QGuiApplication::focusWindowChanged, keyboard, [&](QWindow* focus) {
            if (focus != window || reached) {
                return;
            }
            reached = true;
            focused = QGuiApplication::focusWindow() == window;
            typed = keyboard->press(id);
        });
    window->setFlag(Qt::WindowDoesNotAcceptFocus, false);
    window->requestActivate();
    const bool waited = QTest::qWaitFor([&reached] { return reached; });
    QObject::disconnect(connection);
    return waited && focused && typed;
}

// Tout le fichier dans cet espace de noms anonyme (exécutable unique : aucun nom global hors du registre).
class TstUiKeyboard : public QObject
{
    Q_OBJECT

private slots:
    // Chaque touche part en QKeyEvent vers le champ qui a le focus dans ActCraft ; l'injecteur (frappe vers
    // une autre application) n'est pas appelé. Mutant : press() envoie toujours à l'injecteur.
    void keyboardTypesIntoTheFocusedField()
    {
        MainWindow main;
        QVERIFY(main.window());
        KeyboardController* keyboard = keyboardOf(main);
        QVERIFY(keyboard);
        QStringList injected;
        keyboard->setInjector([&injected](const QString& text) {
            injected << text;
            return true;
        });
        const std::unique_ptr<QQuickItem> field = focusedField(main);
        QVERIFY(field);
        QVERIFY(openKeyboard(main, keyboard));
        QVERIFY(keyboard->press(QStringLiteral("b")));
        QVERIFY(keyboard->press(QStringLiteral("shift")));
        QVERIFY(keyboard->press(QStringLiteral("o")));
        QVERIFY(keyboard->press(QStringLiteral("n")));
        QCOMPARE(textOf(field.get()), QStringLiteral("bOn"));
        QVERIFY(injected.isEmpty());
        QCOMPARE(QGuiApplication::focusObject(), field.get());
    }

    // Fenêtre propre aux trois drapeaux de 7b, sans fenêtre parente : la fenêtre principale réduite
    // (annotation du bureau) ne l'emporte pas (piège de L7-T31 : un Window enfant d'un Item prend la fenêtre
    // principale pour parent). Un appui à la souris sur une touche ne déplace pas le focus. Que Windows ne
    // lui donne vraiment pas le focus se prouve à l'écran. Mutants : drapeau WindowDoesNotAcceptFocus retiré
    // ; Window créé avec la racine pour parent.
    void keyboardWindowNeverTakesFocus()
    {
        MainWindow main;
        QVERIFY(main.window());
        KeyboardController* keyboard = keyboardOf(main);
        QVERIFY(keyboard);
        QStringList injected;
        keyboard->setInjector([&injected](const QString& text) {
            injected << text;
            return true;
        });
        const std::unique_ptr<QQuickItem> field = focusedField(main);
        QVERIFY(field);
        QVERIFY(openKeyboard(main, keyboard));
        QQuickWindow* window = keyboard->window();
        QVERIFY(window != main.window());
        QVERIFY(window->flags().testFlag(Qt::WindowDoesNotAcceptFocus));
        QVERIFY(window->flags().testFlag(Qt::WindowStaysOnTopHint));
        QCOMPARE(window->type(), Qt::Tool);
        QCOMPARE(window->transientParent(), nullptr);
        const QPointF center = keyCenter(window, QStringLiteral("a"));
        QVERIFY2(!center.isNull(), "touche « a » absente ou hors de la fenêtre du clavier");
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, center.toPoint());
        QTRY_COMPARE(textOf(field.get()), QStringLiteral("a"));
        QCoreApplication::processEvents();
        QCOMPARE(QGuiApplication::focusWindow(), main.window());
        QCOMPARE(QGuiApplication::focusObject(), field.get());
        QVERIFY(injected.isEmpty());
    }

    // Décision 4 bis : le pointeur est envoyé à la fenêtre du clavier, SON effet est lu (la touche tape son
    // caractère dans le champ de test), et la page de la fenêtre principale reste inchangée. Patron écarté :
    // le clavier posé sur la page de la fenêtre principale et déclaré couverture (keyboardBlocksTheStylus de
    // 7b) : pas de fenêtre à lui, rouge. Mutant : touches sans gestionnaire du stylet → rouge sur `stylet`.
    void keyboardWindowTakesEveryPointer_data() { presentationharness::addPointerRows(); }
    void keyboardWindowTakesEveryPointer()
    {
        QFETCH(Pointer, pointer);
        MainWindow main;
        QVERIFY(main.window());
        KeyboardController* keyboard = keyboardOf(main);
        QVERIFY(keyboard);
        QStringList injected;
        keyboard->setInjector([&injected](const QString& text) {
            injected << text;
            return true;
        });
        QVERIFY(main.pageView());
        DocumentController* open = main.pageView()->documentController();
        QVERIFY(open);
        const QString openBefore = open->fingerprintText();
        const int mainObjects = main.pageObjectCount();
        const std::unique_ptr<QQuickItem> field = focusedField(main);
        QVERIFY(field);
        QVERIFY(openKeyboard(main, keyboard));
        QQuickWindow* window = keyboard->window();
        QVERIFY(window != main.window());
        const QPointF center = keyCenter(window, QStringLiteral("z"));
        QVERIFY2(!center.isNull(), "touche « z » absente ou hors de la fenêtre du clavier");
        presentationharness::tapAt(window, pointer, center);
        QTRY_COMPARE(textOf(field.get()), QStringLiteral("z"));
        QVERIFY(injected.isEmpty());
        QCOMPARE(QGuiApplication::focusObject(), field.get());
        QCOMPARE(open->fingerprintText(), openBefore);
        QCOMPARE(main.pageObjectCount(), mainObjects);
    }

    // Fermeture par la croix de la fenêtre : le clavier se ferme, sa fenêtre est détruite ; la commande le
    // rouvre dans une fenêtre neuve, et la ferme. Mutant : la fermeture ne remet pas `open` à faux.
    void keyboardClosesByItsCross()
    {
        MainWindow main;
        QVERIFY(main.window());
        KeyboardController* keyboard = keyboardOf(main);
        QVERIFY(keyboard);
        QVERIFY(!keyboard->isOpen());
        QVERIFY(!keyboard->window());
        QVERIFY(openKeyboard(main, keyboard));
        QPointer<QQuickWindow> first(keyboard->window());
        QVERIFY(first->close());
        QTRY_VERIFY(!keyboard->isOpen());
        QTRY_VERIFY(first.isNull());
        QVERIFY(!keyboard->window());
        QVERIFY(openKeyboard(main, keyboard));
        QPointer<QQuickWindow> second(keyboard->window());
        QVERIFY(main.run(QStringLiteral("tool.keyboard")));
        QVERIFY(!keyboard->isOpen());
        QTRY_VERIFY(second.isNull());
    }

    // Relecture de L7-T29 : la disposition Compact n'a pas de touche de verrouillage des majuscules ; y
    // passer verrouillé enlève le verrouillage, sinon tout sortirait en majuscules sans issue. Mutant :
    // setLayout garde le verrouillage.
    void compactLayoutDropsTheCapsLock()
    {
        MainWindow main;
        QVERIFY(main.window());
        KeyboardController* keyboard = keyboardOf(main);
        QVERIFY(keyboard);
        QStringList injected;
        keyboard->setInjector([&injected](const QString& text) {
            injected << text;
            return true;
        });
        const std::unique_ptr<QQuickItem> field = focusedField(main);
        QVERIFY(field);
        keyboard->setLayout(KeyboardController::Layout::Standard);
        QVERIFY(keyboard->press(QStringLiteral("caps")));
        QVERIFY(keyboard->capsLocked());
        QVERIFY(keyboard->press(QStringLiteral("a")));
        keyboard->setLayout(KeyboardController::Layout::Compact);
        QCOMPARE(keyboard->layout(), KeyboardController::Layout::Compact);
        QVERIFY(!keyboard->capsLocked());
        QVERIFY(keyboard->press(QStringLiteral("b")));
        QCOMPARE(textOf(field.get()), QStringLiteral("Ab"));
        QVERIFY(injected.isEmpty());
    }

    // Repli d'une plateforme qui donne quand même le focus au clavier : une touche tapée à cet instant part
    // vers la fenêtre d'ActCraft d'avant (pas vers le clavier ni vers l'injecteur), et le contrôleur rend le
    // focus à cette fenêtre. Mutants : focusTarget sans repli vers la fenêtre d'avant ; onFocusWindowChanged
    // sans requestActivate.
    void focusTakenByTheKeyboardGoesBack()
    {
        MainWindow main;
        QVERIFY(main.window());
        KeyboardController* keyboard = keyboardOf(main);
        QVERIFY(keyboard);
        QStringList injected;
        keyboard->setInjector([&injected](const QString& text) {
            injected << text;
            return true;
        });
        const std::unique_ptr<QQuickItem> field = focusedField(main);
        QVERIFY(field);
        QVERIFY(openKeyboard(main, keyboard));
        QTRY_COMPARE(QGuiApplication::focusWindow(), main.window());
        KeyPressCounter toMain(main.window());
        KeyPressCounter toKeyboard(keyboard->window());
        QVERIFY(typeWhileTheKeyboardHasTheFocus(keyboard, QStringLiteral("q")));
        QCOMPARE(toMain.count(), 1);
        QCOMPARE(toKeyboard.count(), 0);
        QVERIFY(injected.isEmpty());
        QTRY_COMPARE(QGuiApplication::focusWindow(), main.window());
        QTRY_COMPARE(QGuiApplication::focusObject(), field.get());
    }

    // Relecture de L7-T29 : quand le focus quitte ActCraft (une autre application au premier plan), le
    // contrôleur oublie la fenêtre d'avant. Si une plateforme donne ensuite le focus au clavier, la frappe
    // part à l'injecteur (vers l'application au premier plan), pas vers une fenêtre d'ActCraft périmée. Hors
    // écran, la plateforme ne retire jamais le focus à toutes les fenêtres : le test émet la notification que
    // Qt émet alors. Mutant : la fenêtre d'avant gardée quand le focus quitte ActCraft.
    void keyboardForgetsActCraftWhenTheFocusLeavesIt()
    {
        MainWindow main;
        QVERIFY(main.window());
        KeyboardController* keyboard = keyboardOf(main);
        QVERIFY(keyboard);
        QStringList injected;
        keyboard->setInjector([&injected](const QString& text) {
            injected << text;
            return true;
        });
        const std::unique_ptr<QQuickItem> field = focusedField(main);
        QVERIFY(field);
        QVERIFY(openKeyboard(main, keyboard));
        QTRY_COMPARE(QGuiApplication::focusWindow(), main.window());
        KeyPressCounter toMain(main.window());
        emit qGuiApp->focusWindowChanged(nullptr); // le focus quitte ActCraft
        QVERIFY(typeWhileTheKeyboardHasTheFocus(keyboard, QStringLiteral("q")));
        QCOMPARE(injected, QStringList{QStringLiteral("q")});
        QCOMPARE(toMain.count(), 0);
        QCOMPARE(textOf(field.get()), QString());
    }

    // Fonction de bureau : la commande n'existe pas sur Android (plateformes de commands.json).
    void keyboardAbsentOnAndroid()
    {
        QTemporaryDir profile;
        Commands commands;
        commands.setProfileDir(profile.path());
        commands.setPlatform(actcraft::actions::Platform::Windows);
        QVERIFY(commands.isAvailable(QStringLiteral("tool.keyboard")));
        commands.setPlatform(actcraft::actions::Platform::Android);
        QVERIFY(!commands.isAvailable(QStringLiteral("tool.keyboard")));
        QVERIFY(!commands.trigger(QStringLiteral("tool.keyboard")));
    }
};

} // namespace

ACTCRAFT_LOT7_TEST("keyboard", TstUiKeyboard)
#include "tst_ui_keyboard.moc"
