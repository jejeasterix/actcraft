#pragma once
#include <actcraft/ui/documentcontroller.h>

#include <QHash>
#include <QImage>
#include <QObject>
#include <QQmlEngine>
#include <QQmlParserStatus>
#include <QString>
#include <QStringList>

namespace actcraft::ui {

// Boîte d'impression (F-670 à F-679) ; remplie par L5-T28.
class PrintController : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_ELEMENT
    Q_PROPERTY(DocumentController* document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(int rows READ rows WRITE setRows NOTIFY settingsChanged)
    Q_PROPERTY(int columns READ columns WRITE setColumns NOTIFY settingsChanged)
    Q_PROPERTY(bool frames READ frames WRITE setFrames NOTIFY settingsChanged)
    Q_PROPERTY(bool pageNumbers READ pageNumbers WRITE setPageNumbers NOTIFY settingsChanged)
    Q_PROPERTY(QString range READ range WRITE setRange NOTIFY settingsChanged) // all, current, from-to
    Q_PROPERTY(int from READ from WRITE setFrom NOTIFY settingsChanged)
    Q_PROPERTY(int to READ to WRITE setTo NOTIFY settingsChanged)
    Q_PROPERTY(int copies READ copies WRITE setCopies NOTIFY settingsChanged)
    Q_PROPERTY(double marginMm READ marginMm WRITE setMarginMm NOTIFY settingsChanged)
    Q_PROPERTY(bool gray READ gray WRITE setGray NOTIFY settingsChanged)
    Q_PROPERTY(QString notes READ notes WRITE setNotes NOTIFY settingsChanged) // none, with-pages, only
    Q_PROPERTY(bool noteBreak READ noteBreak WRITE setNoteBreak NOTIFY settingsChanged)
    Q_PROPERTY(bool noteSeparator READ noteSeparator WRITE setNoteSeparator NOTIFY settingsChanged)
    Q_PROPERTY(QString printer READ printer WRITE setPrinter NOTIFY settingsChanged)
    Q_PROPERTY(QStringList printers READ printers CONSTANT)
    Q_PROPERTY(int sheetCount READ sheetCount NOTIFY settingsChanged)
    // Ajoutés par L5-T28. `settingsFile` : fichier de réglages (tests, banc), comme ToolHost ; vide : les
    // réglages de l'application, seulement si elle a un nom d'organisation. `available` : l'impression par
    // QPrinter est construite (ACTCRAFT_PRINT). `previewRenders` : feuilles composées pour l'aperçu depuis
    // le début (mesure de la paresse de l'aperçu).
    Q_PROPERTY(QString settingsFile READ settingsFile WRITE setSettingsFile NOTIFY settingsFileChanged)
    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(int previewRenders READ previewRenders NOTIFY previewRendersChanged)
public:
    explicit PrintController(QObject* parent = nullptr);
    void classBegin() override {}
    void componentComplete() override; // fournisseur d'images « printpreview » du moteur
    DocumentController* document() const;
    void setDocument(DocumentController* document);
    int rows() const;
    void setRows(int rows);
    int columns() const;
    void setColumns(int columns);
    bool frames() const;
    void setFrames(bool frames);
    bool pageNumbers() const;
    void setPageNumbers(bool pageNumbers);
    QString range() const;
    void setRange(const QString& range);
    int from() const;
    void setFrom(int from);
    int to() const;
    void setTo(int to);
    int copies() const;
    void setCopies(int copies);
    double marginMm() const;
    void setMarginMm(double marginMm);
    bool gray() const;
    void setGray(bool gray);
    QString notes() const;
    void setNotes(const QString& notes);
    bool noteBreak() const;
    void setNoteBreak(bool noteBreak);
    bool noteSeparator() const;
    void setNoteSeparator(bool noteSeparator);
    QString printer() const;
    void setPrinter(const QString& printer);
    QStringList printers() const;
    int sheetCount() const;
    QString settingsFile() const;
    void setSettingsFile(const QString& path);
    bool available() const;
    int previewRenders() const;

    Q_INVOKABLE QImage previewSheet(int index, int dpi);
    // Adresse (image://printpreview/…) de la feuille `index` ; change quand le document ou un réglage change.
    Q_INVOKABLE QString previewSource(int index, int dpi) const;
    Q_INVOKABLE bool print(const QString& outputFile = {});
    Q_INVOKABLE void saveSettings();
    Q_INVOKABLE void loadSettings();

signals:
    void documentChanged();
    void settingsChanged();
    void settingsFileChanged();
    void previewRendersChanged();
    void finished(QString message);
    void failed(QString message);

private:
    void invalidate();
    void changed();
    bool report(bool ok, const QString& message);
    DocumentController* m_document = nullptr;
    int m_rows = 1;
    int m_columns = 1;
    bool m_frames = false;
    bool m_pageNumbers = false;
    QString m_range = QStringLiteral("all");
    int m_from = 1;
    int m_to = 1;
    int m_copies = 1;
    double m_marginMm = 5.0;
    bool m_gray = false;
    QString m_notes = QStringLiteral("none");
    bool m_noteBreak = true;
    bool m_noteSeparator = false;
    QString m_printer;
    QString m_settingsFile;
    int m_previewRenders = 0;
    int m_revision = 0;
    QHash<qint64, QImage> m_cache;
};

} // namespace actcraft::ui
