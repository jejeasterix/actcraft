#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/core/pageedits.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/documenttabs.h>
#include <actcraft/ui/insertion.h>
#include <actcraft/ui/mediainsert.h>

#include <QFile>
#include <QImage>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

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
};

QTEST_MAIN(TestInsertion)
#include "tst_ui_insertion.moc"
