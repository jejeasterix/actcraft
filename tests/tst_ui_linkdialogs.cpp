// Dialogues de liens (lot 8, L8-T22) : création d'un lien vers un fichier ou un site, en une étape, avec les
// règles d'ouverture de LinkLauncher ; aucune ouverture sans confirmation, aucun argument passé.
#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/core/replace.h>
#include <actcraft/ui/actionrunner.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/designmode.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/linkdialogs.h>
#include <actcraft/ui/linklauncher.h>

#include <QCryptographicHash>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSemaphore>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QThread>
#include <QtQml/qqmlextensionplugin.h>
#include <QtTest>

#include <atomic>
#include <memory>
#include <variant>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

constexpr core::ObjectId kExisting = 7;

QJsonObject jsonOf(const std::string& text)
{
    return QJsonDocument::fromJson(QByteArray::fromStdString(text)).object();
}

} // namespace

class TstUiLinkdialogs : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void textFormCreatesATextObject();
    void existingObjectGetsTheAction();
    void storedFileBecomesAResource();
    void externalPathKeptAsIs();
    void commandLineArgumentsNeverPassed();
    void websiteNeedsHttpOrHttps();
    void oneStep();
    void fileRefusedWithTheLaunchersReason();
    void closedWithoutSourcesOrUnderLock();
    void iconAndActionFormsCreateObjects();
    void storedFileIsReadOffTheUiThread();
    void redoDoesNotStoreTheResourceAgain();
    void resultDroppedWhenTheDocumentIsReplaced();
    void resultDroppedWhenTheObjectIsGone();
    void localPathKeepsUrlSyntaxCharacters();

private:
    QString makeFile(const QString& name, const QByteArray& bytes) const;
    const core::Page& page() const { return m_controller->document().page(0); }
    static const core::Action& onlyAction(const core::PageObject& o) { return o.common.actions.back(); }
    void load();
    void click(QPointF point);

    QTemporaryDir m_settings;
    QTemporaryDir m_dir;
    std::unique_ptr<ui::DocumentController> m_controller;
    std::unique_ptr<ui::Commands> m_commands;
    std::unique_ptr<ui::DesignMode> m_design;
    std::unique_ptr<ui::LinkLauncher> m_links;
    std::unique_ptr<ui::ActionRunner> m_runner;
    std::unique_ptr<ui::LinkDialogs> m_dialogs;
    QList<QUrl> m_opened;
};

void TstUiLinkdialogs::initTestCase()
{
    QVERIFY(m_settings.isValid());
    QVERIFY(m_dir.isValid());
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings.path());
    QStandardPaths::setTestModeEnabled(true);
    QSettings probe;
    QCOMPARE(probe.format(), QSettings::IniFormat);
    QVERIFY(probe.fileName().startsWith(m_settings.path()));
}

void TstUiLinkdialogs::init()
{
    m_controller = std::make_unique<ui::DocumentController>();
    m_commands = std::make_unique<ui::Commands>();
    m_design = std::make_unique<ui::DesignMode>();
    m_links = std::make_unique<ui::LinkLauncher>();
    m_opened.clear();
    m_links->setOpener([this](const QUrl& url) {
        m_opened << url;
        return true;
    });
    m_runner = std::make_unique<ui::ActionRunner>();
    m_runner->setCommands(m_commands.get());
    m_runner->setDocument(m_controller.get());
    m_runner->setDesignMode(m_design.get());
    m_runner->setLinks(m_links.get());
    m_dialogs = std::make_unique<ui::LinkDialogs>();
    m_dialogs->setCommands(m_commands.get());
    m_dialogs->setDocument(m_controller.get());
    m_dialogs->setLinks(m_links.get());
    load();
}

void TstUiLinkdialogs::cleanup()
{
    ui::LinkDialogs::setReadProbe({});
    m_dialogs.reset();
    m_runner.reset();
    m_links.reset();
    m_design.reset();
    m_commands.reset();
    m_controller.reset();
}

// Une page de 1024 × 768 avec un rectangle 7 qui porte déjà une action.
void TstUiLinkdialogs::load()
{
    core::Document document(1024, 768);
    core::Shape shape;
    shape.kind = core::ShapeKind::Rectangle;
    shape.bounds = core::RectF{100, 100, 100, 100};
    core::PageObject rect{shape};
    rect.common.id = kExisting;
    core::Action before;
    before.catalogId = "object.toggle-hidden";
    before.parameters = "{}";
    rect.common.actions.push_back(before);
    document.page(0).objects.push_back(rect);
    document.reserveIdsBelow(kExisting + 1);
    m_controller->replaceDocument(std::move(document),
                                  QUrl::fromLocalFile(m_dir.filePath(QStringLiteral("essai.actcraft"))),
                                  QStringLiteral("essai"));
}

QString TstUiLinkdialogs::makeFile(const QString& name, const QByteArray& bytes) const
{
    const QString path = m_dir.filePath(name);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size())
        return {};
    return path;
}

void TstUiLinkdialogs::click(QPointF point)
{
    m_runner->setContactActive(true);
    m_runner->press(point, Qt::LeftButton);
    m_runner->release(point);
    m_runner->setContactActive(false);
    QCoreApplication::processEvents();
}

void TstUiLinkdialogs::textFormCreatesATextObject()
{
    QSignalSpy created(m_dialogs.get(), &ui::LinkDialogs::created);
    QVERIFY(
        m_dialogs->createWebsiteLink(QStringLiteral("https://example.org/"), ui::LinkDialogs::TextForm, 0));
    QCOMPARE(created.size(), 1);
    QCOMPARE(page().objects.size(), size_t(2));
    const core::PageObject& made = page().objects.back();
    QCOMPARE(made.common.id, created.first().first().toULongLong());
    const auto* text = std::get_if<core::Text>(&core::body(made));
    QVERIFY(text != nullptr);
    QVERIFY(QString::fromStdString(text->html).contains(QStringLiteral("example.org")));
    QCOMPARE(made.common.actions.size(), size_t(1));
    QCOMPARE(onlyAction(made).catalogId, std::string("document.open-website"));
    QCOMPARE(jsonOf(onlyAction(made).parameters).value(QStringLiteral("url")).toString(),
             QStringLiteral("https://example.org/"));
}

void TstUiLinkdialogs::existingObjectGetsTheAction()
{
    QVERIFY(m_dialogs->createWebsiteLink(QStringLiteral("https://example.org/"),
                                         ui::LinkDialogs::ExistingForm, kExisting));
    QCOMPARE(page().objects.size(), size_t(1)); // aucun objet neuf
    const core::PageObject& rect = page().objects.front();
    QCOMPARE(rect.common.actions.size(), size_t(2));
    QCOMPARE(rect.common.actions.front().catalogId, std::string("object.toggle-hidden")); // gardée
    QCOMPARE(onlyAction(rect).catalogId, std::string("document.open-website"));
    // Un objet absent de la page : refus, rien ne change.
    QSignalSpy refused(m_dialogs.get(), &ui::LinkDialogs::refused);
    QVERIFY(!m_dialogs->createWebsiteLink(QStringLiteral("https://example.org/"),
                                          ui::LinkDialogs::ExistingForm, 999));
    QCOMPARE(refused.size(), 1);
    m_controller->undo();
    QCOMPARE(page().objects.front().common.actions.size(), size_t(1));
}

void TstUiLinkdialogs::storedFileBecomesAResource()
{
    const QByteArray bytes("%PDF-1.4 contenu d'essai");
    const QString path = makeFile(QStringLiteral("cours.pdf"), bytes);
    QVERIFY(!path.isEmpty());
    QVERIFY(m_controller->document().resources().empty());
    QSignalSpy storedCreated(m_dialogs.get(), &ui::LinkDialogs::created);
    QVERIFY(m_dialogs->createFileLink(path, ui::LinkDialogs::TextForm, ui::LinkDialogs::DocumentStorage, 0));
    // Le fichier rangé est lu hors du fil de l'interface : le lien arrive au retour (L8-C8).
    QTRY_COMPARE_WITH_TIMEOUT(storedCreated.size(), 1, 10000);
    QCOMPARE(m_controller->document().resources().size(), size_t(1));
    const core::PageObject& made = page().objects.back();
    const core::Action& action = onlyAction(made);
    QCOMPARE(action.catalogId, std::string("document.open-file"));
    const core::Resource* stored = m_controller->document().resource(action.resource);
    QVERIFY(stored != nullptr);
    QCOMPARE(stored->data, bytes);
    QCOMPARE(stored->extension, std::string("pdf"));
    // Le document ne garde que le nom, jamais le dossier de l'auteur.
    const QString file = jsonOf(action.parameters).value(QStringLiteral("file")).toString();
    QCOMPARE(file, QStringLiteral("cours.pdf"));
    // Le clic demande une confirmation sur une copie extraite : rien ne s'ouvre.
    QSignalSpy ask(m_links.get(), &ui::LinkLauncher::confirmationNeeded);
    const auto* text = std::get_if<core::Text>(&core::body(made));
    QVERIFY(text != nullptr);
    click(QPointF(text->bounds.x + 2, text->bounds.y + 2));
    QCOMPARE(ask.size(), 1);
    QVERIFY(m_opened.isEmpty());
}

void TstUiLinkdialogs::externalPathKeptAsIs()
{
    const QString path = makeFile(QStringLiteral("fiche.txt"), "bonjour");
    QVERIFY(!path.isEmpty());
    QVERIFY(m_dialogs->createFileLink(path, ui::LinkDialogs::TextForm, ui::LinkDialogs::ExternalStorage, 0));
    QVERIFY(m_controller->document().resources().empty());
    const core::Action& action = onlyAction(page().objects.back());
    QCOMPARE(action.catalogId, std::string("document.open-file"));
    QVERIFY(action.resource.empty());
    QCOMPARE(jsonOf(action.parameters).value(QStringLiteral("file")).toString(), path);
}

void TstUiLinkdialogs::commandLineArgumentsNeverPassed()
{
    const QString path = makeFile(QStringLiteral("fiche.txt"), "bonjour");
    QVERIFY(!path.isEmpty());
    m_dialogs->setArguments(QStringLiteral("--evil /c calc"));
    QVERIFY(m_dialogs->createFileLink(path, ui::LinkDialogs::TextForm, ui::LinkDialogs::ExternalStorage, 0));
    const core::PageObject& made = page().objects.back();
    // Gardés dans les paramètres de l'action…
    QCOMPARE(jsonOf(onlyAction(made).parameters).value(QStringLiteral("arguments")).toString(),
             QStringLiteral("--evil /c calc"));
    // … et jamais passés : le faux ouvreur reçoit le seul fichier.
    QSignalSpy ask(m_links.get(), &ui::LinkLauncher::confirmationNeeded);
    const auto* text = std::get_if<core::Text>(&core::body(made));
    QVERIFY(text != nullptr);
    click(QPointF(text->bounds.x + 2, text->bounds.y + 2));
    QCOMPARE(ask.size(), 1);
    QVERIFY(m_opened.isEmpty());
    m_links->answer(ask.first().at(1).toInt(), true);
    QCOMPARE(m_opened.size(), 1);
    QCOMPARE(m_opened.first(), QUrl::fromLocalFile(path));
    QVERIFY(!m_opened.first().toString().contains(QStringLiteral("evil")));
}

void TstUiLinkdialogs::websiteNeedsHttpOrHttps()
{
    QSignalSpy refused(m_dialogs.get(), &ui::LinkDialogs::refused);
    const QStringList bad{QStringLiteral("ftp://example.org/"),
                          QStringLiteral("javascript:alert(1)"),
                          QStringLiteral("file:///etc/passwd"),
                          QStringLiteral("example.org"),
                          QStringLiteral("https://ecole.fr@evil.example/"),
                          QString()};
    for (const QString& address : bad) {
        refused.clear();
        QVERIFY2(!m_dialogs->createWebsiteLink(address, ui::LinkDialogs::TextForm, 0), qPrintable(address));
        QCOMPARE(refused.size(), 1);
        // Même motif que celui de l'ouverture.
        QCOMPARE(refused.first().first().toString(),
                 ui::LinkLauncher::refusalFor(QUrl(address.trimmed(), QUrl::StrictMode),
                                              ui::LinkLauncher::Website));
    }
    QCOMPARE(page().objects.size(), size_t(1));
    QVERIFY(!m_controller->canUndo());
    QVERIFY(m_dialogs->createWebsiteLink(QStringLiteral("http://example.org/a?b=1"),
                                         ui::LinkDialogs::TextForm, 0));
    QVERIFY(m_dialogs->createWebsiteLink(QStringLiteral("  https://example.org/ "), ui::LinkDialogs::TextForm,
                                         0));
}

void TstUiLinkdialogs::oneStep()
{
    const QString path = makeFile(QStringLiteral("cours.pdf"), "%PDF-1.4");
    QVERIFY(!path.isEmpty());
    QVERIFY(!m_controller->canUndo());
    QSignalSpy storedCreated(m_dialogs.get(), &ui::LinkDialogs::created);
    QVERIFY(m_dialogs->createFileLink(path, ui::LinkDialogs::TextForm, ui::LinkDialogs::DocumentStorage, 0));
    QTRY_COMPARE_WITH_TIMEOUT(storedCreated.size(), 1, 10000); // lecture hors du fil (L8-C8)
    QVERIFY(m_controller->canUndo());
    m_controller->undo(); // une seule annulation défait objet et lien
    QCOMPARE(page().objects.size(), size_t(1));
    QVERIFY(!m_controller->canUndo());
    m_controller->redo();
    QCOMPARE(page().objects.size(), size_t(2));
    QCOMPARE(m_controller->document().resources().size(), size_t(1));
}

void TstUiLinkdialogs::fileRefusedWithTheLaunchersReason()
{
    QSignalSpy refused(m_dialogs.get(), &ui::LinkDialogs::refused);
    const QString exe = makeFile(QStringLiteral("outil.exe"), "MZ");
    const QString missing = m_dir.filePath(QStringLiteral("absent.pdf"));
    const QStringList paths{exe, missing, QStringLiteral("//serveur/partage/doc.pdf")};
    for (const QString& path : paths) {
        for (const int storage :
             {int(ui::LinkDialogs::ExternalStorage), int(ui::LinkDialogs::DocumentStorage)}) {
            refused.clear();
            const QUrl url = QUrl::fromLocalFile(path);
            QVERIFY2(!m_dialogs->createFileLink(path, ui::LinkDialogs::TextForm, storage, 0),
                     qPrintable(path));
            QCOMPARE(refused.size(), 1);
            const int kind = storage == ui::LinkDialogs::ExternalStorage ? int(ui::LinkLauncher::File)
                                                                         : int(ui::LinkLauncher::StoredFile);
            QCOMPARE(refused.first().first().toString(), ui::LinkLauncher::refusalFor(url, kind));
        }
    }
    QCOMPARE(page().objects.size(), size_t(1));
    QVERIFY(m_controller->document().resources().empty());
    QVERIFY(!m_controller->canUndo());
}

void TstUiLinkdialogs::closedWithoutSourcesOrUnderLock()
{
    QSignalSpy refused(m_dialogs.get(), &ui::LinkDialogs::refused);
    m_commands->setLocked(true); // verrou de classe
    QVERIFY(
        !m_dialogs->createWebsiteLink(QStringLiteral("https://example.org/"), ui::LinkDialogs::TextForm, 0));
    QCOMPARE(refused.size(), 1);
    m_commands->setLocked(false);
    m_dialogs->setLinks(nullptr);
    QVERIFY(
        !m_dialogs->createWebsiteLink(QStringLiteral("https://example.org/"), ui::LinkDialogs::TextForm, 0));
    QCOMPARE(refused.size(), 2);
    m_dialogs->setLinks(m_links.get());
    m_dialogs->setDocument(nullptr);
    QVERIFY(
        !m_dialogs->createWebsiteLink(QStringLiteral("https://example.org/"), ui::LinkDialogs::TextForm, 0));
    QCOMPARE(refused.size(), 3);
    QCOMPARE(page().objects.size(), size_t(1));
}

void TstUiLinkdialogs::iconAndActionFormsCreateObjects()
{
    QVERIFY(
        m_dialogs->createWebsiteLink(QStringLiteral("https://example.org/"), ui::LinkDialogs::IconForm, 0));
    QVERIFY(std::holds_alternative<core::ImageObject>(core::body(page().objects.back())));
    QCOMPARE(onlyAction(page().objects.back()).catalogId, std::string("document.open-website"));
    QVERIFY(
        m_dialogs->createWebsiteLink(QStringLiteral("https://example.org/"), ui::LinkDialogs::ActionForm, 0));
    QVERIFY(std::holds_alternative<core::Shape>(core::body(page().objects.back())));
    QCOMPARE(onlyAction(page().objects.back()).catalogId, std::string("document.open-website"));
    QCOMPARE(page().objects.size(), size_t(3));
}

// Le patron écarté lit le fichier dans l'appel (fil de l'interface) et pose le lien avant de rendre la main.
// Ici la sonde bloque le fil de lecture : l'appel doit rendre la main pendant ce blocage, rien n'est encore
// posé, puis le lien arrive une fois la lecture finie, lue ailleurs que sur le fil de l'interface.
void TstUiLinkdialogs::storedFileIsReadOffTheUiThread()
{
    const QString path = makeFile(QStringLiteral("cours.pdf"), "%PDF-1.4 contenu d'essai");
    QVERIFY(!path.isEmpty());
    QSemaphore release;
    std::atomic<QThread*> readThread{nullptr};
    ui::LinkDialogs::setReadProbe([&release, &readThread] {
        readThread = QThread::currentThread();
        release.tryAcquire(1, 2000); // borné : un fil bloqué à tort ne gèle pas la suite
    });
    QSignalSpy created(m_dialogs.get(), &ui::LinkDialogs::created);
    QVERIFY(m_dialogs->createFileLink(path, ui::LinkDialogs::TextForm, ui::LinkDialogs::DocumentStorage, 0));
    QCOMPARE(created.size(), 0);
    QVERIFY(m_dialogs->busy());
    QCOMPARE(page().objects.size(), size_t(1));
    QVERIFY(!m_controller->canUndo());
    QVERIFY(m_controller->document().resources().empty());
    release.release();
    QTRY_COMPARE_WITH_TIMEOUT(created.size(), 1, 10000);
    QVERIFY(!m_dialogs->busy());
    QVERIFY(readThread.load() != nullptr);
    QVERIFY(readThread.load() != QThread::currentThread());
    QCOMPARE(m_controller->document().resources().size(), size_t(1));
}

// Le patron écarté range les octets à chaque redo, donc recalcule leur SHA-256 à chaque rejeu.
void TstUiLinkdialogs::redoDoesNotStoreTheResourceAgain()
{
    const QByteArray bytes("%PDF-1.4 contenu d'essai");
    const QString path = makeFile(QStringLiteral("cours.pdf"), bytes);
    QVERIFY(!path.isEmpty());
    const int before = ui::LinkDialogs::resourceInsertions();
    QSignalSpy created(m_dialogs.get(), &ui::LinkDialogs::created);
    QVERIFY(m_dialogs->createFileLink(path, ui::LinkDialogs::TextForm, ui::LinkDialogs::DocumentStorage, 0));
    QTRY_COMPARE_WITH_TIMEOUT(created.size(), 1, 10000);
    const std::string key = onlyAction(page().objects.back()).resource;
    QCOMPARE(QString::fromStdString(key),
             QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex()));
    for (int round = 0; round < 2; ++round) {
        m_controller->undo();
        QCOMPARE(page().objects.size(), size_t(1));
        m_controller->redo();
        QCOMPARE(page().objects.size(), size_t(2));
        QCOMPARE(onlyAction(page().objects.back()).resource, key); // la même clé à chaque rejeu
    }
    QCOMPARE(m_controller->document().resources().size(), size_t(1));
    QCOMPARE(ui::LinkDialogs::resourceInsertions() - before, 1); // rangée une fois, jamais par redo
}

// Document remplacé pendant la lecture (Nouveau, Ouvrir) : le résultat est jeté, rien n'est posé dans le
// document qui est maintenant à l'écran.
void TstUiLinkdialogs::resultDroppedWhenTheDocumentIsReplaced()
{
    const QString path = makeFile(QStringLiteral("cours.pdf"), "%PDF-1.4");
    QVERIFY(!path.isEmpty());
    QSemaphore release;
    ui::LinkDialogs::setReadProbe([&release] { release.tryAcquire(1, 2000); });
    QSignalSpy created(m_dialogs.get(), &ui::LinkDialogs::created);
    QSignalSpy refused(m_dialogs.get(), &ui::LinkDialogs::refused);
    QVERIFY(m_dialogs->createFileLink(path, ui::LinkDialogs::TextForm, ui::LinkDialogs::DocumentStorage, 0));
    QVERIFY(m_dialogs->busy());
    load(); // un autre document prend la place pendant la lecture
    release.release();
    QTRY_VERIFY_WITH_TIMEOUT(!m_dialogs->busy(), 10000);
    QCOMPARE(created.size(), 0);
    QCOMPARE(refused.size(), 0);
    QCOMPARE(page().objects.size(), size_t(1));
    QVERIFY(m_controller->document().resources().empty());
    QVERIFY(!m_controller->canUndo());
}

// Objet visé supprimé pendant la lecture : aucune ressource orpheline, aucune étape d'annulation.
void TstUiLinkdialogs::resultDroppedWhenTheObjectIsGone()
{
    const QString path = makeFile(QStringLiteral("cours.pdf"), "%PDF-1.4");
    QVERIFY(!path.isEmpty());
    QSemaphore release;
    ui::LinkDialogs::setReadProbe([&release] { release.tryAcquire(1, 2000); });
    QSignalSpy created(m_dialogs.get(), &ui::LinkDialogs::created);
    QVERIFY(m_dialogs->createFileLink(path, ui::LinkDialogs::ExistingForm, ui::LinkDialogs::DocumentStorage,
                                      kExisting));
    QVERIFY(m_dialogs->busy());
    m_controller->execute(std::make_unique<core::ReplaceObjectsCommand>(
        0, std::vector<core::ObjectId>{kExisting}, std::vector<core::PageObject>{}));
    QCOMPARE(page().objects.size(), size_t(0));
    release.release();
    QTRY_VERIFY_WITH_TIMEOUT(!m_dialogs->busy(), 10000);
    QCOMPARE(created.size(), 0);
    QCOMPARE(page().objects.size(), size_t(0));
    QVERIFY(m_controller->document().resources().empty());
    m_controller->undo(); // l'étape de suppression, seule de la pile
    QCOMPARE(page().objects.size(), size_t(1));
    QCOMPARE(page().objects.front().common.actions.size(), size_t(1));
}

// « Parcourir… » rend une URL de fichier, le champ garde un chemin : `#`, `?` et `%` sont des caractères du
// nom. Le patron écarté rendait le texte de l'URL (`file:///…a%23b`), échappé, qui n'est pas ce fichier.
void TstUiLinkdialogs::localPathKeepsUrlSyntaxCharacters()
{
    const QStringList names{QStringLiteral("a#b.txt"), QStringLiteral("c%23d.txt"), QStringLiteral("e?f.txt"),
                            QStringLiteral("100%.txt")};
    QSignalSpy created(m_dialogs.get(), &ui::LinkDialogs::created);
    for (const QString& name : names) {
        const QString path = makeFile(name, "bonjour");
        QVERIFY2(!path.isEmpty(), qPrintable(name));
        const QString chosen = m_dialogs->localPath(QUrl::fromLocalFile(path)); // le rendu du sélecteur
        QCOMPARE(chosen, path);
        QVERIFY2(
            m_dialogs->createFileLink(chosen, ui::LinkDialogs::TextForm, ui::LinkDialogs::ExternalStorage, 0),
            qPrintable(name));
        QCOMPARE(
            jsonOf(onlyAction(page().objects.back()).parameters).value(QStringLiteral("file")).toString(),
            path);
        // Rangé : le contenu est celui du fichier de ce nom, le document ne garde que le nom.
        created.clear();
        QVERIFY2(
            m_dialogs->createFileLink(chosen, ui::LinkDialogs::TextForm, ui::LinkDialogs::DocumentStorage, 0),
            qPrintable(name));
        QTRY_COMPARE_WITH_TIMEOUT(created.size(), 1, 10000);
        const core::Action& stored = onlyAction(page().objects.back());
        QCOMPARE(jsonOf(stored.parameters).value(QStringLiteral("file")).toString(), name);
        const core::Resource* resource = m_controller->document().resource(stored.resource);
        QVERIFY2(resource != nullptr, qPrintable(name));
        QCOMPARE(resource->data, QByteArray("bonjour"));
    }
}

QTEST_MAIN(TstUiLinkdialogs)
#include "tst_ui_linkdialogs.moc"
