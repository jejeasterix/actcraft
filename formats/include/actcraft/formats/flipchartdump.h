#pragma once

#include <actcraft/core/document.h>
#include <actcraft/formats/flipchart.h>

#include <QJsonObject>
#include <QJsonValue>
#include <QStringList>

namespace actcraft::formats::flipchart {

inline constexpr char kDumpFormat[] = "actcraft-flipchart-dump/1";

// Export canonique d'un document lu d'un .flipchart (décision 19), comparable à celui de l'oracle Python
// (REA - Activinspire/tools/oracle_flipchart.py). Règles : plan du lot 2a, tâche L2-T10, complétées ici ;
// elles sont appliquées aux valeurs du modèle, jamais recopiées telles quelles :
// - racine : format, title, warnings (nombre d'avertissements du lecteur, surcharge à FlipchartReadResult),
// pages ;
// - page : width, height, name, notes ; background {color, gradient, color2, image} ; grid {visible,
// snapEnabled,
//   drawnAbove, previewZoom, baseZoom, zoomIncrement, levels [{visible, snap, start, spacingX, spacingY,
//   color, pattern, angle, line, linesAlongX, linesAlongY}]} ; objects ;
// - tout objet : kind, layer (0 à 2 : le modèle numérote 1 à 3), name, visible, locked, translucency,
// movable,
//   resizable, rotatable, label, matrix (objet → page), actions [{type, command, target, resource}] ;
// - plume {color, width, style (0 sans largeur), cap (0 plat, 1 carré, 2 rond), join (0 angle, 1 biseau, 2
// rond)} ;
//   brosse {kind, color, color2 ("" hors dégradé), gradient (0 hors dégradé), pattern (0 hors motif), opaque,
//   hatch} ;
// - annotation : pen {color, width, cap}, strokeMode, points ; line : pen, points ; shape : pen, brush,
// paths,
//   open, winding, bounds ; image : bounds, image, transparent, keyColor ("" sans), pixels ([0, 0] sans
//   données) ; text : bounds, html (SHA-256), runs [{text, font, size, weight, italic, underline, strikeOut,
//   color, baseline, bounds}] ; media : bounds, image, media, file, playOnOpen, loop ; group : members ;
//   connector : pen, firstEnd, secondEnd, objectA, objectB.
// Points un par un ; références (members, target, objectA, objectB) en indices de la liste exportée, −1 si
// aucun.
QJsonObject canonicalDump(const core::Document& document);
QJsonObject canonicalDump(const FlipchartReadResult& result);

// Écarts entre deux exports, au plus `maxDiffs` : « /pages/0/objects/3/name : "a" → "b" ». Compare des
// valeurs, pas des textes JSON. Nombres égaux à 10⁻³ près (plus 10⁻⁹ relatif), −0 et +0 égaux ; clés absentes
// d'un côté ou de l'autre, tailles de listes et types différents comptent.
QStringList compareCanonical(const QJsonValue& expected, const QJsonValue& actual, int maxDiffs = 20);

} // namespace actcraft::formats::flipchart
