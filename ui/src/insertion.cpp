#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/core/replace.h>
#include <actcraft/core/undo.h>
#include <actcraft/formats/urlio.h>
#include <actcraft/ui/insertion.h>

#include <QBuffer>
#include <QCoreApplication>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QTextDocument>
#include <QThread>
#include <QtConcurrent>

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace actcraft::ui {

enum class Mode
{
    Image,
    Media,
    Any, // dépôt : image si le fichier se décode, sinon média
    Pasted,
};

struct InsertionJob
{
    QUrl url;
    Mode mode = Mode::Image;
    QImage pasted;
    bool hasCenter = false;
    QPointF center;
};

namespace {

constexpr double kMaxShare = 0.8;   // D19 : au plus 80 % de la page
constexpr double kMediaWidth = 320; // cadre de l'espace réservé d'un média
constexpr double kMediaHeight = 240;

struct Item
{
    bool isMedia = false;
    QByteArray data;
    std::string extension;
    std::string name;
    std::string fileName;
    int width = 0;
    int height = 0;
    bool hasCenter = false;
    QPointF center;
};

struct Result
{
    std::vector<Item> items;
    QString error;
    bool offMainThread = false;
};

bool isMediaExtension(const QString& suffix)
{
    static const QStringList known{"mp3", "wav", "ogg",  "oga", "m4a", "aac",  "flac", "mp4", "m4v",
                                   "mov", "avi", "webm", "mkv", "mpg", "mpeg", "wmv",  "opus"};
    return known.contains(suffix.toLower());
}

// Fil secondaire : lecture et décodage. Aucune requête réseau : formats::readUrl n'ouvre que fichier et
// content://.
Result work(const std::vector<InsertionJob>& jobs, QThread* mainThread)
{
    Result result;
    result.offMainThread = QThread::currentThread() != mainThread;
    for (const InsertionJob& job : jobs) {
        Item item;
        item.hasCenter = job.hasCenter;
        item.center = job.center;
        if (job.mode == Mode::Pasted) {
            QBuffer buffer(&item.data);
            buffer.open(QIODevice::WriteOnly);
            if (job.pasted.isNull() || !job.pasted.save(&buffer, "PNG")) {
                result.error = QStringLiteral("Image collée illisible");
                continue;
            }
            item.extension = "png";
            item.width = job.pasted.width();
            item.height = job.pasted.height();
            result.items.push_back(std::move(item));
            continue;
        }
        if (job.url.scheme() != QLatin1String("file") && job.url.scheme() != QLatin1String("content")) {
            result.error = QStringLiteral("Adresse non prise en charge");
            continue;
        }
        const formats::UrlReadResult read = formats::readUrl(job.url);
        if (!read.ok) {
            result.error = read.error.isEmpty() ? QStringLiteral("Lecture impossible") : read.error;
            continue;
        }
        const QFileInfo info(job.url.isLocalFile() ? job.url.toLocalFile() : job.url.path());
        item.name = info.completeBaseName().toStdString();
        item.fileName = info.fileName().toStdString();
        item.extension = info.suffix().toLower().toStdString();
        item.data = read.data;
        bool media = job.mode == Mode::Media;
        if (!media) {
            QImage image;
            if (image.loadFromData(read.data)) {
                item.width = image.width();
                item.height = image.height();
            } else if (job.mode == Mode::Any && isMediaExtension(info.suffix())) {
                media = true;
            } else {
                result.error = QStringLiteral("Image illisible : %1").arg(info.fileName());
                continue;
            }
        }
        item.isMedia = media;
        result.items.push_back(std::move(item));
    }
    return result;
}

// Une étape : ressources (clé SHA-256 tirée au premier redo) puis ReplaceObjectsCommand({}, objets).
class InsertCommand final : public core::UndoCommand
{
public:
    struct Entry
    {
        core::PageObject object;
        QByteArray data;
        std::string extension;
    };
    InsertCommand(std::size_t page, std::vector<Entry> entries) : m_page(page), m_entries(std::move(entries))
    {}
    void redo(core::Document& document, core::Changes& changes) override
    {
        const bool first = !m_inner;
        std::vector<core::PageObject> objects;
        for (Entry& entry : m_entries) {
            const std::string key = document.addResource(entry.data, entry.extension);
            if (first) {
                if (auto* image = std::get_if<core::ImageObject>(&core::body(entry.object)))
                    image->resource = key;
                else if (auto* media = std::get_if<core::Media>(&core::body(entry.object)))
                    media->media = key;
                objects.push_back(entry.object);
            }
        }
        if (first)
            m_inner = std::make_unique<core::ReplaceObjectsCommand>(m_page, std::vector<core::ObjectId>{},
                                                                    std::move(objects));
        m_inner->redo(document, changes);
    }
    void undo(core::Document& document, core::Changes& changes) override
    {
        if (m_inner)
            m_inner->undo(document, changes);
    }

private:
    std::size_t m_page;
    std::vector<Entry> m_entries;
    std::unique_ptr<core::ReplaceObjectsCommand> m_inner;
};

core::RectF centeredRect(QPointF center, double width, double height)
{
    return {center.x() - width / 2, center.y() - height / 2, width, height};
}

} // namespace

Insertion::Insertion(QObject* parent) : QObject(parent) {}

DocumentController* Insertion::document() const
{
    return m_document;
}

void Insertion::setDocument(DocumentController* document)
{
    if (m_document == document)
        return;
    m_document = document;
    emit documentChanged();
}

bool Insertion::isBusy() const
{
    return m_pending > 0;
}

bool Insertion::lastDecodeOffMainThread() const
{
    return m_lastOff;
}

namespace {

// Pose les objets sur la page `pageIndex` en une étape. Retour : nombre d'objets posés.
int place(DocumentController& controller, std::vector<Item>& items, std::size_t pageIndex)
{
    const core::Page& target = controller.document().page(pageIndex);
    const QSizeF size(target.width, target.height);
    std::vector<InsertCommand::Entry> entries;
    for (Item& item : items) {
        const QPointF center = item.hasCenter ? item.center : QPointF(size.width() / 2, size.height() / 2);
        InsertCommand::Entry entry;
        if (item.isMedia) {
            core::Media media;
            media.bounds = centeredRect(center, kMediaWidth, kMediaHeight);
            media.fileName = item.fileName;
            entry.object = std::move(media);
        } else {
            double scale = 1.0;
            if (item.width > 0 && item.height > 0)
                scale = std::min(
                    {1.0, kMaxShare * size.width() / item.width, kMaxShare * size.height() / item.height});
            core::ImageObject image;
            image.sourceWidth = item.width;
            image.sourceHeight = item.height;
            image.bounds = centeredRect(center, item.width * scale, item.height * scale);
            entry.object = std::move(image);
        }
        entry.object.common.layer = core::Layer::Middle;
        entry.object.common.name = item.name;
        entry.data = std::move(item.data);
        entry.extension = item.extension;
        entries.push_back(std::move(entry));
    }
    if (entries.empty())
        return 0;
    const int count = static_cast<int>(entries.size());
    controller.execute(std::make_unique<InsertCommand>(pageIndex, std::move(entries)));
    return count;
}

} // namespace

void Insertion::start(std::vector<InsertionJob> jobs)
{
    if (m_document == nullptr)
        return;
    if (m_pending++ == 0)
        emit busyChanged();
    auto* watcher = new QFutureWatcher<Result>(this);
    QThread* mainThread = QThread::currentThread();
    // Document et page devant lesquels l'insertion est demandée : un changement d'onglet pendant le décodage
    // ne doit pas verser l'image dans l'autre.
    const std::uint64_t generation = m_document->generation();
    const std::size_t originPage = static_cast<std::size_t>(m_document->currentIndex());
    connect(watcher, &QFutureWatcher<Result>::finished, this, [this, watcher, generation, originPage]() {
        Result result = watcher->result();
        watcher->deleteLater();
        m_lastOff = result.offMainThread;
        int count = 0;
        if (m_document != nullptr && !result.items.empty()) {
            if (m_document->generation() == generation && originPage < m_document->document().pageCount())
                count = place(*m_document, result.items, originPage);
            else if (result.error.isEmpty())
                result.error = QStringLiteral("Insertion abandonnée : le document affiché a changé.");
        }
        if (--m_pending == 0)
            emit busyChanged();
        if (!result.error.isEmpty())
            emit failed(result.error);
        if (count > 0)
            emit inserted(count);
    });
    watcher->setFuture(
        QtConcurrent::run([jobs = std::move(jobs), mainThread]() { return work(jobs, mainThread); }));
}

void Insertion::insertImage(const QUrl& url)
{
    InsertionJob job;
    job.url = url;
    job.mode = Mode::Image;
    start({job});
}

void Insertion::insertMedia(const QUrl& url)
{
    InsertionJob job;
    job.url = url;
    job.mode = Mode::Media;
    start({job});
}

void Insertion::insertImageData(const QImage& image)
{
    InsertionJob job;
    job.mode = Mode::Pasted;
    job.pasted = image;
    start({job});
}

bool Insertion::acceptDrop(const QVariantList& urls, const QString& text, QPointF pagePoint)
{
    if (m_document == nullptr)
        return false;
    std::vector<InsertionJob> jobs;
    for (const QVariant& value : urls) {
        const QUrl url = value.canConvert<QUrl>() ? value.toUrl() : QUrl::fromUserInput(value.toString());
        if (!url.isValid() || url.isEmpty())
            continue;
        InsertionJob job;
        job.url = url;
        job.mode = Mode::Any;
        job.hasCenter = true;
        job.center = pagePoint;
        jobs.push_back(std::move(job));
    }
    if (!jobs.empty()) {
        start(std::move(jobs));
        return true;
    }
    if (text.trimmed().isEmpty())
        return false;
    // Texte brut seulement : jamais d'HTML interprété, donc rien à charger (M13).
    QTextDocument doc;
    doc.setPlainText(text);
    core::Text object;
    object.html = doc.toHtml().toStdString();
    const QSizeF size = doc.size();
    object.bounds = {pagePoint.x(), pagePoint.y(), std::max(size.width(), 1.0), std::max(size.height(), 1.0)};
    core::PageObject page = std::move(object);
    page.common.layer = core::Layer::Middle;
    m_document->execute(std::make_unique<core::ReplaceObjectsCommand>(
        static_cast<std::size_t>(m_document->currentIndex()), std::vector<core::ObjectId>{},
        std::vector<core::PageObject>{std::move(page)}));
    emit inserted(1);
    return true;
}

} // namespace actcraft::ui
