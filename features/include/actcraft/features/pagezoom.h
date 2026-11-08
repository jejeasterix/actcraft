#pragma once

#include <QPointF>

#include <cstdint>

namespace actcraft::features {

// Zoom de page (F-260) : appui maintenu = zoom continu autour du point (gauche : avant ; droit : arrière),
// glisser = panoramique (l'appui qui devient panoramique annule son zoom), double-clic = « Ajuster ». Temps
// en nanosecondes fourni par l'appelant (core::monotonicNowNs()).
class PageZoom
{
public:
    enum class Direction
    {
        In,
        Out
    };
    static constexpr double kRatePerSecond =
        2.0; // le zoom double en une seconde (valeur à nous ; L7-T0 peut la corriger)
    static constexpr double kMinZoom = 0.25;   // = ui::kMinZoom (ui/include/actcraft/ui/viewtransform.h)
    static constexpr double kMaxZoom = 8.0;    // = ui::kMaxZoom
    static constexpr double kPanThreshold = 8; // pixels de vue avant qu'un appui devienne un panoramique
    static constexpr std::int64_t kDoubleClickNs = 400'000'000; // deux appuis en 400 ms au plus
    static constexpr double kDoubleClickPixels = 6;             // et à 6 pixels au plus

    explicit PageZoom(double startZoom);
    void setZoom(double zoom) noexcept; // zoom courant de la vue, lu avant chaque appui
    void press(QPointF viewPoint, Direction direction, std::int64_t nowNs);
    bool move(QPointF viewPoint, std::int64_t nowNs); // vrai quand l'appui est (ou devient) un panoramique
    QPointF panDelta() noexcept;                      // déplacement depuis le dernier appel ; remis à zéro
    void release(std::int64_t nowNs);
    double zoomAt(std::int64_t nowNs) const noexcept; // borné à [kMinZoom, kMaxZoom]
    QPointF anchor() const noexcept;                  // point de vue fixe pendant le zoom
    bool panning() const noexcept;
    bool pressed() const noexcept;
    bool isDoubleClick(QPointF viewPoint, std::int64_t nowNs); // à appeler à chaque appui ; vrai au second

private:
    double m_startZoom;
    double m_frozenZoom;
    Direction m_direction = Direction::In;
    QPointF m_anchor;
    QPointF m_lastPoint;
    QPointF m_panDelta;
    std::int64_t m_pressNs = 0;
    bool m_pressed = false;
    bool m_panning = false;
    bool m_hasLastClick = false;
    QPointF m_lastClickPoint;
    std::int64_t m_lastClickNs = 0;
};

} // namespace actcraft::features
