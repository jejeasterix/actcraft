#pragma once

#include <actcraft/core/inkprobe.h>
#include <actcraft/input/sample.h>

#include <cstdint>

namespace actcraft::input {

// Écarte un déplacement identique (position et pression) au dernier échantillon accepté.
class DuplicateFilter
{
public:
    bool accept(const Sample& sample) noexcept;
    void reset() noexcept;

private:
    bool m_hasLast = false;
    Sample m_last;
};

// Étage de prédiction du stylet : identité à l'étape 1 (hypothèse N1-9), gardé pour la suite.
class IdentityPredictor
{
public:
    Sample predict(const Sample& sample) const noexcept { return sample; }
};

// Numérote → signale à la sonde → filtre → prédit → transmet à l'outil.
class Pipeline
{
public:
    explicit Pipeline(SampleSink& sink);

    void setProbe(core::InkProbe* probe) noexcept;
    void resetSequence(std::uint64_t next = 0) noexcept;
    std::uint64_t nextSequence() const noexcept;
    // Retourne le numéro attribué (même si l'échantillon est filtré).
    std::uint64_t push(Sample sample);

private:
    SampleSink& m_sink;
    core::InkProbe* m_probe = nullptr;
    DuplicateFilter m_filter;
    IdentityPredictor m_predictor;
    std::uint64_t m_next = 0;
};

} // namespace actcraft::input
