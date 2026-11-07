#pragma once
#include <actcraft/export/pageexport.h>
#include <actcraft/export/pdfimport.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/documentportal.h>
#include <actcraft/ui/documenttabs.h>

#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QString>
#include <QUrl>

namespace actcraft::ui {

// Export, import PDF et PowerPoint (F-020 à F-023) ; rempli par L5-T25 (export, import PDF, impression par
// PDF) et L5-T30 (PowerPoint). Le rendu et l'écriture se font sur un fil secondaire (module export) ;
// `finished` et `failed` sont émis sur le fil principal, la bannière de la fenêtre les montre.
class FileExchange : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(DocumentController* document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(DocumentTabs* tabs READ tabs WRITE setTabs NOTIFY tabsChanged)
    Q_PROPERTY(actcraft::ui::DocumentPortal* portal READ portal WRITE setPortal)
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(bool pdfImportAvailable READ pdfImportAvailable CONSTANT)
    Q_PROPERTY(bool powerpointAvailable READ powerpointAvailable NOTIFY converterChanged)
public:
    explicit FileExchange(QObject* parent = nullptr);
    DocumentController* document() const;
    void setDocument(DocumentController* document);
    DocumentTabs* tabs() const;
    void setTabs(DocumentTabs* tabs);
    DocumentPortal* portal() const { return m_portal; }
    void setPortal(DocumentPortal* portal) { m_portal = portal; } // noms d'URL content://
    bool isBusy() const;
    bool pdfImportAvailable() const;
    bool powerpointAvailable() const;

    Q_INVOKABLE void exportPdf(const QUrl& url);
    Q_INVOKABLE void exportPageImage(const QUrl& url);
    Q_INVOKABLE void importPdf(const QUrl& url);
    Q_INVOKABLE void importPowerpointImages(const QUrl& url);
    Q_INVOKABLE void printThroughPdf(); // hors Windows, N1-15
    // Valide l'édition en cours et dépose ce qui est soulevé (sans étape d'annulation) : ce qui sort ou
    // s'imprime est ce que l'utilisateur voit. Appelée par chaque export et par Fichier > Imprimer.
    Q_INVOKABLE void settleEditing();

signals:
    void documentChanged();
    void tabsChanged();
    void busyChanged();
    void converterChanged();
    void finished(QString message);
    void failed(QString message);

private:
    void setBusy(bool busy);
    // Un travail à la fois (`busy`) ; refus visible (`failed`) sinon. Vrai si le travail peut partir.
    bool begin();
    void fail(const QString& message);
    // Fin commune des exports : `okFormat` et `failFormat` portent un « %1 ».
    void finishExport(const exporting::ExportResult& result, const QUrl& target, const QString& okFormat,
                      const QString& failFormat);
    // Fin commune des imports : installe le document dans un onglet neuf ; les trois textes portent un « %1
    // ».
    void finishImport(exporting::ImportResult result, const QUrl& source, const QString& failFormat,
                      const QString& busyMessage, const QString& doneFormat);
    DocumentController* m_document = nullptr;
    DocumentTabs* m_tabs = nullptr;
    QPointer<DocumentPortal> m_portal;
    bool m_busy = false;
};

} // namespace actcraft::ui
