#pragma once

// Module media — Lecteur (média inséré sur la page) et enregistreurs de son et d'écran, par Qt Multimedia. Ne
// dépend que de core et de Qt : aucun ui/.

namespace actcraft::media {

// Identifiant du module, utilisé par les journaux et le banc.
const char* moduleId() noexcept;

} // namespace actcraft::media
