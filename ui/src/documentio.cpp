#include <actcraft/formats/flipchart.h>
#include <actcraft/formats/nativefile.h>
#include <actcraft/formats/urlio.h>
#include <actcraft/render/pagepainter.h>
#include <actcraft/ui/documentio.h>

#include <QByteArray>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QSysInfo>
#include <QThread>

#include <cstddef>
#include <utility>
#include <vector>

namespace actcraft::ui {

namespace {

// Plus grand côté d'une image décodée pour les miniatures. Une miniature fait kThumbnailWidth pixels de
// large, une image n'y paraît pas plus grande (sauf à déborder de la page) ; le double laisse de la marge.
// Décodées en grand, 30 photos de 12 Mpx tiendraient 1,4 Go à chaque enregistrement ; réduites, 37 Mo.
constexpr int kThumbnailImageSide = 2 * render::kThumbnailWidth;

// Nom du fichier visé : fichier local, ou dernier segment du chemin d'une URL content:// (décodé une fois).
QString targetName(const QUrl& url)
{
    return url.isLocalFile() ? QFileInfo(url.toLocalFile()).fileName()
                             : url.path(QUrl::FullyDecoded).section(u'/', -1);
}

bool isFlipchartName(const QString& name)
{
    return name.endsWith(QStringLiteral(".flipchart"), Qt::CaseInsensitive);
}

QString flipchartRefusal()
{
    return QStringLiteral("Un paperboard .flipchart ne se réécrit pas : enregistrez-le au format ActCraft "
                          "(.actcraft).");
}

} // namespace

LoadOutcome loadDocument(const QUrl& url)
{
    LoadOutcome out;
    out.offMainThread = !QThread::isMainThread();
    const formats::UrlReadResult read = formats::readUrl(url);
    if (!read.ok) {
        out.error = read.error;
        return out;
    }
    out.title = titleFromUrl(url);
    if (formats::looksLikeActcraft(read.data)) {
        formats::DecodeResult r = formats::decodeActcraft(read.data);
        if (!r.ok) {
            out.error = r.error;
            return out;
        }
        out.ok = true;
        out.native = true;
        out.warnings = r.warnings;
        out.document = std::move(r.document);
        return out;
    }
    if (formats::flipchart::looksLikeFlipchart(read.data)) {
        formats::flipchart::FlipchartReadResult r = formats::flipchart::readFlipchart(read.data);
        out.warnings = r.warnings;
        if (!r.ok) {
            out.error = r.error;
            return out;
        }
        out.ok = true;
        out.document = std::move(r.document);
        return out;
    }
    out.error = QStringLiteral("Format non reconnu : ni paperboard ActCraft, ni .flipchart.");
    return out;
}

SaveOutcome saveDocument(const core::Document& snapshot, const QUrl& url)
{
    SaveOutcome out;
    out.offMainThread = !QThread::isMainThread();
    if (isFlipchartName(targetName(url))) {
        out.error = flipchartRefusal(); // décision 18, même appelé sans passer par actcraftTarget
        return out;
    }
    const render::DecodedImages images = render::decodeImages(snapshot, kThumbnailImageSide);
    std::vector<QByteArray> thumbnails;
    thumbnails.reserve(snapshot.pageCount());
    for (std::size_t i = 0; i < snapshot.pageCount(); ++i) {
        thumbnails.push_back(render::thumbnailPng(snapshot.page(i), images.lookup()));
    }
    QString error;
    const QByteArray archive = formats::encodeActcraft(snapshot, thumbnails, &error);
    if (archive.isEmpty()) {
        out.error = error;
        return out;
    }
    if (!formats::writeUrl(url, archive, &error)) {
        out.error = error;
        return out;
    }
    out.ok = true;
    out.fingerprint = core::fingerprint(snapshot);
    return out;
}

QUrl actcraftTarget(const QUrl& url, QString* error)
{
    const QString name = targetName(url);
    if (isFlipchartName(name)) {
        if (error) {
            *error = flipchartRefusal();
        }
        return {};
    }
    if (url.isLocalFile() && !name.endsWith(QStringLiteral(".actcraft"), Qt::CaseInsensitive)) {
        return QUrl::fromLocalFile(url.toLocalFile() + QStringLiteral(".actcraft"));
    }
    return url;
}

QString titleFromUrl(const QUrl& url)
{
    // content:// : « primary:Download/cours.actcraft » (fournisseur de documents) → « cours.actcraft ».
    const QString name =
        url.isLocalFile() ? targetName(url) : targetName(url).section(u':', -1).section(u'/', -1);
    const qsizetype dot = name.lastIndexOf(u'.');
    return dot > 0 ? name.left(dot) : name;
}

QString defaultDocumentsFolder(const QString& documents, bool create)
{
    // Choix à l'exécution (pas de #ifdef hors de app/ et des fichiers *_android, DA10).
    const QString base = !documents.isEmpty() ? documents + QStringLiteral("/ActCraft")
                         : QSysInfo::productType() == QLatin1String("android")
                             ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                             : QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) +
                                   QStringLiteral("/ActCraft");
    if (create) {
        QDir().mkpath(base);
    }
    return base;
}

} // namespace actcraft::ui
