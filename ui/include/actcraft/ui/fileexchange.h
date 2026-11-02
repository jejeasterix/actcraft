#pragma once
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/documenttabs.h>

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QUrl>

namespace actcraft::ui {

// Export, import PDF et PowerPoint (F-020 à F-023) ; rempli par L5-T25 et L5-T30.
class FileExchange : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(DocumentController* document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(DocumentTabs* tabs READ tabs WRITE setTabs NOTIFY tabsChanged)
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(bool pdfImportAvailable READ pdfImportAvailable CONSTANT)
    Q_PROPERTY(bool powerpointAvailable READ powerpointAvailable NOTIFY converterChanged)
public:
    explicit FileExchange(QObject* parent = nullptr);
    DocumentController* document() const;
    void setDocument(DocumentController* document);
    DocumentTabs* tabs() const;
    void setTabs(DocumentTabs* tabs);
    bool isBusy() const;
    bool pdfImportAvailable() const;
    bool powerpointAvailable() const;

    Q_INVOKABLE void exportPdf(const QUrl& url);
    Q_INVOKABLE void exportPageImage(const QUrl& url);
    Q_INVOKABLE void importPdf(const QUrl& url);
    Q_INVOKABLE void importPowerpointImages(const QUrl& url);
    Q_INVOKABLE void printThroughPdf(); // hors Windows, N1-15

signals:
    void documentChanged();
    void tabsChanged();
    void busyChanged();
    void converterChanged();
    void finished(QString message);
    void failed(QString message);

private:
    DocumentController* m_document = nullptr;
    DocumentTabs* m_tabs = nullptr;
};

} // namespace actcraft::ui
