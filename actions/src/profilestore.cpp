#include <actcraft/actions/profilestore.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QSaveFile>

#include <utility>

namespace actcraft::actions {

namespace {

const QString kExportFormatPrefix = QStringLiteral("actcraft-profile-export/");
const QString kExportFormat = QStringLiteral("actcraft-profile-export/1");
const QString kLayoutFile = QStringLiteral("/layout.json");

// `^[a-z0-9-]+$` sans le piège du `$` de QRegularExpression, qui accepte un saut de ligne final.
bool isValidId(const QString& id)
{
    if (id.isEmpty())
        return false;
    for (const QChar c : id) {
        const char16_t u = c.unicode();
        if (!((u >= u'a' && u <= u'z') || (u >= u'0' && u <= u'9') || u == u'-'))
            return false;
    }
    return true;
}

// Vrai si l'identifiant est refusé ; `error` dit pourquoi.
bool refuseId(const QString& id, QString& error)
{
    if (isValidId(id))
        return false;
    error = QStringLiteral("identifiant de profil refusé : « %1 » (lettres a à z, chiffres, tirets)").arg(id);
    return true;
}

bool readBytes(const QString& path, QByteArray& bytes, QString& error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("lecture impossible : %1 (%2)").arg(path, file.errorString());
        return false;
    }
    bytes = file.readAll();
    return true;
}

// Écriture atomique : le fichier précédent reste intact tant que `commit` n'a pas réussi.
bool writeAtomically(const QString& path, const QByteArray& bytes, QString& error)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        error = QStringLiteral("écriture impossible : %1 (%2)").arg(path, file.errorString());
        file.cancelWriting();
        return false;
    }
    return true;
}

std::optional<QJsonObject> objectFrom(const QByteArray& bytes)
{
    QJsonParseError parse{};
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &parse);
    if (parse.error != QJsonParseError::NoError || !document.isObject())
        return std::nullopt;
    return document.object();
}

// « actcraft-profile-export/1 », « …/1.2 » : version majeure 1.
bool exportMajorVersionIsOne(const QJsonValue& format)
{
    const QString text = format.toString();
    return format.isString() && text.startsWith(kExportFormatPrefix) &&
           text.mid(kExportFormatPrefix.size()).section(QLatin1Char('.'), 0, 0) == QLatin1String("1");
}

} // namespace

ProfileStore::ProfileStore(QString userRoot, QString builtinRoot, QStringList builtinOrder)
    : m_userRoot(std::move(userRoot)), m_builtinRoot(std::move(builtinRoot)),
      m_builtinOrder(std::move(builtinOrder))
{}

QString ProfileStore::userFile(const QString& id) const
{
    return isValidId(id) ? m_userRoot + QLatin1Char('/') + id + QStringLiteral(".json") : QString();
}

QStringList ProfileStore::ids() const
{
    QStringList builtins;
    for (const QString& id : m_builtinOrder) {
        if (isBuiltin(id) && !builtins.contains(id))
            builtins << id;
    }
    QStringList users;
    const QStringList files = QDir(m_userRoot).entryList({QStringLiteral("*.json")}, QDir::Files);
    for (const QString& name : files) {
        const QString id = name.chopped(5); // « .json »
        if (isValidId(id) && !isBuiltin(id))
            users << id;
    }
    users.sort();
    return builtins + users;
}

bool ProfileStore::isBuiltin(const QString& id) const
{
    return isValidId(id) && m_builtinOrder.contains(id) &&
           QFileInfo(m_builtinRoot + QLatin1Char('/') + id + QStringLiteral(".json")).isFile();
}

bool ProfileStore::hasUserCopy(const QString& id) const
{
    return isValidId(id) && QFileInfo(userFile(id)).isFile();
}

std::optional<Profile> ProfileStore::load(const QString& id, QString& error) const
{
    if (refuseId(id, error))
        return std::nullopt;
    QString path;
    if (hasUserCopy(id))
        path = userFile(id);
    else if (isBuiltin(id))
        path = m_builtinRoot + QLatin1Char('/') + id + QStringLiteral(".json");
    else {
        error = QStringLiteral("profil introuvable : %1").arg(id);
        return std::nullopt;
    }
    QByteArray bytes;
    if (!readBytes(path, bytes, error))
        return std::nullopt;
    Profile profile;
    QString why;
    if (!profileFromJson(bytes, profile, why)) {
        error = QStringLiteral("%1 : %2").arg(path, why);
        return std::nullopt;
    }
    profile.id = id; // le nom du fichier fait foi
    return profile;
}

QString ProfileStore::directory(const QString& id) const
{
    return isValidId(id) ? m_userRoot + QLatin1Char('/') + id : QString();
}

bool ProfileStore::save(const Profile& profile, QString& error)
{
    if (refuseId(profile.id, error))
        return false;
    if (!QDir().mkpath(m_userRoot)) {
        error = QStringLiteral("dossier des profils impossible à créer : %1").arg(m_userRoot);
        return false;
    }
    return writeAtomically(userFile(profile.id), profileToJson(profile), error);
}

bool ProfileStore::saveAs(const Profile& profile, const QString& newId, const QString& label, QString& error)
{
    if (refuseId(newId, error))
        return false;
    if (isBuiltin(newId) || hasUserCopy(newId)) {
        error = QStringLiteral("un profil « %1 » existe déjà").arg(newId);
        return false;
    }
    Profile copy = profile;
    copy.id = newId;
    copy.label = label;
    return save(copy, error);
}

bool ProfileStore::remove(const QString& id, QString& error)
{
    if (refuseId(id, error))
        return false;
    if (isBuiltin(id)) {
        error = QStringLiteral("un profil fourni ne se supprime pas : %1 (réinitialiser retire la copie)")
                    .arg(id);
        return false;
    }
    if (!hasUserCopy(id)) {
        error = QStringLiteral("profil introuvable : %1").arg(id);
        return false;
    }
    if (!QFile::remove(userFile(id)) || !QDir(directory(id)).removeRecursively()) {
        error = QStringLiteral("suppression impossible : %1").arg(id);
        return false;
    }
    return true;
}

bool ProfileStore::reset(const QString& id, QString& error)
{
    if (refuseId(id, error))
        return false;
    if (!isBuiltin(id)) {
        error = QStringLiteral("seul un profil fourni se réinitialise : %1").arg(id);
        return false;
    }
    if ((hasUserCopy(id) && !QFile::remove(userFile(id))) || !QDir(directory(id)).removeRecursively()) {
        error = QStringLiteral("réinitialisation impossible : %1").arg(id);
        return false;
    }
    return true;
}

bool ProfileStore::exportTo(const QString& id, const QString& filePath, QString& error) const
{
    if (refuseId(id, error))
        return false;
    const std::optional<Profile> profile = load(id, error);
    if (!profile)
        return false;
    QJsonValue layout(QJsonValue::Null);
    const QString layoutPath = directory(id) + kLayoutFile;
    if (QFileInfo(layoutPath).isFile()) {
        QByteArray bytes;
        if (!readBytes(layoutPath, bytes, error))
            return false;
        const std::optional<QJsonObject> object = objectFrom(bytes);
        if (!object) {
            error = QStringLiteral("disposition illisible : %1").arg(layoutPath);
            return false;
        }
        layout = *object;
    }
    // Le profil passe par profileToJson, qui réécrit aussi ses clés inconnues.
    const QJsonObject envelope{
        {QStringLiteral("format"), kExportFormat},
        {QStringLiteral("profile"), objectFrom(profileToJson(*profile)).value_or(QJsonObject())},
        {QStringLiteral("layout"), layout},
    };
    return writeAtomically(filePath, QJsonDocument(envelope).toJson(QJsonDocument::Indented), error);
}

std::optional<QString> ProfileStore::importFrom(const QString& filePath, QString& error)
{
    // Tout est vérifié avant la première écriture.
    QByteArray bytes;
    if (!readBytes(filePath, bytes, error))
        return std::nullopt;
    const std::optional<QJsonObject> envelope = objectFrom(bytes);
    if (!envelope) {
        error = QStringLiteral("fichier d'export illisible : %1").arg(filePath);
        return std::nullopt;
    }
    if (!exportMajorVersionIsOne(envelope->value(QStringLiteral("format")))) {
        error = QStringLiteral("format d'export non pris en charge : %1").arg(filePath);
        return std::nullopt;
    }
    const QJsonValue profileValue = envelope->value(QStringLiteral("profile"));
    if (!profileValue.isObject()) {
        error = QStringLiteral("fichier d'export sans profil : %1").arg(filePath);
        return std::nullopt;
    }
    Profile profile;
    QString why;
    if (!profileFromJson(QJsonDocument(profileValue.toObject()).toJson(QJsonDocument::Compact), profile,
                         why)) {
        error = QStringLiteral("%1 : %2").arg(filePath, why);
        return std::nullopt;
    }
    if (refuseId(profile.id, error))
        return std::nullopt;
    const QJsonValue layout = envelope->value(QStringLiteral("layout"));
    if (!layout.isNull() && !layout.isUndefined() && !layout.isObject()) {
        error = QStringLiteral("disposition illisible : %1").arg(filePath);
        return std::nullopt;
    }

    // Jamais un identifiant existant (profil fourni, copie ou dossier) : <id>, <id>-2, <id>-3…
    QString id = profile.id;
    for (int n = 2; isBuiltin(id) || hasUserCopy(id) || QFileInfo::exists(directory(id)); ++n)
        id = profile.id + QLatin1Char('-') + QString::number(n);
    profile.id = id;
    if (!save(profile, error))
        return std::nullopt;
    if (layout.isObject()) {
        const QString dir = directory(id);
        bool written = QDir().mkpath(dir);
        if (!written)
            error = QStringLiteral("dossier du profil impossible à créer : %1").arg(dir);
        else
            written = writeAtomically(
                dir + kLayoutFile, QJsonDocument(layout.toObject()).toJson(QJsonDocument::Indented), error);
        if (!written) {
            QFile::remove(userFile(id));
            QDir(dir).removeRecursively();
            return std::nullopt;
        }
    }
    return id;
}

} // namespace actcraft::actions
