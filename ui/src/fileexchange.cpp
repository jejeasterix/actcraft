#include <actcraft/export/officeconvert.h>
#include <actcraft/export/pageexport.h>
#include <actcraft/export/pdfexport.h>
#include <actcraft/export/pdfimport.h>
#include <actcraft/ui/fileexchange.h>

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QTemporaryFile>

namespace actcraft::ui {

namespace {

// Un chemin donné en texte par une commande (« C:/x.pdf » devient un schéma « c » pour QUrl) devient une URL
// de fichier ; une URL à schéma (file:, content:) reste telle quelle.
QUrl localized(const QUrl& url)
{
    if (url.isEmpty() || url.scheme().size() > 1) {
        return url;
    }
    return QUrl::fromLocalFile(QDir::cleanPath(QDir::current().absoluteFilePath(url.toString())));
}

QString fileName(const QUrl& url)
{
    return QFileInfo(url.isLocalFile() ? url.toLocalFile() : url.path()).fileName();
}

// Suit un travail du module export : `done` est appelé sur le fil principal avec son résultat, puis
// l'observateur disparaît.
template <typename Result, typename Done> void watch(QObject* owner, QFuture<Result> future, Done done)
{
    auto* watcher = new QFutureWatcher<Result>(owner);
    QObject::connect(watcher, &QFutureWatcherBase::finished, owner, [watcher, done]() {
        Result result = watcher->result();
        watcher->deleteLater();
        done(std::move(result));
    });
    watcher->setFuture(future);
}

// Installe un document importé dans un onglet neuf. Faux si le modèle refuse de basculer (opération en cours).
bool installInNewTab(DocumentTabs* tabs, DocumentController* document, core::Document&& imported,
                     const QString& title)
{
    const int before = tabs->rowCount();
    tabs->newTab();
    if (tabs->rowCount() == before) {
        return false;
    }
    document->replaceDocument(std::move(imported), QUrl(), title);
    return true;
}

} // namespace

FileExchange::FileExchange(QObject* parent) : QObject(parent) {}

DocumentController* FileExchange::document() const
{
    return m_document;
}

void FileExchange::setDocument(DocumentController* document)
{
    if (m_document == document) {
        return;
    }
    m_document = document;
    emit documentChanged();
}

DocumentTabs* FileExchange::tabs() const
{
    return m_tabs;
}

void FileExchange::setTabs(DocumentTabs* tabs)
{
    if (m_tabs == tabs) {
        return;
    }
    m_tabs = tabs;
    emit tabsChanged();
}

bool FileExchange::isBusy() const
{
    return m_busy;
}

void FileExchange::setBusy(bool busy)
{
    if (m_busy != busy) {
        m_busy = busy;
        emit busyChanged();
    }
}

void FileExchange::fail(const QString& message)
{
    emit failed(message);
}

bool FileExchange::begin()
{
    if (!m_document) {
        fail(tr("Aucun paperboard à traiter."));
        return false;
    }
    if (m_busy) {
        fail(tr("Une autre opération de fichier est en cours."));
        return false;
    }
    setBusy(true);
    return true;
}

// Vrai si le module d'import est construit avec Qt PDF (L5-T24).
bool FileExchange::pdfImportAvailable() const
{
    return exporting::pdfImportAvailable();
}

// Vrai si un convertisseur de présentations existe (LibreOffice, PowerPoint ou la variable de test) et que
// l'import PDF est construit : la présentation passe par un PDF.
bool FileExchange::powerpointAvailable() const
{
    return exporting::pdfImportAvailable() && exporting::findConverter().has_value();
}

// La copie du document est prise ici, sur le fil principal ; le rendu et l'écriture tournent sur un fil
// secondaire (jamais le cache de la vue).
void FileExchange::exportPdf(const QUrl& url)
{
    if (!begin()) {
        return;
    }
    const QUrl target = localized(url);
    watch(this, exporting::exportPdf(m_document->document(), target),
          [this, target](exporting::ExportResult r) {
              setBusy(false);
              if (r.ok) {
                  emit finished(tr("PDF enregistré : %1").arg(fileName(target)));
              } else {
                  fail(tr("Export PDF impossible : %1").arg(r.error));
              }
          });
}

void FileExchange::exportPageImage(const QUrl& url)
{
    if (!begin()) {
        return;
    }
    const QUrl target = localized(url);
    const auto page = static_cast<std::size_t>(m_document->currentIndex());
    watch(this, exporting::exportPageImage(m_document->document(), page, target),
          [this, target](exporting::ExportResult r) {
              setBusy(false);
              if (r.ok) {
                  emit finished(tr("Image enregistrée : %1").arg(fileName(target)));
              } else {
                  fail(tr("Export de la page impossible : %1").arg(r.error));
              }
          });
}

// Le PDF est lu sur un fil secondaire ; le document rendu s'installe dans un onglet neuf (L5-T20), jamais
// dans l'onglet affiché.
void FileExchange::importPdf(const QUrl& url)
{
    if (!exporting::pdfImportAvailable()) {
        fail(tr("L'import PDF n'est pas disponible dans cette version."));
        return;
    }
    if (!m_tabs) {
        fail(tr("Aucun onglet pour recevoir le PDF."));
        return;
    }
    if (!begin()) {
        return;
    }
    const QUrl source = localized(url);
    watch(this, exporting::importPdf(source), [this, source](exporting::ImportResult r) {
        setBusy(false);
        if (!r.ok) {
            fail(tr("Import PDF impossible : %1").arg(r.error));
            return;
        }
        const QString title =
            QFileInfo(source.isLocalFile() ? source.toLocalFile() : source.path()).completeBaseName();
        if (!installInNewTab(m_tabs, m_document, std::move(r.document), title)) {
            fail(tr("Import PDF impossible : un autre fichier est en cours de lecture."));
            return;
        }
        emit finished(tr("PDF importé : %1").arg(fileName(source)));
    });
}

// Présentation -> PDF par le convertisseur (fil dédié du module export), puis le PDF est lu comme par
// importPdf et s'installe dans un onglet neuf. Le dossier temporaire du PDF converti est supprimé après
// l'import, succès ou échec.
void FileExchange::importPowerpointImages(const QUrl& url)
{
    if (!exporting::pdfImportAvailable()) {
        fail(tr("L'import de présentations n'est pas disponible dans cette version."));
        return;
    }
    const std::optional<exporting::Converter> converter = exporting::findConverter();
    if (!converter) {
        fail(tr("Aucun convertisseur de présentations n'est installé (LibreOffice ou PowerPoint)."));
        return;
    }
    if (!m_tabs) {
        fail(tr("Aucun onglet pour recevoir la présentation."));
        return;
    }
    const QUrl source = localized(url);
    if (!source.isLocalFile()) {
        fail(tr("Import de la présentation impossible : seul un fichier local est accepté."));
        return;
    }
    if (!begin()) {
        return;
    }
    const QString path = source.toLocalFile();
    watch(this, exporting::convertToPdf(*converter, path), [this, source](exporting::ConvertResult c) {
        if (!c.ok) {
            setBusy(false);
            fail(tr("Import de la présentation impossible : %1").arg(c.error));
            return;
        }
        const QString folder = QFileInfo(c.pdfPath).absolutePath();
        watch(this, exporting::importPdf(QUrl::fromLocalFile(c.pdfPath)),
              [this, source, folder](exporting::ImportResult r) {
                  QDir(folder).removeRecursively();
                  setBusy(false);
                  if (!r.ok) {
                      fail(tr("Import de la présentation impossible : %1").arg(r.error));
                      return;
                  }
                  if (!installInNewTab(m_tabs, m_document, std::move(r.document),
                                       QFileInfo(source.toLocalFile()).completeBaseName())) {
                      fail(tr("Import de la présentation impossible : un autre fichier est en cours de "
                              "lecture."));
                      return;
                  }
                  emit finished(tr("Présentation importée : %1").arg(fileName(source)));
              });
    });
}

// Hors Windows (N1-15) : la mise en page de la boîte d'impression n'existe pas, le paperboard part par
// l'export PDF de L5-T23 (même rendu que l'écran) dans le dossier temporaire, puis le lecteur de PDF du
// système l'ouvre et imprime.
void FileExchange::printThroughPdf()
{
    if (!begin()) {
        return;
    }
    QTemporaryFile file(QDir::temp().absoluteFilePath(QStringLiteral("ActCraft-impression-XXXXXX.pdf")));
    file.setAutoRemove(false);
    if (!file.open()) {
        setBusy(false);
        fail(tr("Impression impossible : le dossier temporaire n'est pas accessible."));
        return;
    }
    const QUrl target = QUrl::fromLocalFile(file.fileName());
    file.close();
    watch(this, exporting::exportPdf(m_document->document(), target),
          [this, target](exporting::ExportResult r) {
              setBusy(false);
              if (!r.ok) {
                  QFile::remove(target.toLocalFile());
                  fail(tr("Impression impossible : %1").arg(r.error));
              } else if (!QDesktopServices::openUrl(target)) {
                  fail(tr("Impression impossible : aucun lecteur de PDF n'a pu ouvrir %1.")
                           .arg(target.toLocalFile()));
              } else {
                  emit finished(tr("PDF ouvert pour l'impression."));
              }
          });
}

} // namespace actcraft::ui
