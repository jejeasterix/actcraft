#include <actcraft/actions/profilestore.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/formats/fragment.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/preferences.h>
#include <actcraft/ui/profilecontroller.h>
#include <actcraft/ui/resourcelibrary.h>
#include <actcraft/ui/toolhost.h>

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QMutex>
#include <QMutexLocker>
#include <QRectF>
#include <QScopeGuard>
#include <QSemaphore>
#include <QSet>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>
#include <QThreadPool>
#include <QUrl>

#include <atomic>
#include <memory>
#include <string>
#include <variant>
#include <vector>

using namespace actcraft;

namespace {

// Fils sur lesquels les vignettes ont été rendues (observateur posé par l'essai, appelé depuis la tâche).
QMutex g_threadsMutex;
QSet<QThread*> g_renderThreads;

QString cacheDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/vignettes");
}

bool writeImage(const QString& path, const char* format, QColor color = Qt::red)
{
    QImage image(32, 24, QImage::Format_ARGB32);
    image.fill(color);
    return image.save(path, format);
}

core::ObjectId addStroke(ui::DocumentController& document, double x, double y)
{
    core::Stroke stroke;
    stroke.points = {{x, y, 0.5f}, {x + 100, y + 60, 0.5f}, {x + 200, y + 20, 0.5f}};
    core::PageObject object = stroke;
    object.common.layer = core::Layer::Middle;
    document.execute(std::make_unique<core::AddObjectCommand>(0, std::move(object)));
    return document.document().page(0).objects.back().common.id;
}

// Fragment d'un trait ; `keywords` posés sur la page du fragment (gardés par writeFragment, L6-T19).
bool writeStrokeFragment(const QString& path, const std::vector<std::string>& keywords = {})
{
    ui::DocumentController source;
    source.newDocument(1024, 768);
    const core::ObjectId id = addStroke(source, 100, 100);
    const_cast<core::Document&>(source.document()).page(0).keywords = keywords;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    QString error;
    return formats::writeFragment(source.document(), 0, {id}, file, error);
}

// Fragment de deux traits et du groupe qui les réunit.
bool writeGroupFragment(const QString& path)
{
    ui::DocumentController source;
    source.newDocument(1024, 768);
    const core::ObjectId a = addStroke(source, 100, 100);
    const core::ObjectId b = addStroke(source, 100, 300);
    core::Group group;
    group.members = {a, b};
    core::PageObject object = group;
    object.common.layer = core::Layer::Middle;
    source.execute(std::make_unique<core::AddObjectCommand>(0, std::move(object)));
    const core::ObjectId g = source.document().page(0).objects.back().common.id;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    QString error;
    return formats::writeFragment(source.document(), 0, {a, b, g}, file, error);
}

// Membres du groupe inséré à partir du rang `first` qui désignent un trait inséré avec lui ; -1 sans groupe.
int insertedGroupMembers(const core::Page& page, std::size_t first)
{
    const core::Group* group = nullptr;
    for (std::size_t i = first; i < page.objects.size(); ++i)
        if (const auto* g = std::get_if<core::Group>(&page.objects[i]))
            group = g;
    if (group == nullptr)
        return -1;
    int resolved = 0;
    for (const core::ObjectId member : group->members)
        for (std::size_t i = first; i < page.objects.size(); ++i)
            if (page.objects[i].common.id == member && std::holds_alternative<core::Stroke>(page.objects[i]))
                ++resolved;
    return resolved;
}

// Emprise des objets de la page à partir du rang `first` (un groupe n'en a pas : ses membres).
QRectF insertedBounds(const core::Page& page, std::size_t first)
{
    QRectF out;
    for (std::size_t i = first; i < page.objects.size(); ++i) {
        if (!core::hasExtent(page.objects[i]))
            continue;
        const core::RectF box = core::boundsOnPage(page.objects[i]);
        out = out.united(QRectF(box.x, box.y, box.width, box.height));
    }
    return out;
}

// Observateur qui compte ses appels et retient le premier jusqu'à `gate.release()` (5 s au plus).
std::function<void()> holdFirst(std::atomic<int>& calls, QSemaphore& gate)
{
    return [&calls, &gate] {
        if (calls.fetch_add(1) == 0)
            gate.tryAcquire(1, 5000);
    };
}

// Bibliothèque montée sur des documents, commandes et profil d'essai réels ; racines dans un dossier
// temporaire (aucun réglage ni dossier du poste touché).
struct Fixture
{
    QTemporaryDir dir;
    ui::Commands commands;
    ui::Preferences prefs;
    ui::DocumentController document;
    std::unique_ptr<ui::ProfileController> profile;
    ui::ToolHost tools;
    ui::ResourceLibrary library;
    QString mine;
    QString shared;
    QString other;

    Fixture() : prefs(dir.filePath(QStringLiteral("poste.ini")), nullptr)
    {
        actions::ProfileStore store(dir.filePath(QStringLiteral("profiles")),
                                    QStringLiteral(":/actcraft/actions/profiles"),
                                    ui::ProfileController::builtinOrder());
        profile = std::make_unique<ui::ProfileController>(std::move(store), &prefs, &commands);
        profile->start();
        document.newDocument(1024, 768);
        tools.setSettingsFile(dir.filePath(QStringLiteral("outils.ini")));
        tools.setDocumentController(&document);
        mine = dir.filePath(QStringLiteral("mes-ressources"));
        shared = dir.filePath(QStringLiteral("partagees"));
        other = dir.filePath(QStringLiteral("autre"));
        for (const QString& path : {mine, shared, other})
            QDir().mkpath(path);
        ui::ResourceLibrary::setRootPathsForTests(mine, shared);
        ui::ResourceLibrary::setTrashForTests(false); // jamais la corbeille du poste
        library.setDocument(&document);
        library.setProfile(profile.get());
        library.setCommands(&commands);
        library.setTools(&tools);
        library.setRoot(QStringLiteral("mine"));
    }
    const core::Page& page() const { return document.document().page(0); }
    QStringList paths() const
    {
        QStringList out;
        for (int row = 0; row < library.rowCount(); ++row)
            out << library.data(library.index(row), ui::ResourceLibrary::PathRole).toString();
        return out;
    }
    QString kindOf(const QString& path) const
    {
        for (int row = 0; row < library.rowCount(); ++row)
            if (library.data(library.index(row), ui::ResourceLibrary::PathRole).toString() == path)
                return library.data(library.index(row), ui::ResourceLibrary::KindRole).toString();
        return {};
    }
    QStringList folderNames() const
    {
        QStringList out;
        for (const QVariant& entry : library.folders())
            out << entry.toMap().value(QStringLiteral("name")).toString();
        return out;
    }
    QList<int> folderDepths() const
    {
        QList<int> out;
        for (const QVariant& entry : library.folders())
            out << entry.toMap().value(QStringLiteral("depth")).toInt();
        return out;
    }
};

} // namespace

class TstUiResources : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase() { QStandardPaths::setTestModeEnabled(true); }
    void init()
    {
        QDir(cacheDir()).removeRecursively(); // chaque essai rend ses vignettes
        ui::ResourceLibrary::setRenderObserverForTests({});
        ui::ResourceLibrary::setSearchObserverForTests({});
        QMutexLocker lock(&g_threadsMutex);
        g_renderThreads.clear();
    }

    void threeRoots()
    {
        Fixture f;
        QCOMPARE(f.library.root(), QStringLiteral("mine"));
        QCOMPARE(ui::ResourceLibrary::mineRoot(), f.mine);
        QCOMPARE(ui::ResourceLibrary::sharedRoot(), f.shared);
        QCOMPARE(f.library.folder(), f.mine);
        f.library.setRoot(QStringLiteral("shared"));
        QCOMPARE(f.library.root(), QStringLiteral("shared"));
        QCOMPARE(f.library.folder(), f.shared);
        f.library.setOtherFolder(QUrl::fromLocalFile(f.other));
        QCOMPARE(f.library.root(), QStringLiteral("other"));
        QCOMPARE(f.library.folder(), f.other);
        f.library.setRoot(QStringLiteral("mine"));
        QCOMPARE(f.library.folder(), f.mine);
        f.library.setRoot(QStringLiteral("other")); // le dossier choisi est gardé
        QCOMPARE(f.library.folder(), f.other);
    }

    void foldersListed()
    {
        Fixture f;
        QDir(f.mine).mkpath(QStringLiteral("a/b"));
        QDir(f.mine).mkpath(QStringLiteral("c"));
        f.library.setRoot(QStringLiteral("mine"));
        QTRY_COMPARE(f.folderNames(),
                     (QStringList{QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")}));
        QCOMPARE(f.folderDepths(), (QList<int>{0, 1, 0}));
        QCOMPARE(f.library.folders().at(1).toMap().value(QStringLiteral("path")).toString(),
                 QDir(f.mine).filePath(QStringLiteral("a/b")));
        QVERIFY(f.library.createFolder(QStringLiteral("d")));
        QVERIFY(QFileInfo(QDir(f.mine).filePath(QStringLiteral("d"))).isDir());
        QTRY_COMPARE(f.folderNames(), (QStringList{QStringLiteral("a"), QStringLiteral("b"),
                                                   QStringLiteral("c"), QStringLiteral("d")}));
    }

    void itemsOfAFolder()
    {
        Fixture f;
        const QString png = QDir(f.mine).filePath(QStringLiteral("rouge.png"));
        const QString jpg = QDir(f.mine).filePath(QStringLiteral("bleu.jpg"));
        const QString fragment = QDir(f.mine).filePath(QStringLiteral("trait.actcraft"));
        QVERIFY(writeImage(png, "PNG"));
        QVERIFY(writeImage(jpg, "JPG", Qt::blue));
        QVERIFY(writeStrokeFragment(fragment));
        QFile notes(QDir(f.mine).filePath(QStringLiteral("notes.txt")));
        QVERIFY(notes.open(QIODevice::WriteOnly));
        notes.write("rien");
        notes.close();
        QDir(f.mine).mkpath(QStringLiteral("sous-dossier"));
        f.library.openFolder(f.mine);
        QTRY_COMPARE(f.library.itemCount(), 3);
        QCOMPARE(f.library.rowCount(), 3);
        QCOMPARE(f.kindOf(png), QStringLiteral("image"));
        QCOMPARE(f.kindOf(jpg), QStringLiteral("image"));
        QCOMPARE(f.kindOf(fragment), QStringLiteral("fragment"));
        const int row = static_cast<int>(f.paths().indexOf(fragment));
        QCOMPARE(f.library.data(f.library.index(row), ui::ResourceLibrary::NameRole).toString(),
                 QStringLiteral("trait"));
    }

    void thumbnailsOffTheMainThread()
    {
        Fixture f;
        ui::ResourceLibrary::setRenderObserverForTests([] {
            QMutexLocker lock(&g_threadsMutex);
            g_renderThreads.insert(QThread::currentThread());
        });
        QVERIFY(writeImage(QDir(f.mine).filePath(QStringLiteral("a.png")), "PNG"));
        QVERIFY(writeImage(QDir(f.mine).filePath(QStringLiteral("b.jpg")), "JPG"));
        QVERIFY(writeStrokeFragment(QDir(f.mine).filePath(QStringLiteral("c.actcraft"))));
        f.library.openFolder(f.mine);
        QTRY_COMPARE_WITH_TIMEOUT(f.library.thumbnailsRendered(), 3, 10000);
        {
            QMutexLocker lock(&g_threadsMutex);
            QVERIFY(!g_renderThreads.isEmpty());
            QVERIFY(!g_renderThreads.contains(QThread::currentThread()));
        }
        for (int row = 0; row < 3; ++row) {
            QTRY_VERIFY(f.library.data(f.library.index(row), ui::ResourceLibrary::ThumbnailRole)
                            .toUrl()
                            .isLocalFile());
            const QString file = f.library.data(f.library.index(row), ui::ResourceLibrary::ThumbnailRole)
                                     .toUrl()
                                     .toLocalFile();
            QVERIFY2(file.startsWith(cacheDir() + QLatin1Char('/')), qPrintable(file));
            QVERIFY(file.endsWith(QLatin1String(".png")));
            QVERIFY(!QImage(file).isNull());
        }
    }

    void searchByNameAndKeywords()
    {
        Fixture f;
        QDir(f.mine).mkpath(QStringLiteral("plages"));
        const QString byName = QDir(f.mine).filePath(QStringLiteral("soleil-couchant.png"));
        const QString byKeywords = QDir(f.mine).filePath(QStringLiteral("plages/plage.actcraft"));
        QVERIFY(writeImage(byName, "PNG"));
        QVERIFY(writeImage(QDir(f.mine).filePath(QStringLiteral("lune.png")), "PNG"));
        QVERIFY(writeStrokeFragment(byKeywords, {"été", "soleil"}));
        QVERIFY(
            writeStrokeFragment(QDir(f.mine).filePath(QStringLiteral("plages/vent.actcraft")), {"nuage"}));

        f.library.search(QStringLiteral("Soleil"), true, true);
        QTRY_VERIFY(!f.library.searching());
        QStringList found = f.paths();
        found.sort();
        QCOMPARE(found, (QStringList{byKeywords, byName}));

        f.library.search(QStringLiteral("soleil"), true, false);
        QTRY_VERIFY(!f.library.searching());
        QCOMPARE(f.paths(), QStringList{byName});

        f.library.search(QStringLiteral("soleil"), false, true);
        QTRY_VERIFY(!f.library.searching());
        QCOMPARE(f.paths(), QStringList{byKeywords});
        QCOMPARE(f.library.itemCount(), 1);
    }

    // Le fil de recherche est retenu au premier fichier ; annulée, la recherche s'arrête au suivant (le fil
    // ne parcourt pas les 199 autres).
    void searchIsCancellable()
    {
        std::atomic<int> visited{0};
        QSemaphore gate;
        ui::ResourceLibrary::setSearchObserverForTests(holdFirst(visited, gate));
        Fixture f;
        const auto settle = qScopeGuard([&gate] { // le fil de recherche ne survit pas à l'essai
            gate.release();
            QThreadPool::globalInstance()->waitForDone(10000);
        });
        for (int i = 0; i < 200; ++i)
            QVERIFY(
                writeStrokeFragment(QDir(f.mine).filePath(QStringLiteral("f%1.actcraft").arg(i)), {"mot"}));
        f.library.openFolder(f.mine);
        QTRY_COMPARE_WITH_TIMEOUT(f.library.itemCount(), 200, 10000);
        f.library.search(QStringLiteral("absent"), true, true);
        QVERIFY(f.library.searching());
        QTRY_COMPARE_WITH_TIMEOUT(visited.load(), 1, 10000);
        QElapsedTimer timer;
        timer.start();
        f.library.cancelSearch();
        QTRY_VERIFY_WITH_TIMEOUT(!f.library.searching(), 200);
        QVERIFY(timer.elapsed() < 200);
        gate.release();
        QVERIFY(QThreadPool::globalInstance()->waitForDone(10000));
        QCOMPARE(visited.load(), 1);
        // Le résultat d'une recherche annulée n'est jamais appliqué.
        QTest::qWait(500);
        QVERIFY(!f.library.searching());
        QCOMPARE(f.library.itemCount(), 200);
    }

    void renameAndDelete()
    {
        Fixture f;
        const QString a = QDir(f.mine).filePath(QStringLiteral("a.png"));
        const QString b = QDir(f.mine).filePath(QStringLiteral("b.png"));
        const QString c = QDir(f.mine).filePath(QStringLiteral("c.png"));
        QVERIFY(writeImage(a, "PNG"));
        QVERIFY(writeImage(c, "PNG"));
        f.library.openFolder(f.mine);
        QTRY_COMPARE(f.library.itemCount(), 2);
        QVERIFY(f.library.rename(a, QStringLiteral("b")));
        QVERIFY(!QFile::exists(a));
        QVERIFY(QFile::exists(b));
        QTRY_VERIFY(f.paths().contains(b));
        QVERIFY(!f.library.rename(b, QStringLiteral("c"))); // nom pris : refusé, rien d'écrasé
        QVERIFY(QFile::exists(b));
        QVERIFY(!f.library.rename(b, QString()));
        QVERIFY(f.library.remove(b));
        QVERIFY(!QFile::exists(b));
        QTRY_COMPARE(f.library.itemCount(), 1);
        QCOMPARE(f.paths(), QStringList{c});
    }

    void addSelectionCreatesTheNextName()
    {
        Fixture f;
        const core::ObjectId id = addStroke(f.document, 300, 200);
        f.tools.user().setSelection({id});
        const QString first = f.library.addSelection();
        QCOMPARE(first, QDir(f.mine).filePath(QStringLiteral("Ressource1.actcraft")));
        const QString second = f.library.addSelection();
        QCOMPARE(second, QDir(f.mine).filePath(QStringLiteral("Ressource2.actcraft")));
        QFile file(second);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QString error;
        const std::optional<formats::Fragment> fragment = formats::readFragment(file, error);
        QVERIFY2(fragment.has_value(), qPrintable(error));
        QCOMPARE(fragment->page.objects.size(), std::size_t{1});
        QTRY_COMPARE(f.library.itemCount(), 2);
        // N = premier libre.
        QVERIFY(QFile::remove(first));
        QCOMPARE(f.library.addSelection(), first);
        // Sans sélection : rien.
        f.tools.user().setSelection({});
        QVERIFY(f.library.addSelection().isEmpty());
    }

    // Piège de L6-T19 : prepareInsertion rend de vrais identifiants pris dans la cible ; les passer à un
    // collage qui attend des rangs 1..n viderait le groupe. La cible porte déjà quatre objets : les
    // identifiants neufs du fragment sortent des rangs 1..3.
    void insertImageAndFragment()
    {
        Fixture f;
        for (int i = 0; i < 4; ++i)
            addStroke(f.document, 50, 50 + 100 * i);
        const QString image = QDir(f.mine).filePath(QStringLiteral("photo.png"));
        const QString fragment = QDir(f.mine).filePath(QStringLiteral("groupe.actcraft"));
        QVERIFY(writeImage(image, "PNG"));
        QVERIFY(writeGroupFragment(fragment));
        const std::size_t before = f.page().objects.size();

        QVERIFY(f.library.insert(image, QPointF(300, 300)));
        QTRY_COMPARE(f.page().objects.size(), before + 1); // image : décodée hors du fil principal (lot 5)
        const auto* picture = std::get_if<core::ImageObject>(&f.page().objects.back());
        QVERIFY(picture != nullptr);
        QVERIFY(f.document.document().resource(picture->resource) != nullptr);

        QVERIFY(f.library.insert(fragment, QPointF(600, 400)));
        QCOMPARE(f.page().objects.size(), before + 4);
        QCOMPARE(insertedGroupMembers(f.page(), before + 1), 2);
        const QRectF placed = insertedBounds(f.page(), before + 1); // centre de l'ensemble sur le point visé
        QVERIFY2(qAbs(placed.center().x() - 600) < 0.5 && qAbs(placed.center().y() - 400) < 0.5,
                 qPrintable(QStringLiteral("%1, %2").arg(placed.center().x()).arg(placed.center().y())));

        f.document.undo(); // une étape : le fragment entier
        QCOMPARE(f.page().objects.size(), before + 1);
        QVERIFY(std::holds_alternative<core::ImageObject>(f.page().objects.back()));
        f.document.undo(); // une étape : l'image
        QCOMPARE(f.page().objects.size(), before);

        f.document.redo();
        f.document.redo();
        QCOMPARE(f.page().objects.size(), before + 4);
        QCOMPARE(insertedGroupMembers(f.page(), before + 1), 2);
    }

    void thumbnailsPostponedDuringContact()
    {
        Fixture f;
        QVERIFY(writeImage(QDir(f.mine).filePath(QStringLiteral("a.png")), "PNG"));
        QVERIFY(writeImage(QDir(f.mine).filePath(QStringLiteral("b.png")), "PNG"));
        QVERIFY(writeStrokeFragment(QDir(f.mine).filePath(QStringLiteral("c.actcraft"))));
        f.library.setContactActive(true);
        f.library.openFolder(f.mine);
        QTRY_COMPARE(f.library.itemCount(), 3);
        QTest::qWait(500);
        QCOMPARE(f.library.thumbnailsRendered(), 0);
        f.library.setContactActive(false);
        QTRY_COMPARE_WITH_TIMEOUT(f.library.thumbnailsRendered(), 3, 10000);
    }

    // Aucun accès disque sur le fil principal pendant un contact : les éditions de fichiers sont refusées,
    // sans effet.
    void diskEditsRefusedDuringContact()
    {
        Fixture f;
        const QString image = QDir(f.mine).filePath(QStringLiteral("photo.png"));
        QVERIFY(writeImage(image, "PNG"));
        const QString fragment = QDir(f.mine).filePath(QStringLiteral("trait.actcraft"));
        QVERIFY(writeStrokeFragment(fragment));
        const core::ObjectId id = addStroke(f.document, 300, 200);
        f.tools.user().setSelection({id});
        const std::size_t before = f.page().objects.size();
        f.library.setContactActive(true);
        // Un fragment se lit sur le fil principal : insert est refusé pendant le contact, image comprise.
        QVERIFY(!f.library.insert(fragment, QPointF(300, 300)));
        QVERIFY(!f.library.insert(image, QPointF(300, 300)));
        QVERIFY(!f.library.rename(image, QStringLiteral("autre")));
        QVERIFY(QFile::exists(image));
        QVERIFY(!f.library.remove(image));
        QVERIFY(QFile::exists(image));
        QVERIFY(!f.library.createFolder(QStringLiteral("neuf")));
        QVERIFY(!QFileInfo::exists(QDir(f.mine).filePath(QStringLiteral("neuf"))));
        QVERIFY(f.library.addSelection().isEmpty());
        QVERIFY(!QFile::exists(QDir(f.mine).filePath(QStringLiteral("Ressource1.actcraft"))));
        QTest::qWait(300); // aucune insertion différée
        QCOMPARE(f.page().objects.size(), before);
        f.library.setContactActive(false); // relâché : de nouveau permis
        QVERIFY(f.library.insert(fragment, QPointF(300, 300)));
        QCOMPARE(f.page().objects.size(), before + 1);
        QVERIFY(f.library.createFolder(QStringLiteral("neuf")));
        QVERIFY(f.library.rename(image, QStringLiteral("autre")));
    }

    void refusedWhileLocked()
    {
        Fixture f;
        const QString image = QDir(f.mine).filePath(QStringLiteral("photo.png"));
        QVERIFY(writeImage(image, "PNG"));
        const core::ObjectId id = addStroke(f.document, 300, 200);
        f.tools.user().setSelection({id});
        const std::size_t before = f.page().objects.size();
        const QString fragment = QDir(f.mine).filePath(QStringLiteral("trait.actcraft"));
        QVERIFY(writeStrokeFragment(fragment));
        f.commands.setLocked(true);
        QVERIFY(!f.library.insert(image, QPointF(300, 300)));
        QVERIFY(!f.library.insert(fragment, QPointF(300, 300)));
        QVERIFY(f.library.addSelection().isEmpty());
        QTest::qWait(300); // aucune insertion différée
        QCOMPARE(f.page().objects.size(), before);
        QVERIFY(!QFile::exists(QDir(f.mine).filePath(QStringLiteral("Ressource1.actcraft"))));
    }

    void refusedWithoutCommands()
    {
        Fixture f;
        const QString image = QDir(f.mine).filePath(QStringLiteral("photo.png"));
        QVERIFY(writeImage(image, "PNG"));
        const core::ObjectId id = addStroke(f.document, 300, 200);
        f.tools.user().setSelection({id});
        ui::ResourceLibrary bare;
        bare.setDocument(&f.document);
        bare.setProfile(f.profile.get());
        bare.setTools(&f.tools);
        bare.setRoot(QStringLiteral("mine"));
        const std::size_t before = f.page().objects.size();
        QVERIFY(!bare.insert(image, QPointF(300, 300)));
        QCOMPARE(f.page().objects.size(), before);
        QVERIFY(bare.addSelection().isEmpty());
        QVERIFY(!QFile::exists(QDir(f.mine).filePath(QStringLiteral("Ressource1.actcraft"))));
        QVERIFY(!bare.rename(image, QStringLiteral("autre")));
        QVERIFY(QFile::exists(image));
        QVERIFY(!bare.remove(image));
        QVERIFY(QFile::exists(image));
        QVERIFY(!bare.createFolder(QStringLiteral("neuf")));
        QVERIFY(!QFileInfo::exists(QDir(f.mine).filePath(QStringLiteral("neuf"))));
    }

    // Patron de PageThumbnails::pump (une vignette en vol au plus, contact vérifié avant chacune). Le premier
    // rendu est retenu : rien d'autre ne part ; un contact posé APRÈS le lancement arrête la file au retour
    // de celle en vol, jusqu'au relâché.
    void thumbnailsOneAtATimeStoppedByContact()
    {
        std::atomic<int> renders{0};
        QSemaphore gate;
        ui::ResourceLibrary::setRenderObserverForTests(holdFirst(renders, gate));
        Fixture f;
        constexpr int kCount = 12;
        for (int i = 0; i < kCount; ++i)
            QVERIFY(writeImage(QDir(f.mine).filePath(QStringLiteral("i%1.png").arg(i)), "PNG"));
        f.library.openFolder(f.mine);
        QTRY_COMPARE_WITH_TIMEOUT(renders.load(), 1, 10000);
        QTest::qWait(200);
        QCOMPARE(renders.load(), 1); // une seule en vol
        // Relâché puis reposé pendant le vol : rien ne part en plus (la suivante attend le retour).
        f.library.setContactActive(true);
        f.library.setContactActive(false);
        f.library.setContactActive(true);
        gate.release();
        QTRY_COMPARE_WITH_TIMEOUT(f.library.thumbnailsRendered(), 1, 10000); // celle en vol aboutit
        QTest::qWait(300);
        QCOMPARE(renders.load(), 1); // aucune autre pendant le contact
        QCOMPARE(f.library.thumbnailsRendered(), 1);
        f.library.setContactActive(false);
        QTRY_COMPARE_WITH_TIMEOUT(f.library.thumbnailsRendered(), kCount, 10000);
        QCOMPARE(renders.load(), kCount);
    }

    // Dossier quitté pendant un rendu : la vignette en vol est jetée, aucune autre de l'ancien dossier n'est
    // rendue, le nouveau dossier est servi ensuite.
    void thumbnailsOfALeftFolderNotRendered()
    {
        std::atomic<int> renders{0};
        QSemaphore gate;
        ui::ResourceLibrary::setRenderObserverForTests(holdFirst(renders, gate));
        Fixture f;
        for (int i = 0; i < 12; ++i)
            QVERIFY(writeImage(QDir(f.mine).filePath(QStringLiteral("i%1.png").arg(i)), "PNG"));
        const QString next = QDir(f.mine).filePath(QStringLiteral("suivant"));
        const QString only = QDir(next).filePath(QStringLiteral("seule.png"));
        QVERIFY(QDir().mkpath(next));
        QVERIFY(writeImage(only, "PNG"));
        f.library.openFolder(f.mine);
        QTRY_COMPARE_WITH_TIMEOUT(renders.load(), 1, 10000);
        f.library.openFolder(next);
        gate.release();
        QTRY_COMPARE_WITH_TIMEOUT(f.library.thumbnailsRendered(), 1, 10000);
        QCOMPARE(renders.load(), 2); // celle en vol (jetée) et celle du nouveau dossier
        QTest::qWait(300);
        QCOMPARE(renders.load(), 2);
        QCOMPARE(f.paths(), QStringList{only});
        QVERIFY(f.library.data(f.library.index(0), ui::ResourceLibrary::ThumbnailRole).toUrl().isLocalFile());
    }

    // Recherche lancée pendant un rendu (recherche retenue au premier fichier, ses résultats ne peuvent donc
    // pas encore remplacer les éléments) : la vignette en vol est jetée, la file vidée.
    void thumbnailsDroppedWhenASearchStarts()
    {
        std::atomic<int> renders{0};
        std::atomic<int> visited{0};
        QSemaphore renderGate;
        QSemaphore searchGate;
        ui::ResourceLibrary::setRenderObserverForTests(holdFirst(renders, renderGate));
        ui::ResourceLibrary::setSearchObserverForTests(holdFirst(visited, searchGate));
        Fixture f;
        const auto settle = qScopeGuard([&renderGate, &searchGate] {
            renderGate.release();
            searchGate.release();
            QThreadPool::globalInstance()->waitForDone(10000);
        });
        for (int i = 0; i < 12; ++i)
            QVERIFY(writeImage(QDir(f.mine).filePath(QStringLiteral("i%1.png").arg(i)), "PNG"));
        f.library.openFolder(f.mine);
        QTRY_COMPARE_WITH_TIMEOUT(renders.load(), 1, 10000);
        f.library.search(QStringLiteral("absent"), true, false);
        QTRY_COMPARE_WITH_TIMEOUT(visited.load(), 1, 10000);
        renderGate.release();
        QTest::qWait(300);
        QCOMPARE(renders.load(), 1);
        QCOMPARE(f.library.thumbnailsRendered(), 0); // celle en vol, jetée
        searchGate.release();
        QTRY_VERIFY(!f.library.searching());
        QCOMPARE(f.library.itemCount(), 0);
    }

    // Vignette trouvée dans le cache : montrée sans nouveau rendu.
    void thumbnailFoundInCache()
    {
        std::atomic<int> renders{0};
        ui::ResourceLibrary::setRenderObserverForTests([&renders] { ++renders; });
        Fixture f;
        QVERIFY(writeImage(QDir(f.mine).filePath(QStringLiteral("a.png")), "PNG"));
        QVERIFY(writeStrokeFragment(QDir(f.mine).filePath(QStringLiteral("b.actcraft"))));
        f.library.openFolder(f.mine);
        QTRY_COMPARE_WITH_TIMEOUT(f.library.thumbnailsRendered(), 2, 10000);
        QCOMPARE(renders.load(), 2);
        ui::ResourceLibrary second;
        second.setProfile(f.profile.get());
        second.setRoot(QStringLiteral("mine"));
        QTRY_COMPARE(second.itemCount(), 2);
        for (int row = 0; row < 2; ++row) {
            QTRY_VERIFY(
                second.data(second.index(row), ui::ResourceLibrary::ThumbnailRole).toUrl().isLocalFile());
            QCOMPARE(second.data(second.index(row), ui::ResourceLibrary::ThumbnailRole),
                     f.library.data(f.library.index(row), ui::ResourceLibrary::ThumbnailRole));
        }
        QCOMPARE(renders.load(), 2);
        QCOMPARE(second.thumbnailsRendered(), 0);
    }

    // Un dossier relu pendant une recherche (ajout, renommage) la termine : searching faux tout de suite, le
    // dossier affiché, le résultat abandonné jamais appliqué.
    void rereadEndsTheSearch()
    {
        Fixture f;
        const QString a = QDir(f.mine).filePath(QStringLiteral("a.png"));
        QVERIFY(writeImage(a, "PNG"));
        QVERIFY(writeImage(QDir(f.mine).filePath(QStringLiteral("b.png")), "PNG"));
        f.library.openFolder(f.mine);
        QTRY_COMPARE(f.library.itemCount(), 2);
        const core::ObjectId id = addStroke(f.document, 300, 200);
        f.tools.user().setSelection({id});

        f.library.search(QStringLiteral("a"), true, false);
        QVERIFY(f.library.searching());
        const QString added = f.library.addSelection();
        QVERIFY(!added.isEmpty());
        QVERIFY(!f.library.searching());
        QTRY_COMPARE(f.library.itemCount(), 3);
        QVERIFY(f.paths().contains(added));

        f.library.search(QStringLiteral("a"), true, false);
        QVERIFY(f.library.searching());
        QVERIFY(f.library.rename(a, QStringLiteral("z")));
        QVERIFY(!f.library.searching());
        QTRY_VERIFY(f.paths().contains(QDir(f.mine).filePath(QStringLiteral("z.png"))));
        QTest::qWait(300);
        QVERIFY(!f.library.searching());
        QCOMPARE(f.library.itemCount(), 3);
    }

    // cancelSearch après une recherche finie : retour au dossier (resourcelibrary.h, cancelSearch).
    void cancelAfterTheSearchRestoresTheFolder()
    {
        Fixture f;
        for (const char* name : {"rouge.png", "vert.png", "bleu.png"})
            QVERIFY(writeImage(QDir(f.mine).filePath(QString::fromLatin1(name)), "PNG"));
        f.library.openFolder(f.mine);
        QTRY_COMPARE(f.library.itemCount(), 3);
        f.library.search(QStringLiteral("rouge"), true, false);
        QTRY_VERIFY(!f.library.searching());
        QCOMPARE(f.library.itemCount(), 1);
        f.library.cancelSearch();
        QTRY_COMPARE(f.library.itemCount(), 3);
    }

    // Dossier courant (ou un de ses parents) renommé ou supprimé : le dossier courant passe au parent de
    // l'élément touché ; l'ancien chemin n'est jamais recréé.
    void currentFolderRenamedOrRemoved()
    {
        Fixture f;
        const QString x = QDir(f.mine).filePath(QStringLiteral("x"));
        const QString inner = QDir(x).filePath(QStringLiteral("dedans"));
        const QString near = QDir(f.mine).filePath(QStringLiteral("xy")); // même début de nom, épargné
        QVERIFY(QDir().mkpath(inner));
        QVERIFY(QDir().mkpath(near));
        const core::ObjectId id = addStroke(f.document, 300, 200);
        f.tools.user().setSelection({id});
        f.library.openFolder(near);
        QVERIFY(f.library.rename(x, QStringLiteral("w")));
        QCOMPARE(f.library.folder(), near);
        const QString w = QDir(f.mine).filePath(QStringLiteral("w"));
        f.library.openFolder(QDir(w).filePath(QStringLiteral("dedans")));
        QVERIFY(f.library.rename(w, QStringLiteral("y")));
        QCOMPARE(f.library.folder(), f.mine);
        QCOMPARE(f.library.addSelection(), QDir(f.mine).filePath(QStringLiteral("Ressource1.actcraft")));
        QVERIFY(!QFileInfo::exists(w));
        const QString y = QDir(f.mine).filePath(QStringLiteral("y"));
        f.library.openFolder(y);
        QVERIFY(f.library.remove(y));
        QCOMPARE(f.library.folder(), f.mine);
        QCOMPARE(f.library.addSelection(), QDir(f.mine).filePath(QStringLiteral("Ressource2.actcraft")));
        QVERIFY(!QFileInfo::exists(y));
        QVERIFY(!f.library.remove(f.mine)); // la racine elle-même : refusée
        QVERIFY(QFileInfo(f.mine).isDir());
    }

    // Corbeille en échec : remove rend faux et le dit (trashFailed) ; le QML peut alors proposer la
    // suppression définitive.
    void trashFailureReported()
    {
        Fixture f;
        const QString image = QDir(f.mine).filePath(QStringLiteral("photo.png"));
        QVERIFY(writeImage(image, "PNG"));
        ui::ResourceLibrary::setTrashForTests(true,
                                              true); // corbeille simulée en échec, jamais celle du poste
        QSignalSpy failed(&f.library, &ui::ResourceLibrary::trashFailed);
        QVERIFY(!f.library.remove(image));
        QVERIFY(QFile::exists(image));
        QCOMPARE(failed.count(), 1);
        QCOMPARE(failed.at(0).at(0).toString(), image);
        QVERIFY(f.library.remove(image, true));
        QVERIFY(!QFile::exists(image));
        ui::ResourceLibrary::setTrashForTests(false);
    }
};

QTEST_MAIN(TstUiResources)
#include "tst_ui_resources.moc"
