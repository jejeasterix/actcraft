#include "actcraft/ui/launchers.h"

#include "actcraft/ui/secretstore.h"

#include <QCryptographicHash>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageAuthenticationCode>
#include <QProcess>
#include <QRandomGenerator>
#include <QSet>
#include <QUrl>
#include <QUuid>

#include <QDateTime>
#include <algorithm>
#include <cstdio>
#include <utility>

namespace actcraft::ui {

namespace {

void diag(const char* step)
{
    std::fprintf(stderr, "DIAG lib %s %lld\n", step, static_cast<long long>(QDateTime::currentMSecsSinceEpoch()));
    std::fflush(stderr);
}

const QString kEntries = QStringLiteral("entries");
const QString kId = QStringLiteral("id");
const QString kPath = QStringLiteral("path");
const QString kArguments = QStringLiteral("arguments");
const QString kToolTip = QStringLiteral("toolTip");
const QString kSignature = QStringLiteral("signature");

// Temps constant : la durée de la comparaison ne dit pas combien d'octets concordent.
bool sameBytes(const QByteArray& a, const QByteArray& b)
{
    if (a.size() != b.size())
        return false;
    int diff = 0;
    for (qsizetype i = 0; i < a.size(); ++i)
        diff |= a.at(i) ^ b.at(i);
    return diff == 0;
}

// Un caractère de mise en forme (U+202E, U+200B, U+2066 à U+2069…) maquille ce que la boîte montre. Un
// caractère de contrôle (NUL, tabulation…) coupe ou déplace la ligne de commande de Windows, que Qt ne
// protège pas ; dans le chemin, un guillemet ou l'un de <>|?* aussi : le suffixe lu ici ne serait plus celui
// qui part.
bool refusedCharacter(const QString& text, bool controls, QLatin1String forbidden)
{
    for (qsizetype i = 0; i < text.size(); ++i) { // par point de code : U+E0001 tient en deux unités UTF-16
        const QChar ch = text.at(i);
        char32_t code = ch.unicode();
        if (ch.isHighSurrogate() && i + 1 < text.size() && text.at(i + 1).isLowSurrogate())
            code = QChar::surrogateToUcs4(ch, text.at(++i));
        const QChar::Category category = QChar::category(code);
        if (category == QChar::Other_Format || (controls && category == QChar::Other_Control) ||
            forbidden.contains(ch))
            return true;
    }
    return false;
}

// Un deux-points ailleurs qu'après une lettre de lecteur désigne un flux NTFS (`e.bat::$DATA`) ou un volume.
bool misplacedColon(const QString& path)
{
    const char16_t first = path.isEmpty() ? u'\0' : path.at(0).unicode();
    const bool drive = path.size() > 1 && path.at(1) == QLatin1Char(':') &&
                       ((first >= u'A' && first <= u'Z') || (first >= u'a' && first <= u'z'));
    return path.indexOf(QLatin1Char(':'), drive ? 2 : 0) >= 0;
}

bool unsafeFields(const QString& path, const QStringList& arguments, const QString& toolTip)
{
    if (refusedCharacter(path, true, QLatin1String("\"<>|?*")) || refusedCharacter(toolTip, false, {}) ||
        misplacedColon(path) || path.endsWith(QLatin1Char('/')) || path.endsWith(QLatin1Char('\\')))
        return true;
    return std::any_of(arguments.cbegin(), arguments.cend(),
                       [](const QString& argument) { return refusedCharacter(argument, true, {}); });
}

// Suffixe du nom visé, lu sur la chaîne seule (aucun accès disque avant la confirmation), séparateurs « / »
// et « \ » compris ; Windows retire du nom les points et espaces finaux.
QString suffixOf(const QString& path)
{
    // « . » et « .. » résolus comme Windows (`x.lnk\.`, `x.vbs\y\..`) ; QDir::fromNativeSeparators ne
    // convertirait « \ » que sous Windows.
    QString name = QString(path).replace(QLatin1Char('\\'), QLatin1Char('/'));
    for (QString before; before != name;) {
        before = name;
        name = QDir::cleanPath(name);
        while (name.endsWith(QLatin1Char('.')) || name.endsWith(QLatin1Char(' ')))
            name.chop(1);
    }
    name = name.mid(name.lastIndexOf(QLatin1Char('/')) + 1);
    const qsizetype dot = name.lastIndexOf(QLatin1Char('.'));
    return dot < 0 ? QString() : name.mid(dot + 1);
}

// Types que Windows exécute : cmd.exe interpréterait un .bat ou un .cmd et ses paramètres, openUrl
// (ShellExecute) lance tout type associé, et un .lnk porte une commande que la boîte ne peut pas montrer.
bool executedType(const QString& path)
{
    static const QStringList kTypes =
        QStringLiteral("bat cmd com scr pif vbs vbe js jse wsf wsh hta ps1 psm1 msi msp msc cpl reg lnk url "
                       "inf scf appref-ms "
                       "settingcontent-ms application gadget jar library-ms search-ms website xbap diagcab")
            .split(QLatin1Char(' '));
    return kTypes.contains(suffixOf(path), Qt::CaseInsensitive);
}

// Programme reconnu par son nom (.exe, ou nom sans suffixe cherché dans le PATH) : seul il prend des
// paramètres.
bool namedProgram(const QString& path)
{
    const QString suffix = suffixOf(path);
#ifdef Q_OS_WIN
    // CreateProcess compléterait un nom sans suffixe (.com avant .exe) : un .exe explicite seulement.
    return suffix.compare(QLatin1String("exe"), Qt::CaseInsensitive) == 0;
#else
    return suffix.isEmpty() || suffix.compare(QLatin1String("exe"), Qt::CaseInsensitive) == 0;
#endif
}

// Le critère de launch() : un document s'ouvre par openUrl, qui ne transmet aucun paramètre.
bool opensAsDocument(const QString& path)
{
    const QFileInfo info(path);
    return info.exists() && !(info.isFile() && info.isExecutable());
}

QString documentRefusal(const QString& path)
{
    return Launchers::tr("Un document ne prend pas de paramètres : %1").arg(path);
}

QString idOf(const QJsonArray& items, qsizetype index)
{
    return items.at(index).toObject().value(kId).toString();
}

} // namespace

Launchers::Launchers(QObject* parent) : Launchers(nullptr, QString::fromLatin1(kKeyName), parent) {}

Launchers::Launchers(ProfileController* profile, QString keyName, QObject* parent)
    : QObject(parent), m_keyName(std::move(keyName))
{
    setProfile(profile);
}

ProfileController* Launchers::profile() const
{
    return m_profile;
}

void Launchers::setProfile(ProfileController* profile)
{
    if (m_profile == profile)
        return;
    if (m_profile)
        disconnect(m_profile, nullptr, this, nullptr);
    m_profile = profile;
    m_pending.clear();
    if (m_profile)
        connect(m_profile, &ProfileController::profileChanged, this, &Launchers::entriesChanged);
    emit profileChanged();
    emit entriesChanged();
}

QVariantList Launchers::entries() const
{
    QVariantList out;
    const QByteArray poste = key(false);
    for (const Entry& entry : readEntries()) {
        out.append(QVariantMap{
            {kId, entry.id},
            {kPath, entry.path},
            {kArguments, entry.arguments},
            {kToolTip, entry.toolTip},
            {QStringLiteral("trusted"), trusted(entry, poste)},
        });
    }
    return out;
}

QString Launchers::localPath(const QUrl& url)
{
    return url.toLocalFile();
}

QString Launchers::add(const QString& path, const QStringList& arguments, const QString& toolTip)
{
    const QString program = path.trimmed();
    if (!m_profile || program.isEmpty())
        return {};
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QByteArray poste = key(true); // vide sans trousseau : le raccourci demandera confirmation
    const QByteArray signature =
        poste.isEmpty() ? QByteArray() : signatureOf(program, arguments, toolTip, poste);
    QJsonArray items = storedItems();
    items.append(QJsonObject{
        {kId, id},
        {kPath, program},
        {kArguments, QJsonArray::fromStringList(arguments)},
        {kToolTip, toolTip},
        {kSignature, QString::fromLatin1(signature.toBase64())},
    });
    store(items);
    return id;
}

bool Launchers::remove(const QString& id)
{
    if (!m_profile || id.isEmpty())
        return false;
    QJsonArray items = storedItems();
    const qsizetype before = items.size();
    for (qsizetype i = items.size() - 1; i >= 0; --i) {
        if (idOf(items, i) == id)
            items.removeAt(i);
    }
    if (items.size() == before)
        return false;
    m_pending.remove(id);
    store(items);
    return true;
}

int Launchers::run(const QString& id)
{
    const std::optional<Entry> entry = find(id);
    if (!entry) {
        emit failed(tr("Raccourci introuvable."));
        return kFailed;
    }
    // Refus avant toute confirmation, sur les chaînes seules (le disque n'est lu qu'au lancement), la garde
    // des caractères d'abord : les contrôles suivants lisent ce chemin.
    if (unsafeFields(entry->path, entry->arguments, entry->toolTip)) {
        emit failed(
            tr("Raccourci refusé : caractère invisible ou de contrôle, ou l'un de \" < > | ? * dans le "
               "chemin."));
        return kFailed;
    }
    if (executedType(entry->path)) {
        emit failed(tr("Ce type de fichier ne se lance pas depuis un raccourci : %1").arg(entry->path));
        return kFailed;
    }
    if (!entry->arguments.isEmpty() && !namedProgram(entry->path)) {
        emit failed(tr("Seul un programme (.exe ; sans suffixe hors Windows) prend des paramètres : %1")
                        .arg(entry->path));
        return kFailed;
    }
    diag("run avant key(false)");
    if (!trusted(*entry, key(false))) {
        diag("run apres key(false)");
        m_pending.insert(id, *entry);
        emit confirmationNeeded(entry->id, entry->path, entry->arguments);
        return kConfirmationNeeded;
    }
    return launch(*entry) ? kStarted : kFailed;
}

bool Launchers::confirm(const QString& id, bool accepted)
{
    if (!m_pending.contains(id))
        return false;
    const Entry shown = m_pending.take(id);
    if (!accepted)
        return false;
    // Ce qui part est ce qui a été montré : un raccourci changé depuis la demande ne part pas.
    const std::optional<Entry> current = find(id);
    if (!current || current->path != shown.path || current->arguments != shown.arguments ||
        current->toolTip != shown.toolTip) {
        emit failed(tr("Le raccourci a changé depuis la demande de confirmation."));
        return false;
    }
    diag("confirm avant key(true)");
    const QByteArray poste = key(true);
    diag("confirm apres key(true)");
    if (!poste.isEmpty())
        sign(shown, poste);
    diag("confirm apres sign");
    return launch(shown);
}

QByteArray Launchers::signatureOf(const QString& path, const QStringList& arguments, const QString& toolTip,
                                  const QByteArray& key)
{
    const QByteArray message = QJsonDocument(QJsonArray{path, QJsonArray::fromStringList(arguments), toolTip})
                                   .toJson(QJsonDocument::Compact);
    return QMessageAuthenticationCode::hash(message, key, QCryptographicHash::Sha256);
}

QJsonArray Launchers::storedItems() const
{
    return m_profile ? m_profile->profile().launchers.value(kEntries).toArray() : QJsonArray();
}

void Launchers::store(const QJsonArray& items)
{
    m_profile->editProfile().launchers.insert(kEntries, items);
    m_profile->touch();
}

// Lu du profil à chaque appel : une retouche à la main ou un profil importé est vu tout de suite. Un champ
// « trusted » écrit dans le profil n'est jamais lu.
QList<Launchers::Entry> Launchers::readEntries() const
{
    QList<Entry> out;
    QSet<QString> seen;
    const QJsonArray items = storedItems();
    for (qsizetype i = 0; i < items.size(); ++i) {
        const QJsonObject item = items.at(i).toObject();
        Entry entry;
        entry.id = item.value(kId).toString();
        // Identifiant en double : seule sa première occurrence compte, celle que sign() signe.
        if (seen.contains(entry.id))
            continue;
        seen.insert(entry.id);
        entry.path = item.value(kPath).toString();
        if (entry.id.isEmpty() || entry.path.isEmpty())
            continue;
        const QJsonArray arguments = item.value(kArguments).toArray();
        for (qsizetype a = 0; a < arguments.size(); ++a)
            entry.arguments.append(arguments.at(a).toString());
        entry.toolTip = item.value(kToolTip).toString();
        entry.signature = QByteArray::fromBase64(item.value(kSignature).toString().toLatin1());
        out.append(entry);
    }
    return out;
}

std::optional<Launchers::Entry> Launchers::find(const QString& id) const
{
    if (id.isEmpty())
        return std::nullopt;
    for (const Entry& entry : readEntries()) {
        if (entry.id == id)
            return entry;
    }
    return std::nullopt;
}

QByteArray Launchers::key(bool create) const
{
    if (!SecretStore::available())
        return {};
    const QByteArray stored = SecretStore::read(m_keyName).value_or(QByteArray());
    if (stored.size() == kKeyBytes)
        return stored;
    if (!create)
        return {};
    QByteArray fresh(kKeyBytes, '\0');
    for (char& byte : fresh)
        byte = static_cast<char>(QRandomGenerator::system()->bounded(256));
    return SecretStore::write(m_keyName, fresh) ? fresh : QByteArray();
}

bool Launchers::trusted(const Entry& entry, const QByteArray& key)
{
    return !key.isEmpty() &&
           sameBytes(entry.signature, signatureOf(entry.path, entry.arguments, entry.toolTip, key));
}

void Launchers::sign(const Entry& entry, const QByteArray& key)
{
    QJsonArray items = storedItems();
    for (qsizetype i = 0; i < items.size(); ++i) {
        if (idOf(items, i) != entry.id)
            continue;
        QJsonObject item = items.at(i).toObject();
        item.insert(kSignature, QString::fromLatin1(
                                    signatureOf(entry.path, entry.arguments, entry.toolTip, key).toBase64()));
        items.replace(i, item);
        store(items);
        return;
    }
}

// Un document s'ouvre avec son application ; un programme (ou un nom cherché dans le PATH) démarre avec ses
// paramètres en liste, sans interpréteur.
bool Launchers::launch(const Entry& entry)
{
    diag("launch avant opensAsDocument");
    const bool document = opensAsDocument(entry.path);
    diag(document ? "launch document" : "launch programme");
    if (document && !entry.arguments.isEmpty()) { // devenu document depuis la demande : rien ne part
        emit failed(documentRefusal(entry.path));
        return false;
    }
    const bool started =
        document ? QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(entry.path).absoluteFilePath()))
                 : QProcess::startDetached(entry.path, entry.arguments);
    diag(started ? "launch parti" : "launch echec");
    if (!started)
        emit failed(tr("Impossible de lancer %1.").arg(entry.path));
    return started;
}

} // namespace actcraft::ui
