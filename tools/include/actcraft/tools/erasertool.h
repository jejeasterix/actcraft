#pragma once

#include <actcraft/tools/tool.h>
#include <actcraft/tools/toolcontext.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace actcraft::tools {

// Gomme (F-230) : soulève chaque annotation touchée (une fois), la recoupe dans la couche soulevée pendant le
// geste, dépose une core::ReplaceObjectsCommand au relâché.
class EraserTool final : public Tool
{
public:
    explicit EraserTool(ToolContext& context);
    QString id() const override; // « eraser »
    void onSample(const input::Sample& sample) override;
    bool isBusy() const noexcept override;
    void cancel() override;
    double radius() const; // largeur réglée de la gomme / 2, pixels de page
    // Appels de core::cutByDisk depuis la création de l'outil (tests et banc : le coût d'un mouvement ne doit
    // suivre que les morceaux proches de la gomme).
    std::size_t cutCalls() const noexcept;

private:
    // Un morceau courant : ligne centrale en points de page et son emprise (sans l'épaisseur).
    struct Piece
    {
        std::vector<core::StrokePoint> points;
        core::RectF bounds;
    };
    // Un trait soulevé pendant le geste : modèle des morceaux (commun et trait d'origine, sans ses points,
    // transformation identité, épaisseur à l'échelle visible) et morceaux courants.
    struct Lifted
    {
        core::ObjectId id = 0;
        core::PageObject original;
        std::vector<Piece> pieces;
        bool touched = false; // le disque a retiré une partie du trait
    };

    void begin(const input::Sample& sample);
    void sweep(core::PointF from, core::PointF to, bool first = false);
    void finish();
    void reset();
    std::vector<core::PageObject> pieceObjects(const Lifted& lifted) const;
    void showLifted();

    ToolContext& m_context;
    bool m_active = false;
    std::uint64_t m_pointer = 0; // pointeur de l'appui : seuls ses mouvements et son relâché comptent
    std::size_t m_cuts = 0;
    core::PointF m_last;
    std::vector<Lifted> m_lifted;
};

} // namespace actcraft::tools
