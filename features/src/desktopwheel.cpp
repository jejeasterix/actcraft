#include <actcraft/features/desktopwheel.h>

#include <cmath>
#include <numbers>
#include <utility>

namespace actcraft::features {

std::vector<QPointF> wheelPositions(int count, double radius)
{
    std::vector<QPointF> positions;
    if (count <= 0)
        return positions;
    positions.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        // Départ en haut (-90 degrés), puis sens horaire à l'écran (y vers le bas).
        const double angle = (-90.0 + i * 360.0 / count) * std::numbers::pi / 180.0;
        positions.emplace_back(radius * std::cos(angle), radius * std::sin(angle));
    }
    return positions;
}

std::vector<WheelEntry> defaultWheel()
{
    auto command = [](const char* id, const QString& label) {
        return WheelEntry{QString::fromLatin1(id), label, {}, false};
    };
    auto mainWindowTool = [](const char* id, const QString& label) {
        return WheelEntry{QString::fromLatin1(id), label, {}, true};
    };
    auto ring = [](const QString& label, std::vector<WheelEntry> entries) {
        return WheelEntry{QString(), label, std::move(entries), false};
    };
    return {
        command("desktop.restore", QStringLiteral("Revenir à ActCraft")),
        ring(QStringLiteral("Caméra"), {command("tool.camera-area", QStringLiteral("Zone")),
                                        command("tool.camera-polygon", QStringLiteral("Point par point")),
                                        command("tool.camera-freehand", QStringLiteral("Main levée")),
                                        command("tool.camera-window", QStringLiteral("Fenêtre")),
                                        command("tool.camera-screen", QStringLiteral("Écran entier"))}),
        command("tool.keyboard", QStringLiteral("Clavier à l'écran")),
        ring(QStringLiteral("Autres outils"),
             {mainWindowTool("tool.ticker", QStringLiteral("Message défilant")),
              mainWindowTool("tool.clock", QStringLiteral("Horloge")),
              mainWindowTool("tool.calculator", QStringLiteral("Calculatrice")),
              mainWindowTool("tool.dice", QStringLiteral("Dés"))}),
    };
}

} // namespace actcraft::features
