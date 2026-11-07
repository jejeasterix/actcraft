#pragma once

#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>

#include <QObject>
#include <QPointer>
#include <QString>
#include <QThreadPool>
#include <QtQml/qqmlparserstatus.h>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Reprise sans perte après une interruption (D11-8, L11-T13).
//
// Mise en arrière-plan (Qt::ApplicationSuspended, et Qt::ApplicationHidden sous Android) : les contacts en
// cours sont terminés et gardés (PageView::finishContacts), puis, si le document est modifié, une copie de
// reprise est écrite hors du fil principal dans `recoveryDir` : `reprise.actcraft`, écrit par l'écrivain
// natif (saveDocument, le chemin d'« Enregistrer » : jamais un second format du document), puis
// `reprise.json` (URL d'origine, page, échelle, heure, empreinte ; rien d'autre).
//
// Démarrage (start(), appelé par componentComplete) : une copie au moins aussi récente que le fichier
// d'origine est relue hors du fil principal et rouverte avec l'URL d'origine et l'état « modifié » ;
// `recovered` devient vrai (bandeau). Un enregistrement réussi ou discard() supprime la copie. Rien n'est
// écrit tant que rien n'a changé.
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

    QString recoveryDir() const;
    void setRecoveryDir(const QString& dir);
    static QString defaultRecoveryDir();
    bool recovered() const noexcept;
    bool lastWriteOk() const noexcept;

    // Relit la copie de reprise et la rouvre ; une fois par objet.
    Q_INVOKABLE void start();
    // recovery.discard : supprime la copie ; le bandeau disparaît, le document affiché reste.
    Q_INVOKABLE void discard();
    Q_INVOKABLE void applyState(Qt::ApplicationState state);

    void classBegin() override {}
    void componentComplete() override;

signals:
    void documentChanged();
    void viewChanged();
    void recoveryDirChanged();
    void recoveredChanged();
    void lastWriteOkChanged();
    void writeFinished(bool ok);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    struct Found;
    void suspend();
    void finishWrite(bool ok, const QString& fingerprint);
    void restore(Found& found, std::uint64_t documentKey);
    void removeCopy();
    void setRecovered(bool recovered);

    QPointer<DocumentController> m_document;
    QPointer<PageView> m_view;
    QMetaObject::Connection m_saved;
    QString m_recoveryDir;
    Qt::ApplicationState m_state = Qt::ApplicationActive;
    bool m_started = false;
    bool m_recovered = false;
    bool m_lastWriteOk = false;
    bool m_mayHaveCopy = false;
    // Un seul fil : écritures, suppressions et lecture du démarrage dans l'ordre où elles sont demandées.
    // Déclaré en dernier : détruit le premier, il attend ses tâches avant tout autre membre.
    QThreadPool m_pool;
};

} // namespace actcraft::ui
