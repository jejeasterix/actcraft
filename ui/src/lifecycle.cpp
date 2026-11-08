#include "parkedsession.h"

#include <actcraft/ui/documentio.h>
#include <actcraft/ui/lifecycle.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>
#include <QtGui/qevent.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace actcraft::ui {

namespace {

Q_STATIC_LOGGING_CATEGORY(lcLifecycle, "actcraft.lifecycle", QtInfoMsg)

// Même forme que DocumentController::fingerprint (16 chiffres hexadécimaux).
QString fingerprintHex(std::uint64_t fingerprint)
{
    return QStringLiteral("%1").arg(fingerprint, 16, 16, QLatin1Char('0'));
}

// Rang 1 : reprise.actcraft ; rang n > 1 : reprise-<n>.actcraft (et leur .json).
QString baseName(int rank)
{
    return rank == 1 ? QStringLiteral("reprise") : QStringLiteral("reprise-%1").arg(rank);
}

// Copies de reprise du dossier, rangées par rang (reprise, puis reprise-2…).
QStringList copyNames(const QString& dir)
{
    static const QRegularExpression name(QStringLiteral("^reprise(-([0-9]+))?\\.actcraft$"));
    QStringList names = QDir(dir).entryList({QStringLiteral("reprise*.actcraft")}, QDir::Files);
    names.removeIf([](const QString& n) { return !name.match(n).hasMatch(); });
    const auto rank = [](const QString& n) {
        const QString digits = name.match(n).captured(2);
        return digits.isEmpty() ? 1 : digits.toInt();
    };
    std::sort(names.begin(), names.end(),
              [&rank](const QString& a, const QString& b) { return rank(a) < rank(b); });
    return names;
}

void removePair(const QDir& folder, const QString& copyName)
{
    QFile::remove(folder.filePath(copyName));
    QFile::remove(folder.filePath(copyName.chopped(int(sizeof("actcraft") - 1)) + QStringLiteral("json")));
}

void removeAll(const QString& dir, const QStringList& held)
{
    const QDir folder(dir);
    for (const QString& name : copyNames(dir)) {
        if (!held.contains(name)) {
            removePair(folder, name);
        }
    }
    QDir().rmdir(dir); // vide seulement
}

// Fichier d'origine vu du disque : fichier local, ou content:// (moteur de fichiers de Qt sous Android).
QFileInfo originInfo(const QUrl& origin)
{
    if (origin.isLocalFile()) {
        return QFileInfo(origin.toLocalFile());
    }
    if (origin.scheme() == QLatin1String("content")) {
        return QFileInfo(origin.toString());
    }
    return QFileInfo();
}

struct Entry
{
    std::shared_ptr<const core::Document> snapshot;
    QUrl origin;
    int page = 0;
    double scale = 0; // 0 : inconnue (onglet garé)
};

// Sur le fil de la réserve. Chaque copie est écrite par l'écrivain natif, puis son .json (chacun atomique) ;
// le .json n'est cru à la reprise que s'il porte l'empreinte de la copie. Les rangs des copies gardées
// (`held`) sont sautés ; les autres copies d'une écriture précédente sont effacées. Le journal est écrit
// d'ici : sous Android, la boucle principale peut être arrêtée après Suspended.
bool writeSet(const QString& dir, const std::vector<Entry>& entries, qint64 time, const QStringList& held)
{
    const QDir folder(dir);
    if (!QDir().mkpath(dir)) {
        qCWarning(lcLifecycle, "copie de reprise impossible : dossier %s", qPrintable(dir));
        return false;
    }
    bool ok = true;
    QString first;
    QStringList written;
    int rank = 0;
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const Entry& e = entries[i];
        do {
            ++rank;
        } while (held.contains(baseName(rank) + QStringLiteral(".actcraft")));
        const QString base = baseName(rank);
        written << base + QStringLiteral(".actcraft");
        const SaveOutcome saved = saveDocument(
            *e.snapshot, QUrl::fromLocalFile(folder.filePath(base + QStringLiteral(".actcraft"))));
        if (!saved.ok) {
            qCWarning(lcLifecycle, "copie de reprise impossible : %s", qPrintable(saved.error));
            ok = false;
            continue;
        }
        const QString fingerprint = fingerprintHex(saved.fingerprint);
        if (i == 0) {
            first = fingerprint;
        }
        const QJsonObject info{{QStringLiteral("url"), e.origin.toString(QUrl::FullyEncoded)},
                               {QStringLiteral("page"), e.page},
                               {QStringLiteral("scale"), e.scale},
                               {QStringLiteral("time"), QJsonValue(time)},
                               {QStringLiteral("fingerprint"), fingerprint}};
        QSaveFile file(folder.filePath(base + QStringLiteral(".json")));
        if (!file.open(QIODevice::WriteOnly) ||
            file.write(QJsonDocument(info).toJson(QJsonDocument::Compact)) <= 0 || !file.commit()) {
            qCWarning(lcLifecycle, "copie de reprise : %s.json non écrit", qPrintable(base));
            ok = false;
        }
    }
    for (const QString& name : copyNames(dir)) {
        if (!written.contains(name) && !held.contains(name)) {
            removePair(folder, name);
        }
    }
    if (ok) {
        qCInfo(lcLifecycle, "ACTCRAFT COPIE DE REPRISE %s (%d document(s))", qPrintable(first),
               int(entries.size()));
    }
    return ok;
}

} // namespace

struct LifecycleKeeper::Held
{
    std::mutex mutex;
    QStringList names;

    QStringList get()
    {
        const std::lock_guard<std::mutex> lock(mutex);
        return names;
    }
    void set(QStringList held)
    {
        const std::lock_guard<std::mutex> lock(mutex);
        names = std::move(held);
    }
};

struct LifecycleKeeper::Found
{
    QString name; // nom de la copie dans le dossier de reprise
    LoadOutcome outcome;
    QUrl origin; // vide si le .json ne décrit pas cette copie
    int page = 0;
    double scale = 0;
    std::shared_ptr<const core::Document> reference; // fichier d'origine relu
};

namespace {

// Sur le fil de la réserve : copies relues ; périmées (plus anciennes que leur fichier d'origine) ou
// illisibles, effacées.
std::vector<LifecycleKeeper::Found> readSet(const QString& dir)
{
    std::vector<LifecycleKeeper::Found> out;
    const QDir folder(dir);
    for (const QString& name : copyNames(dir)) {
        const QString copy = folder.filePath(name);
        LifecycleKeeper::Found found;
        found.name = name;
        QString fingerprint;
        QFile info(folder.filePath(name.chopped(int(sizeof("actcraft") - 1)) + QStringLiteral("json")));
        if (info.open(QIODevice::ReadOnly)) {
            const QJsonObject o = QJsonDocument::fromJson(info.readAll()).object();
            found.origin = QUrl(o.value(QStringLiteral("url")).toString(), QUrl::StrictMode);
            found.page = o.value(QStringLiteral("page")).toInt();
            found.scale = o.value(QStringLiteral("scale")).toDouble();
            fingerprint = o.value(QStringLiteral("fingerprint")).toString();
        }
        const QFileInfo original = originInfo(found.origin);
        if (original.exists() && QFileInfo(copy).lastModified() < original.lastModified()) {
            qCInfo(lcLifecycle, "copie de reprise %s plus ancienne que %s : effacée", qPrintable(name),
                   qPrintable(original.fileName()));
            removePair(folder, name);
            continue;
        }
        found.outcome = loadDocument(QUrl::fromLocalFile(copy));
        if (!found.outcome.ok) {
            qCWarning(lcLifecycle, "copie de reprise %s illisible (%s) : effacée", qPrintable(name),
                      qPrintable(found.outcome.error));
            removePair(folder, name);
            continue;
        }
        if (fingerprint != fingerprintHex(found.outcome.fingerprint)) {
            found.origin = QUrl(); // .json d'une autre copie : jamais attaché au fichier d'un autre document
        }
        if (!found.origin.isEmpty() && original.exists()) {
            LoadOutcome reference = loadDocument(found.origin);
            if (reference.ok) {
                found.reference = std::make_shared<const core::Document>(std::move(reference.document));
            }
        }
        out.push_back(std::move(found));
    }
    if (out.empty()) {
        QDir().rmdir(dir); // vide seulement
    }
    return out;
}

} // namespace

LifecycleKeeper::LifecycleKeeper(QObject* parent) : QObject(parent), m_held(std::make_shared<Held>())
{
    m_pool.setMaxThreadCount(1);
    if (auto* app = QCoreApplication::instance()) {
        app->installEventFilter(this);
    }
    if (qGuiApp != nullptr) {
        m_state = qGuiApp->applicationState();
        connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, &LifecycleKeeper::applyState);
    }
}

LifecycleKeeper::~LifecycleKeeper()
{
    if (auto* app = QCoreApplication::instance()) {
        app->removeEventFilter(this);
    }
    m_pool.waitForDone(); // un résultat posté avant la fin est retiré par ~QObject
}

DocumentController* LifecycleKeeper::document() const noexcept
{
    return m_document;
}

void LifecycleKeeper::setDocument(DocumentController* document)
{
    if (m_document == document) {
        return;
    }
    disconnect(m_saved);
    disconnect(m_waitIdle);
    m_document = document;
    if (m_document) {
        // Un enregistrement réussi efface la copie seulement si plus rien n'est à enregistrer (révision
        // bougée pendant l'écriture, onglet garé modifié : la copie reste).
        m_saved =
            connect(m_document, &DocumentController::saveFinished, this, [this](bool ok, const QString&) {
                if (ok && m_ownsCopy && !anyModified()) {
                    removeCopy();
                }
            });
    }
    emit documentChanged();
}

PageView* LifecycleKeeper::view() const noexcept
{
    return m_view;
}

void LifecycleKeeper::setView(PageView* view)
{
    if (m_view == view) {
        return;
    }
    m_view = view;
    emit viewChanged();
}

DocumentTabs* LifecycleKeeper::tabs() const noexcept
{
    return m_tabs;
}

void LifecycleKeeper::setTabs(DocumentTabs* tabs)
{
    if (m_tabs == tabs) {
        return;
    }
    m_tabs = tabs;
    emit tabsChanged();
}

QString LifecycleKeeper::defaultRecoveryDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/reprise");
}

QString LifecycleKeeper::recoveryDir() const
{
    return m_recoveryDir.isEmpty() ? defaultRecoveryDir() : m_recoveryDir;
}

void LifecycleKeeper::setRecoveryDir(const QString& dir)
{
    if (m_recoveryDir == dir) {
        return;
    }
    m_recoveryDir = dir;
    emit recoveryDirChanged();
}

bool LifecycleKeeper::recovered() const noexcept
{
    return m_recovered;
}

bool LifecycleKeeper::lastWriteOk() const noexcept
{
    return m_lastWriteOk;
}

void LifecycleKeeper::componentComplete()
{
    start();
}

bool LifecycleKeeper::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == QCoreApplication::instance() && event->type() == QEvent::ApplicationStateChange) {
        applyState(static_cast<QApplicationStateChangeEvent*>(event)->applicationState());
    }
    return QObject::eventFilter(watched, event);
}

void LifecycleKeeper::applyState(Qt::ApplicationState state)
{
    if (state == m_state) {
        return; // même état par l'événement puis par le signal
    }
    m_state = state;
    bool background = state == Qt::ApplicationSuspended;
#ifdef Q_OS_ANDROID
    background = background || state == Qt::ApplicationHidden; // onStop : le processus peut être tué
#endif
    if (background) {
        suspend();
    }
}

bool LifecycleKeeper::anyModified() const
{
    if (m_document && m_document->isModified()) {
        return true;
    }
    const std::vector<DocumentController::ParkedSession*>& parked = DocumentController::ParkedSession::live();
    return std::any_of(parked.begin(), parked.end(), [](const auto* s) { return s->isModified(); });
}

void LifecycleKeeper::suspend()
{
    if (!m_document) {
        return;
    }
    // Le trait en cours entre dans la copie ; puis la copie est prise comme par « Enregistrer » (texte en
    // cours, objets soulevés compris).
    if (m_view) {
        m_view->finishContacts();
    }
    std::shared_ptr<const core::Document> snapshot = m_document->snapshotForSave();
    std::vector<Entry> entries;
    const std::uint64_t key = m_document->documentKey();
    const std::uint64_t revision = m_document->revision();
    if (m_document->isModified()) {
        entries.push_back({std::move(snapshot), m_document->fileUrl(), m_document->currentIndex(),
                           m_view ? double(m_view->zoom()) : 1.0});
    }
    // Un onglet garé ne change que par « Copier vers l'onglet » (pages ajoutées) : clé et nombre de pages.
    std::vector<std::pair<std::uint64_t, std::size_t>> parked;
    for (const DocumentController::ParkedSession* s : DocumentController::ParkedSession::live()) {
        if (s->isModified()) {
            parked.emplace_back(s->documentKey, s->document.pageCount());
            entries.push_back(
                {std::make_shared<const core::Document>(s->document), s->fileUrl, int(s->current), 0.0});
        }
    }
    if (entries.empty()) {
        // Plus rien à enregistrer (document abandonné, annulé jusqu'à propre) : la copie de ce gardien n'a
        // plus d'objet ; rouverte après un arrêt, elle ressusciterait un travail abandonné.
        if (m_ownsCopy) {
            removeCopy();
        }
        return;
    }
    // Android : Hidden puis Suspended ; rien de changé depuis la copie : pas de seconde écriture.
    if (m_copyValid && m_copyKey == key && m_copyRevision == revision && m_copyParked == parked) {
        return;
    }
    m_copyValid = true;
    m_copyKey = key;
    m_copyRevision = revision;
    m_copyParked = std::move(parked);
    m_ownsCopy = true;
    const QString dir = recoveryDir();
    const qint64 time = QDateTime::currentMSecsSinceEpoch();
    m_pool.start([this, entries = std::move(entries), dir, time, held = m_held] {
        const bool ok = writeSet(dir, entries, time, held->get());
        QMetaObject::invokeMethod(this, [this, ok] { finishWrite(ok); }, Qt::QueuedConnection);
    });
}

void LifecycleKeeper::finishWrite(bool ok)
{
    if (!ok) {
        m_copyValid = false;
    }
    if (ok != m_lastWriteOk) {
        m_lastWriteOk = ok;
        emit lastWriteOkChanged();
    }
    emit writeFinished(ok);
}

void LifecycleKeeper::start()
{
    if (m_started) {
        return;
    }
    m_started = true;
    const QString dir = recoveryDir();
    const std::uint64_t key = m_document ? m_document->documentKey() : 0;
    const std::uint64_t revision = m_document ? m_document->revision() : 0;
    m_pool.start([this, dir, key, revision, held = m_held] {
        if (!QFileInfo::exists(dir)) {
            return; // aucune copie : ni lecture ni écriture
        }
        auto found = std::make_shared<std::vector<Found>>(readSet(dir));
        if (found->empty()) {
            return;
        }
        // Gardées jusqu'à ce que la reprise les montre : une écriture ou un effacement demandé entre-temps
        // (fermeture pendant la relecture) les laisse.
        QStringList names;
        for (const Found& f : *found) {
            names << f.name;
        }
        held->set(std::move(names));
        QMetaObject::invokeMethod(
            this, [this, found, key, revision] { restore(found, key, revision); }, Qt::QueuedConnection);
    });
}

void LifecycleKeeper::restore(const std::shared_ptr<std::vector<Found>>& found, std::uint64_t key,
                              std::uint64_t revision)
{
    if (!m_document) {
        return;
    }
    // Une lecture (fichier ouvert au lancement) ou une écriture en cours finit d'abord : la reprise la
    // ferait annuler (révision changée).
    if (m_document->isBusy()) {
        disconnect(m_waitIdle);
        m_waitIdle =
            connect(m_document, &DocumentController::busyChanged, this, [this, found, key, revision] {
                if (m_document && !m_document->isBusy()) {
                    disconnect(m_waitIdle);
                    // En file : busyChanged est émis avant la fin de l'ouverture (contrôle de révision, puis
                    // replaceDocument) ; la reprise vient après.
                    QMetaObject::invokeMethod(
                        this, [this, found, key, revision] { restore(found, key, revision); },
                        Qt::QueuedConnection);
                }
            });
        return;
    }
    // L'onglet affiché n'a pas changé depuis start() : la première copie y est rouverte. Sinon (trait déjà
    // fait, fichier ouvert), chaque copie a son onglet neuf ; sans onglets, la copie est gardée telle quelle.
    const bool untouched = m_document->documentKey() == key && m_document->revision() == revision;
    if (!untouched && !m_tabs) {
        qCInfo(lcLifecycle, "copie de reprise gardée : document changé avant la reprise");
        return;
    }
    const int home = m_tabs ? m_tabs->currentIndex() : 0;
    bool intoHome = untouched;
    int shown = 0;
    QStringList kept;
    for (Found& f : *found) {
        if (!intoHome) {
            // Onglet neuf vérifié : refusé (verrou de classe sur « Nouveau », lecture en cours), la copie est
            // gardée, jamais versée dans l'onglet affiché. Le verrou n'est pas contourné : la reprise ne crée
            // pas d'onglet que l'enseignant a interdit ; rien n'est perdu, elle revient au lancement suivant.
            const int before = m_tabs ? m_tabs->rowCount() : 0;
            if (m_tabs) {
                m_tabs->newTab();
            }
            if (!m_tabs || m_tabs->rowCount() == before) {
                kept << f.name;
                continue;
            }
        }
        const QString title = f.origin.isEmpty() ? QString() : titleFromUrl(f.origin);
        m_document->replaceWithRecovered(std::move(f.outcome.document), f.origin, title, f.reference);
        if (f.page > 0 && f.page < m_document->pageCount()) {
            m_document->showPage(f.page);
        }
        if (intoHome && m_view && f.scale > 0) {
            m_view->setZoom(f.scale);
        }
        qCInfo(lcLifecycle, "ACTCRAFT REPRISE %s", qPrintable(fingerprintHex(f.outcome.fingerprint)));
        intoHome = false;
        ++shown;
    }
    m_held->set(kept);
    if (!kept.isEmpty()) {
        qCInfo(lcLifecycle, "%d copie(s) de reprise gardée(s) : onglet neuf refusé", int(kept.size()));
    }
    if (shown == 0) {
        return;
    }
    if (m_tabs && m_tabs->currentIndex() != home) {
        m_tabs->activate(home);
    }
    m_ownsCopy = true;
    setRecovered(true);
}

void LifecycleKeeper::discard()
{
    removeCopy();
}

void LifecycleKeeper::removeCopy()
{
    setRecovered(false);
    m_copyValid = false;
    m_ownsCopy = false;
    const QString dir = recoveryDir();
    // Après toute écriture déjà demandée ; les copies gardées restent.
    m_pool.start([dir, held = m_held] { removeAll(dir, held->get()); });
}

void LifecycleKeeper::setRecovered(bool recovered)
{
    if (recovered == m_recovered) {
        return;
    }
    m_recovered = recovered;
    emit recoveredChanged();
}

} // namespace actcraft::ui
