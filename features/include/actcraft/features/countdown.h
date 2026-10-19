#pragma once

#include <QString>

#include <cstdint>

namespace actcraft::features {

// Compte à rebours (F-290) : temps en nanosecondes fourni par l'appelant.
class Countdown
{
public:
    void start(int seconds, std::int64_t nowNs);
    void pause(std::int64_t nowNs);
    void resume(std::int64_t nowNs);
    void reset(std::int64_t nowNs); // repart de la durée complète, sans changer pause ni répétition
    void setRepeat(bool repeat) noexcept;
    bool repeat() const noexcept;
    bool running() const noexcept;
    bool paused() const noexcept;
    int remainingSeconds(std::int64_t nowNs) const noexcept; // arrondi au supérieur ; 0 une fois fini
    bool poll(std::int64_t nowNs); // vrai une fois par fin ; en répétition, repart de l'échéance prévue

private:
    std::int64_t m_durationNs = 0;
    std::int64_t m_endNs = 0;
    std::int64_t m_pausedRemainingNs = 0;
    bool m_running = false;
    bool m_paused = false;
    bool m_repeat = false;
};

// Minuterie (compte en avant).
class Stopwatch
{
public:
    void start(std::int64_t nowNs);
    void pause(std::int64_t nowNs);
    void resume(std::int64_t nowNs);
    int elapsedSeconds(std::int64_t nowNs) const noexcept; // arrondi à l'inférieur

private:
    std::int64_t m_startNs = 0;
    std::int64_t m_pausedElapsedNs = 0;
    bool m_running = false;
    bool m_paused = false;
};

QString formatHms(int seconds);    // « hh:mm:ss »
int parseHms(const QString& text); // « hh:mm:ss » (heures sur 1 ou 2 chiffres) ; -1 si invalide

} // namespace actcraft::features
