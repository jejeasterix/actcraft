#pragma once

#include <actcraft/core/model.h>

#include <QImage>

#include <string>

namespace actcraft::render {

// Image procédurale d'un objet (décision 4) : aucune image lue sur disque. ARGB32 prémultiplié, taille
// source. Appelée sur un fil secondaire.
QImage synthesizeImage(const core::ImageObject& object);

// Les champs qui décident de l'image procédurale d'un objet (motif, couleurs, graine, taille source, couleur
// transparente), en texte : deux objets de même clé se décodent pareil. Écrite ici pour que le décodage de la
// page (FixedPageItem) et la couverture du magasin (ImageStore::covers) comptent les mêmes champs.
std::string synthesisKey(const core::ImageObject& object);

} // namespace actcraft::render
