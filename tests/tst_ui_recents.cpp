// L6-T24 : documents récents du poste et résumé à l'enregistrement. Dossiers temporaires seulement.
#include <actcraft/core/document.h>
#include <actcraft/formats/nativefile.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/documentio.h>
#include <actcraft/ui/preferences.h>
#include <actcraft/ui/recentdocuments.h>

#include <QBuffer>
#include <QFile>
#include <QImage>
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

// Archive d'une page, avec ou sans miniature `thumbnails/page-1.png`.
QUrl archive(const QTemporaryDir& dir, const QString& name, bool withThumbnail)
{
    std::vector<QByteArray> thumbnails;
    if (withThumbnail) {
        QImage image(8, 6, QImage::Format_RGB32);
        image.fill(Qt::red);
        QByteArray png;
        QBuffer buffer(&png);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG");
        thumbnails.push_back(png);
    }
    const core::Document document(std::vector<core::Page>(1));
    const QByteArray bytes = formats::encodeActcraft(document, thumbnails);
    QFile file(dir.filePath(name));
    if (bytes.isEmpty() || !file.open(QIODevice::WriteOnly)) {
        return {};
    }
    file.write(bytes);
    return QUrl::fromLocalFile(file.fileName());
}

QString thumbnailOf(const ui::RecentDocuments& list, int row)
{
    return list.entries().value(row).toMap().value(QStringLiteral("thumbnail")).toString();
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

    // L6-T23 (amendement 11) : la vignette est la page 1 de l'archive, lue hors du fil principal.
    void thumbnailReadOffMainThread()
    {
        QTemporaryDir dir;
        ui::RecentDocuments list;
        list.note(archive(dir, QStringLiteral("a.actcraft"), true));
        QVERIFY(QTest::qWaitFor([&] { return !thumbnailOf(list, 0).isEmpty(); }, 5000));
        QVERIFY(thumbnailOf(list, 0).startsWith(QStringLiteral("data:image/png;base64,")));
        const QImage image =
            QImage::fromData(QByteArray::fromBase64(thumbnailOf(list, 0).mid(22).toLatin1()), "PNG");
        QCOMPARE(image.size(), QSize(8, 6));
        QCOMPARE(list.thumbnailsRead(), 1);
        QVERIFY(list.lastReadOffMainThread());
    }

    void archiveWithoutThumbnailHasNone()
    {
        QTemporaryDir dir;
        ui::RecentDocuments list;
        list.note(archive(dir, QStringLiteral("sans.actcraft"), false));
        list.note(touch(dir, QStringLiteral("illisible.actcraft"))); // pas une archive
        QVERIFY(QTest::qWaitFor([&] { return list.thumbnailsRead() == 2; }, 5000));
        QCOMPARE(list.count(), 2);
        QVERIFY(thumbnailOf(list, 0).isEmpty());
        QVERIFY(thumbnailOf(list, 1).isEmpty());
        // Lues une fois : une nouvelle demande ne relit pas.
        list.note(QUrl::fromLocalFile(dir.filePath(QStringLiteral("sans.actcraft"))));
        QTest::qWait(100);
        QCOMPARE(list.thumbnailsRead(), 2);
    }

    void noReadDuringContact()
    {
        QTemporaryDir dir;
        ui::RecentDocuments list;
        list.setContactActive(true);
        list.note(archive(dir, QStringLiteral("a.actcraft"), true));
        QTest::qWait(300);
        QCOMPARE(list.thumbnailsRead(), 0);
        QVERIFY(thumbnailOf(list, 0).isEmpty());
        list.setContactActive(false); // relâché : la lecture part
        QVERIFY(QTest::qWaitFor([&] { return !thumbnailOf(list, 0).isEmpty(); }, 5000));
        QCOMPARE(list.thumbnailsRead(), 1);
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
        QObject::connect(&controller, &ui::DocumentController::saveFinished, &list,
                         [&](bool ok, const QString&) {
                             if (ok) {
                                 list.note(controller.fileUrl());
                             }
                         });
        QObject::connect(&controller, &ui::DocumentController::loadFinished, &list,
                         [&](bool ok, const QString&) {
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
