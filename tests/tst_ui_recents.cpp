// L6-T24 : documents récents du poste et résumé à l'enregistrement. Dossiers temporaires seulement.
#include <actcraft/core/document.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/documentio.h>
#include <actcraft/ui/preferences.h>
#include <actcraft/ui/recentdocuments.h>

#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

using namespace actcraft;

namespace {

QUrl touch(const QTemporaryDir& dir, const QString& name)
{
    QFile file(dir.filePath(name));
    if (!file.open(QIODevice::WriteOnly)) {
        return {};
    }
    file.write("x");
    return QUrl::fromLocalFile(file.fileName());
}

QStringList names(const ui::RecentDocuments& list)
{
    QStringList out;
    for (const QVariant& entry : list.entries()) {
        out << entry.toMap().value(QStringLiteral("name")).toString();
    }
    return out;
}

} // namespace

class TstUiRecents : public QObject
{
    Q_OBJECT
private slots:
    void openedFileGoesFirst()
    {
        QTemporaryDir dir;
        ui::RecentDocuments list;
        const QUrl a = touch(dir, QStringLiteral("a.actcraft"));
        const QUrl b = touch(dir, QStringLiteral("b.actcraft"));
        list.note(a);
        list.note(b);
        QCOMPARE(names(list), (QStringList{"b.actcraft", "a.actcraft"}));
        list.note(a); // déjà présent : remonte, sans doublon
        QCOMPARE(names(list), (QStringList{"a.actcraft", "b.actcraft"}));
        QCOMPARE(list.count(), 2);
    }

    void boundedToTen()
    {
        QTemporaryDir dir;
        ui::RecentDocuments list;
        for (int i = 0; i < 12; ++i) {
            list.note(touch(dir, QStringLiteral("f%1.actcraft").arg(i)));
        }
        QCOMPARE(ui::RecentDocuments::kMax, 10);
        QCOMPARE(list.count(), 10);
        QCOMPARE(names(list).first(), QStringLiteral("f11.actcraft"));
        QCOMPARE(names(list).last(), QStringLiteral("f2.actcraft"));
    }

    void missingFilesHidden()
    {
        QTemporaryDir dir;
        ui::RecentDocuments list;
        const QUrl kept = touch(dir, QStringLiteral("kept.actcraft"));
        list.note(kept);
        list.note(QUrl::fromLocalFile(dir.filePath(QStringLiteral("absent.actcraft"))));
        QCOMPARE(names(list), (QStringList{"kept.actcraft"}));
        QCOMPARE(list.count(), 1);
    }

    void contentUrlsKept()
    {
        ui::RecentDocuments list;
        const QUrl url(QStringLiteral("content://com.example.documents/document/42"));
        list.note(url);
        QCOMPARE(list.count(), 1);
        QCOMPARE(list.entries().first().toMap().value(QStringLiteral("url")).toUrl(), url);
    }

    void clearEmptiesTheList()
    {
        QTemporaryDir dir;
        ui::Preferences prefs(dir.filePath(QStringLiteral("prefs.ini")), nullptr);
        ui::RecentDocuments list;
        list.setPreferences(&prefs);
        list.note(touch(dir, QStringLiteral("a.actcraft")));
        QCOMPARE(list.count(), 1);
        // Gardée dans les préférences : une autre liste du même poste la relit.
        ui::RecentDocuments other;
        other.setPreferences(&prefs);
        QCOMPARE(other.count(), 1);
        list.clear();
        QCOMPARE(list.count(), 0);
        QCOMPARE(other.count(), 0);
        QVERIFY(!prefs.value(QString::fromLatin1(ui::Preferences::kRecent)).isValid());
    }

    void summaryCountsPagesAndObjects()
    {
        QTemporaryDir dir;
        core::Document document(std::vector<core::Page>(2));
        for (int i = 0; i < 3; ++i) {
            core::ImageObject shape;
            shape.bounds = core::RectF{10.0 * i, 10, 50, 50};
            core::PageObject object = shape;
            object.common.id = document.allocateId();
            document.page(static_cast<std::size_t>(i % 2)).objects.push_back(object);
        }
        ui::DocumentController controller;
        const QUrl url = QUrl::fromLocalFile(dir.filePath(QStringLiteral("deux.actcraft")));
        controller.replaceDocument(document, url, QStringLiteral("deux"));
        QVERIFY(ui::saveDocument(controller.document(), url).ok);
        const ui::RecentDocuments list;
        const QVariantMap summary = list.summary(&controller, url);
        QCOMPARE(summary.value(QStringLiteral("name")).toString(), QStringLiteral("deux.actcraft"));
        QCOMPARE(summary.value(QStringLiteral("pages")).toInt(), 2);
        QCOMPARE(summary.value(QStringLiteral("objects")).toInt(), 3);
        QCOMPARE(summary.value(QStringLiteral("resources")).toInt(), 0);
        QVERIFY(summary.value(QStringLiteral("bytes")).toLongLong() > 0);
    }

    // Comme FileActions.qml : la fin d'un enregistrement ou d'une ouverture note l'URL du contrôleur.
    void noteFollowsSaveAndLoad()
    {
        QTemporaryDir dir;
        ui::DocumentController controller;
        ui::RecentDocuments list;
        QObject::connect(&controller, &ui::DocumentController::saveFinished, &list, [&](bool ok, const QString&) {
            if (ok) {
                list.note(controller.fileUrl());
            }
        });
        QObject::connect(&controller, &ui::DocumentController::loadFinished, &list, [&](bool ok, const QString&) {
            if (ok) {
                list.note(controller.fileUrl());
            }
        });
        const QUrl first = QUrl::fromLocalFile(dir.filePath(QStringLiteral("premier.actcraft")));
        const QUrl second = QUrl::fromLocalFile(dir.filePath(QStringLiteral("second.actcraft")));
        QVERIFY(ui::saveDocument(core::Document(), second).ok);
        controller.newDocument(1024, 768);
        QSignalSpy saved(&controller, &ui::DocumentController::saveFinished);
        controller.saveToUrl(first);
        QTRY_COMPARE_WITH_TIMEOUT(saved.count(), 1, 10000);
        QSignalSpy loaded(&controller, &ui::DocumentController::loadFinished);
        controller.openUrl(second);
        QTRY_COMPARE_WITH_TIMEOUT(loaded.count(), 1, 10000);
        QCOMPARE(names(list), (QStringList{"second.actcraft", "premier.actcraft"}));
    }
};

QTEST_MAIN(TstUiRecents)
#include "tst_ui_recents.moc"
