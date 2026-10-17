#include <actcraft/formats/urlio.h>

#include <QDir>
#include <QFile>
#include <QSaveFile>

#include <algorithm>

namespace actcraft::formats {

using namespace Qt::StringLiterals;

namespace {

constexpr qsizetype kChunk = qsizetype{1} << 20; // 1 Mio
constexpr qint64 kMaxReserve = qint64{64} << 20; // 64 Mio

bool isContent(const QUrl& url)
{
    return url.scheme() == u"content"_s;
}

} // namespace

std::optional<QByteArray> readBounded(QIODevice& device, qint64 maxBytes, QString* error)
{
    const auto fail = [error](const QString& why) -> std::optional<QByteArray> {
        if (error) {
            *error = why;
        }
        return std::nullopt;
    };
    const auto tooBig = [&fail, maxBytes] {
        return fail(u"fichier trop volumineux (plus de %1 octets)"_s.arg(maxBytes));
    };
    // Jamais device.read(maxBytes) : QIODevice réserve la taille demandée avant de lire (relecture du plan :
    // 2 Gio réservés pour un fichier de 7 octets). Taille annoncée au-delà de la borne : refusé sans lire.
    // Réservé : la taille annoncée, plafonnée à 64 Mio (celle d'un content:// vient du fournisseur) ;
    // au-delà, le tampon grandit par blocs.
    QByteArray data;
    if (!device.isSequential()) {
        const qint64 remaining = device.size() - device.pos();
        if (remaining > maxBytes) {
            return tooBig();
        }
        data.reserve(static_cast<qsizetype>(std::clamp<qint64>(remaining, 0, kMaxReserve)));
    }
    // Jamais atEnd() : un appareil séquentiel de taille inconnue (content://) dit « fini » avant la première
    // lecture. Blocs de 1 Mio jusqu'à un bloc vide ; -1 est une erreur, pas une fin.
    QByteArray chunk(kChunk, Qt::Uninitialized);
    for (;;) {
        const qint64 n = device.read(chunk.data(), kChunk);
        if (n < 0) {
            return fail(u"lecture impossible (%1)"_s.arg(device.errorString()));
        }
        if (n == 0) {
            break;
        }
        if (static_cast<qint64>(data.size()) + n > maxBytes) {
            return tooBig();
        }
        data.append(chunk.constData(), static_cast<qsizetype>(n));
    }
    return data;
}

UrlReadResult readUrl(const QUrl& url, qint64 maxBytes)
{
    UrlReadResult r;
    if (!url.isLocalFile() && !isContent(url)) {
        r.error = u"URL non prise en charge : %1"_s.arg(url.toString());
        return r;
    }
    QFile file(url.isLocalFile() ? url.toLocalFile() : url.toString());
    if (!file.open(QIODevice::ReadOnly)) {
        r.error = u"ouverture impossible : %1 (%2)"_s.arg(url.toDisplayString(), file.errorString());
        return r;
    }
    // Un seul chemin pour les deux schémas : taille annoncée contrôlée d'abord (fichier local), puis lecture
    // bornée par blocs, qui tient aussi si le fichier grandit entre-temps.
    QString why;
    std::optional<QByteArray> data = readBounded(file, maxBytes, &why);
    if (!data) {
        r.error = u"%1 : %2"_s.arg(url.toDisplayString(), why);
        return r;
    }
    r.ok = true;
    r.data = std::move(*data);
    return r;
}

bool writeUrl(const QUrl& url, const QByteArray& data, QString* error)
{
    const auto fail = [error](const QString& why) {
        if (error) {
            *error = why;
        }
        return false;
    };
    if (url.isLocalFile()) {
        // Jamais d'écriture en place d'un fichier existant : temporaire voisin, puis renommage par-dessus.
        // Sans repli en écriture directe (dossier sans droit de création) : mieux vaut échouer qu'écraser à
        // moitié.
        QSaveFile file(url.toLocalFile());
        file.setDirectWriteFallback(false);
        if (!file.open(QIODevice::WriteOnly)) {
            return fail(u"écriture impossible : %1 (%2)"_s.arg(url.toDisplayString(), file.errorString()));
        }
        if (file.write(data) != data.size()) {
            const QString why = file.errorString();
            file.cancelWriting();
            return fail(u"écriture incomplète : %1 (%2)"_s.arg(url.toDisplayString(), why));
        }
        if (!file.commit()) { // en cas d'échec, QSaveFile supprime son temporaire
            return fail(
                u"enregistrement impossible : %1 (%2)"_s.arg(url.toDisplayString(), file.errorString()));
        }
        return true;
    }
    if (isContent(url)) {
        QFile file(url.toString());
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            return fail(u"écriture impossible : %1 (%2)"_s.arg(url.toDisplayString(), file.errorString()));
        }
        if (file.write(data) != data.size() || !file.flush()) {
            return fail(u"écriture incomplète : %1 (%2)"_s.arg(url.toDisplayString(), file.errorString()));
        }
        file.close(); // erreur possible à la fermeture (fournisseur de documents) : la lire
        if (file.error() != QFileDevice::NoError) {
            return fail(
                u"enregistrement impossible : %1 (%2)"_s.arg(url.toDisplayString(), file.errorString()));
        }
        return true;
    }
    return fail(u"URL non prise en charge : %1"_s.arg(url.toString()));
}

QUrl urlFromArgument(const QVariant& argument)
{
    if (argument.typeId() == QMetaType::QUrl) {
        return argument.toUrl();
    }
    const QString text = argument.toString().trimmed();
    if (text.isEmpty()) {
        return {};
    }
    if (text.startsWith(u"content:"_s) || text.startsWith(u"file:"_s)) {
        return QUrl(text);
    }
    return QUrl::fromLocalFile(QDir::cleanPath(QDir::current().absoluteFilePath(text)));
}

} // namespace actcraft::formats
