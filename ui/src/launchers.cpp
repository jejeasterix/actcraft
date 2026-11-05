#include "actcraft/ui/launchers.h"

#include "actcraft/ui/secretstore.h"

#include <QCryptographicHash>
#include <QDesktopServices>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageAuthenticationCode>
#include <QProcess>
#include <QRandomGenerator>
#include <QSet>
#include <QUrl>
#include <QUuid>

#include <utility>

namespace actcraft::ui {

namespace {

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

// Un guillemet, un caractère de contrôle (NUL et tabulation compris) ou l'un de <>|?* déplace la fin du nom
// de programme dans la ligne de commande de Windows, que Qt ne protège pas : le suffixe lu ici ne serait plus
// celui qui part, et des paramètres non montrés suivraient.
bool unsafePath(const QString& path)
{
    for (const QChar ch : path) {
        if (ch.category() == QChar::Other_Control || QLatin1String("\"<>|?*").contains(ch))
            return true;
    }
    return false;
}

// Types que Windows exécute : cmd.exe interpréterait un .bat ou un .cmd et ses paramètres, openUrl
// (ShellExecute) lance tout type associé, et un .lnk porte une commande que la boîte ne peut pas montrer.
// Lu sur la chaîne seule : aucun accès disque avant la confirmation. Windows retire du nom les points et
// espaces finaux.
bool executedType(const QString& path)
{
    static const QStringList kTypes =
        QStringLiteral(
            "bat cmd com scr pif vbs vbe js jse wsf wsh hta ps1 psm1 msi msp msc cpl reg lnk url inf scf")
            .split(QLatin1Char(' '));
    QString name = path;
    while (name.endsWith(QLatin1Char('.')) || name.endsWith(QLatin1Char(' ')))
        name.chop(1);
    return kTypes.contains(QFileInfo(name).suffix(), Qt::CaseInsensitive);
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
    // Refus avant toute confirmation, la garde du chemin d'abord : les contrôles suivants lisent ce chemin.
    if (unsafePath(entry->path)) {
        emit failed(
            tr("Chemin refusé : il contient un guillemet, un caractère de contrôle ou l'un de < > | ? *."));
        return kFailed;
    }
    if (executedType(entry->path)) {
        emit failed(tr("Ce type de fichier ne se lance pas depuis un raccourci : %1").arg(entry->path));
        return kFailed;
    }
    // Le disque n'est lu que pour une entrée qui porte des paramètres.
    if (!entry->arguments.isEmpty() && opensAsDocument(entry->path)) {
        emit failed(documentRefusal(entry->path));
        return kFailed;
    }
    if (!trusted(*entry, key(false))) {
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
    const QByteArray poste = key(true);
    if (!poste.isEmpty())
        sign(shown, poste);
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
    const bool document = opensAsDocument(entry.path);
    if (document && !entry.arguments.isEmpty()) { // devenu document depuis la demande : rien ne part
        emit failed(documentRefusal(entry.path));
        return false;
    }
    const bool started =
        document ? QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(entry.path).absoluteFilePath()))
                 : QProcess::startDetached(entry.path, entry.arguments);
    if (!started)
        emit failed(tr("Impossible de lancer %1.").arg(entry.path));
    return started;
}

} // namespace actcraft::ui
