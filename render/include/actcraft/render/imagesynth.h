#pragma once

#include <actcraft/core/model.h>

#include <QImage>

namespace actcraft::render {

// Image procédurale d'un objet (décision 4) : aucune image lue sur disque. ARGB32 prémultiplié, taille
// source. Appelée sur un fil secondaire.
QImage synthesizeImage(const core::ImageObject& object);

} // namespace actcraft::render
