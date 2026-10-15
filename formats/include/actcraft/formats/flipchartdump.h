#pragma once

#include <actcraft/core/document.h>

#include <QJsonObject>
#include <QJsonValue>
#include <QStringList>

namespace actcraft::formats::flipchart {

inline constexpr char kDumpFormat[] = "actcraft-flipchart-dump/1";

// Export canonique d'un document lu d'un .flipchart (décision 19), comparable à celui de l'oracle Python
// (REA - Activinspire/tools/oracle_flipchart.py). Règles : plan du lot 2a, tâche L2-T10 ; elles sont
// appliquées ici aux valeurs du modèle (couche 0 à 2, indices dans la liste exportée, style nul sans
// largeur…), jamais recopiées telles quelles.
QJsonObject canonicalDump(const core::Document& document);

// Écarts entre deux exports, au plus `maxDiffs` : « /pages/0/objects/3/name : "a" → "b" ». Compare des
// valeurs, pas des textes JSON. Nombres égaux à 10⁻³ près (plus 10⁻⁹ relatif), −0 et +0 égaux ; clés absentes
// d'un côté, tailles de listes et types différents comptent.
QStringList compareCanonical(const QJsonValue& expected, const QJsonValue& actual, int maxDiffs = 20);

} // namespace actcraft::formats::flipchart
