#pragma once

#include <actcraft/render/objectrenderer.h>
#include <actcraft/render/pagepainter.h>

#include <QImage>
#include <QRectF>

class QSGNode;

namespace actcraft::render {

// Bornes d'une texture rastérisée (RHI) : au-delà, le nœud est rastérisé à une échelle réduite (plus flou,
// jamais plus lourd). Mémoire : 4 octets par pixel, 64 Mo au plus par nœud ; la page fixe tient une texture
// pour le fond peint, une pour la grille et une par objet peint, soit (2 + objets peints) × 64 Mo au pire (un
// objet usuel ne couvre qu'une petite part de la page).
inline constexpr double kMaxRasterPixels = 16.0 * 1024 * 1024;
inline constexpr double kMaxRasterSide = 8192; // côté de texture accepté par les cartes visées

// Échelle de rastérisation d'un rectangle de `width` × `height` unités : `scale` (0,01 au moins), réduite
// pour que l'image tienne sous kMaxRasterPixels et kMaxRasterSide.
double boundedRasterScale(double width, double height, double scale) noexcept;

// Vrai si l'objet passe par le peintre partagé plutôt que par un moteur du prototype : texte, ligne, média,
// connecteur, forme à chemins, trait autre que le stylo opaque (marqueur, encre révélatrice, trait
// translucide).
bool usesPaintedRenderer(const core::PageObject& object) noexcept;

// Objet en coordonnées de l'objet, rastérisé à `scale` (borné par boundedRasterScale) dans une image ARGB
// prémultipliée ; `localRect` reçoit le rectangle couvert (coordonnées de l'objet, largeur réellement
// dessinée, bouts et onglets compris), que l'image recouvre exactement.
QImage rasterizeLocal(const core::PageObject& object, const ImageLookup& images, double scale,
                      QRectF* localRect);

// Moteur peint (DA5, décision 37). Sa transformation et sa translucidité sont posées par la page fixe (nœuds
// parents) : il dessine en coordonnées de l'objet. Logiciel : nœud QPainter vectoriel ; RHI : texture
// rastérisée à context.rasterScale. Copie à sa création les images dont il a besoin.
class PaintedRenderer
{
public:
    QSGNode* createNode(const core::PageObject& object, const RenderContext& context) const;
};

// Vrai si le fond est peint (dégradé, motif, image) plutôt qu'uni.
bool backgroundIsPainted(const core::Page& page) noexcept;
// Fond de la page sur le blanc de la page, comme renderPage : rectangle uni si possible (couleur translucide
// composée sur le blanc), sinon rectangle blanc portant le fond peint. Grille : nullptr si invisible ou sans
// niveau.
QSGNode* createBackgroundNode(const core::Page& page, const RenderContext& context);
QSGNode* createGridNode(const core::Page& page, const RenderContext& context);

} // namespace actcraft::render
