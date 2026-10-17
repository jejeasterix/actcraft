#pragma once

#include <actcraft/core/model.h>

#include <QImage>

namespace actcraft::render {

// Image procédurale d'un objet (décision 4) : aucune image lue sur disque. ARGB32 prémultiplié, taille
// source. Appelée sur un fil secondaire.
QImage synthesizeImage(const core::ImageObject& object);

// Image sans table de couleurs (L2-C7) : une image indexée (Mono, MonoLSB, Indexed8) est convertie en ARGB32,
// sa table d'abord complétée de couleurs transparentes jusqu'au dernier index possible (2 ou 256). Le peintre
// de Qt lit sinon la table hors de ses bornes (assertion de QList::at en debug, lecture hors bornes en
// release) pour tout octet d'index sans couleur : index hors table venu du fichier (non sûr), table vide, ou
// octet de bourrage de fin de ligne, jamais écrit par le décodeur, que Qt lit en réduction lissée. Règle :
// un index sans couleur dans la table donne un pixel transparent. Autres formats : l'image telle quelle.
// Appelée au décodage (decodeImages, ImageStore) et au dessin (peintre partagé), sur tout fil.
QImage withoutColorTable(const QImage& image);

} // namespace actcraft::render
