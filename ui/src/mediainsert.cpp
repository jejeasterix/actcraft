#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/core/replace.h>
#include <actcraft/core/undo.h>
#include <actcraft/formats/urlio.h>
#include <actcraft/ui/mediainsert.h>

#include <QBuffer>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QPolygonF>
#include <QStringList>
#include <QThread>
#include <QtConcurrent>

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace actcraft::ui {

bool isSoundExtension(const QString& suffix)
{
    static const QStringList sounds{"mp3", "wav", "ogg", "oga", "m4a", "aac", "flac", "opus"};
    return sounds.contains(suffix.toLower());
}

bool isVideoExtension(const QString& suffix)
{
    static const QStringList videos{"mp4", "m4v", "mov", "avi", "webm", "mkv", "mpg", "mpeg", "wmv"};
    return videos.contains(suffix.toLower());
}

namespace {

enum class Kind
{
    Sound,
    Video,
    Flash, // objet d'une ancienne technologie : jamais lu, seulement son espace réservé (N1-A15, D15)
    Unknown,
};

constexpr double kMaxShare = 0.8;  // au plus 80 % de la page
constexpr double kGap = 20;        // espace entre deux médias voisins
constexpr double kSoundSide = 120; // icône de son
constexpr double kVideoMaxW = 480; // cadre d'une vidéo
constexpr double kVideoMaxH = 360;
constexpr double kFlashW = 320;
constexpr double kFlashH = 240;

Kind kindOf(const QString& suffix)
{
    static const QStringList flash{"swf", "flv"};
    if (isSoundExtension(suffix))
        return Kind::Sound;
    if (isVideoExtension(suffix))
        return Kind::Video;
    if (flash.contains(suffix.toLower()))
        return Kind::Flash;
    return Kind::Unknown;
}

// Image d'espace réservé, dessinée par nous (formes simples, aucune ressource extérieure).
QImage card(int width, int height, Kind kind)
{
    QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(0x2B, 0x2F, 0x36));
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::white);
    const double cx = width / 2.0;
    const double cy = height / 2.0;
    const double s = std::min(width, height) / 4.0;
    if (kind == Kind::Sound) {
        painter.drawPolygon(QPolygonF({{cx - s, cy - s / 2},
                                       {cx - s / 3, cy - s / 2},
                                       {cx + s / 2, cy - s},
                                       {cx + s / 2, cy + s},
                                       {cx - s / 3, cy + s / 2},
                                       {cx - s, cy + s / 2}}));
        painter.setPen(QPen(Qt::white, std::max(2.0, s / 6), Qt::SolidLine, Qt::RoundCap));
        painter.setBrush(Qt::NoBrush);
        painter.drawArc(QRectF(cx, cy - s, s, 2 * s), -50 * 16, 100 * 16);
    } else if (kind == Kind::Flash) {
        painter.setPen(QPen(Qt::white, std::max(2.0, s / 6)));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(QRectF(cx - 2 * s, cy - 1.5 * s, 4 * s, 3 * s));
        painter.drawLine(QPointF(cx - 2 * s, cy - 1.5 * s), QPointF(cx + 2 * s, cy + 1.5 * s));
        painter.drawLine(QPointF(cx - 2 * s, cy + 1.5 * s), QPointF(cx + 2 * s, cy - 1.5 * s));
    } else {
        painter.drawPolygon(QPolygonF({{cx - s / 2, cy - s}, {cx + s, cy}, {cx - s / 2, cy + s}}));
    }
    return image;
}

QByteArray png(const QImage& image)
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return bytes;
}

struct Box
{
    double width = 0;
    double height = 0;
};

// Boîte et image d'espace réservé d'un média ; `frame` : première image de la vidéo (nulle : 16:9 par
// défaut).
Box placeholderFor(Kind kind, const QImage& frame, QByteArray& placeholder)
{
    if (kind == Kind::Sound) {
        placeholder = png(card(static_cast<int>(kSoundSide), static_cast<int>(kSoundSide), kind));
        return {kSoundSide, kSoundSide};
    }
    if (kind == Kind::Video) {
        double ratio = 16.0 / 9.0;
        if (!frame.isNull() && frame.height() > 0)
            ratio = static_cast<double>(frame.width()) / frame.height();
        Box box{kVideoMaxW, kVideoMaxW / ratio};
        if (box.height > kVideoMaxH)
            box = {kVideoMaxH * ratio, kVideoMaxH};
        const QSize size(std::max(1, static_cast<int>(box.width)), std::max(1, static_cast<int>(box.height)));
        QImage image = frame.isNull() ? card(size.width(), size.height(), kind)
                                      : frame.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                                            .convertToFormat(QImage::Format_ARGB32_Premultiplied);
        if (!frame.isNull()) {
            QPainter painter(&image);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(255, 255, 255, 200));
            const double s = std::min(size.width(), size.height()) / 8.0;
            painter.drawPolygon(QPolygonF({{size.width() / 2.0 - s / 2, size.height() / 2.0 - s},
                                           {size.width() / 2.0 + s, size.height() / 2.0},
                                           {size.width() / 2.0 - s / 2, size.height() / 2.0 + s}}));
        }
        placeholder = png(image);
        return box;
    }
    placeholder = png(card(static_cast<int>(kFlashW), static_cast<int>(kFlashH), Kind::Flash));
    return {kFlashW, kFlashH};
}

struct Prepared
{
    Kind kind = Kind::Unknown;
    QByteArray data;
    std::string extension;
    std::string name;
    std::string fileName;
    QByteArray placeholder;
    Box box;
    QString error;
};

// Fil du pool : lecture, première image, espace réservé. Aucune requête réseau (file et content seulement).
Prepared prepare(const QUrl& url, const MediaInsert::FrameProbe& probe)
{
    Prepared out;
    if (url.scheme() != QLatin1String("file") && url.scheme() != QLatin1String("content")) {
        out.error = QStringLiteral("Adresse non prise en charge");
        return out;
    }
    const QFileInfo info(url.isLocalFile() ? url.toLocalFile() : url.path());
    out.kind = kindOf(info.suffix());
    if (out.kind == Kind::Unknown) {
        out.error = QStringLiteral("Format de média non pris en charge : %1").arg(info.fileName());
        return out;
    }
    const formats::UrlReadResult read = formats::readUrl(url);
    if (!read.ok) {
        out.error = read.error.isEmpty() ? QStringLiteral("Lecture impossible") : read.error;
        return out;
    }
    out.data = read.data;
    out.name = info.completeBaseName().toStdString();
    out.fileName = info.fileName().toStdString();
    out.extension = info.suffix().toLower().toStdString();
    QImage frame;
    if (out.kind == Kind::Video && probe && url.isLocalFile())
        frame = probe(url.toLocalFile());
    out.box = placeholderFor(out.kind, frame, out.placeholder);
    return out;
}

// Une étape : ressources puis ReplaceObjectsCommand({}, objet).
class InsertMediaCommand final : public core::UndoCommand
{
public:
    InsertMediaCommand(std::size_t page, core::PageObject object, QByteArray data, std::string extension,
                       std::string existingKey, QByteArray placeholder, bool playable)
        : m_page(page), m_object(std::move(object)), m_data(std::move(data)),
          m_extension(std::move(extension)), m_existingKey(std::move(existingKey)),
          m_placeholder(std::move(placeholder)), m_playable(playable)
    {}
    void redo(core::Document& document, core::Changes& changes) override
    {
        const std::string key =
            m_existingKey.empty() ? document.addResource(m_data, m_extension) : m_existingKey;
        const std::string placeholderKey = document.addResource(m_placeholder, "png");
        if (!m_inner) {
            if (auto* media = std::get_if<core::Media>(&core::body(m_object))) {
                media->media = key;
                media->placeholder = placeholderKey;
                if (m_playable) {
                    core::Action action;
                    action.catalogId = "document.play-sound";
                    action.resource = key;
                    QJsonObject parameters;
                    parameters.insert(QStringLiteral("file"), QString::fromStdString(media->fileName));
                    action.parameters =
                        QJsonDocument(parameters).toJson(QJsonDocument::Compact).toStdString();
                    m_object.common.actions.push_back(std::move(action));
                }
            }
            m_inner = std::make_unique<core::ReplaceObjectsCommand>(m_page, std::vector<core::ObjectId>{},
                                                                    std::vector<core::PageObject>{m_object});
        }
        m_inner->redo(document, changes);
    }
    void undo(core::Document& document, core::Changes& changes) override
    {
        if (m_inner)
            m_inner->undo(document, changes);
    }

private:
    std::size_t m_page;
    core::PageObject m_object;
    QByteArray m_data;
    std::string m_extension;
    std::string m_existingKey;
    QByteArray m_placeholder;
    bool m_playable;
    std::unique_ptr<core::ReplaceObjectsCommand> m_inner;
};

// À droite du dernier média de la page (V126), sinon au centre ; sous lui si la page n'a plus de place à
// droite.
core::RectF placeBox(const core::Page& page, Box box, std::optional<QPointF> center)
{
    const double share =
        std::min({1.0, kMaxShare * page.width / box.width, kMaxShare * page.height / box.height});
    const double width = box.width * share;
    const double height = box.height * share;
    if (center) // déposé à un point de la page : centré dessus, comme une image
        return {center->x() - width / 2, center->y() - height / 2, width, height};
    const core::Media* last = nullptr;
    for (const core::PageObject& object : page.objects)
        if (const auto* media = std::get_if<core::Media>(&core::body(object)))
            last = media;
    if (last == nullptr)
        return {(page.width - width) / 2, (page.height - height) / 2, width, height};
    double x = last->bounds.x + last->bounds.width + kGap;
    double y = last->bounds.y;
    if (x + width > page.width) {
        x = last->bounds.x;
        y = last->bounds.y + last->bounds.height + kGap;
    }
    return {x, y, width, height};
}

void place(DocumentController& controller, std::size_t pageIndex, Kind kind, Box box, const std::string& name,
           const std::string& fileName, QByteArray data, std::string extension, std::string existingKey,
           QByteArray placeholder, std::optional<QPointF> center = std::nullopt)
{
    core::Media media;
    media.bounds = placeBox(controller.document().page(pageIndex), box, center);
    media.fileName = fileName;
    media.aspectLocked = kind == Kind::Video;
    core::PageObject object = std::move(media);
    object.common.layer = core::Layer::Middle;
    object.common.name = name;
    controller.execute(std::make_unique<InsertMediaCommand>(pageIndex, std::move(object), std::move(data),
                                                            std::move(extension), std::move(existingKey),
                                                            std::move(placeholder), kind != Kind::Flash));
}

} // namespace

MediaInsert::MediaInsert(QObject* parent) : QObject(parent) {}

Commands* MediaInsert::commands() const
{
    return m_commands;
}

void MediaInsert::setCommands(Commands* commands)
{
    if (m_commands == commands)
        return;
    m_commands = commands;
    emit commandsChanged();
}

DocumentController* MediaInsert::document() const
{
    return m_document;
}

void MediaInsert::setDocument(DocumentController* document)
{
    if (m_document == document)
        return;
    m_document = document;
    emit documentChanged();
}

bool MediaInsert::isBusy() const
{
    return m_pending > 0;
}

void MediaInsert::setFrameProbe(FrameProbe probe)
{
    m_probe = std::move(probe);
}

// Sous le verrou de classe, « insert.media » est refusée comme au menu.
bool MediaInsert::allowed()
{
    if (!Commands::allowsWhileLocked(m_commands.data(), "insert.media")) {
        emit failed(QStringLiteral("Insertion refusée : la classe est verrouillée"));
        return false;
    }
    if (m_document == nullptr) {
        emit failed(QStringLiteral("Aucun document ouvert"));
        return false;
    }
    return true;
}

void MediaInsert::insertFile(const QUrl& requested)
{
    startFile(requested, std::nullopt);
}

void MediaInsert::insertFileAt(const QUrl& requested, QPointF center)
{
    startFile(requested, center);
}

void MediaInsert::startFile(const QUrl& requested, std::optional<QPointF> center)
{
    if (!allowed())
        return;
    QUrl url = requested;
    // Un chemin nu (argument de commande) est un fichier local.
    if (url.scheme().isEmpty() || (url.scheme().size() == 1 && !url.isLocalFile()))
        url = QUrl::fromLocalFile(requested.toString());
    const std::uint64_t generation = m_document->generation();
    const std::size_t origin = static_cast<std::size_t>(m_document->currentIndex());
    if (m_pending++ == 0)
        emit busyChanged();
    auto* watcher = new QFutureWatcher<Prepared>(this);
    connect(watcher, &QFutureWatcher<Prepared>::finished, this,
            [this, watcher, generation, origin, center]() {
                Prepared prepared = watcher->result();
                watcher->deleteLater();
                bool done = false;
                QString error = prepared.error;
                if (error.isEmpty()) {
                    if (m_document != nullptr && m_document->generation() == generation &&
                        static_cast<std::size_t>(m_document->currentIndex()) == origin &&
                        origin < m_document->document().pageCount()) {
                        place(*m_document, origin, prepared.kind, prepared.box, prepared.name,
                              prepared.fileName, std::move(prepared.data), std::move(prepared.extension), {},
                              std::move(prepared.placeholder), center);
                        done = true;
                    } else {
                        error = QStringLiteral("Insertion abandonnée : le document affiché a changé.");
                    }
                }
                if (--m_pending == 0)
                    emit busyChanged();
                if (!error.isEmpty())
                    emit failed(error);
                if (done)
                    emit inserted(1);
            });
    watcher->setFuture(QtConcurrent::run([url, probe = m_probe]() { return prepare(url, probe); }));
}

void MediaInsert::insertResource(const QString& key)
{
    if (!allowed())
        return;
    const core::Resource* stored = m_document->document().resource(key.toStdString());
    if (stored == nullptr) {
        emit failed(QStringLiteral("Ressource introuvable"));
        return;
    }
    const Kind kind = kindOf(QString::fromStdString(stored->extension));
    if (kind == Kind::Unknown) {
        emit failed(QStringLiteral("Cette ressource n'est pas un média"));
        return;
    }
    QByteArray placeholder;
    const Box box = placeholderFor(kind, {}, placeholder);
    const std::string fileName = "media." + stored->extension;
    place(*m_document, static_cast<std::size_t>(m_document->currentIndex()), kind, box, std::string{},
          fileName, {}, stored->extension, key.toStdString(), std::move(placeholder));
    emit inserted(1);
}

} // namespace actcraft::ui
