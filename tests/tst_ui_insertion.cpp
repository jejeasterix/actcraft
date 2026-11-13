#include "support/testserver.h"

#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/core/pageedits.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/documenttabs.h>
#include <actcraft/ui/insertion.h>
#include <actcraft/ui/mediainsert.h>
#include <actcraft/ui/webframeitem.h>

#include <QBuffer>
#include <QFile>
#include <QImage>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <QVariantList>

#include <memory>
#include <string>
#include <variant>

using namespace actcraft;

namespace {

QString writePng(const QTemporaryDir& dir, const QString& name, int width, int height)
{
    QImage image(width, height, QImage::Format_RGB32);
    image.fill(Qt::red);
    const QString path = dir.filePath(name);
    image.save(path, "PNG");
    return path;
}

const core::Page& firstPage(const ui::DocumentController& controller)
{
    return controller.document().page(0);
}

template <typename T> const T* lastAs(const ui::DocumentController& controller)
{
    const auto& objects = firstPage(controller).objects;
    return objects.empty() ? nullptr : std::get_if<T>(&core::body(objects.back()));
}

QByteArray pngBytes(int width, int height)
{
    QImage image(width, height, QImage::Format_RGB32);
    image.fill(Qt::blue);
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return bytes;
}

QByteArray jpegBytes(int width, int height)
{
    QImage image(width, height, QImage::Format_RGB32);
    image.fill(Qt::blue);
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "JPEG");
    return bytes;
}

} // namespace

class TestInsertion : public QObject
{
    Q_OBJECT
private slots:
    void imageFitsIn80PercentCentered()
    {
        QTemporaryDir dir;
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        insertion.insertImage(QUrl::fromLocalFile(writePng(dir, "grande.png", 2000, 1000)));
        QTRY_COMPARE(inserted.count(), 1);
        const auto* image = lastAs<core::ImageObject>(controller);
        QVERIFY(image != nullptr);
        QCOMPARE(image->bounds.width, 819.2);
        QCOMPARE(image->bounds.height, 409.6);
        QCOMPARE(image->bounds.center().x, 512.0);
        QCOMPARE(image->bounds.center().y, 384.0);
        QCOMPARE(image->sourceWidth, 2000);
        QCOMPARE(image->sourceHeight, 1000);
        QCOMPARE(firstPage(controller).objects.back().common.layer, core::Layer::Middle);
        // Ressource telle quelle (octets d'origine), jamais réencodée.
        const core::Resource* resource = controller.document().resource(image->resource);
        QVERIFY(resource != nullptr);
        QFile file(dir.filePath("grande.png"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(resource->data, file.readAll());
        QCOMPARE(resource->extension, std::string("png"));
    }

    // M9 : Ctrl+Tab pendant le décodage ne verse pas l'image dans l'autre onglet.
    void decodedImageNeverLandsInAnotherTab()
    {
        QTemporaryDir dir;
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::DocumentTabs tabs;
        tabs.setDocument(&controller);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        QSignalSpy failed(&insertion, &ui::Insertion::failed);
        insertion.insertImage(QUrl::fromLocalFile(writePng(dir, "a.png", 100, 100)));
        tabs.newTab(); // avant la fin du décodage (la boucle d'événements n'a pas tourné)
        QCOMPARE(tabs.currentIndex(), 1);
        QTRY_VERIFY(!insertion.isBusy());
        QCOMPARE(inserted.count(), 0);
        QCOMPARE(failed.count(), 1);                    // abandon annoncé
        QVERIFY(firstPage(controller).objects.empty()); // l'onglet 1 n'a rien reçu
        tabs.activate(0);
        QVERIFY(firstPage(controller).objects.empty()); // ni l'onglet 0 : abandon
    }

    // M9 : dans le même onglet, l'image va sur la page où l'insertion a été demandée.
    void decodedImageStaysOnItsOriginPage()
    {
        QTemporaryDir dir;
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        insertion.insertImage(QUrl::fromLocalFile(writePng(dir, "a.png", 100, 100)));
        controller.nextPage(); // page 2 (neuve) affichée pendant le décodage
        QCOMPARE(controller.currentIndex(), 1);
        QTRY_COMPARE(inserted.count(), 1);
        QCOMPARE(controller.document().page(0).objects.size(), std::size_t(1));
        QVERIFY(controller.document().page(1).objects.empty());
    }

    // L5-C23 (mineur D, H5-32) : la page d'origine est retirée pendant le décodage : l'image n'est versée sur
    // aucune autre page (celle qui prend son indice) ; abandon annoncé.
    void imageIsDroppedWhenItsPageDisappears()
    {
        QTemporaryDir dir;
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        controller.nextPage();
        controller.pageBack(); // page 1 affichée, page 2 derrière
        QCOMPARE(controller.currentIndex(), 0);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        QSignalSpy failed(&insertion, &ui::Insertion::failed);
        insertion.insertImage(QUrl::fromLocalFile(writePng(dir, "a.png", 100, 100)));
        controller.execute(std::make_unique<core::RemovePagesCommand>(std::vector<std::size_t>{0}));
        QCOMPARE(controller.document().pageCount(), std::size_t(1));
        QTRY_VERIFY(!insertion.isBusy());
        QCOMPARE(inserted.count(), 0);
        QCOMPARE(failed.count(), 1);
        QVERIFY(controller.document().page(0).objects.empty());
    }

    // L5-C23 (mineur D) : une page insérée avant l'origine pendant le décodage décale son indice ; l'image
    // reste sur sa page.
    void imageFollowsItsPageWhenOneIsInsertedBefore()
    {
        QTemporaryDir dir;
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        insertion.insertImage(QUrl::fromLocalFile(writePng(dir, "a.png", 100, 100)));
        controller.execute(
            std::make_unique<core::InsertPagesCommand>(0, std::vector<core::Page>{core::Page{}}));
        QCOMPARE(controller.document().pageCount(), std::size_t(2));
        QTRY_COMPARE(inserted.count(), 1);
        QVERIFY(controller.document().page(0).objects.empty());
        QCOMPARE(controller.document().page(1).objects.size(), std::size_t(1));
    }

    void smallImageKeepsItsSize()
    {
        QTemporaryDir dir;
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        insertion.insertImage(QUrl::fromLocalFile(writePng(dir, "petite.png", 100, 50)));
        QTRY_COMPARE(inserted.count(), 1);
        const auto* image = lastAs<core::ImageObject>(controller);
        QVERIFY(image != nullptr);
        QCOMPARE(image->bounds.width, 100.0);
        QCOMPARE(image->bounds.height, 50.0);
        QCOMPARE(image->bounds.center().x, 512.0);
    }

    void imageNamedAfterFile()
    {
        QTemporaryDir dir;
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        insertion.insertImage(QUrl::fromLocalFile(writePng(dir, "ma photo.png", 40, 40)));
        QTRY_COMPARE(inserted.count(), 1);
        QCOMPARE(firstPage(controller).objects.back().common.name, std::string("ma photo"));
    }

    void decodeOffMainThread()
    {
        QTemporaryDir dir;
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        insertion.insertImage(QUrl::fromLocalFile(writePng(dir, "a.png", 300, 300)));
        QVERIFY(insertion.isBusy()); // rien n'a été décodé sur le fil principal avant le retour
        QCOMPARE(firstPage(controller).objects.size(), std::size_t{0});
        QTRY_COMPARE(inserted.count(), 1);
        QVERIFY(insertion.lastDecodeOffMainThread());
        QVERIFY(!insertion.isBusy());
    }

    void mediaInsertedAtCenter()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("son.mp3");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("ID3-pas-vraiment-du-son");
        file.close();
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        insertion.insertMedia(QUrl::fromLocalFile(path));
        QTRY_COMPARE(inserted.count(), 1);
        const auto* media = lastAs<core::Media>(controller);
        QVERIFY(media != nullptr);
        QCOMPARE(media->bounds.center().x, 512.0);
        QCOMPARE(media->bounds.center().y, 384.0);
        QCOMPARE(media->fileName, std::string("son.mp3"));
        const core::Resource* resource = controller.document().resource(media->media);
        QVERIFY(resource != nullptr);
        QCOMPARE(resource->extension, std::string("mp3"));
        QCOMPARE(firstPage(controller).objects.back().common.layer, core::Layer::Middle);
    }

    // Un son déposé sur la page passe par MediaInsert : action « jouer un son » et espace réservé (ressource
    // d'image), au point de dépôt. Sous l'ancien chemin, Insertion posait un core::Media nu : ni action, ni
    // espace réservé.
    void droppedSoundGoesThroughMediaInsert()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("cloche.wav");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("RIFF-pas-vraiment-du-son");
        file.close();
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::MediaInsert mediaInsert;
        mediaInsert.setDocument(&controller);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        insertion.setProperty("mediaInsert", QVariant::fromValue(&mediaInsert));
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        QVERIFY(insertion.acceptDrop({QUrl::fromLocalFile(path)}, QString(), {300, 200}));
        QTRY_COMPARE(inserted.count(), 1);
        const auto* media = lastAs<core::Media>(controller);
        QVERIFY(media != nullptr);
        const core::PageObject& object = firstPage(controller).objects.back();
        QCOMPARE(object.common.actions.size(), std::size_t{1});
        QCOMPARE(object.common.actions[0].catalogId, std::string("document.play-sound"));
        QVERIFY2(controller.document().resource(media->placeholder) != nullptr,
                 "le média déposé doit porter un espace réservé");
        QCOMPARE(media->bounds.center().x, 300.0);
        QCOMPARE(media->bounds.center().y, 200.0);
    }

    // Le verrou de classe « insert.media » vaut aussi pour le dépôt : seul MediaInsert le connaît.
    void droppedSoundIsRefusedWhileLocked()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("verrou.wav");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("RIFF-pas-vraiment-du-son");
        file.close();
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Commands commands;
        commands.setLocked(true);
        ui::MediaInsert mediaInsert;
        mediaInsert.setDocument(&controller);
        mediaInsert.setCommands(&commands);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        insertion.setProperty("mediaInsert", QVariant::fromValue(&mediaInsert));
        QSignalSpy failed(&insertion, &ui::Insertion::failed);
        insertion.acceptDrop({QUrl::fromLocalFile(path)}, QString(), {300, 200});
        QTest::qWait(150);
        QCOMPARE(failed.count(), 1);
        QVERIFY(!controller.canUndo());
        QCOMPARE(firstPage(controller).objects.size(), std::size_t{0});
    }

    void droppedTextBecomesText()
    {
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QVERIFY(insertion.acceptDrop({}, "abc", {100, 100}));
        const auto* text = lastAs<core::Text>(controller);
        QVERIFY(text != nullptr);
        QCOMPARE(text->bounds.x, 100.0);
        QCOMPARE(text->bounds.y, 100.0);
        QVERIFY(text->html.find("abc") != std::string::npos);
        QVERIFY(!insertion.acceptDrop({}, QString(), {1, 1}));
    }

    void droppedHtmlLoadsNothing()
    {
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        const std::size_t resourcesBefore = controller.document().resources().size();
        QVERIFY(insertion.acceptDrop({}, "<img src=\"http://exemple.invalide/a.png\">", {10, 10}));
        QCOMPARE(firstPage(controller).objects.size(), std::size_t{1});
        QVERIFY(lastAs<core::Text>(controller) != nullptr);
        QVERIFY(lastAs<core::ImageObject>(controller) == nullptr);
        QCOMPARE(controller.document().resources().size(), resourcesBefore);
        QVERIFY(!insertion.isBusy()); // aucune lecture lancée
        const auto* text = lastAs<core::Text>(controller);
        QVERIFY(text->html.find("&lt;img") != std::string::npos); // du texte, pas une balise
    }

    void droppedFileIsInserted()
    {
        QTemporaryDir dir;
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        QVariantList urls;
        urls << QUrl::fromLocalFile(writePng(dir, "d.png", 100, 100));
        QVERIFY(insertion.acceptDrop(urls, QString(), {300, 200}));
        QTRY_COMPARE(inserted.count(), 1);
        const auto* image = lastAs<core::ImageObject>(controller);
        QVERIFY(image != nullptr);
        QCOMPARE(image->bounds.center().x, 300.0);
        QCOMPARE(image->bounds.center().y, 200.0);
    }

    void unreadableFileFails()
    {
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy failed(&insertion, &ui::Insertion::failed);
        insertion.insertImage(QUrl::fromLocalFile("/chemin/inexistant/x.png"));
        QTRY_COMPARE(failed.count(), 1);
        QCOMPARE(firstPage(controller).objects.size(), std::size_t{0});
    }

    void insertionIsOneStep()
    {
        QTemporaryDir dir;
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        insertion.insertImage(QUrl::fromLocalFile(writePng(dir, "u.png", 500, 500)));
        QTRY_COMPARE(inserted.count(), 1);
        QVERIFY(controller.canUndo());
        controller.undo();
        QCOMPARE(firstPage(controller).objects.size(), std::size_t{0});
        QVERIFY(!controller.canUndo()); // une seule étape
        for (const core::PageObject& object : firstPage(controller).objects)
            QVERIFY(std::get_if<core::ImageObject>(&core::body(object)) == nullptr);
        controller.redo();
        QVERIFY(lastAs<core::ImageObject>(controller) != nullptr);
        QVERIFY(controller.document().resource(lastAs<core::ImageObject>(controller)->resource) != nullptr);
    }

    // S1, S2 : image http(s) téléchargée, extension lue dans les octets (le type annoncé ment), une étape.
    void webDropDownloadsAnImage()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/image-sans-extension"), pngBytes(120, 60)); // annoncé text/html
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        QVERIFY(insertion.acceptWebDrop({server.url(QStringLiteral("/image-sans-extension"))}, {}, {},
                                        {300, 200}));
        QTRY_COMPARE_WITH_TIMEOUT(inserted.count(), 1, 10000);
        const auto* image = lastAs<core::ImageObject>(controller);
        QVERIFY(image != nullptr);
        QCOMPARE(image->bounds.center().x, 300.0);
        QCOMPARE(controller.document().resource(image->resource)->extension, std::string("png"));
        QVERIFY(insertion.lastDecodeOffMainThread());
        controller.undo();
        QCOMPARE(firstPage(controller).objects.size(), std::size_t{0});
        QVERIFY(!controller.canUndo());
    }

    // n° 24 : le format vient des octets, quel que soit le nom de l'adresse. Il se lit avant le décodage :
    // QImageReader::format() peut être vide une fois l'image lue (cas du GIF).
    void webDropReadsTheFormatFromTheBytes_data()
    {
        QTest::addColumn<QByteArray>("bytes");
        QTest::addColumn<std::string>("extension");
        // GIF de 1 × 1 : Qt n'écrit pas ce format.
        QTest::newRow("gif") << QByteArray::fromBase64(
                                    "R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7")
                             << std::string("gif");
        QTest::newRow("jpeg") << jpegBytes(16, 16) << std::string("jpg");
    }

    void webDropReadsTheFormatFromTheBytes()
    {
        QFETCH(QByteArray, bytes);
        QFETCH(std::string, extension);
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/fausse.png"), bytes);
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        QVERIFY(insertion.acceptWebDrop({server.url(QStringLiteral("/fausse.png"))}, {}, {}, {100, 100}));
        QTRY_COMPARE_WITH_TIMEOUT(inserted.count(), 1, 10000);
        const auto* image = lastAs<core::ImageObject>(controller);
        QVERIFY(image != nullptr);
        QCOMPARE(controller.document().resource(image->resource)->extension, extension);
    }

    void webDropOfLocalFileUsesTheFilePath()
    {
        QTemporaryDir dir;
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        QVERIFY(insertion.acceptWebDrop({QUrl::fromLocalFile(writePng(dir, "t.png", 40, 40))}, {}, {},
                                        {100, 100}));
        QTRY_COMPARE(inserted.count(), 1);
        QVERIFY(lastAs<core::ImageObject>(controller) != nullptr);
    }

    void webDropOfDataUrlDecodesOnTheSpot()
    {
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        const QUrl url(QStringLiteral("data:image/png;base64,") +
                       QString::fromLatin1(pngBytes(10, 10).toBase64()));
        QVERIFY(insertion.acceptWebDrop({url}, {}, {}, {50, 50}));
        QTRY_COMPARE(inserted.count(), 1);
        QVERIFY(lastAs<core::ImageObject>(controller) != nullptr);
    }

    void webDropOfNonImageDataUrlFails()
    {
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy failed(&insertion, &ui::Insertion::failed);
        QVERIFY(insertion.acceptWebDrop({QUrl(QStringLiteral("data:text/plain;base64,aGVsbG8="))}, {}, {},
                                        {1, 1}));
        QCOMPARE(failed.count(), 1);
        QCOMPARE(failed.first().first().toString(), QStringLiteral("Adresse non prise en charge"));
        QCOMPARE(firstPage(controller).objects.size(), std::size_t{0});
    }

    void webDropOfPercentEncodedDataUrlDecodes()
    {
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        const QUrl url =
            QUrl::fromEncoded(QByteArray("data:image/png,") + pngBytes(10, 10).toPercentEncoding());
        QVERIFY(insertion.acceptWebDrop({url}, {}, {}, {50, 50}));
        QTRY_COMPARE(inserted.count(), 1);
        QVERIFY(lastAs<core::ImageObject>(controller) != nullptr);
    }

    // Revue T23 : la ressource insérée porte les octets décodés, pas le texte codé de l'adresse.
    void webDropOfDataUrlInsertsTheDecodedBytes_data()
    {
        QTest::addColumn<bool>("base64");
        QTest::newRow("base64") << true;
        QTest::newRow("pourcent") << false;
    }

    void webDropOfDataUrlInsertsTheDecodedBytes()
    {
        QFETCH(bool, base64);
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        const QByteArray png = pngBytes(10, 10);
        const QUrl url = base64 ? QUrl::fromEncoded(QByteArray("data:image/png;base64,") + png.toBase64())
                                : QUrl::fromEncoded(QByteArray("data:image/png,") + png.toPercentEncoding());
        QVERIFY(insertion.acceptWebDrop({url}, {}, {}, {50, 50}));
        QTRY_COMPARE(inserted.count(), 1);
        const auto* image = lastAs<core::ImageObject>(controller);
        QVERIFY(image != nullptr);
        const auto* resource = controller.document().resource(image->resource);
        QVERIFY(resource != nullptr);
        QCOMPARE(resource->data, png);
    }

    // Au-delà de la borne : le message des 25 Mo, pas « Adresse non prise en charge ».
    void webDropOfOversizedDataUrlReportsTheSize()
    {
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy failed(&insertion, &ui::Insertion::failed);
        const QByteArray big(static_cast<qsizetype>(26) * 1024 * 1024, 'a');
        const QUrl url = QUrl::fromEncoded(QByteArray("data:image/png;base64,") + big.toBase64());
        QVERIFY(insertion.acceptWebDrop({url}, {}, {}, {1, 1}));
        QCOMPARE(failed.count(), 1);
        QVERIFY(failed.first().first().toString().contains(QStringLiteral("25 Mo")));
        QCOMPARE(firstPage(controller).objects.size(), std::size_t{0});
    }

    // La taille est bornée avant le décodage : un contenu énorme est refusé sans être décodé (ici, des
    // octets hors alphabet, qui donneraient un résultat vide une fois décodés).
    void webDropOfHugeDataUrlIsRefusedBeforeDecoding()
    {
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy failed(&insertion, &ui::Insertion::failed);
        const QUrl url = QUrl::fromEncoded(QByteArray("data:image/png;base64,") +
                                           QByteArray(static_cast<qsizetype>(40) * 1024 * 1024, '*'));
        QVERIFY(insertion.acceptWebDrop({url}, {}, {}, {1, 1}));
        QCOMPARE(failed.count(), 1);
        QVERIFY(failed.first().first().toString().contains(QStringLiteral("25 Mo")));
    }

    // Cadrage n° 26 : un lien mailto: ou ftp: déposé garde son texte.
    void webDropOfOtherSchemeFallsBackToText_data()
    {
        QTest::addColumn<QString>("address");
        QTest::newRow("mailto") << QStringLiteral("mailto:a@b.c");
        QTest::newRow("ftp") << QStringLiteral("ftp://a.example/x");
    }

    void webDropOfOtherSchemeFallsBackToText()
    {
        QFETCH(QString, address);
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        QVERIFY(insertion.acceptWebDrop({QUrl(address)}, QStringLiteral("Contact"), {}, {100, 100}));
        QTRY_COMPARE(inserted.count(), 1);
        const auto* text = lastAs<core::Text>(controller);
        QVERIFY(text != nullptr);
        QVERIFY(text->html.find("Contact") != std::string::npos);
    }

    void webDropOfBlobFails()
    {
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy failed(&insertion, &ui::Insertion::failed);
        QVERIFY(insertion.acceptWebDrop({QUrl(QStringLiteral("blob:https://x/abc"))}, {}, {}, {1, 1}));
        QCOMPARE(failed.count(), 1);
        QCOMPARE(firstPage(controller).objects.size(), std::size_t{0});
    }

    // E9 : page qui n'est pas une image → objet texte, à la même place, après la réponse.
    void webDropOfNonImageFallsBackToText()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/page"), "<p>pas une image</p>");
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        QVERIFY(insertion.acceptWebDrop({server.url(QStringLiteral("/page"))},
                                        QStringLiteral("Titre du lien"), {}, {200, 150}));
        QTRY_COMPARE_WITH_TIMEOUT(inserted.count(), 1, 10000);
        const auto* text = lastAs<core::Text>(controller);
        QVERIFY(text != nullptr);
        QVERIFY(text->html.find("Titre du lien") != std::string::npos);
        QCOMPARE(text->bounds.x, 200.0);
    }

    // OA27 : une vidéo glissée depuis le cadre web (lien direct) est téléchargée dans les ressources du
    // document et posée en objet vidéo au point du dépôt, comme un fichier local : action, espace réservé,
    // une étape.
    void webDropOfAVideoLinkBecomesAMedia()
    {
        TestServer server;
        QVERIFY(server.listening());
        // Le type annoncé peut être générique : l'extension de l'adresse suffit.
        server.rawResponse(QStringLiteral("/clip.mp4"),
                           "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\n"
                           "Content-Length: 20\r\nConnection: close\r\n\r\npas-vraiment-du-h264");
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        QList<bool> selects; // lu pendant le signal : il redevient vrai juste après
        connect(&insertion, &ui::Insertion::inserted, this,
                [&] { selects.append(insertion.lastInsertionSelects()); });
        QVERIFY(insertion.acceptWebDrop({server.url(QStringLiteral("/clip.mp4"))}, {}, {}, {300, 200}));
        QTRY_COMPARE_WITH_TIMEOUT(inserted.count(), 1, 10000);
        const auto* media = lastAs<core::Media>(controller);
        QVERIFY(media != nullptr);
        QCOMPARE(media->fileName, std::string("clip.mp4"));
        QCOMPARE(media->bounds.center().x, 300.0);
        QCOMPARE(media->bounds.center().y, 200.0);
        const core::Resource* resource = controller.document().resource(media->media);
        QVERIFY(resource != nullptr);
        QCOMPARE(resource->extension, std::string("mp4"));
        QCOMPARE(resource->data, QByteArray("pas-vraiment-du-h264"));
        QVERIFY(controller.document().resource(media->placeholder) != nullptr);
        QVERIFY(!insertion.isBusy());
        // Posée après une attente réseau : l'appelant ne change pas d'outil et ne sélectionne rien.
        QCOMPARE(selects, QList<bool>{false});
        controller.undo();
        QCOMPARE(firstPage(controller).objects.size(), std::size_t{0});
        QVERIFY(!controller.canUndo());
    }

    // Un lien « .mp4 » qui répond par une page (connexion exigée) n'est pas une vidéo : message, rien de
    // posé.
    void webDropOfAVideoLinkAnsweringAPageFails()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/cours.mp4"), "<html>Connexion</html>"); // annoncé text/html
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy failed(&insertion, &ui::Insertion::failed);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        QVERIFY(insertion.acceptWebDrop({server.url(QStringLiteral("/cours.mp4"))}, {}, {}, {1, 1}));
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 10000);
        QVERIFY(failed.at(0).at(0).toString().contains(QStringLiteral("Vidéo")));
        QCOMPARE(inserted.count(), 0);
        QVERIFY(!insertion.isBusy());
        QCOMPARE(firstPage(controller).objects.size(), std::size_t{0});
    }

    void webDropOfAVideoByItsMimeType()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.rawResponse(QStringLiteral("/lecteur"),
                           "HTTP/1.1 200 OK\r\nContent-Type: video/webm\r\nContent-Length: 4\r\n"
                           "Connection: close\r\n\r\nWEBM");
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        QVERIFY(insertion.acceptWebDrop({server.url(QStringLiteral("/lecteur"))}, {}, {}, {100, 100}));
        QTRY_COMPARE_WITH_TIMEOUT(inserted.count(), 1, 10000);
        const auto* media = lastAs<core::Media>(controller);
        QVERIFY(media != nullptr);
        QCOMPARE(controller.document().resource(media->media)->extension, std::string("webm"));
    }

    void webDropOfAVideoElementUsesItsSource()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.rawResponse(QStringLiteral("/v/film.m4v?x=1"),
                           "HTTP/1.1 200 OK\r\nContent-Type: video/x-m4v\r\nContent-Length: 4\r\n"
                           "Connection: close\r\n\r\nm4v!");
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        QVERIFY(insertion.acceptWebDrop({server.url(QStringLiteral("/v/film.m4v?x=1"))}, {},
                                        QStringLiteral("<video src=\"x\"></video>"), {10, 10}));
        QTRY_COMPARE_WITH_TIMEOUT(inserted.count(), 1, 10000);
        QCOMPARE(controller.document().resource(lastAs<core::Media>(controller)->media)->extension,
                 std::string("m4v"));
    }

    // Un échec laisse la page intacte et dit pourquoi.
    void webDropOfAVideoThatFailsLeavesThePageIntact()
    {
        TestServer server;
        QVERIFY(server.listening());
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy failed(&insertion, &ui::Insertion::failed);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        QVERIFY(insertion.acceptWebDrop({server.url(QStringLiteral("/absente.mp4"))}, QStringLiteral("x"), {},
                                        {1, 1}));
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 10000);
        QVERIFY(failed.at(0).at(0).toString().contains(QStringLiteral("Vidéo")));
        QVERIFY(failed.at(0).at(0).toString().contains(QStringLiteral("404")));
        QCOMPARE(inserted.count(), 0);
        QVERIFY(!insertion.isBusy());
        QCOMPARE(firstPage(controller).objects.size(), std::size_t{0});
        QVERIFY(!controller.canUndo());
    }

    // Le téléchargement dure : l'utilisateur change de page, la vidéo n'atterrit pas ailleurs.
    void webDropOfAVideoNeverLandsInAnotherTab()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.rawResponse(QStringLiteral("/lente.mp4"),
                           "HTTP/1.1 200 OK\r\nContent-Type: video/mp4\r\nContent-Length: 3\r\n"
                           "Connection: close\r\n\r\nmp4");
        server.hold(QStringLiteral("/lente.mp4"));
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::DocumentTabs tabs;
        tabs.setDocument(&controller);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy failed(&insertion, &ui::Insertion::failed);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        QVERIFY(insertion.acceptWebDrop({server.url(QStringLiteral("/lente.mp4"))}, {}, {}, {10, 10}));
        QTRY_COMPARE_WITH_TIMEOUT(server.requests(), 1, 10000);
        tabs.newTab();
        server.release();
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 10000);
        QCOMPARE(inserted.count(), 0);
        QVERIFY(firstPage(controller).objects.empty());
        tabs.activate(0);
        QVERIFY(firstPage(controller).objects.empty());
    }

    // Une adresse qui n'est pas une vidéo garde son comportement : le texte du lien, ici.
    void webDropOfAPageNamedLikeAVideoFolderKeepsTheTextFallback()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/mp4/accueil"), "<p>page</p>");
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        QVERIFY(insertion.acceptWebDrop({server.url(QStringLiteral("/mp4/accueil"))},
                                        QStringLiteral("Accueil"), {}, {5, 5}));
        QTRY_COMPARE_WITH_TIMEOUT(inserted.count(), 1, 10000);
        QVERIFY(lastAs<core::Text>(controller) != nullptr);
    }

    // S2, H14 : réseau en échec : bandeau, aucun objet.
    void webDropOnMissingPageFails()
    {
        TestServer server;
        QVERIFY(server.listening());
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy failed(&insertion, &ui::Insertion::failed);
        QVERIFY(insertion.acceptWebDrop({server.url(QStringLiteral("/absent"))}, QStringLiteral("x"), {},
                                        {1, 1}));
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 10000);
        QVERIFY(!insertion.isBusy());
        QCOMPARE(firstPage(controller).objects.size(), std::size_t{0});
    }

    // S2, n° 25 : plus de 25 Mo : message, document inchangé.
    void webDropOfOversizeFails()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/gros.png"), QByteArray(int(ui::WebImageFetch::kMaxBytes) + 1024, 'x'));
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy failed(&insertion, &ui::Insertion::failed);
        QVERIFY(insertion.acceptWebDrop({server.url(QStringLiteral("/gros.png"))}, QStringLiteral("x"), {},
                                        {1, 1}));
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 30000);
        QVERIFY(failed.at(0).at(0).toString().contains(QStringLiteral("25 Mo")));
        QCOMPARE(firstPage(controller).objects.size(), std::size_t{0});
    }

    // S2, n° 27 : changer d'onglet pendant le téléchargement.
    void downloadNeverLandsInAnotherTab()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.hold(QStringLiteral("/lente"));
        server.page(QStringLiteral("/lente"), pngBytes(20, 20));
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::DocumentTabs tabs;
        tabs.setDocument(&controller);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        QSignalSpy failed(&insertion, &ui::Insertion::failed);
        QVERIFY(insertion.acceptWebDrop({server.url(QStringLiteral("/lente"))}, {}, {}, {10, 10}));
        QTRY_COMPARE(server.requests(), 1);
        tabs.newTab();
        server.release();
        QTRY_VERIFY_WITH_TIMEOUT(!insertion.isBusy(), 10000);
        QCOMPARE(inserted.count(), 0);
        QCOMPARE(failed.count(), 1);
        QVERIFY(firstPage(controller).objects.empty());
        tabs.activate(0);
        QVERIFY(firstPage(controller).objects.empty());
    }

    // n° 27 : dans le même onglet, l'image va sur la page du dépôt. Une origine prise à l'arrivée, dans
    // start(), la verserait sur la page affichée à ce moment : la génération n'a pas changé.
    void downloadStaysOnItsOriginPage()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.hold(QStringLiteral("/lente"));
        server.page(QStringLiteral("/lente"), pngBytes(20, 20));
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        QVERIFY(insertion.acceptWebDrop({server.url(QStringLiteral("/lente"))}, {}, {}, {10, 10}));
        QTRY_COMPARE(server.requests(), 1);
        controller.nextPage();
        QCOMPARE(controller.currentIndex(), 1);
        server.release();
        QTRY_COMPARE_WITH_TIMEOUT(inserted.count(), 1, 10000);
        QCOMPARE(controller.document().page(0).objects.size(), std::size_t(1));
        QVERIFY(controller.document().page(1).objects.empty());
    }

    // E9, n° 27 : le texte de repli suit la même origine ; currentIndex() à l'arrivée le verserait ailleurs.
    void textFallbackStaysOnItsOriginPage()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.hold(QStringLiteral("/lente"));
        server.page(QStringLiteral("/lente"), "<p>pas une image</p>");
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        QVERIFY(insertion.acceptWebDrop({server.url(QStringLiteral("/lente"))},
                                        QStringLiteral("Titre du lien"), {}, {10, 10}));
        QTRY_COMPARE(server.requests(), 1);
        controller.nextPage();
        QCOMPARE(controller.currentIndex(), 1);
        server.release();
        QTRY_COMPARE_WITH_TIMEOUT(inserted.count(), 1, 10000);
        QCOMPARE(controller.document().page(0).objects.size(), std::size_t(1));
        QVERIFY(lastAs<core::Text>(controller) != nullptr);
        QVERIFY(controller.document().page(1).objects.empty());
    }

    // E9, n° 27 : page d'origine retirée pendant le téléchargement : le texte n'est versé sur aucune autre
    // page.
    void textFallbackIsDroppedWhenItsPageDisappears()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.hold(QStringLiteral("/lente"));
        server.page(QStringLiteral("/lente"), "<p>pas une image</p>");
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        controller.nextPage();
        controller.pageBack(); // page 1 affichée, page 2 derrière
        QCOMPARE(controller.currentIndex(), 0);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        QSignalSpy failed(&insertion, &ui::Insertion::failed);
        QVERIFY(insertion.acceptWebDrop({server.url(QStringLiteral("/lente"))},
                                        QStringLiteral("Titre du lien"), {}, {10, 10}));
        QTRY_COMPARE(server.requests(), 1);
        controller.execute(std::make_unique<core::RemovePagesCommand>(std::vector<std::size_t>{0}));
        QCOMPARE(controller.document().pageCount(), std::size_t(1));
        server.release();
        QTRY_VERIFY_WITH_TIMEOUT(!insertion.isBusy(), 10000);
        QCOMPARE(inserted.count(), 0);
        QCOMPARE(failed.count(), 1);
        QVERIFY(controller.document().page(0).objects.empty());
    }

    // n° 28 : Ctrl+Z pendant le téléchargement agit sur l'étape précédente ; l'image arrive quand même.
    void undoDuringDownloadKeepsTheDownload()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.hold(QStringLiteral("/lente"));
        server.page(QStringLiteral("/lente"), pngBytes(20, 20));
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QVERIFY(insertion.acceptDrop({}, QStringLiteral("avant"), {1, 1}));
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        QVERIFY(insertion.acceptWebDrop({server.url(QStringLiteral("/lente"))}, {}, {}, {10, 10}));
        QTRY_COMPARE(server.requests(), 1);
        controller.undo();
        QCOMPARE(firstPage(controller).objects.size(), std::size_t{0});
        server.release();
        QTRY_COMPARE_WITH_TIMEOUT(inserted.count(), 1, 10000);
        QVERIFY(lastAs<core::ImageObject>(controller) != nullptr);
    }

    // n° 25 : gardes allumées, une adresse non saisie est refusée sans requête.
    void guardsRefuseAWebDrop()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/i"), pngBytes(10, 10));
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::WebFrameItems frames;
        frames.setGuardsEnabled(true);
        frames.setDocument(&controller);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        insertion.setWebFrames(&frames);
        QSignalSpy failed(&insertion, &ui::Insertion::failed);
        QVERIFY(insertion.acceptWebDrop({server.url(QStringLiteral("/i"))}, {}, {}, {1, 1}));
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 5000);
        QCOMPARE(server.requests(), 0);
    }

    // Pendant positif : gardes allumées, origine saisie (autorisée) : l'image est téléchargée.
    void guardsLetAnAllowedOriginThrough()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/i"), pngBytes(10, 10));
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::WebFrameItems frames;
        frames.setGuardsEnabled(true);
        frames.setDocument(&controller);
        QVERIFY(frames.insertFrame({}, server.url(QStringLiteral("/page")).toString()));
        QCOMPARE(frames.decideAddress(server.url(QStringLiteral("/i"))), ui::WebContentPolicy::Verdict::Load);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        insertion.setWebFrames(&frames);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        QSignalSpy failed(&insertion, &ui::Insertion::failed);
        QVERIFY(insertion.acceptWebDrop({server.url(QStringLiteral("/i"))}, {}, {}, {1, 1}));
        QTRY_COMPARE_WITH_TIMEOUT(inserted.count(), 1, 10000);
        QCOMPARE(failed.count(), 0);
    }

    // Review Focus 4 (n° 20) : le fichier temporaire du moteur a disparu : message, document intact.
    void vanishedDropFileFails()
    {
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy failed(&insertion, &ui::Insertion::failed);
        QVERIFY(insertion.acceptWebDrop(
            {QUrl::fromLocalFile(QStringLiteral("/chemin/inexistant/glisse.png"))}, {}, {}, {10, 10}));
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 5000);
        QCOMPARE(firstPage(controller).objects.size(), std::size_t{0});
    }

    // Seul acceptWebDrop télécharge : acceptDrop et insertImage refusent toujours http(s), sans requête.
    void fileEntryPointsStillRefuseHttp()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/i.png"), pngBytes(10, 10));
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy failed(&insertion, &ui::Insertion::failed);
        QVERIFY(insertion.acceptDrop({server.url(QStringLiteral("/i.png"))}, {}, {10, 10}));
        QTRY_COMPARE(failed.count(), 1);
        insertion.insertImage(server.url(QStringLiteral("/i.png")));
        QTRY_COMPARE(failed.count(), 2);
        QCOMPARE(server.requests(), 0);
        QCOMPARE(firstPage(controller).objects.size(), std::size_t{0});
    }
};

QTEST_MAIN(TestInsertion)
#include "tst_ui_insertion.moc"
