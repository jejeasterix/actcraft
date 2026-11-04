#include <actcraft/render/fixedpageitem.h>
#include <actcraft/ui/pageview.h>

#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QVariantMap>
#include <QtQml/qqmlextensionplugin.h>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

struct Shell
{
    QTemporaryDir profiles; // amendement 3, N1 : détruit après le moteur
    QQmlApplicationEngine engine;
    QQuickWindow* window = nullptr;
    QObject* profile = nullptr;
};

bool start(Shell& s)
{
    s.engine.setInitialProperties({{QStringLiteral("cacheEnabled"), true},
                                   {QStringLiteral("generatedPage"), false},
                                   {QStringLiteral("profileRoot"), s.profiles.path()}});
    s.engine.loadFromModule("ActCraft.Ui", "Main");
    if (s.engine.rootObjects().isEmpty()) {
        return false;
    }
    s.window = qobject_cast<QQuickWindow*>(s.engine.rootObjects().constFirst());
    if (!s.window || !QTest::qWaitForWindowExposed(s.window)) {
        return false;
    }
    s.profile = s.window->findChild<QObject*>(QStringLiteral("appProfile"));
    auto* view = s.window->findChild<ui::PageView*>(QStringLiteral("pageView"));
    auto* fixed = s.window->findChild<render::FixedPageItem*>(QStringLiteral("fixedPage"));
    return s.profile && view && fixed &&
           QTest::qWaitFor([&] { return fixed->isReady() && view->cacheReady(); }, 10000);
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

QQuickItem* item(Shell& s, const char* name)
{
    return findItem(s.window->contentItem(), QString::fromLatin1(name));
}

bool run(Shell& s, const QString& id, const QVariant& argument = QVariant())
{
    QVariant result;
    const bool invoked = QMetaObject::invokeMethod(s.window, "runCommand", Q_RETURN_ARG(QVariant, result),
                                                   Q_ARG(QVariant, QVariant(id)), Q_ARG(QVariant, argument));
    return invoked && result.toBool();
}

QVariantMap entry(Shell& s, const QString& id)
{
    const QVariantList list = s.profile->property("profiles").toVariantList();
    for (const QVariant& v : list) {
        if (v.toMap().value(QStringLiteral("id")).toString() == id) {
            return v.toMap();
        }
    }
    return {};
}

QVariant setting(Shell& s, const char* section, const char* key)
{
    QVariant result;
    QMetaObject::invokeMethod(s.profile, "setting", Q_RETURN_ARG(QVariant, result),
                              Q_ARG(QString, QString::fromLatin1(section)),
                              Q_ARG(QString, QString::fromLatin1(key)), Q_ARG(QVariant, QVariant(false)));
    return result;
}

void click(Shell& s, QQuickItem* target)
{
    const QPointF center = target->mapToScene(QPointF(target->width() / 2, target->height() / 2));
    QTest::mouseClick(s.window, Qt::LeftButton, Qt::NoModifier, center.toPoint());
}

} // namespace

class TstUiProfileeditor : public QObject
{
    Q_OBJECT
    QTemporaryDir m_settingsDir;

private slots:
    void initTestCase()
    {
        QVERIFY(m_settingsDir.isValid());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
    }

    void chooserActivatesAndCloses()
    {
        Shell s;
        QVERIFY(start(s));
        QVERIFY(run(s, QStringLiteral("profile.choose")));
        QQuickItem* chooser = item(s, "profileChooser");
        QVERIFY(chooser);
        QVERIFY(QTest::qWaitFor([&] { return chooser->property("opened").toBool(); }, 5000));
        QQuickItem* row = item(s, "profileRow-en-classe");
        QVERIFY(row);
        click(s, row);
        QTRY_COMPARE(s.profile->property("currentId").toString(), QStringLiteral("en-classe"));
        QTRY_VERIFY(!chooser->property("opened").toBool());
    }

    void saveAsCreatesAUserProfile()
    {
        Shell s;
        QVERIFY(start(s));
        QVERIFY(run(s, QStringLiteral("profile.save-as"), QStringLiteral("mon-profil")));
        const QVariantMap created = entry(s, QStringLiteral("mon-profil"));
        QVERIFY(!created.isEmpty());
        QVERIFY(!created.value(QStringLiteral("builtin")).toBool());
        QCOMPARE(s.profile->property("currentId").toString(), QStringLiteral("mon-profil"));
    }

    void deleteRefusedForBuiltin()
    {
        Shell s;
        QVERIFY(start(s));
        QVERIFY(run(s, QStringLiteral("profile.edit")));
        QQuickItem* button = nullptr;
        QTRY_VERIFY((button = item(s, "deleteButton")) != nullptr);
        QTRY_VERIFY(button->isVisible());
        QVERIFY(!button->isEnabled());
        const QString before = s.profile->property("currentId").toString();
        const int count = s.profile->property("profiles").toList().size();
        run(s, QStringLiteral("profile.delete"));
        QCOMPARE(s.profile->property("currentId").toString(), before);
        QCOMPARE(s.profile->property("profiles").toList().size(), count);
        QVERIFY(!entry(s, before).isEmpty());
        // Un profil de l'utilisateur, lui, se supprime.
        QVERIFY(run(s, QStringLiteral("profile.save-as"), QStringLiteral("mon-profil")));
        QVERIFY(run(s, QStringLiteral("profile.delete")));
        QVERIFY(entry(s, QStringLiteral("mon-profil")).isEmpty());
        QCOMPARE(s.profile->property("currentId").toString(), before);
    }

    void resetRestoresTheBuiltin()
    {
        Shell s;
        QVERIFY(start(s));
        const int original = s.profile->property("iconPixels").toInt();
        const QString id = s.profile->property("currentId").toString();
        s.profile->setProperty("iconPixels", original == 30 ? 28 : 30);
        QVERIFY(QMetaObject::invokeMethod(s.profile, "saveNow"));
        QVERIFY(entry(s, id).value(QStringLiteral("modified")).toBool());
        QVERIFY(run(s, QStringLiteral("profile.reset")));
        QCOMPARE(s.profile->property("iconPixels").toInt(), original);
        QVERIFY(!entry(s, id).value(QStringLiteral("modified")).toBool());
    }

    void formatPageEditsTheProfile()
    {
        Shell s;
        QVERIFY(start(s));
        QVERIFY(run(s, QStringLiteral("profile.edit")));
        QQuickItem* slider = nullptr;
        QTRY_VERIFY((slider = item(s, "iconPixelsSlider")) != nullptr);
        QTRY_VERIFY(slider->isVisible());
        const int target = s.profile->property("iconPixels").toInt() == 28 ? 20 : 28;
        slider->setProperty("value", target);
        QVERIFY(QMetaObject::invokeMethod(slider, "moved"));
        QCOMPARE(s.profile->property("iconPixels").toInt(), target);
        // Corbeille visible et plein écran au démarrage.
        QQuickItem* trash = item(s, "trashVisibleBox");
        QVERIFY(trash);
        const bool trashBefore = s.profile->property("trashVisible").toBool();
        click(s, trash);
        QTRY_COMPARE(s.profile->property("trashVisible").toBool(), !trashBefore);
        QQuickItem* full = item(s, "fullScreenBox");
        QVERIFY(full);
        click(s, full);
        QTRY_VERIFY(setting(s, "format", "fullScreenAtStart").toBool());
    }
};

QTEST_MAIN(TstUiProfileeditor)
#include "tst_ui_profileeditor.moc"
