#pragma once

#include <actcraft/core/document.h>
#include <actcraft/core/lift.h>
#include <actcraft/core/model.h>
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
#include <functional>
#include <memory>
#include <vector>

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
    // Vrai si la dernière lecture ou écriture a tourné hors du fil principal (S5 la lit). Faux seulement si
    // elle a tourné SUR le fil principal : un fil principal qui attend une écriture faite sur un fil
    // secondaire le laisse vrai. Cette attente-là se prouve sans ce drapeau (tst_ui_files,
    // saveNeverWaitsForTheWriterOnTheMainThread).
    Q_PROPERTY(bool lastIoOffMainThread READ lastIoOffMainThread NOTIFY lastIoChanged)
    Q_PROPERTY(int shownObjectCount READ shownObjectCount NOTIFY documentChanged)
    Q_PROPERTY(bool lifting READ isLifting NOTIFY liftingChanged)
    Q_PROPERTY(bool canRevertPage READ canRevertPage NOTIFY canRevertPageChanged)

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
    // Clôt l'étape en cours de fusion (core::UndoStack::closeMerge) : la réglette de translucidité l'appelle
    // à sa fermeture, une étape par ouverture (D10, P23). Ne modifie ni le document ni l'état de la pile vu
    // de l'interface (canUndo, canRedo, modifié).
    Q_INVOKABLE void closeMerge();
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
    // Banc (scénario « page », L2-T23) : document neuf de `count` pages générées (au moins une), graines
    // core::kDefaultSeed + i ; `painted` : chaque page paire (2, 4…) reçoit en plus 20 textes et 20 formes à
    // chemins (moteur peint). Identifiants tirés du compteur du document, comme loadGeneratedFirstPage :
    // aucun doublon d'une page à l'autre. Comme une ouverture : pile vidée, document propre, page 1 affichée.
    Q_INVOKABLE void loadGeneratedPages(int count, bool painted = false);

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
    // (pile et drapeau hors pile) si le document n'a pas changé entre-temps (révision comparée, même
    // document).
    Q_INVOKABLE void openUrl(const QUrl& url);
    Q_INVOKABLE void saveToUrl(const QUrl& url);
    // Argument d'une commande de fichier → URL (formats::urlFromArgument) ; vide si absent.
    Q_INVOKABLE QUrl urlFromArgument(const QVariant& argument) const;
    // `create` : le dossier est créé s'il manque — le dialogue d'ouverture ou d'enregistrement le demande en
    // s'ouvrant (L2-T18) ; sans lui, aucun accès disque.
    Q_INVOKABLE QUrl defaultFolderUrl(bool create = false) const;
    // Tests : dossier qui remplace Documents pour defaultFolderUrl (un test n'écrit jamais dans le profil).
    void setDocumentsRoot(const QString& documents);

    int shownObjectCount() const noexcept;
    // Soulever (DA11, lot 3) : retire les objets de la page affichée hors pile et rend leurs copies ; le
    // cache reçoit core::liftChanges. N'émet PAS editAboutToApply (B1 : l'outil qui soulève ne doit pas
    // s'annuler lui-même). Pendant un soulèvement, Enregistrer, Ouvrir et les changements de page sont
    // refusés ou attendent le dépôt.
    std::vector<core::PageObject> liftObjects(const std::vector<core::ObjectId>& ids);
    // Remet les objets soulevés, exécute `command` par la pile (nullptr : aucune étape), et le cache reçoit
    // core::changesAfterLift.
    void dropLifted(std::unique_ptr<core::UndoCommand> command);
    bool isLifting() const noexcept;
    // Trait d'un outil d'encre avec sa translucidité (marqueur, F-225) : couche supérieure, par la pile.
    void commitAnnotation(core::Stroke stroke, std::uint8_t translucency);
    void deleteObjects(const std::vector<core::ObjectId>& ids);
    Q_INVOKABLE void clearPage(int kind); // core::ClearKind (F-130)
    Q_INVOKABLE void revertPage();        // F-131
    bool canRevertPage() const;
    // Tests (L3-C8) : comparaisons d'empreintes de page entière faites par canRevertPage depuis la création.
    int revertComparisonCount() const noexcept { return m_revertComparisons; }

    // Origine d'une modification versée (observateur unique, socle des lots 3 et 7).
    enum class ChangeOrigin : std::uint8_t
    {
        Command, // execute()
        Undo,
        Redo,
        Stroke, // commitStroke / commitAnnotation : trait versé par un outil d'encre
        Lift,   // liftObjects (lot 3)
        Drop,   // dropLifted (lot 3)
    };
    // Reçoit, après chaque modification de la page affichée, exactement les changements passés au cache
    // (core::applyToCache) et leur origine. Appelé sur le fil principal, après la page fixe.
    // Appelé juste après core::applyToCache, avant que la page affichée suive les pages insérées ou retirées
    // : pendant l'appel, currentIndex() est l'indice que la page fixe a reçu, donc
    // core::applyToCache(applied, currentIndex(), espion) rejoue exactement ses appels. Un observateur peut
    // en retirer un (lui-même compris) pendant l'appel : le retiré n'est plus appelé.
    using ChangeObserver = std::function<void(const core::Changes& applied, ChangeOrigin origin)>;
    int addChangeObserver(ChangeObserver observer); // jeton > 0
    void removeChangeObserver(int token);

signals:
    void fixedPageChanged();
    // Juste avant un changement de page affichée demandé (page suivante ou précédente, ajout compris,
    // remplacement du document) : la vue y termine le trait en cours, qui rejoint la page qu'il quitte. Aussi
    // à la fin d'une lecture réussie, avant le contrôle « changé pendant la lecture » (L2-C9).
    void pageAboutToChange();
    // Une fois par changement de page affichée ou du nombre de pages, l'élément montrant déjà la nouvelle
    // page.
    void pagesChanged();
    void modifiedChanged();
    void titleChanged();
    void fileUrlChanged();
    void documentChanged();
    // Avec documentChanged, sauf au soulèvement et au dépôt sans étape (réponse inchangée, L3-C8), et à la
    // fin d'un enregistrement (référence changée).
    void canRevertPageChanged();
    void undoChanged();
    void busyChanged();
    void warningsChanged();
    void lastErrorChanged();
    void lastIoChanged();
    void loadFinished(bool ok, const QString& message);
    void saveFinished(bool ok, const QString& message);
    // Avant Annuler, Rétablir, Effacer, Rétablir la page, Supprimer : la vue termine le trait en cours et les
    // outils leur geste (L2-T15).
    void editAboutToApply();
    void liftingChanged();

private:
    void setBusy(bool busy);
    // Refus visible d'une opération demandée pendant une autre : dernière erreur et signal de fin, en file.
    void refuseWhileBusy(const QString& message, void (DocumentController::*finished)(bool, const QString&));
    void setWarnings(const QStringList& warnings);
    void setLastError(const QString& error);
    void setLastIo(bool offMainThread);
    // Exécute la commande ; elle n'entre dans la pile que si elle a changé quelque chose (changements
    // rendus).
    core::Changes run(std::unique_ptr<core::UndoCommand> command);
    // execute() avec l'origine annoncée aux observateurs (commitStroke : Stroke).
    void executeAs(std::unique_ptr<core::UndoCommand> command, ChangeOrigin origin);
    // Changements d'une commande : cache de la page affichée (applyToCache), puis observateurs (même liste,
    // `origin`), puis page affichée recalée sur l'indice qu'il rend (relue par la page fixe si elle a bougé
    // ou disparu). Seul appel du cache. Vrai si le nombre de pages ou la page affichée a changé.
    bool settle(const core::Changes& changes, ChangeOrigin origin);
    void notifyObservers(const core::Changes& changes, ChangeOrigin origin);
    // Signaux d'état après une modification : pagesChanged (si `pages`), document, pile, modifié.
    void notifyState(bool pages);
    void updateModified(); // modifiedChanged si l'état « modifié » a changé
    void attach();         // la page fixe lit la page affichée
    void show(std::size_t index);

    core::Document m_document;
    core::UndoStack m_undo;
    std::size_t m_current = 0;
    QPointer<render::FixedPageItem> m_fixed;
    QUrl m_fileUrl;
    QString m_title;
    bool m_wasModified = false;
    // Modification faite hors de la pile : page du banc sur un document modifié, ou fichier écrit avec un
    // contenu que l'écran n'a plus (changé pendant l'écriture). Remise à faux par replaceDocument et par un
    // enregistrement réussi du contenu affiché, avec setClean().
    bool m_modifiedOutsideStack = false;
    bool m_busy = false;
    QStringList m_warnings;
    QString m_lastError;
    bool m_lastIoOffMainThread = false;
    QString m_documentsRoot; // vide : Documents du système
    // Change à chaque replaceDocument : un enregistrement fini après « Nouveau » ne rattache pas le fichier
    // au document neuf, même d'empreinte égale.
    std::uint64_t m_generation = 0;
    // Change à chaque modification du document (commande de la pile, annuler, rétablir, page du banc,
    // replaceDocument) : « a-t-il changé pendant la lecture ou l'écriture ? » sans parcourir le document sur
    // le fil principal (L2-C9). Un changement puis son annulation la font bouger deux fois : jamais «
    // inchangé ».
    std::uint64_t m_revision = 0;
    // Le fil secondaire rend son résultat par ce relais, tant que le contrôleur vit (documentcontroller.cpp).
    std::shared_ptr<IoRelay> m_relay;
    // Observateurs des modifications versées (D-S7), dans l'ordre d'ajout.
    struct ObserverEntry
    {
        int token = 0;
        ChangeObserver call;
    };
    std::vector<ObserverEntry> m_observers;
    int m_lastObserverToken = 0;

    // Lot 3 (L3-T12) : soulèvement et page de référence de Rétablir la page.
    // Dépôt forcé (M2) : ce qui reste soulevé après l'annonce d'une édition revient sans étape.
    void dropRemaining();
    // Annonce d'une édition (editAboutToApply), puis dépôt forcé.
    void announceEdit();
    // Annonce d'un changement de page (pageAboutToChange), puis dépôt forcé.
    void announcePageChange();
    // Objets soulevés d'un document remplacé depuis (replaceDocument direct) : oubliés, liftingChanged.
    void forgetStaleLift();
    // Page de la référence pour la page affichée ; nullptr : jamais enregistrée ni ouverte (page vierge, D9).
    const core::Page* savedPage() const;
    std::vector<core::LiftedObject> m_lifted; // page affichée, ordre de la page
    std::uint64_t m_liftGeneration = 0;       // m_generation au soulèvement
    // Document tel qu'ouvert ou enregistré en dernier (D9), valable pour la génération m_referenceGeneration.
    // Partagé avec la copie que le fil d'écriture a enregistrée : aucune copie de plus sur le fil principal.
    std::shared_ptr<const core::Document> m_reference;
    std::uint64_t m_referenceGeneration = 0;
    // canRevertPage, relue à chaque documentChanged (Main.qml) : une empreinte de page par révision.
    struct RevertCheck
    {
        bool valid = false;
        std::uint64_t revision = 0;
        std::size_t page = 0;
        bool answer = false;
    };
    mutable RevertCheck m_revertCheck;
    mutable int m_revertComparisons = 0;
};

} // namespace actcraft::ui
