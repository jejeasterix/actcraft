// Dépôt sur la page, côté page (cadrage n° 31) : QDropEvent envoyé à la fenêtre, avec les formats exacts du
// moteur. Le glisser réel de la vue vers la page exige une vraie souris : Reste pour Jérôme R1.
#include "startup.h"
#include "support/presentationharness.h"
#include "support/testserver.h"

#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/insertion.h>
#include <actcraft/ui/objectediting.h>

#include <QBuffer>
#include <QClipboard>
#include <QDropEvent>
#include <QGuiApplication>
#include <QImage>
#include <QMimeData>
#include <QTemporaryDir>
#include <QtQml/qqmlextensionplugin.h>
#include <QtTest>

#include <variant>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;
using presentationharness::MainWindow;

namespace {

void dropAt(QQuickWindow* window, QMimeData* mime, QPointF point)
{
    QDragEnterEvent enter(point, Qt::CopyAction, mime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &enter);
    QDragMoveEvent move(point, Qt::CopyAction, mime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &move);
    QDropEvent drop(point, Qt::CopyAction, mime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &drop);
}

QByteArray png(int width, int height)
{
    QImage image(width, height, QImage::Format_RGB32);
    image.fill(Qt::red);
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return bytes;
}

ui::DocumentController* documentOf(const MainWindow& main)
{
    return main.window()->findChild<ui::DocumentController*>(QStringLiteral("documentController"));
}

template <typename T> const T* lastAs(const ui::DocumentController& controller)
{
    const auto& objects = controller.document().page(0).objects;
    return objects.empty() ? nullptr : std::get_if<T>(&core::body(objects.back()));
}

} // namespace

class TstUiWebDrop : public QObject
{
    Q_OBJECT
private slots:
    // Texte seul : un objet texte, Ctrl+Z en une étape.
    void droppedTextBecomesAText()
    {
        MainWindow main;
        QVERIFY(main.window());
        ui::DocumentController* document = documentOf(main);
        QVERIFY(document);
        QMimeData mime;
        mime.setText(QStringLiteral("Texte glissé"));
        dropAt(main.window(), &mime, main.pageView()->pageToScene(QPointF(300, 200)));
        QTRY_VERIFY(lastAs<core::Text>(*document) != nullptr);
        QVERIFY(lastAs<core::Text>(*document)->html.find("Texte glissé") != std::string::npos);
        document->undo();
        QCOMPARE(document->document().page(0).objects.size(), std::size_t{0});
    }

    // Image en URL file: (fichier temporaire du moteur) : une image locale.
    void droppedTemporaryFileImageIsInserted()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("glisse.png"));
        QVERIFY(QImage::fromData(png(60, 40)).save(path, "PNG"));
        MainWindow main;
        QVERIFY(main.window());
        ui::DocumentController* document = documentOf(main);
        QMimeData mime;
        mime.setUrls({QUrl::fromLocalFile(path)});
        dropAt(main.window(), &mime, main.pageView()->pageToScene(QPointF(300, 200)));
        QTRY_VERIFY_WITH_TIMEOUT(lastAs<core::ImageObject>(*document) != nullptr, 10000);
    }

    // E4 : un fichier glissé du système s'insère (même chemin qu'un fichier du moteur).
    void droppedSystemFileIsInserted()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("du-bureau.png"));
        QVERIFY(QImage::fromData(png(30, 30)).save(path, "PNG"));
        MainWindow main;
        QVERIFY(main.window());
        ui::DocumentController* document = documentOf(main);
        QMimeData mime;
        mime.setUrls({QUrl::fromLocalFile(path)});
        dropAt(main.window(), &mime, main.pageView()->pageToScene(QPointF(500, 400)));
        QTRY_VERIFY_WITH_TIMEOUT(lastAs<core::ImageObject>(*document) != nullptr, 10000);
        QCOMPARE(lastAs<core::ImageObject>(*document)->bounds.center().x, 500.0);
    }

    // URL avec son titre, HTML en plus : le HTML n'est jamais interprété (M13), la page non image donne le
    // titre.
    void droppedLinkWithTitleFallsBackToItsTitle()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/page"), "<p>page</p>");
        MainWindow main;
        QVERIFY(main.window());
        ui::DocumentController* document = documentOf(main);
        QMimeData mime;
        mime.setUrls({server.url(QStringLiteral("/page"))});
        mime.setText(QStringLiteral("Titre du lien"));
        mime.setHtml(QStringLiteral("<a href=\"/page\"><img src=\"/espion.png\">Titre</a>"));
        dropAt(main.window(), &mime, main.pageView()->pageToScene(QPointF(200, 150)));
        QTRY_VERIFY_WITH_TIMEOUT(lastAs<core::Text>(*document) != nullptr, 10000);
        QVERIFY(lastAs<core::Text>(*document)->html.find("Titre du lien") != std::string::npos);
        QVERIFY(!server.paths().contains(QStringLiteral("/espion.png")));
    }

    // S1 : image http(s), téléchargée ; document enregistré puis rouvert serveur coupé : elle est là.
    void droppedRemoteImageSurvivesReopening()
    {
        QTemporaryDir dir;
        const QString file = dir.filePath(QStringLiteral("depot.actcraft"));
        const QByteArray bytes = png(80, 50);
        MainWindow main;
        QVERIFY(main.window());
        ui::DocumentController* document = documentOf(main);
        {
            TestServer server;
            QVERIFY(server.listening());
            server.page(QStringLiteral("/photo.png"), bytes);
            QMimeData mime;
            mime.setUrls({server.url(QStringLiteral("/photo.png"))});
            dropAt(main.window(), &mime, main.pageView()->pageToScene(QPointF(300, 200)));
            QTRY_VERIFY_WITH_TIMEOUT(lastAs<core::ImageObject>(*document) != nullptr, 10000);
            QVERIFY(document->canUndo());
            document->saveToUrl(QUrl::fromLocalFile(file));
            QTRY_VERIFY_WITH_TIMEOUT(!document->isBusy(), 10000);
        } // serveur coupé
        document->openUrl(QUrl::fromLocalFile(file));
        QTRY_VERIFY_WITH_TIMEOUT(!document->isBusy() && document->fileUrl().isValid(), 10000);
        const auto* image = lastAs<core::ImageObject>(*document);
        QVERIFY(image != nullptr);
        const core::Resource* resource = document->document().resource(image->resource);
        QVERIFY(resource != nullptr);
        QCOMPARE(resource->data, bytes);
    }

    // S1, n° 31 : image sans octets, URL http(s) en échec : bandeau, document intact.
    void droppedUnreachableImageReportsTheFailure()
    {
        TestServer server;
        QVERIFY(server.listening());
        MainWindow main;
        QVERIFY(main.window());
        ui::DocumentController* document = documentOf(main);
        auto* insertion = main.window()->findChild<ui::Insertion*>(QStringLiteral("appInsertion"));
        QVERIFY(insertion);
        QSignalSpy failed(insertion, &ui::Insertion::failed);
        QMimeData mime;
        mime.setUrls({server.url(QStringLiteral("/absente.png"))});
        dropAt(main.window(), &mime, main.pageView()->pageToScene(QPointF(300, 200)));
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 10000);
        QCOMPARE(document->document().page(0).objects.size(), std::size_t{0});
    }

    // Revue T20 : l'échec d'un dépôt arrive au bandeau de la fenêtre (le signal seul ne montre rien à
    // l'écran).
    void failedDropReachesTheBanner()
    {
        TestServer server;
        QVERIFY(server.listening());
        MainWindow main;
        QVERIFY(main.window());
        QQuickItem* banner = main.item(QStringLiteral("messageBanner"));
        QVERIFY(banner);
        QVERIFY(banner->property("message").toString().isEmpty());
        QMimeData mime;
        mime.setUrls({server.url(QStringLiteral("/absente.png"))});
        dropAt(main.window(), &mime, main.pageView()->pageToScene(QPointF(300, 200)));
        QTRY_VERIFY_WITH_TIMEOUT(!banner->property("message").toString().isEmpty(), 10000);
    }

    // Revue T20 : de même pour un collage d'adresse refusé (gardes allumées, hôte non autorisé).
    void failedPasteReachesTheBanner()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/photo.png"), png(40, 40));
        MainWindow main;
        QVERIFY(main.window());
        main.window()
            ->findChild<QObject*>(QStringLiteral("appWebFrames"))
            ->setProperty("guardsEnabled", true);
        QQuickItem* banner = main.item(QStringLiteral("messageBanner"));
        QVERIFY(banner);
        QVERIFY(banner->property("message").toString().isEmpty());
        QGuiApplication::clipboard()->setText(server.url(QStringLiteral("/photo.png")).toString());
        main.window()->findChild<ui::ObjectEditing*>(QStringLiteral("appEditing"))->paste();
        QTRY_VERIFY_WITH_TIMEOUT(!banner->property("message").toString().isEmpty(), 10000);
        QCOMPARE(server.requests(), 0);
    }

    // Revue de T10 : le câblage `insertion.webFrames` / `editing.webFrames` de la couche. Gardes allumées, un
    // dépôt puis un collage vers un hôte non autorisé sont refusés sans requête ; sans le câblage, le
    // téléchargement partirait.
    void guardsCoverDropAndPasteThroughTheWiring()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/photo.png"), png(40, 40));
        MainWindow main;
        QVERIFY(main.window());
        ui::DocumentController* document = documentOf(main);
        QObject* frames = main.window()->findChild<QObject*>(QStringLiteral("appWebFrames"));
        QVERIFY(frames);
        frames->setProperty("guardsEnabled", true);
        QCOMPARE(frames->property("guardsEnabled").toBool(), true);
        auto* insertion = main.window()->findChild<ui::Insertion*>(QStringLiteral("appInsertion"));
        auto* editing = main.window()->findChild<ui::ObjectEditing*>(QStringLiteral("appEditing"));
        QVERIFY(insertion);
        QVERIFY(editing);
        // `editing` a sa propre insertion : son refus sort par `insertionFailed`.
        QSignalSpy failed(insertion, &ui::Insertion::failed);
        QSignalSpy pasteFailed(editing, &ui::ObjectEditing::insertionFailed);
        QMimeData mime;
        mime.setUrls({server.url(QStringLiteral("/photo.png"))});
        dropAt(main.window(), &mime, main.pageView()->pageToScene(QPointF(300, 200)));
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 10000);
        QGuiApplication::clipboard()->setText(server.url(QStringLiteral("/photo.png")).toString());
        editing->paste();
        QTRY_COMPARE_WITH_TIMEOUT(pasteFailed.count(), 1, 10000);
        QCOMPARE(server.requests(), 0);
        QCOMPARE(document->document().page(0).objects.size(), std::size_t{0});
    }
};

QTEST_MAIN(TstUiWebDrop)
#include "tst_ui_webdrop.moc"
