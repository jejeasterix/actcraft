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
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>
#include <QtGui/qevent.h>

#include <cstdint>
#include <memory>
#include <utility>

namespace actcraft::ui {

namespace {

Q_STATIC_LOGGING_CATEGORY(lcLifecycle, "actcraft.lifecycle", QtInfoMsg)

const QString kCopyName = QStringLiteral("reprise.actcraft");
const QString kInfoName = QStringLiteral("reprise.json");

// Même forme que DocumentController::fingerprint (16 chiffres hexadécimaux).
QString fingerprintHex(std::uint64_t fingerprint)
{
    return QStringLiteral("%1").arg(fingerprint, 16, 16, QLatin1Char('0'));
}

struct Written
{
    bool ok = false;
    QString fingerprint;
};

// Sur le fil de la réserve. L'ancien reprise.json part d'abord : un reprise.json présent a toujours été écrit
// après la copie qui l'accompagne (une interruption entre les deux laisse une copie sans URL, jamais une URL
// attachée à une autre copie).
Written writeRecovery(const QString& dir, const core::Document& snapshot, const QUrl& origin, int page,
                      double scale, qint64 time)
{
    Written out;
    const QDir folder(dir);
    if (!QDir().mkpath(dir)) {
        qCWarning(lcLifecycle, "copie de reprise impossible : dossier %s", qPrintable(dir));
        return out;
    }
    QFile::remove(folder.filePath(kInfoName));
    const SaveOutcome saved = saveDocument(snapshot, QUrl::fromLocalFile(folder.filePath(kCopyName)));
    if (!saved.ok) {
        qCWarning(lcLifecycle, "copie de reprise impossible : %s", qPrintable(saved.error));
        return out;
    }
    out.fingerprint = fingerprintHex(saved.fingerprint);
    const QJsonObject info{{QStringLiteral("url"), origin.toString(QUrl::FullyEncoded)},
                           {QStringLiteral("page"), page},
                           {QStringLiteral("scale"), scale},
                           {QStringLiteral("time"), QJsonValue(time)},
                           {QStringLiteral("fingerprint"), out.fingerprint}};
    QSaveFile file(folder.filePath(kInfoName));
    out.ok = file.open(QIODevice::WriteOnly) &&
             file.write(QJsonDocument(info).toJson(QJsonDocument::Compact)) > 0 && file.commit();
    if (!out.ok) {
        qCWarning(lcLifecycle, "copie de reprise : %s non écrit", qPrintable(kInfoName));
    }
    return out;
}

void removeRecovery(const QString& dir)
{
    const QDir folder(dir);
    QFile::remove(folder.filePath(kInfoName));
    QFile::remove(folder.filePath(kCopyName));
    QDir().rmdir(dir); // vide seulement
}

} // namespace

// Copie trouvée au démarrage, relue sur le fil de la réserve.
struct LifecycleKeeper::Found
{
    LoadOutcome outcome;
    QUrl origin;
    int page = 0;
    double scale = 0;
};

LifecycleKeeper::LifecycleKeeper(QObject* parent) : QObject(parent)
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
    m_document = document;
    if (m_document) {
        m_saved =
            connect(m_document, &DocumentController::saveFinished, this, [this](bool ok, const QString&) {
                if (ok) {
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

void LifecycleKeeper::suspend()
{
    if (!m_document) {
        return;
    }
    // Le trait en cours entre dans la copie (comme « Enregistrer », qui termine le trait avant sa copie).
    if (m_view) {
        m_view->finishContacts();
    }
    if (!m_document->isModified()) {
        return;
    }
    // Copie prise sur le fil principal (ressources partagées implicitement), comme saveToUrl : le fil de la
    // réserve ne lit jamais le document affiché.
    auto snapshot = std::make_shared<const core::Document>(m_document->document());
    const QString dir = recoveryDir();
    const QUrl origin = m_document->fileUrl();
    const int page = m_document->currentIndex();
    const double scale = m_view ? double(m_view->zoom()) : 1.0;
    const qint64 time = QDateTime::currentMSecsSinceEpoch();
    m_mayHaveCopy = true;
    m_pool.start([this, snapshot, dir, origin, page, scale, time] {
        const Written written = writeRecovery(dir, *snapshot, origin, page, scale, time);
        QMetaObject::invokeMethod(
            this, [this, written] { finishWrite(written.ok, written.fingerprint); }, Qt::QueuedConnection);
    });
}

void LifecycleKeeper::finishWrite(bool ok, const QString& fingerprint)
{
    if (ok) {
        qCInfo(lcLifecycle, "ACTCRAFT COPIE DE REPRISE %s", qPrintable(fingerprint));
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
    m_pool.start([this, dir, key] {
        const QDir folder(dir);
        const QString copy = folder.filePath(kCopyName);
        if (!QFileInfo::exists(copy)) {
            return; // aucune copie : ni lecture ni écriture
        }
        auto found = std::make_shared<Found>();
        QFile info(folder.filePath(kInfoName));
        if (info.open(QIODevice::ReadOnly)) {
            const QJsonObject o = QJsonDocument::fromJson(info.readAll()).object();
            found->origin = QUrl(o.value(QStringLiteral("url")).toString(), QUrl::StrictMode);
            found->page = o.value(QStringLiteral("page")).toInt();
            found->scale = o.value(QStringLiteral("scale")).toDouble();
        }
        // Fichier d'origine enregistré après la copie : la copie est périmée.
        if (found->origin.isLocalFile()) {
            const QFileInfo original(found->origin.toLocalFile());
            if (original.exists() && QFileInfo(copy).lastModified() < original.lastModified()) {
                qCInfo(lcLifecycle, "copie de reprise plus ancienne que %s : non rouverte",
                       qPrintable(original.fileName()));
                return;
            }
        }
        found->outcome = loadDocument(QUrl::fromLocalFile(copy));
        if (!found->outcome.ok) {
            qCWarning(lcLifecycle, "copie de reprise illisible : %s", qPrintable(found->outcome.error));
            return;
        }
        QMetaObject::invokeMethod(this, [this, found, key] { restore(*found, key); }, Qt::QueuedConnection);
    });
}

void LifecycleKeeper::restore(Found& found, std::uint64_t documentKey)
{
    m_mayHaveCopy = true;
    // Un document ouvert entre-temps n'est jamais remplacé : la copie reste pour un prochain lancement.
    if (!m_document || m_document->documentKey() != documentKey) {
        qCInfo(lcLifecycle, "copie de reprise gardée : document remplacé avant la reprise");
        return;
    }
    const QString title = found.origin.isEmpty() ? QString() : titleFromUrl(found.origin);
    m_document->replaceDocument(std::move(found.outcome.document), found.origin, title);
    // replaceDocument rend un document propre ; la session garée garde l'URL et rend l'état « modifié »
    // (ce qui n'est pas dans le fichier d'origine reste à enregistrer).
    std::unique_ptr<DocumentController::ParkedSession> session = m_document->parkSession();
    session->modifiedOutsideStack = true;
    m_document->resumeSession(std::move(session));
    if (found.page > 0 && found.page < m_document->pageCount()) {
        m_document->showPage(found.page);
    }
    if (m_view && found.scale > 0) {
        m_view->setZoom(found.scale);
    }
    setRecovered(true);
    qCInfo(lcLifecycle, "ACTCRAFT REPRISE %s", qPrintable(m_document->fingerprintText()));
}

void LifecycleKeeper::discard()
{
    setRecovered(false);
    removeCopy();
}

void LifecycleKeeper::removeCopy()
{
    setRecovered(false);
    if (!m_mayHaveCopy) {
        return;
    }
    m_mayHaveCopy = false;
    const QString dir = recoveryDir();
    m_pool.start([dir] { removeRecovery(dir); }); // après toute écriture déjà demandée
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
