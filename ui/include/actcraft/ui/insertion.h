#pragma once
#include <QImage>
#include <QObject>
#include <QPointF>
#include <QQmlEngine>
#include <QString>
#include <QUrl>
#include <QVariantList>

namespace actcraft::ui {
class DocumentController;

// Insérer une image ou un média (F-560 à F-562) ; remplie par L5-T15.
class Insertion : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(DocumentController* document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(bool lastDecodeOffMainThread READ lastDecodeOffMainThread NOTIFY inserted)
public:
    explicit Insertion(QObject* parent = nullptr);
    DocumentController* document() const;
    void setDocument(DocumentController* document);
    bool isBusy() const;
    bool lastDecodeOffMainThread() const;

    Q_INVOKABLE void insertImage(const QUrl& url);
    Q_INVOKABLE void insertMedia(const QUrl& url);
    Q_INVOKABLE void insertImageData(const QImage& image); // collage, L5-T14
    Q_INVOKABLE bool acceptDrop(const QVariantList& urls, const QString& text, QPointF pagePoint);

signals:
    void documentChanged();
    void busyChanged();
    void inserted(int count);
    void failed(QString message);

private:
    DocumentController* m_document = nullptr;
};

} // namespace actcraft::ui
