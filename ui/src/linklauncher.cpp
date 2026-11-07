#include "actcraft/ui/linklauncher.h"

#include <QCryptographicHash>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QSaveFile>
#include <QStandardPaths>
#include <QStringList>
#include <QTemporaryDir>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>
#include <atomic>
#include <mutex>
#include <utility>

#ifdef Q_OS_MACOS
#include <ctime>
#include <sys/xattr.h>
#endif

namespace actcraft::ui {
namespace {

std::atomic<int> g_diskQueries{0};

// Seule occurrence de QDesktopServices::openUrl pour le contenu d'un document (D8-4) ; appelée par answer()
// seul.
bool systemOpener(const QUrl& url)
{
    return QDesktopServices::openUrl(url);
}

// Liste blanche fermée (D8-4), jamais une liste noire : tout type absent est refusé. Exclus à dessein : svg
// (ses scripts s'exécutent dans le navigateur), doc, xls, ppt (macros), rtf (objets incorporés), tout format
// à macros.
const QStringList& allowedSuffixes()
{
    static const QStringList kAllowed =
        QStringLiteral(
            "pdf docx xlsx pptx odt ods odp txt png jpg jpeg gif bmp wav mp3 ogg m4a mp4 webm avi mov wmv")
            .split(QLatin1Char(' '));
    return kAllowed;
}

// Texte venu d'un document, tel qu'on le montre : les caractères de mise en forme (Cf : U+202E retourne
// l'affichage, U+200B est invisible), de contrôle (Cc) et les demi-codets isolés s'écrivent « <U+XXXX> ».
QString displayed(const QString& text)
{
    QString out;
    out.reserve(text.size());
    for (qsizetype i = 0; i < text.size(); ++i) {
        char32_t code = text.at(i).unicode();
        qsizetype length = 1;
        if (QChar::isHighSurrogate(code) && i + 1 < text.size() && text.at(i + 1).isLowSurrogate()) {
            code = QChar::surrogateToUcs4(text.at(i), text.at(i + 1));
            length = 2;
        }
        const QChar::Category category = QChar::category(code);
        if (category == QChar::Other_Format || category == QChar::Other_Control ||
            category == QChar::Other_Surrogate)
            out += QStringLiteral("<U+%1>").arg(
                QString::number(uint(code), 16).toUpper().rightJustified(4, u'0'));
        else
            out += QStringView(text).mid(i, length);
        i += length - 1;
    }
    return out;
}

// Windows retire les points et espaces finaux d'un nom : « a.exe. » et « a.exe  » désignent « a.exe ».
QString withoutTrailingDotsAndSpaces(QString name)
{
    while (!name.isEmpty() && (name.back() == u'.' || name.back() == u' '))
        name.chop(1);
    return name;
}

QString fileNameOf(const QString& path)
{
    return path.mid(path.lastIndexOf(u'/') + 1);
}

// Motif de refus de l'extension d'un nom déjà débarrassé de ses points et espaces finaux ; vide : acceptée.
// La seule comparaison des extensions : sans casse.
QString suffixRefusal(const QString& name)
{
    const qsizetype dot = name.lastIndexOf(u'.');
    const QString suffix = dot < 0 ? QString() : name.mid(dot + 1);
    if (suffix.isEmpty())
        return LinkLauncher::tr("Un fichier sans extension ne s'ouvre pas depuis un paperboard : %1")
            .arg(name);
    if (!allowedSuffixes().contains(suffix, Qt::CaseInsensitive))
        return LinkLauncher::tr("Ce type de fichier (.%1) ne s'ouvre pas depuis un paperboard : %2")
            .arg(suffix, name);
    return {};
}

// « \\hôte\… », « //hôte/… », « file://hôte/… », « file:////hôte/… » : un chemin réseau, quel que soit le
// système (un document fait sous Windows porte des antislashs). Aucun accès au disque.
bool isNetworkPath(const QUrl& url)
{
    QString path = url.toLocalFile();
    path.replace(u'\\', u'/');
    return !url.host().isEmpty() || path.startsWith(QStringLiteral("//"));
}

// Chemin d'un fichier local, nettoyé, nom débarrassé de ses points et espaces finaux : le fichier que le
// système ouvrirait. À n'appeler qu'après le refus des chemins réseau (cleanPath réduirait « // » hors de
// Windows).
QString strippedLocalPath(const QUrl& url)
{
    const QString path = QDir::cleanPath(QDir::fromNativeSeparators(url.toLocalFile()));
    const qsizetype slash = path.lastIndexOf(u'/');
    return path.left(slash + 1) + withoutTrailingDotsAndSpaces(path.mid(slash + 1));
}

// Seule porte de refusalFor vers le disque, comptée (diskQueries) : rien ne la franchit avant le refus des
// chemins réseau.
QFileInfo diskInfo(const QString& path)
{
    ++g_diskQueries;
    return QFileInfo(path);
}

bool isPortableNameChar(char16_t c)
{
    return (c >= u'a' && c <= u'z') || (c >= u'A' && c <= u'Z') || (c >= u'0' && c <= u'9') || c == u'.' ||
           c == u'_' || c == u' ' || c == u'-';
}

// Nom sûr d'un fichier rangé dans le document : son dernier composant seul (aucun chemin fourni par le
// document), réduit aux caractères [A-Za-z0-9._ -], sans points ni espaces en tête (ni nom caché, ni « .. »)
// ni en fin (le nom que Windows retiendrait). Un nom de périphérique de Windows (CON, NUL, COM0…) reçoit un
// « _ » en tête.
QString safeFileName(const QString& originalName)
{
    const qsizetype cut = std::max(originalName.lastIndexOf(u'/'), originalName.lastIndexOf(u'\\'));
    QString name;
    for (const QChar c : QStringView(originalName).mid(cut + 1))
        if (isPortableNameChar(c.unicode()))
            name += c;
    while (!name.isEmpty() && (name.front() == u'.' || name.front() == u' '))
        name.remove(0, 1);
    name = withoutTrailingDotsAndSpaces(name);
    static const QStringList kDevices =
        QStringLiteral("CON PRN AUX NUL COM0 COM1 COM2 COM3 COM4 COM5 COM6 COM7 COM8 COM9 "
                       "LPT0 LPT1 LPT2 LPT3 LPT4 LPT5 LPT6 LPT7 LPT8 LPT9")
            .split(QLatin1Char(' '));
    if (kDevices.contains(name.section(u'.', 0, 0).trimmed(), Qt::CaseInsensitive))
        name.prepend(u'_');
    return name;
}

// Préfixe du dossier d'extraction d'un document : la clé hachée (deux clés distinctes, deux préfixes
// distincts ; rien de la clé n'entre dans un chemin).
QString hashedKey(const QString& documentKey)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(documentKey.toUtf8(), QCryptographicHash::Sha256).toHex());
}

// Marque « venu d'ailleurs » du système sur un fichier extrait : le système et les lecteurs le traitent comme
// un téléchargement (mode protégé, avertissement). Windows : flux Zone.Identifier, zone Internet ; macOS :
// attribut com.apple.quarantine ; ailleurs, rien à poser. Faux : marque non posée.
bool markAsDownloaded(const QString& path)
{
#if defined(Q_OS_WIN)
    QFile zone(path + QStringLiteral(":Zone.Identifier"));
    const QByteArray text("[ZoneTransfer]\r\nZoneId=3\r\n");
    return zone.open(QIODevice::WriteOnly | QIODevice::Truncate) && zone.write(text) == text.size() &&
           zone.flush();
#elif defined(Q_OS_MACOS)
    const QByteArray value = QByteArrayLiteral("0081;") + QByteArray::number(qint64(std::time(nullptr)), 16) +
                             QByteArrayLiteral(";ActCraft;");
    return setxattr(QFile::encodeName(path).constData(), "com.apple.quarantine", value.constData(),
                    size_t(value.size()), 0, 0) == 0;
#else
    Q_UNUSED(path);
    return true;
#endif
}

QString rawRefusal(const QUrl& url, int kind)
{
    const QString shown = url.toString(QUrl::FullyEncoded);
    if (kind == LinkLauncher::Website) {
        // QUrl rend le schéma en minuscules.
        if (!url.isValid() ||
            (url.scheme() != QLatin1String("http") && url.scheme() != QLatin1String("https")))
            return LinkLauncher::tr("Seules les adresses http et https s'ouvrent : %1").arg(shown);
        if (url.host().isEmpty())
            return LinkLauncher::tr("Adresse sans nom d'hôte : %1").arg(shown);
        // « https://ecole.fr@evil.example/ » mène à evil.example.
        if (!url.userInfo().isEmpty())
            return LinkLauncher::tr("Une adresse qui porte des identifiants ne s'ouvre pas : %1").arg(shown);
        return {};
    }
    if (kind != LinkLauncher::File && kind != LinkLauncher::StoredFile && kind != LinkLauncher::SystemMedia)
        return LinkLauncher::tr("Type de lien inconnu.");
    if (!url.isValid() || !url.isLocalFile())
        return LinkLauncher::tr("Seul un fichier de cet ordinateur s'ouvre : %1").arg(shown);
    // Avant tout accès au disque : sous Windows, tester l'existence d'un chemin réseau envoie
    // l'authentification du compte à l'hôte.
    if (isNetworkPath(url))
        return LinkLauncher::tr("Un chemin réseau ne s'ouvre pas depuis un paperboard : %1").arg(shown);
    const QString path = strippedLocalPath(url);
    const QString native = QDir::toNativeSeparators(path);
    const QString name = fileNameOf(path);
    if (name.contains(u':')) // flux de données d'un fichier sous Windows (« a.exe:b.pdf »)
        return LinkLauncher::tr("Nom de fichier refusé : %1").arg(native);
    if (const QString reason = suffixRefusal(name); !reason.isEmpty())
        return reason;
    if (!QDir::isAbsolutePath(path))
        return LinkLauncher::tr("Un chemin relatif ne s'ouvre pas depuis un paperboard : %1").arg(native);
    const QFileInfo info =
        diskInfo(path); // existence testée sur le nom après retrait des points et espaces finaux
    if (!info.isFile())
        return LinkLauncher::tr("Fichier introuvable : %1").arg(native);
    if (info.isSymLink() && !suffixRefusal(fileNameOf(info.canonicalFilePath())).isEmpty())
        return LinkLauncher::tr("Ce lien désigne un type de fichier refusé : %1").arg(native);
    return {};
}

} // namespace

LinkLauncher::LinkLauncher(QObject* parent) : QObject(parent), m_opener(systemOpener) {}

LinkLauncher::~LinkLauncher() = default;

void LinkLauncher::setOpener(Opener opener)
{
    m_opener = opener ? std::move(opener) : Opener(systemOpener);
}

void LinkLauncher::request(const QString& target, int kind)
{
    if (kind == Website) {
        ask(QUrl(target.trimmed(), QUrl::StrictMode), kind);
        return;
    }
    // Une lettre de lecteur (« C:\… ») n'est pas un schéma : un chemin se lit comme un fichier local, une
    // adresse (« file:… », « smb:… ») comme une adresse.
    const QUrl parsed(target);
    ask(parsed.scheme().size() > 1 ? parsed : QUrl::fromLocalFile(target), kind);
}

namespace {

std::mutex g_probeMutex;
std::function<void()> g_writeProbe;

void runWriteProbe()
{
    std::function<void()> probe;
    {
        const std::lock_guard<std::mutex> lock(g_probeMutex);
        probe = g_writeProbe;
    }
    if (probe)
        probe();
}

} // namespace

void LinkLauncher::setWriteProbe(std::function<void()> probe)
{
    const std::lock_guard<std::mutex> lock(g_probeMutex);
    g_writeProbe = std::move(probe);
}

namespace {

enum class Extracted
{
    Written,
    WriteFailed,
    MarkFailed
};

// Fil d'écriture du lanceur : n'émet rien et ne touche à aucun objet de l'interface.
Extracted extractFile(const QString& path, const QByteArray& bytes)
{
    runWriteProbe();
    QSaveFile out(path);
    if (!(out.open(QIODevice::WriteOnly) && out.write(bytes) == bytes.size() && out.commit()))
        return Extracted::WriteFailed;
    if (!markAsDownloaded(path)) {
        QFile::remove(path);
        return Extracted::MarkFailed;
    }
    return Extracted::Written;
}

} // namespace

QString LinkLauncher::extractionDir(const QString& documentKey)
{
    std::unique_ptr<QTemporaryDir>& dir = m_extractionDirs[documentKey];
    if (!dir) {
        // Nom imprévisible (mkdtemp), droits 0700 hors de Windows ; retiré avec le lanceur.
        auto made = std::make_unique<QTemporaryDir>(
            QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QStringLiteral("/actcraft-") +
            hashedKey(documentKey) + QStringLiteral("-XXXXXX"));
        if (!made->isValid())
            return {};
        dir = std::move(made);
    }
    return dir->path();
}

void LinkLauncher::requestStored(const QByteArray& bytes, const QString& originalName,
                                 const QString& documentKey)
{
    const QString name = safeFileName(originalName);
    const QString reason = name.isEmpty()
                               ? tr("Le fichier rangé dans le paperboard n'a pas de nom utilisable.")
                               : suffixRefusal(name);
    if (!reason.isEmpty()) {
        emit refused(reason); // refusé avant toute écriture
        return;
    }
    const QString dir = extractionDir(documentKey);
    if (dir.isEmpty()) {
        emit refused(tr("Impossible d'extraire le fichier %1 du paperboard.").arg(name));
        return;
    }
    const QString path = dir + u'/' + name;
    if (!m_writers) {
        m_writers = std::make_unique<QThreadPool>();
        m_writers->setMaxThreadCount(1); // l'ordre des demandes est celui des confirmations
    }
    // Écriture et marque « venu d'ailleurs » hors du fil de l'interface (même patron que
    // DocumentController::saveToUrl) ; le QFutureWatcher, fils de ce lanceur, ne livre plus rien une fois
    // celui-ci détruit.
    auto* watcher = new QFutureWatcher<Extracted>(this);
    const QString written = QUrl::fromLocalFile(path).toLocalFile();
    m_writing.insert(written, m_writing.value(written, 0) + 1);
    connect(watcher, &QFutureWatcher<Extracted>::finished, this, [this, watcher, path, name, written] {
        const Extracted result = watcher->result();
        watcher->deleteLater();
        if (--m_writing[written] <= 0)
            m_writing.remove(written);
        switch (result) {
        case Extracted::WriteFailed:
            emit refused(tr("Impossible d'extraire le fichier %1 du paperboard.").arg(name));
            return;
        case Extracted::MarkFailed:
            emit refused(
                tr("Le fichier %1 n'a pas pu être marqué comme venu d'un paperboard : il ne s'ouvre pas.")
                    .arg(name));
            return;
        case Extracted::Written:
            break;
        }
        ask(QUrl::fromLocalFile(path), StoredFile);
    });
    watcher->setFuture(
        QtConcurrent::run(m_writers.get(), [path, bytes] { return extractFile(path, bytes); }));
}

void LinkLauncher::answer(int token, bool accepted)
{
    if (!m_pending.contains(token))
        return; // jeton inconnu ou déjà servi
    const Pending pending = m_pending.take(token);
    if (!accepted)
        return;
    // Le fichier est en train d'être remplacé (même chemin, demande suivante) : il n'a pas encore sa marque
    // « venu d'ailleurs ». La nouvelle demande arrive à la confirmation une fois le fichier marqué.
    if (pending.kind == StoredFile && m_writing.contains(pending.url.toLocalFile())) {
        emit refused(tr("Le fichier %1 est en cours d'extraction : confirmez à nouveau dans un instant.")
                         .arg(QDir::toNativeSeparators(pending.url.toLocalFile())));
        return;
    }
    // Revalidée : le fichier a pu changer depuis la demande.
    if (const QString reason = refusalFor(pending.url, pending.kind); !reason.isEmpty()) {
        emit refused(reason);
        return;
    }
    if (!m_opener(pending.url)) {
        emit refused(tr("Le système n'a pas pu ouvrir %1.").arg(pending.url.toString(QUrl::FullyEncoded)));
        return;
    }
    ++m_openedCount;
    emit opened(pending.url);
}

QString LinkLauncher::refusalFor(const QUrl& url, int kind)
{
    return displayed(rawRefusal(url, kind));
}

int LinkLauncher::diskQueries()
{
    return g_diskQueries.load();
}

void LinkLauncher::ask(const QUrl& url, int kind)
{
    if (const QString reason = refusalFor(url, kind); !reason.isEmpty()) {
        emit refused(reason);
        return;
    }
    Pending pending{url, kind};
    QString shown = url.toString(QUrl::FullyEncoded);
    if (kind != Website) {
        const QString path = strippedLocalPath(url);
        pending.url = QUrl::fromLocalFile(path); // le fichier montré est celui qui s'ouvre
        shown = QDir::toNativeSeparators(path);
    }
    const int token = m_nextToken++;
    m_pending.insert(token, pending);
    emit confirmationNeeded(displayed(shown), token);
}

} // namespace actcraft::ui
