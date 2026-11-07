#pragma once
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/mediainsert.h>

#include <QImage>
#include <QObject>
#include <QPointF>
#include <QPointer>
#include <QQmlEngine>
#include <QString>
#include <QUrl>
#include <QVariantList>

#include <optional>
#include <vector>

namespace actcraft::ui {

struct InsertionJob; // insertion.cpp

// Insérer une image ou un média (F-560 à F-562) ; remplie par L5-T15.
class Insertion : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(DocumentController* document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(MediaInsert* mediaInsert READ mediaInsert WRITE setMediaInsert NOTIFY mediaInsertChanged)
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(bool lastDecodeOffMainThread READ lastDecodeOffMainThread NOTIFY inserted)
public:
    explicit Insertion(QObject* parent = nullptr);
    DocumentController* document() const;
    void setDocument(DocumentController* document);
    bool isBusy() const;
    bool lastDecodeOffMainThread() const;

    // Un son ou une vidéo (déposé, ou choisi dans un dialogue) passe par lui : action de lecture, espace
    // réservé et verrou de classe. Absent, un MediaInsert propre à l'insertion est créé à la demande.
    MediaInsert* mediaInsert() const;
    void setMediaInsert(MediaInsert* insert);
    Q_INVOKABLE void insertImage(const QUrl& url);
    Q_INVOKABLE void insertMedia(const QUrl& url);
    Q_INVOKABLE void insertImageData(const QImage& image); // collage, L5-T14
    Q_INVOKABLE bool acceptDrop(const QVariantList& urls, const QString& text, QPointF pagePoint);

signals:
    void documentChanged();
    void mediaInsertChanged();
    void busyChanged();
    void inserted(int count);
    void failed(QString message);

private:
    void start(std::vector<InsertionJob> jobs);
    DocumentController* m_document = nullptr;
    int m_pending = 0;
    void routeMedia(const QUrl& url, std::optional<QPointF> center);
    QPointer<MediaInsert> m_mediaInsert;
    int m_forwarded = 0; // médias confiés à MediaInsert dont le signal final n'est pas encore revenu
    bool m_lastOff = false;
};

} // namespace actcraft::ui
