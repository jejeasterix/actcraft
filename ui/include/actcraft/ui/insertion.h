#pragma once
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/documentportal.h>
#include <actcraft/ui/mediainsert.h>
#include <actcraft/ui/webframeitem.h>
#include <actcraft/ui/webimagefetch.h>

#include <QImage>
#include <QObject>
#include <QPointF>
#include <QPointer>
#include <QQmlEngine>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariantList>

#include <memory>
#include <optional>
#include <vector>

class QTemporaryDir;

namespace actcraft::ui {

struct InsertionJob;    // insertion.cpp
struct InsertionOrigin; // insertion.cpp

// Insérer une image ou un média (F-560 à F-562) ; remplie par L5-T15.
class Insertion : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(DocumentController* document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(MediaInsert* mediaInsert READ mediaInsert WRITE setMediaInsert NOTIFY mediaInsertChanged)
    Q_PROPERTY(actcraft::ui::DocumentPortal* portal READ portal WRITE setPortal)
    Q_PROPERTY(
        actcraft::ui::WebFrameItems* webFrames READ webFrames WRITE setWebFrames NOTIFY webFramesChanged)
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(bool lastDecodeOffMainThread READ lastDecodeOffMainThread NOTIFY inserted)
public:
    explicit Insertion(QObject* parent = nullptr);
    ~Insertion() override;
    DocumentController* document() const;
    void setDocument(DocumentController* document);
    DocumentPortal* portal() const { return m_portal; }
    void setPortal(DocumentPortal* portal); // noms d'URL content://
    bool isBusy() const;
    bool lastDecodeOffMainThread() const;
    // Faux quand la dernière insertion arrive après une attente réseau : l'utilisateur a pu agir entre-temps,
    // l'appelant ne doit ni changer d'outil ni sélectionner.
    bool lastInsertionSelects() const { return m_lastSelects; }

    // Un son ou une vidéo (déposé, ou choisi dans un dialogue) passe par lui : action de lecture, espace
    // réservé et verrou de classe. Absent, un MediaInsert propre à l'insertion est créé à la demande.
    MediaInsert* mediaInsert() const;
    void setMediaInsert(MediaInsert* insert);
    Q_INVOKABLE void insertImage(const QUrl& url);
    Q_INVOKABLE void insertMedia(const QUrl& url);
    Q_INVOKABLE void insertImageData(const QImage& image); // collage, L5-T14
    Q_INVOKABLE bool acceptDrop(const QVariantList& urls, const QString& text, QPointF pagePoint);
    // Dépôt ou collage venu d'une page web (cadrage n° 21) : `http(s)` téléchargé (une vidéo, reconnue par
    // l'extension de l'adresse ou le type de la réponse, devient un objet vidéo par MediaInsert, OA27),
    // `data:image` décodé, `file:` par acceptDrop, `blob:` refusé. `html` n'est jamais interprété (M13) : il
    // est reçu pour que l'appelant n'ait pas à trier les formats. Vrai si quelque chose a été pris en charge.
    Q_INVOKABLE bool acceptWebDrop(const QVariantList& urls, const QString& text, const QString& html,
                                   QPointF pagePoint);
    WebFrameItems* webFrames() const { return m_webFrames; }
    void setWebFrames(WebFrameItems* frames);

signals:
    void documentChanged();
    void mediaInsertChanged();
    void webFramesChanged();
    void busyChanged();
    void inserted(int count);
    void failed(QString message);

private:
    std::shared_ptr<InsertionOrigin> trackOrigin();
    void releaseOrigin(InsertionOrigin& origin);
    void start(std::vector<InsertionJob> jobs, std::shared_ptr<InsertionOrigin> origin = {});
    void startDownload(const QUrl& url, const QString& fallback, QPointF center, WebImageFetch::Kind kind);
    void placeDownloadedVideo(const QByteArray& data, const QString& extension, const QUrl& url,
                              QPointF center, std::shared_ptr<InsertionOrigin> origin);
    void releaseVideoFiles();
    void abandonDownload(InsertionOrigin& origin, const QString& message);
    bool insertDataUrl(const QUrl& url, QPointF center);
    void placeText(const QString& text, QPointF point, std::size_t page);
    WebImageFetch::Decide webDecider() const;
    QPointer<WebFrameItems> m_webFrames;
    DocumentController* m_document = nullptr;
    int m_pending = 0;
    void routeMedia(const QUrl& url, std::optional<QPointF> center, bool afterWaiting = false);
    QPointer<MediaInsert> m_mediaInsert;
    QPointer<DocumentPortal> m_portal;
    int m_forwardedAfterWait = 0; // parmi eux, ceux d'un téléchargement : l'appelant ne sélectionne pas
    int m_forwarded = 0;          // médias confiés à MediaInsert dont le signal final n'est pas encore revenu
    std::unique_ptr<QTemporaryDir> m_videoDir; // vidéos téléchargées, le temps que MediaInsert les range
    QStringList m_videoFiles;
    int m_videoCount = 0;
    bool m_lastOff = false;
    bool m_lastSelects = true;
};

} // namespace actcraft::ui
