#pragma once

#include <actcraft/core/document.h>
#include <actcraft/core/undo.h>
#include <actcraft/render/fixedpageitem.h>

#include <QObject>
#include <QPointer>
#include <QSizeF>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

#include <cstddef>
#include <cstdint>
#include <memory>

namespace actcraft::ui {

struct IoRelay; // documentcontroller.cpp

// Document ouvert (DA2, décision 16) : le contrôleur possède le document et sa pile d'annulation ; la page
// fixe LIT la page affichée (setSource), elle n'en garde aucune copie ; toute modification passe par
// execute() (la pile), et ses changements vont au cache de la page affichée par core::applyToCache (DA11).
// Relais du lot 1 défait (revue L1-T9) : plus de page recopiée de l'élément vers le document.
class DocumentController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(
        actcraft::render::FixedPageItem* fixedPage READ fixedPage WRITE setFixedPage NOTIFY fixedPageChanged)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY pagesChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY pagesChanged)
    Q_PROPERTY(QString pageLabel READ pageLabel NOTIFY pagesChanged)
    Q_PROPERTY(QSizeF pageSize READ pageSize NOTIFY pagesChanged)
    Q_PROPERTY(bool modified READ isModified NOTIFY modifiedChanged)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QUrl fileUrl READ fileUrl NOTIFY fileUrlChanged)
    Q_PROPERTY(QString fingerprint READ fingerprintText NOTIFY documentChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY undoChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY undoChanged)
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(QStringList warnings READ warnings NOTIFY warningsChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QString suggestedFileName READ suggestedFileName NOTIFY titleChanged)
    // Vrai si la dernière lecture ou écriture a tourné hors du fil principal (S5 la lit ; mutation : écriture
    // synchrone → faux).
    Q_PROPERTY(bool lastIoOffMainThread READ lastIoOffMainThread NOTIFY lastIoChanged)

public:
    explicit DocumentController(QObject* parent = nullptr);
    ~DocumentController() override; // détache la page fixe : elle ne lit jamais un document détruit

    render::FixedPageItem* fixedPage() const noexcept;
    // L'élément montre aussitôt la page affichée ; l'ancien est détaché (page interne vide).
    void setFixedPage(render::FixedPageItem* page);
    int pageCount() const noexcept;
    int currentIndex() const noexcept;
    QString pageLabel() const; // « n / N » (F-002)
    QSizeF pageSize() const;   // page affichée
    bool isModified() const noexcept;
    QString title() const;
    QUrl fileUrl() const;
    QString fingerprintText() const; // core::fingerprint en hexadécimal (16 chiffres)
    bool canUndo() const noexcept;
    bool canRedo() const noexcept;

    const core::Document& document() const noexcept;
    // Seule voie d'écriture du document (décision 16). Une commande dont redo() ne change rien n'entre pas
    // dans la pile (UndoStack::push l'y mettrait et effacerait la branche à rétablir).
    void execute(std::unique_ptr<core::UndoCommand> command);
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    // Le stylo verse son trait (ControllerSink) : couche supérieure de la page affichée (F-510), par la pile.
    void commitStroke(core::Stroke stroke);
    // Remplace le document (ouverture, nouveau) : pile vidée, état propre, page 1 affichée. Titre vide :
    // « Sans titre ».
    void replaceDocument(core::Document document, const QUrl& fileUrl, const QString& title);
    // Paperboard vierge « Sans titre » d'une page `width` × `height` (F-656, F-700).
    Q_INVOKABLE void newDocument(double width, double height);

    // F-125 : sur la dernière page, ajoute une page de la taille de la dernière (commande annulable) et
    // l'affiche.
    Q_INVOKABLE void nextPage();
    Q_INVOKABLE void pageBack();
    // Banc et tests : la page 1 devient la page générée de 300 objets (core::generatePage), renumérotée
    // depuis le compteur du document (1 à 300 dans un document neuf) : aucun doublon avec les autres pages.
    // Pile vidée ; « modifié » inchangé ; ni le nombre de pages ni la page affichée ne changent (pagesChanged
    // seulement si la taille de la page affichée change).
    Q_INVOKABLE void loadGeneratedFirstPage();

    bool isBusy() const noexcept;
    QStringList warnings() const;
    QString lastError() const;
    QString
    suggestedFileName() const; // « <titre>.actcraft » (décision 18 : un .flipchart n'est jamais réécrit)
    bool lastIoOffMainThread() const noexcept;
    // Ouverture et enregistrement sur un fil secondaire (décision 17) ; loadFinished / saveFinished à la fin,
    // toujours émis en file, même pour un refus. Refusés pendant une autre opération (`busy`).
    // Ouvrir : un .actcraft garde son fichier ; un .flipchart devient un document sans fichier (décision 18).
    // Enregistrer : le fil écrit une copie prise à l'appel, sur le fil principal ; à la fin, état propre
    // (pile et drapeau hors pile) si le document n'a pas changé entre-temps (empreinte comparée, même
    // document).
    Q_INVOKABLE void openUrl(const QUrl& url);
    Q_INVOKABLE void saveToUrl(const QUrl& url);
    // Argument d'une commande de fichier → URL (formats::urlFromArgument) ; vide si absent.
    Q_INVOKABLE QUrl urlFromArgument(const QVariant& argument) const;
    Q_INVOKABLE QUrl defaultFolderUrl() const;
    // Tests : dossier qui remplace Documents pour defaultFolderUrl (un test n'écrit jamais dans le profil).
    void setDocumentsRoot(const QString& documents);

signals:
    void fixedPageChanged();
    // Juste avant un changement de page affichée demandé (page suivante ou précédente, ajout compris,
    // remplacement du document) : la vue y termine le trait en cours, qui rejoint la page qu'il quitte.
    void pageAboutToChange();
    // Une fois par changement de page affichée ou du nombre de pages, l'élément montrant déjà la nouvelle
    // page.
    void pagesChanged();
    void modifiedChanged();
    void titleChanged();
    void fileUrlChanged();
    void documentChanged();
    void undoChanged();
    void busyChanged();
    void warningsChanged();
    void lastErrorChanged();
    void lastIoChanged();
    void loadFinished(bool ok, const QString& message);
    void saveFinished(bool ok, const QString& message);

private:
    void setBusy(bool busy);
    void setWarnings(const QStringList& warnings);
    void setLastError(const QString& error);
    void setLastIo(bool offMainThread);
    // Exécute la commande ; elle n'entre dans la pile que si elle a changé quelque chose (changements
    // rendus).
    core::Changes run(std::unique_ptr<core::UndoCommand> command);
    // Changements d'une commande : cache de la page affichée (applyToCache), page affichée recalée sur
    // l'indice qu'il rend (relue par la page fixe si elle a bougé ou disparu). Vrai si le nombre de pages ou
    // la page affichée a changé.
    bool settle(const core::Changes& changes);
    // Signaux d'état après une modification : pagesChanged (si `pages`), document, pile, modifié.
    void notifyState(bool pages);
    void attach(); // la page fixe lit la page affichée
    void show(std::size_t index);

    core::Document m_document;
    core::UndoStack m_undo;
    std::size_t m_current = 0;
    QPointer<render::FixedPageItem> m_fixed;
    QUrl m_fileUrl;
    QString m_title;
    bool m_wasModified = false;
    // Modification faite hors de la pile (page du banc sur un document modifié) : remise à faux par
    // replaceDocument et par un enregistrement réussi, avec setClean().
    bool m_modifiedOutsideStack = false;
    bool m_busy = false;
    QStringList m_warnings;
    QString m_lastError;
    bool m_lastIoOffMainThread = false;
    QString m_documentsRoot; // vide : Documents du système
    // Change à chaque replaceDocument : un enregistrement fini après « Nouveau » ne rattache pas le fichier
    // au document neuf, même d'empreinte égale.
    std::uint64_t m_generation = 0;
    // Le fil secondaire rend son résultat par ce relais, tant que le contrôleur vit (documentcontroller.cpp).
    std::shared_ptr<IoRelay> m_relay;
};

} // namespace actcraft::ui
