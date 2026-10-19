#pragma once

namespace actcraft::input {

struct Point2
{
    double x = 0;
    double y = 0;
};

// Écart relatif des deux doigts sous lequel le zoom ne bouge pas (défilement seul).
inline constexpr double kPinchDeadZone = 0.03;

struct GestureStep
{
    Point2 anchor;    // milieu des doigts à l'appui (vue) : le point de page qui s'y trouvait suit `center`
    Point2 center;    // milieu des doigts maintenant (vue)
    double scale = 1; // écart des doigts / écart à l'appui ; 1 dans la zone morte
};

// Pincer et défiler à deux doigts (décision 25), pixels de vue.
class TwoFingerGesture
{
public:
    void begin(Point2 a, Point2 b);
    GestureStep update(Point2 a, Point2 b) const; // sans begin : scale 1, anchor = center = milieu
    bool active() const noexcept;
    void end() noexcept;

private:
    Point2 m_anchor;
    double m_distance = 0;
    bool m_active = false;
};

} // namespace actcraft::input
