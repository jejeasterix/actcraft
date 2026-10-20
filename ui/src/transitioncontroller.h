#pragma once

#include <actcraft/features/transitions.h>
#include <actcraft/ui/documentcontroller.h>

#include <QObject>
#include <QPointer>
#include <QRandomGenerator>
#include <QString>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Effets de changement de page (F-640) : réglages, choix de l'effet et déroulé d'un changement. L'effet est
// celui de la page QUITTÉE (core::PagePresentation::transition), sinon l'effet global ; « random » est tiré
// par features::pickEffect sur un générateur injectable (seedRandom). Seuls page.next et page.previous
// passent ici (navigate) ; sans effet à jouer, la page change aussitôt, comme sans enveloppe. Avec un effet :
// la surcouche fige un instantané (captureRequested), puis frozen() change la page et l'effet se joue
// (playRequested) ; finished() le termine. Aucune demande de redessin du cache : seule la vue redessine, pour
// la nouvelle page.
class TransitionController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(actcraft::ui::DocumentController* documentController READ documentController WRITE
                   setDocumentController NOTIFY documentControllerChanged)
    Q_PROPERTY(QString globalEffect READ globalEffect WRITE setGlobalEffect NOTIFY settingsChanged)
    Q_PROPERTY(int durationTenths READ durationTenths WRITE setDurationTenths NOTIFY settingsChanged)
    // Vrai pendant le rideau (lié par la surcouche à curtain.active) : aucun effet.
    Q_PROPERTY(bool blocked READ blocked WRITE setBlocked NOTIFY blockedChanged)
    Q_PROPERTY(int phase READ phase NOTIFY phaseChanged)        // Phase
    Q_PROPERTY(bool playing READ playing NOTIFY phaseChanged)   // instantané en cours ou effet joué
    Q_PROPERTY(QString effect READ effect NOTIFY phaseChanged)  // effet en cours ; vide au repos
    Q_PROPERTY(QString played READ played NOTIFY playedChanged) // dernier effet joué (scénarios)
    Q_PROPERTY(QVariantList catalogue READ catalogue CONSTANT)  // [{id, label}] (navigateur, L7-T8)

public:
    enum Phase
    {
        Idle = 0,
        Capturing = 1, // instantané demandé, page encore affichée
        Animating = 2  // page changée, effet joué sur l'instantané
    };
    Q_ENUM(Phase)

    explicit TransitionController(QObject* parent = nullptr);

    DocumentController* documentController() const noexcept;
    void setDocumentController(DocumentController* controller);
    QString globalEffect() const;
    void setGlobalEffect(const QString& id); // identifiant du catalogue, sinon ignoré ; gardé (QSettings)
    int durationTenths() const noexcept;
    void setDurationTenths(int tenths); // borné à 5..30 ; gardé (QSettings)
    bool blocked() const noexcept;
    void setBlocked(bool blocked);
    int phase() const noexcept;
    bool playing() const noexcept;
    QString effect() const;
    QString played() const;
    QVariantList catalogue() const;

    // kind : 0 page suivante, 1 page précédente. Un appel pendant un effet le termine (la page demandée par
    // l'instantané en cours change d'abord) puis enchaîne : aucune page sautée ni perdue.
    Q_INVOKABLE void navigate(int kind);
    Q_INVOKABLE void frozen();   // la surcouche : l'instantané est prêt
    Q_INVOKABLE void finished(); // la surcouche : l'effet est fini
    // Effet propre à la page affichée (page.effect) : identifiant du catalogue, ou vide (effet global) ; une
    // étape d'annulation (core::SetPagePresentationCommand).
    Q_INVOKABLE void setPageEffect(const QString& id);
    // Hasard de « random » : graine fixée (tests, scénarios).
    Q_INVOKABLE void seedRandom(quint32 seed);
    // Pièces de l'ancienne page pour un effet : [{x, y, width, height} en fractions de la page, {dx, dy} en
    // fractions de sa largeur et de sa hauteur, scale, opacity, origin (Item.TransformOrigin), span (part de
    // la durée, 0 à 1)] ; état de départ : en place, opaque, échelle 1. Vide pour « none » ou un inconnu.
    Q_INVOKABLE QVariantList motion(const QString& effect) const;

signals:
    void documentControllerChanged();
    void settingsChanged();
    void blockedChanged();
    void phaseChanged();
    void playedChanged();
    void captureRequested(const QString& effect);
    void playRequested(const QString& effect, int durationMs);
    void stopRequested(); // retirer l'instantané (fin, interruption)

private:
    // Change de page ; vrai si la page affichée a changé.
    bool turn(features::NavigationKind kind);
    void settle(); // termine ce qui est en cours
    void setPhase(Phase phase);
    void save() const;

    QPointer<DocumentController> m_document;
    features::TransitionSettings m_settings;
    QRandomGenerator m_random;
    features::NavigationKind m_pending = features::NavigationKind::Next;
    QString m_effect;
    QString m_played;
    Phase m_phase = Idle;
    bool m_blocked = false;
};

} // namespace actcraft::ui
