#pragma once

#include <actcraft/features/countdown.h>
#include <actcraft/ui/commands.h>

#include <QObject>
#include <QPointer>
#include <QString>
#include <QTime>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <cstdint>

namespace actcraft::ui {

// Horloge, minuterie et compte à rebours (F-290). La source affichée est l'heure, la minuterie (compte en
// avant) ou le compte à rebours ; l'affichage est analogique, numérique ou les deux. Le temps est fourni aux
// objets de features/ par ce contrôleur : l'horloge monotone du programme plus `clockNowOffsetMs` (réglage de
// test : les tests avancent le temps sans attendre). Une minuterie de 250 ms (tant que le panneau est actif)
// interroge le compte à rebours ; à chaque fin, l'action choisie (une commande nommée, ou aucune) est
// déclenchée une seule fois. Les actions de fin de ce lot : aucune, page suivante, page précédente.
class ClockController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(bool paused READ paused NOTIFY pausedChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(
        int countdownSeconds READ countdownSeconds WRITE setCountdownSeconds NOTIFY countdownSettingsChanged)
    Q_PROPERTY(QString countdownText READ countdownText NOTIFY countdownSettingsChanged)
    Q_PROPERTY(bool repeat READ repeat WRITE setRepeat NOTIFY countdownSettingsChanged)
    Q_PROPERTY(QString endAction READ endAction WRITE setEndAction NOTIFY countdownSettingsChanged)
    Q_PROPERTY(QVariantList endActions READ endActions CONSTANT)
    // Affichages : lus à la demande, notifiés au plus une fois par seconde (displayChanged).
    Q_PROPERTY(QString displayText READ displayText NOTIFY displayChanged)
    Q_PROPERTY(QString remainingText READ remainingText NOTIFY displayChanged)
    Q_PROPERTY(qreal hourAngle READ hourAngle NOTIFY displayChanged)
    Q_PROPERTY(qreal minuteAngle READ minuteAngle NOTIFY displayChanged)
    Q_PROPERTY(qreal secondAngle READ secondAngle NOTIFY displayChanged)
    Q_PROPERTY(qint64 clockNowOffsetMs READ clockNowOffsetMs WRITE setClockNowOffsetMs NOTIFY displayChanged)
    Q_PROPERTY(actcraft::ui::Commands* commands READ commands WRITE setCommands NOTIFY commandsChanged)

public:
    enum Source
    {
        Clock = 0,
        Stopwatch = 1,
        Countdown = 2
    };
    Q_ENUM(Source)
    enum Mode
    {
        Analog = 0,
        Digital = 1,
        Both = 2
    };
    Q_ENUM(Mode)

    explicit ClockController(QObject* parent = nullptr);

    int source() const noexcept;
    void setSource(int source);
    int mode() const noexcept;
    void setMode(int mode);
    bool paused() const noexcept;
    bool active() const noexcept;
    void setActive(bool active);
    int countdownSeconds() const noexcept;
    void setCountdownSeconds(int seconds);
    QString countdownText() const;
    bool repeat() const noexcept;
    void setRepeat(bool repeat);
    QString endAction() const;
    // Refuse (sans effet) une commande hors de la liste endActions().
    void setEndAction(const QString& command);
    // [{command, label}] : « aucune » (commande vide), page suivante, page précédente.
    QVariantList endActions() const;
    QString displayText() const;
    QString remainingText() const;
    qreal hourAngle() const;
    qreal minuteAngle() const;
    qreal secondAngle() const;
    qint64 clockNowOffsetMs() const noexcept;
    void setClockNowOffsetMs(qint64 milliseconds);
    Commands* commands() const noexcept;
    void setCommands(Commands* commands);

    // Passe d'un affichage au suivant (analogique, numérique, les deux).
    Q_INVOKABLE void cycleMode();
    // Passe à la source suivante (heure, minuterie, compte à rebours).
    Q_INVOKABLE void cycleSource();
    // Fige ou relance la source affichée.
    Q_INVOKABLE void togglePause();
    // « hh:mm:ss » (heures sur 1 ou 2 chiffres) pris pour durée ; faux, sans effet, si le texte est invalide
    // ou nul.
    Q_INVOKABLE bool applyCountdownText(const QString& text);
    // Lance le compte à rebours depuis sa durée complète (source : compte à rebours).
    Q_INVOKABLE void startCountdown();
    // Repart de la durée complète ; la pause et la répétition sont gardées.
    Q_INVOKABLE void resetCountdown();
    // Arrête compte à rebours et minuterie (fermeture du panneau).
    Q_INVOKABLE void stop();

signals:
    void sourceChanged();
    void modeChanged();
    void pausedChanged();
    void activeChanged();
    void countdownSettingsChanged();
    void displayChanged();
    void commandsChanged();
    void countdownEnded();

private:
    std::int64_t nowNs() const;
    QTime wallTime() const;
    void tick();
    void refreshTimer();

    features::Countdown m_countdown;
    features::Stopwatch m_stopwatch;
    QTimer m_timer;
    QPointer<Commands> m_commands;
    QString m_endAction;
    QString m_lastDisplay;
    QTime m_frozenWall;
    qint64 m_offsetMs = 0;
    int m_seconds = 60;
    int m_source = Clock;
    int m_mode = Both;
    bool m_repeat = false;
    bool m_active = false;
    bool m_countdownStarted = false;
    bool m_stopwatchStarted = false;
    bool m_stopwatchPaused = false; // features::Stopwatch n'expose pas son état de pause
    bool m_clockPaused = false;
};

} // namespace actcraft::ui
