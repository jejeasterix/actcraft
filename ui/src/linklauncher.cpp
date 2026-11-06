#include "actcraft/ui/linklauncher.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QStringList>

#include <algorithm>
#include <utility>

namespace actcraft::ui {
namespace {

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

// Chemin d'un fichier local, nettoyé, nom débarrassé de ses points et espaces finaux : le fichier que le
// système ouvrirait. À n'appeler qu'après le refus des chemins réseau (cleanPath réduirait « // » hors de
// Windows).
QString strippedLocalPath(const QUrl& url)
{
    const QString path = QDir::cleanPath(QDir::fromNativeSeparators(url.toLocalFile()));
    const qsizetype slash = path.lastIndexOf(u'/');
    return path.left(slash + 1) + withoutTrailingDotsAndSpaces(path.mid(slash + 1));
}

bool isPortableNameChar(char16_t c)
{
    return (c >= u'a' && c <= u'z') || (c >= u'A' && c <= u'Z') || (c >= u'0' && c <= u'9') || c == u'.' ||
           c == u'_' || c == u' ' || c == u'-';
}

// Nom sûr d'un fichier rangé dans le document : son dernier composant seul (aucun chemin fourni par le
// document), réduit aux caractères [A-Za-z0-9._ -], sans points ni espaces en tête (ni nom caché, ni « .. »)
// ni en fin (le nom que Windows retiendrait). Un nom de périphérique de Windows (CON, NUL, COM1…) reçoit un «
// _ » en tête.
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
        QStringLiteral("CON PRN AUX NUL COM1 COM2 COM3 COM4 COM5 COM6 COM7 COM8 COM9 "
                       "LPT1 LPT2 LPT3 LPT4 LPT5 LPT6 LPT7 LPT8 LPT9")
            .split(QLatin1Char(' '));
    if (kDevices.contains(name.section(u'.', 0, 0).trimmed(), Qt::CaseInsensitive))
        name.prepend(u'_');
    return name;
}

// Clé du dossier d'extraction : [A-Za-z0-9_-] seuls, 64 au plus.
QString safeKey(const QString& documentKey)
{
    QString key;
    for (const QChar c : documentKey)
        if (isPortableNameChar(c.unicode()) && c != u'.' && c != u' ' && key.size() < 64)
            key += c;
    return key.isEmpty() ? QStringLiteral("document") : key;
}

} // namespace

LinkLauncher::LinkLauncher(QObject* parent) : QObject(parent), m_opener(systemOpener) {}

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
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
                        QStringLiteral("/actcraft-") + safeKey(documentKey);
    const QString path = dir + u'/' + name;
    bool written = QDir().mkpath(dir) && !QFileInfo(dir).isSymLink();
    if (written && QFileInfo(path).isSymLink())
        written = QFile::remove(path); // un lien posé à la place du fichier ne redirige pas l'écriture
    if (written) {
        QSaveFile out(path);
        written = out.open(QIODevice::WriteOnly) && out.write(bytes) == bytes.size() && out.commit();
    }
    if (!written) {
        emit refused(tr("Impossible d'extraire le fichier %1 du paperboard.").arg(name));
        return;
    }
    ask(QUrl::fromLocalFile(path), StoredFile);
}

void LinkLauncher::answer(int token, bool accepted)
{
    if (!m_pending.contains(token))
        return; // jeton inconnu ou déjà servi
    const Pending pending = m_pending.take(token);
    if (!accepted)
        return;
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
    const QString shown = url.toString(QUrl::FullyEncoded);
    if (kind == Website) {
        // QUrl rend le schéma en minuscules.
        if (!url.isValid() ||
            (url.scheme() != QLatin1String("http") && url.scheme() != QLatin1String("https")))
            return tr("Seules les adresses http et https s'ouvrent : %1").arg(shown);
        if (url.host().isEmpty())
            return tr("Adresse sans nom d'hôte : %1").arg(shown);
        return {};
    }
    if (kind != File && kind != StoredFile && kind != SystemMedia)
        return tr("Type de lien inconnu.");
    if (!url.isValid() || !url.isLocalFile())
        return tr("Seul un fichier de cet ordinateur s'ouvre : %1").arg(shown);
    // « \\hôte\… », « file://hôte/… », « file:////hôte/… » : jamais un chemin réseau.
    if (!url.host().isEmpty() ||
        QDir::fromNativeSeparators(url.toLocalFile()).startsWith(QStringLiteral("//")))
        return tr("Un chemin réseau ne s'ouvre pas depuis un paperboard : %1").arg(shown);
    const QString path = strippedLocalPath(url);
    const QString name = fileNameOf(path);
    if (name.contains(u':')) // flux de données d'un fichier sous Windows (« a.exe:b.pdf »)
        return tr("Nom de fichier refusé : %1").arg(QDir::toNativeSeparators(path));
    if (const QString reason = suffixRefusal(name); !reason.isEmpty())
        return reason;
    if (!QDir::isAbsolutePath(path))
        return tr("Un chemin relatif ne s'ouvre pas depuis un paperboard : %1")
            .arg(QDir::toNativeSeparators(path));
    const QFileInfo info(path); // existence testée sur le nom après retrait des points et espaces finaux
    if (!info.isFile())
        return tr("Fichier introuvable : %1").arg(QDir::toNativeSeparators(path));
    if (info.isSymLink() && !suffixRefusal(fileNameOf(info.canonicalFilePath())).isEmpty())
        return tr("Ce lien désigne un type de fichier refusé : %1").arg(QDir::toNativeSeparators(path));
    return {};
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
    emit confirmationNeeded(shown, token);
}

} // namespace actcraft::ui
