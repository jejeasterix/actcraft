#include "startup.h"
#include "support/wcag.h"

#include <actcraft/actions/commandregistry.h>

#include <QAccessible>
#include <QImage>
#include <QPointer>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <QtQml/qqmlextensionplugin.h>

#include <algorithm>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

// L11-T24 : ce que tst_ui_shell prouve pour la fenêtre principale (contrastes AA, focus visible) est étendu à
// chaque boîte et à la mise en page étroite ; chaque bouton a un nom accessible.

namespace {

struct Shell
{
    QTemporaryDir profiles;
    QTemporaryDir recovery;
    QQmlApplicationEngine engine;
    QQuickWindow* window = nullptr;
};

bool start(Shell& s)
{
    StartupOptions options;
    options.profileRoot = s.profiles.path();
    options.recoveryDir = s.recovery.path();
    s.window = loadMainWindow(s.engine, options);
    if (!s.window || !QTest::qWaitForWindowExposed(s.window)) {
        return false;
    }
    s.window->requestActivate();
    return QTest::qWaitForWindowActive(s.window);
}

bool run(Shell& s, const QString& id)
{
    QVariant result;
    const bool invoked =
        QMetaObject::invokeMethod(s.window, "runCommand", Q_RETURN_ARG(QVariant, result),
                                  Q_ARG(QVariant, QVariant(id)), Q_ARG(QVariant, QVariant()));
    return invoked && result.toBool();
}

QColor colorProperty(const QObject* object, const char* name)
{
    return object ? object->property(name).value<QColor>() : QColor();
}

QQuickItem* itemProperty(const QObject* object, const char* name)
{
    return object ? object->property(name).value<QQuickItem*>() : nullptr;
}

double effectiveOpacity(const QQuickItem* item)
{
    double opacity = 1.0;
    for (const QQuickItem* it = item; it; it = it->parentItem()) {
        opacity *= it->opacity();
    }
    return opacity;
}

QColor over(const QColor& fg, const QColor& bg, double alpha)
{
    const double a = fg.alphaF() * alpha;
    return QColor::fromRgbF(static_cast<float>(fg.redF() * a + bg.redF() * (1 - a)),
                            static_cast<float>(fg.greenF() * a + bg.greenF() * (1 - a)),
                            static_cast<float>(fg.blueF() * a + bg.blueF() * (1 - a)));
}

QString describe(const QQuickItem* item)
{
    QStringList path;
    for (const QQuickItem* it = item; it && path.size() < 6; it = it->parentItem()) {
        path << (it->objectName().isEmpty() ? QString::fromLatin1(it->metaObject()->className())
                                            : it->objectName());
    }
    return path.join(QStringLiteral(" < "));
}

bool controlEnabled(const QQuickItem* item)
{
    for (const QQuickItem* it = item; it; it = it->parentItem()) {
        if (it->inherits("QQuickControl")) {
            return it->isEnabled();
        }
    }
    return true;
}

QRect pixelRect(const QQuickItem* item, const QImage& image, qreal dpr)
{
    const QRectF scene = item->mapRectToScene(item->boundingRect());
    return QRectF(scene.topLeft() * dpr, scene.size() * dpr).toAlignedRect().intersected(image.rect());
}

// Tout texte visible et actif sous `root` : couleur du texte contre le fond relevé sur l'image rendue.
void collectTexts(QQuickItem* item, const QImage& image, qreal dpr, QStringList& failures, const QString& scope,
                  int& checked)
{
    if (!item || !item->isVisible() || effectiveOpacity(item) < 0.01) {
        return;
    }
    const bool text =
        item->inherits("QQuickText") || item->inherits("QQuickTextEdit") || item->inherits("QQuickTextInput");
    if (text && controlEnabled(item) && !item->property("text").toString().isEmpty()) {
        const QRect zone = pixelRect(item, image, dpr);
        if (zone.width() > 2 && zone.height() > 2) {
            const QColor bg = wcag::dominantColor(image, zone);
            const QColor fg = over(colorProperty(item, "color"), bg, effectiveOpacity(item));
            const double ratio = wcag::contrastRatio(fg, bg);
            ++checked;
            if (ratio < wcag::kText) {
                failures << QStringLiteral("%1 : texte %2 sur %3 : %4:1 < 4.5:1 (%5)")
                                .arg(scope, fg.name(), bg.name())
                                .arg(ratio, 0, 'f', 2)
                                .arg(describe(item));
            }
        }
    }
    const QList<QQuickItem*> children = item->childItems();
    for (QQuickItem* child : children) {
        collectTexts(child, image, dpr, failures, scope, checked);
    }
}

// Boîtes ouvertes (Popup, Dialog) de la fenêtre, hors menus et info-bulles.
QList<QObject*> openPopups(Shell& s)
{
    QList<QObject*> result;
    const QList<QObject*> all = s.window->findChildren<QObject*>();
    for (QObject* o : all) {
        if (o->inherits("QQuickPopup") && o->property("opened").toBool() && !o->inherits("QQuickMenu") &&
            !o->inherits("QQuickToolTip")) {
            result << o;
        }
    }
    return result;
}

QQuickItem* popupRoot(QObject* popup)
{
    QQuickItem* content = itemProperty(popup, "contentItem");
    return content && content->parentItem() ? content->parentItem() : content;
}

void closeAll(Shell& s)
{
    for (int round = 0; round < 3; ++round) {
        for (QObject* popup : openPopups(s)) {
            QMetaObject::invokeMethod(popup, "close");
        }
        QTest::qWait(60);
    }
}

// Commandes à boîte propres à ce poste : la boîte est un composant QML (les sélecteurs de fichier natifs, qui
// bloqueraient, la sortie de l'application et la boîte de couleur de Qt, dont le repli QML hors écran est le
// style par défaut de Qt, sont exclus).
QStringList dialogCommands()
{
    actions::CommandRegistry registry;
    QString error;
    QStringList ids;
    if (!registry.loadDeclarations(actions::defaultCommandsJson(), error)) {
        return ids;
    }
#if defined(Q_OS_WIN)
    constexpr auto platform = actions::Platform::Windows;
#elif defined(Q_OS_MACOS)
    constexpr auto platform = actions::Platform::MacOS;
#elif defined(Q_OS_ANDROID)
    constexpr auto platform = actions::Platform::Android;
#else
    constexpr auto platform = actions::Platform::Linux;
#endif
    for (const QString& id : registry.ids()) {
        const actions::CommandInfo* info = registry.find(id);
        if (info && info->opensDialog && registry.isAvailable(id, platform) &&
            info->argument != actions::ArgumentKind::Url && id != QLatin1String("file.quit") && id != QLatin1String("color.dialog")) {
            ids << id;
        }
    }
    return ids;
}

// Ouvre la boîte d'une commande ; vrai si une boîte QML est apparue (les autres restent ouvertes pour l'appelant).
bool openDialog(Shell& s, const QString& id)
{
    closeAll(s);
    if (!run(s, id)) {
        return false;
    }
    return QTest::qWaitFor([&] { return !openPopups(s).isEmpty(); }, 1500);
}

// Contrôles atteints par Tab dans une boîte : visibles, actifs, focalisables au clavier.
void collectFocusable(QQuickItem* item, QList<QQuickItem*>& out)
{
    if (!item || !item->isVisible() || !item->isEnabled()) {
        return;
    }
    if (item->activeFocusOnTab() && item->width() > 0 && item->height() > 0) {
        out << item;
    }
    const QList<QQuickItem*> children = item->childItems();
    for (QQuickItem* child : children) {
        collectFocusable(child, out);
    }
}

int changedBy3(const QImage& before, const QImage& after, const QRect& zone)
{
    int count = 0;
    const QRect area = zone.intersected(before.rect());
    for (int y = area.top(); y <= area.bottom(); ++y) {
        for (int x = area.left(); x <= area.right(); ++x) {
            if (wcag::contrastRatio(before.pixelColor(x, y), after.pixelColor(x, y)) >= wcag::kUi) {
                ++count;
            }
        }
    }
    return count;
}

QQuickItem* findItem(QQuickItem* root, const QString& name)
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

void collectButtons(QQuickItem* item, QList<QQuickItem*>& out)
{
    if (!item || !item->isVisible()) {
        return;
    }
    if (item->inherits("QQuickAbstractButton") && !item->inherits("QQuickMenuItem")) {
        out << item;
    }
    const QList<QQuickItem*> children = item->childItems();
    for (QQuickItem* child : children) {
        collectButtons(child, out);
    }
}

QStringList unnamedButtons(QQuickItem* root, const QString& scope)
{
    QStringList failures;
    QList<QQuickItem*> buttons;
    collectButtons(root, buttons);
    for (QQuickItem* b : std::as_const(buttons)) {
        QAccessibleInterface* iface = QAccessible::queryAccessibleInterface(b);
        if (!iface) {
            failures << QStringLiteral("%1 : aucune interface d'accessibilité (%2)").arg(scope, describe(b));
        } else if (iface->text(QAccessible::Name).trimmed().isEmpty()) {
            failures << QStringLiteral("%1 : bouton sans nom accessible (%2)").arg(scope, describe(b));
        }
    }
    return failures;
}

} // namespace

class TstUiAccessibility : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QVERIFY(m_settings.isValid());
        QStandardPaths::setTestModeEnabled(true);
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings.path());
        // La boîte des mises à jour ne fait aucune requête réseau : elle lit un fichier local.
        const QString newer = QFINDTESTDATA("data/updates/newer.json");
        QVERIFY(!newer.isEmpty());
        qputenv("ACTCRAFT_UPDATES_URL", QUrl::fromLocalFile(newer).toEncoded());
        QAccessible::setActive(true);
        QSettings probe;
        QCOMPARE(probe.format(), QSettings::IniFormat);
        QVERIFY(probe.fileName().startsWith(m_settings.path()));
    }

    void cleanup() { QSettings().remove(QStringLiteral("workstation/touch-ui")); }

    // Chaque texte actif de chaque boîte tient 4,5:1 contre le fond réellement rendu à côté de lui.
    void everyDialogMeetsAa()
    {
        Shell s;
        QVERIFY(start(s));
        const qreal dpr = s.window->effectiveDevicePixelRatio();
        QStringList failures;
        int opened = 0;
        int checked = 0;
        for (const QString& id : dialogCommands()) {
            if (!openDialog(s, id)) {
                continue;
            }
            ++opened;
            QTest::qWait(300); // fin de l'animation d'ouverture
            const QImage image = s.window->grabWindow();
            for (QObject* popup : openPopups(s)) {
                collectTexts(popupRoot(popup), image, dpr, failures, id, checked);
            }
        }
        closeAll(s);
        QVERIFY2(opened >= 8, qPrintable(QStringLiteral("%1 boîtes ouvertes seulement").arg(opened)));
        QVERIFY(checked > 20);
        QVERIFY2(failures.isEmpty(), qPrintable(failures.join(QLatin1Char('\n'))));
    }

    // Tab parcourt chaque contrôle d'une boîte, et l'élément focalisé se voit (30 pixels changent de 3:1).
    void focusVisibleInEveryDialog()
    {
        Shell s;
        QVERIFY(start(s));
        const qreal dpr = s.window->effectiveDevicePixelRatio();
        QStringList failures;
        int controls = 0;
        for (const QString& id : dialogCommands()) {
            if (!openDialog(s, id)) {
                continue;
            }
            QTest::qWait(300);
            QList<QQuickItem*> focusable;
            for (QObject* popup : openPopups(s)) {
                collectFocusable(popupRoot(popup), focusable);
            }
            for (QQuickItem* control : std::as_const(focusable)) {
                QPointer<QQuickItem> guard(control);
                const QImage before = s.window->grabWindow();
                control->forceActiveFocus(Qt::TabFocusReason);
                (void)QTest::qWaitFor([&] { return !guard || guard->hasActiveFocus(); }, 300);
                QTest::qWait(60);
                if (!guard) {
                    continue;
                }
                const QImage after = s.window->grabWindow();
                const QRectF scene = control->mapRectToScene(control->boundingRect()).adjusted(-6, -6, 6, 6);
                const QRect zone = QRectF(scene.topLeft() * dpr, scene.size() * dpr).toAlignedRect();
                ++controls;
                const int changed = changedBy3(before, after, zone);
                if (changed < 30) {
                    failures << QStringLiteral("%1 : focus invisible (%2 pixels, focus reçu %3, visuel %4) sur %5")
                                    .arg(id)
                                    .arg(changed)
                                    .arg(control->hasActiveFocus())
                                    .arg(control->property("visualFocus").toBool())
                                    .arg(describe(control));
                }
            }
        }
        closeAll(s);
        QVERIFY(controls > 20);
        QVERIFY2(failures.isEmpty(), qPrintable(failures.join(QLatin1Char('\n'))));
    }

    // Chaque bouton visible de la fenêtre, de ses boîtes et du tiroir a un nom accessible.
    void everyButtonHasAnAccessibleName()
    {
        Shell s;
        QVERIFY(start(s));
        QStringList failures = unnamedButtons(s.window->contentItem(), QStringLiteral("fenêtre"));
        for (const QString& id : dialogCommands()) {
            if (!openDialog(s, id)) {
                continue;
            }
            QTest::qWait(200);
            for (QObject* popup : openPopups(s)) {
                failures << unnamedButtons(popupRoot(popup), id);
            }
        }
        closeAll(s);
        QVERIFY2(failures.isEmpty(), qPrintable(failures.join(QLatin1Char('\n'))));
    }

    // Mise en page étroite : 800 × 1280, interface tactile, palette et tiroir ouverts.
    void narrowLayoutMeetsAa()
    {
        {
            QSettings seed;
            seed.setValue(QStringLiteral("workstation/touch-ui"), true);
        }
        Shell s;
        QVERIFY(start(s));
        s.window->resize(QSize(800, 1280));
        QVERIFY(QTest::qWaitFor([&] { return s.window->size() == QSize(800, 1280); }, 5000));
        QTest::qWait(200);
        for (const char* name : {"paletteToggle"}) {
            QQuickItem* toggle = findItem(s.window->contentItem(), QString::fromLatin1(name));
            QVERIFY(toggle);
            const QPointF p = toggle->mapToScene(QPointF(toggle->width() / 2, toggle->height() / 2));
            QTest::mouseClick(s.window, Qt::LeftButton, Qt::NoModifier, p.toPoint());
        }
        QQuickItem* browser = findItem(s.window->contentItem(), QStringLiteral("browserPanel"));
        QVERIFY(browser);
        QVERIFY(QMetaObject::invokeMethod(browser, "toggleDrawer"));
        QTRY_VERIFY(browser->isVisible());
        QTest::qWait(400);
        const QImage image = s.window->grabWindow();
        QStringList failures;
        int checked = 0;
        collectTexts(s.window->contentItem(), image, s.window->effectiveDevicePixelRatio(), failures,
                     QStringLiteral("étroit"), checked);
        QVERIFY2(checked > 3, qPrintable(QStringLiteral("%1 textes relevés").arg(checked)));
        failures << unnamedButtons(s.window->contentItem(), QStringLiteral("étroit"));
        QVERIFY2(failures.isEmpty(), qPrintable(failures.join(QLatin1Char('\n'))));
    }

private:
    QTemporaryDir m_settings;
};

QTEST_MAIN(TstUiAccessibility)
#include "tst_ui_accessibility.moc"
