#pragma once

#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/documenttabs.h>
#include <actcraft/ui/pageview.h>

#include <QObject>
#include <QPointer>
#include <QString>
#include <QThreadPool>
#include <QtQml/qqmlparserstatus.h>
#include <QtQml/qqmlregistration.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace actcraft::ui {

// Reprise sans perte après une interruption (D11-8, L11-T13).
//
// Mise en arrière-plan (Qt::ApplicationSuspended, et Qt::ApplicationHidden sous Android) : les contacts en
// cours sont terminés et gardés (PageView::finishContacts), la copie du document est prise comme par
// « Enregistrer » (DocumentController::snapshotForSave : texte en cours et objets soulevés compris), puis,
// si un document est modifié (celui de l'onglet affiché ou un onglet garé), la copie de reprise est écrite
// hors du fil principal dans `recoveryDir` : `reprise.actcraft` (onglet affiché s'il est modifié, sinon le
// premier onglet garé modifié), `reprise-2.actcraft`… (les autres), chacun écrit par l'écrivain natif
// (saveDocument, le chemin d'« Enregistrer » : jamais un second format), et son `.json` (URL d'origine, page,
// échelle, heure, empreinte ; rien d'autre). Un document inchangé depuis sa dernière copie n'est pas réécrit.
//
// Démarrage (start(), appelé par componentComplete) : les copies sont relues hors du fil principal ; une
// copie plus ancienne que son fichier d'origine, ou illisible, est effacée ; les autres sont rouvertes avec
// l'URL d'origine (si le .json décrit bien la copie : même empreinte), l'état « modifié » et le fichier
// d'origine comme référence de « Rétablir la page ». La première va dans l'onglet affiché s'il n'a pas changé
// depuis start() ; sinon (trait déjà fait, fichier ouvert au lancement) chacune a son onglet neuf, et
// l'onglet affiché reste devant. Sans onglets, une copie qui ne peut pas être rouverte est gardée telle
// quelle.
//
// La copie est effacée par un enregistrement réussi qui laisse tous les documents propres, par une mise en
// arrière-plan où plus rien n'est modifié, par discard() (recovery.discard, « Quitter » accepté) ; jamais une
// copie que ce gardien n'a ni écrite ni rouverte. Une copie relue mais pas encore montrée (reprise en
// attente, onglet neuf refusé sous verrou de classe) est gardée : ni réécrite ni effacée, rouverte au
// lancement suivant.
//
// État de l'application : un filtre posé sur qApp lit l'événement public QApplicationStateChangeEvent (A22),
// celui que la plateforme et l'étape « appState » du joueur envoient. applyState est aussi relié au signal
// QGuiApplication::applicationStateChanged (repli, H11-17) : un même état reçu deux fois n'agit qu'une fois.
class LifecycleKeeper : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_ELEMENT
    Q_PROPERTY(
        actcraft::ui::DocumentController* document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(actcraft::ui::PageView* view READ view WRITE setView NOTIFY viewChanged)
    Q_PROPERTY(actcraft::ui::DocumentTabs* tabs READ tabs WRITE setTabs NOTIFY tabsChanged)
    // Vide : <AppDataLocation>/reprise, le dossier privé de l'application.
    Q_PROPERTY(QString recoveryDir READ recoveryDir WRITE setRecoveryDir NOTIFY recoveryDirChanged)
    Q_PROPERTY(bool recovered READ recovered NOTIFY recoveredChanged)
    Q_PROPERTY(bool lastWriteOk READ lastWriteOk NOTIFY lastWriteOkChanged)

public:
    explicit LifecycleKeeper(QObject* parent = nullptr);
    ~LifecycleKeeper() override; // attend la fin des écritures déjà lancées

    DocumentController* document() const noexcept;
    void setDocument(DocumentController* document);
    PageView* view() const noexcept;
    void setView(PageView* view);
    DocumentTabs* tabs() const noexcept;
    void setTabs(DocumentTabs* tabs);

    QString recoveryDir() const;
    void setRecoveryDir(const QString& dir);
    static QString defaultRecoveryDir();
    bool recovered() const noexcept;
    bool lastWriteOk() const noexcept;

    // Relit les copies de reprise et les rouvre ; une fois par objet.
    Q_INVOKABLE void start();
    // recovery.discard et « Quitter » accepté : efface la copie ; le bandeau disparaît, les documents
    // restent.
    Q_INVOKABLE void discard();
    Q_INVOKABLE void applyState(Qt::ApplicationState state);

    void classBegin() override {}
    void componentComplete() override;

    struct Found; // copie relue au démarrage (lifecycle.cpp)
    struct Held;  // copies relues mais pas montrées : ni réécrites ni effacées (lifecycle.cpp)

signals:
    void documentChanged();
    void viewChanged();
    void tabsChanged();
    void recoveryDirChanged();
    void recoveredChanged();
    void lastWriteOkChanged();
    void writeFinished(bool ok);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void suspend();
    void finishWrite(bool ok);
    void restore(const std::shared_ptr<std::vector<Found>>& found, std::uint64_t key, std::uint64_t revision);
    bool anyModified() const;
    void removeCopy();
    void setRecovered(bool recovered);

    QPointer<DocumentController> m_document;
    QPointer<PageView> m_view;
    QPointer<DocumentTabs> m_tabs;
    QMetaObject::Connection m_saved;
    QMetaObject::Connection m_waitIdle;
    QString m_recoveryDir;
    Qt::ApplicationState m_state = Qt::ApplicationActive;
    bool m_started = false;
    bool m_recovered = false;
    bool m_lastWriteOk = false;
    // Copie écrite ou rouverte par ce gardien : lui seul l'efface à l'enregistrement.
    bool m_ownsCopy = false;
    // Dernière copie demandée : document de l'onglet affiché (clé, révision) ; valide tant qu'elle est sur le
    // disque et qu'aucun onglet garé n'était modifié.
    bool m_copyValid = false;
    std::uint64_t m_copyKey = 0;
    std::uint64_t m_copyRevision = 0;
    std::vector<std::pair<std::uint64_t, std::size_t>> m_copyParked; // onglets garés modifiés : clé, pages
    std::shared_ptr<Held> m_held;
    // Un seul fil : écritures, effacements et lecture du démarrage dans l'ordre où ils sont demandés.
    // Déclaré en dernier : détruit le premier, il attend ses tâches avant tout autre membre.
    QThreadPool m_pool;
};

} // namespace actcraft::ui
