// Couche des cadres web sans moteur : contrat des vues, livraison du contenu au Loader (cadrage n° 3, 5, 30).
#include "support/presentationharness.h"
#include "support/webframehost.h"

#include <QFile>
#include <QPointer>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QtQml/qqmlextensionplugin.h>
#include <QtTest>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using presentationharness::findItem;

namespace {

QStringList members(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    static const QRegularExpression member(
        QStringLiteral(
            R"re(^    (?:readonly )?(property \S+ (?!_)\w+|signal (?!_)\w+\([^)]*\)|function (?!_)\w+\([^)]*\)))re"),
        QRegularExpression::MultilineOption);
    QStringList found;
    const QString text = QString::fromUtf8(file.readAll());
    for (auto it = member.globalMatch(text); it.hasNext();)
        found << it.next().captured(1);
    found.sort();
    return found;
}

QString source(const char* relative)
{
    return QStringLiteral(ACTCRAFT_SOURCE_DIR "/") + QString::fromLatin1(relative);
}

} // namespace

class TstUiWebFrameLayer : public QObject
{
    Q_OBJECT
public slots:
    // Review Focus 1 : contenu changé pendant le chargement du Loader. Slot public : Qt Test ne lance que les
    // privés.
    void reloadWhileTheLoaderLoads()
    {
        if (!m_frames || m_reloaded || sender()->property("status").toInt() != 2 /* Loader.Loading */)
            return;
        m_reloaded = true;
        m_frames->reload(0);
    }

private:
    actcraft::ui::WebFrameItems* m_frames = nullptr;
    bool m_reloaded = false;

private slots:
    // Le contrat vit en QML, sans contrôle du compilateur (DA1) : les trois composants sont comparés.
    void contractsMatch()
    {
        const QStringList engine = members(source("ui/qml/WebFrameEngineView.qml"));
        QVERIFY(engine.contains(QStringLiteral("function load()")));
        QVERIFY(engine.contains(QStringLiteral("property url directUrl")));
        QCOMPARE(members(source("ui/qml/WebFrameSystemView.qml")), engine);
        QCOMPARE(members(source("tests/qml/FakeWebFrameView.qml")), engine);
    }

    void contentGoesOutAtLoaderReady()
    {
        WebFrameHost host;
        QVERIFY(host.start());
        host.useFakeView();
        QVERIFY(host.frames->insertFrame(QStringLiteral("<p>bonjour</p>"), {}));
        QTRY_VERIFY(findItem(host.root.get(), QStringLiteral("webFrameView")));
        QQuickItem* view = findItem(host.root.get(), QStringLiteral("webFrameView"));
        QTRY_COMPARE(view->property("_loads").toInt(), 1);
        QVERIFY(view->property("html").toString().contains(QStringLiteral("bonjour")));
    }

    // Review Focus 1 (cadrage n° 5) : `serial` avance alors que le Loader charge ; la vue reçoit le dernier
    // contenu.
    void deliversTheLatestSerialAtReady()
    {
        WebFrameHost host;
        QVERIFY(host.start());
        host.useFakeView();
        // Gardes allumées : reload avance `serial` ; gardes coupées, il part à la vue (T4). À T1 la propriété
        // n'existe pas encore (dynamique, sans effet) : le modèle n'a que ce chemin.
        host.frames->setProperty("guardsEnabled", true);
        m_frames = host.frames;
        m_reloaded = false;
        QVERIFY(host.frames->insertFrame(QStringLiteral("<p>a</p>"), {}));
        // Trouvé sans tourner la boucle : le contenu du modèle, livré par elle, n'a pas pu lancer le Loader.
        QTRY_VERIFY_WITH_TIMEOUT(findItem(host.root.get(), QStringLiteral("webFrameLoader")), 5000);
        connect(findItem(host.root.get(), QStringLiteral("webFrameLoader")), SIGNAL(statusChanged()), this,
                SLOT(reloadWhileTheLoaderLoads()));
        QTRY_VERIFY_WITH_TIMEOUT(m_reloaded, 5000);
        QTRY_VERIFY_WITH_TIMEOUT(findItem(host.root.get(), QStringLiteral("webFrameView")), 5000);
        QQuickItem* view = findItem(host.root.get(), QStringLiteral("webFrameView"));
        const QModelIndex row = host.frames->index(0, 0);
        // Le reload avance `serial` par la boucle d'événements : attendre son contenu, sinon 1 = 1 passerait
        // sans lui.
        QTRY_VERIFY_WITH_TIMEOUT(host.frames->data(row, actcraft::ui::WebFrameItems::SerialRole).toInt() >= 2,
                                 5000);
        QTRY_COMPARE(view->property("serial").toInt(),
                     host.frames->data(row, actcraft::ui::WebFrameItems::SerialRole).toInt());
        QVERIFY(view->property("_loads").toInt() >= 1);
        m_frames = nullptr;
    }

    // Sans gardes, l'adresse part telle quelle (showsView accepte une directUrl, n° 11).
    void directUrlGoesToTheView()
    {
        WebFrameHost host;
        QVERIFY(host.start());
        host.useFakeView();
        const QUrl address(QStringLiteral("https://exemple.invalide/"));
        QVERIFY(host.frames->insertFrame({}, address.toString()));
        QTRY_VERIFY_WITH_TIMEOUT(findItem(host.root.get(), QStringLiteral("webFrameView")), 5000);
        QQuickItem* view = findItem(host.root.get(), QStringLiteral("webFrameView"));
        QTRY_COMPARE(view->property("directUrl").toUrl(), address);
        QCOMPARE(view->property("guardsEnabled").toBool(), false);
        QTRY_COMPARE(view->property("_loads").toInt(), 1);
    }

    // E2 : seule une vue native au-dessus de la scène est masquée par une boîte ouverte (n° 16).
    void coveredOnlyHidesAnOverlayView()
    {
        WebFrameHost host;
        QVERIFY(host.start());
        host.useFakeView();
        QVERIFY(host.frames->insertFrame(QStringLiteral("<p>a</p>"), {}));
        QTRY_VERIFY_WITH_TIMEOUT(findItem(host.root.get(), QStringLiteral("webFrameView")), 5000);
        QQuickItem* viewHost = findItem(host.root.get(), QStringLiteral("webFrameViewHost"));
        QQuickItem* view = findItem(host.root.get(), QStringLiteral("webFrameView"));
        QTRY_VERIFY(viewHost->isVisible());
        host.root->setProperty("inputBlocked", true);
        QVERIFY(viewHost->isVisible());
        view->setProperty("_overlays", true);
        QTRY_VERIFY(!viewHost->isVisible());
    }

    // E12 : changer le réglage recrée la vue (profil et JavaScript ne se changent pas à chaud, n° 10).
    void toggleGuardsRecreatesTheView()
    {
        WebFrameHost host;
        QVERIFY(host.start());
        host.useFakeView();
        QVERIFY(host.frames->insertFrame(QStringLiteral("<p>a</p>"), {}));
        QTRY_VERIFY_WITH_TIMEOUT(findItem(host.root.get(), QStringLiteral("webFrameView")), 5000);
        QPointer<QQuickItem> first = findItem(host.root.get(), QStringLiteral("webFrameView"));
        QCOMPARE(first->property("guardsEnabled").toBool(), false);
        host.frames->setGuardsEnabled(true);
        QTRY_VERIFY_WITH_TIMEOUT(first.isNull(), 5000);
        QTRY_VERIFY_WITH_TIMEOUT(findItem(host.root.get(), QStringLiteral("webFrameView")), 5000);
        QCOMPARE(
            findItem(host.root.get(), QStringLiteral("webFrameView"))->property("guardsEnabled").toBool(),
            true);
    }

    // Interdit du plan : une vue à JavaScript actif ne survit pas, cachée, au repère de blocage. Document
    // reçu, adresse distante : gardes coupées, la vue l'ouvre ; gardes allumées, le cadre est bloqué, la vue
    // détruite.
    void blockedFrameKeepsNoLiveView()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QUrl file = QUrl::fromLocalFile(dir.filePath(QStringLiteral("recu.actcraft")));
        WebFrameHost writer;
        QVERIFY(writer.start());
        writer.useFakeView();
        QVERIFY(writer.frames->insertFrame({}, QStringLiteral("https://exemple.invalide/")));
        writer.document.saveToUrl(file);
        QTRY_VERIFY_WITH_TIMEOUT(!writer.document.isBusy(), 10000);
        WebFrameHost reader;
        QVERIFY(reader.start());
        reader.useFakeView();
        reader.document.openUrl(file);
        QTRY_VERIFY_WITH_TIMEOUT(!reader.document.isBusy() && reader.document.fileUrl().isValid(), 10000);
        QTRY_VERIFY_WITH_TIMEOUT(findItem(reader.root.get(), QStringLiteral("webFrameView")), 5000);
        QPointer<QQuickItem> first = findItem(reader.root.get(), QStringLiteral("webFrameView"));
        QCOMPARE(first->property("guardsEnabled").toBool(), false);
        reader.frames->setGuardsEnabled(true);
        QTRY_COMPARE_WITH_TIMEOUT(reader.frames->blockedCount(), 1, 10000);
        QTRY_VERIFY_WITH_TIMEOUT(first.isNull(), 5000);
        QCOMPARE(findItem(reader.root.get(), QStringLiteral("webFrameView")), nullptr);
    }

    // n° 32 : une vue par cadre de la page affichée, détruite au changement de page.
    void pageChangeDestroysTheView()
    {
        WebFrameHost host;
        QVERIFY(host.start());
        host.useFakeView();
        QVERIFY(host.frames->insertFrame(QStringLiteral("<p>a</p>"), {}));
        QTRY_VERIFY_WITH_TIMEOUT(findItem(host.root.get(), QStringLiteral("webFrameView")), 5000);
        QPointer<QQuickItem> first = findItem(host.root.get(), QStringLiteral("webFrameView"));
        host.document.nextPage();
        QTRY_VERIFY_WITH_TIMEOUT(first.isNull(), 5000);
    }

    void reportAndCommandsCrossTheContract()
    {
        WebFrameHost host;
        QVERIFY(host.start());
        host.useFakeView();
        QVERIFY(host.frames->insertFrame({}, QStringLiteral("https://exemple.invalide/")));
        QTRY_VERIFY_WITH_TIMEOUT(findItem(host.root.get(), QStringLiteral("webFrameView")), 5000);
        QQuickItem* view = findItem(host.root.get(), QStringLiteral("webFrameView"));
        QVERIFY(QMetaObject::invokeMethod(view, "_emitReport",
                                          Q_ARG(QVariant, QUrl(QStringLiteral("https://exemple.invalide/b"))),
                                          Q_ARG(QVariant, true), Q_ARG(QVariant, false),
                                          Q_ARG(QVariant, QStringLiteral("Page B")), Q_ARG(QVariant, false)));
        const QModelIndex row = host.frames->index(0, 0);
        QTRY_COMPARE(host.frames->data(row, actcraft::ui::WebFrameItems::TitleRole).toString(),
                     QStringLiteral("Page B"));
        host.frames->goBack(0);
        QTRY_VERIFY(view->property("_calls").toStringList().contains(QStringLiteral("goBack")));
    }
};

QTEST_MAIN(TstUiWebFrameLayer)
#include "tst_ui_webframelayer.moc"
