#pragma once

#include <actcraft/input/sample.h>

#include <optional>
#include <vector>

namespace actcraft::features {

// Un bord ou un arc qui guide l'encre (outils de maths, F-280 à F-283). Coordonnées de page.
class StrokeGuide
{
public:
    virtual ~StrokeGuide() = default;
    // Requête pure : échantillon projeté sur le guide si `sample` en est à moins de `tolerance` (pixels de
    // page) ; sinon rien. Ne mémorise rien (aperçu, choix du guide le plus proche).
    virtual std::optional<input::Sample> project(const input::Sample& sample, double tolerance) const = 0;
    // Comme project(), et mémorise la partie saisie (le bord) que suivra follow().
    virtual std::optional<input::Sample> capture(const input::Sample& sample, double tolerance) = 0;
    // Échantillon projeté sur le guide saisi au dernier capture() réussi (borné à son étendue).
    virtual input::Sample follow(const input::Sample& sample) = 0;
};

// Ensemble des guides ouverts : à l'appui, le plus proche saisit le trait ; il le garde jusqu'au relâché.
class GuideSet
{
public:
    void add(StrokeGuide* guide);
    void remove(StrokeGuide* guide);
    void setTolerance(double pagePixels) noexcept; // 16 pixels d'écran ÷ zoom, tenu à jour par le contrôleur
    input::Sample filter(const input::Sample& sample);
    bool guiding() const noexcept;
    // Cercle d'accroche au survol. Contrat : rien pendant un trait guidé ; ne change jamais le guide saisi,
    // ni l'état d'aucun guide (project() seulement).
    std::optional<input::Sample> snapPreview(const input::Sample& hover) const;

private:
    struct Nearest
    {
        StrokeGuide* guide = nullptr; // nul si aucun guide n'est à la tolérance
        input::Sample sample;         // `sample` projeté sur ce guide
    };
    // Guide le plus proche de `sample` à la tolérance près (distance entre l'échantillon et sa projection) ;
    // requête pure.
    Nearest nearest(const input::Sample& sample) const;

    std::vector<StrokeGuide*> m_guides;
    StrokeGuide* m_active = nullptr;
    double m_tolerance = 16;
};

} // namespace actcraft::features
