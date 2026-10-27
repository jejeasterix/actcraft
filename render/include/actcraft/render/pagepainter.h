#pragma once

#include <actcraft/core/document.h>

#include <QByteArray>
#include <QImage>

#include <functional>
#include <string>
#include <unordered_map>

class QPainter;
class QTextDocument;

namespace actcraft::render {

// Peintre partagé (DA5, décision 24) : rendu logiciel des nouveaux types, miniatures, comparaison aux .thm,
// puis exports (lot 5). Sans état, sans QObject : utilisable sur un fil secondaire. Règles reprises de la
// référence (REA - Activinspire/tools/render_flipchart.py, app/actcraft/canvas/grid.py), écrites par nous.

inline constexpr int kThumbnailWidth = 320;
// Hauteur maximale d'une miniature : une page très haute garde son rapport, sa largeur est réduite d'autant.
inline constexpr int kThumbnailMaxHeight = 2400;

// Image décodée d'une ressource du document par sa clé ; image nulle si absente.
using ImageLookup = std::function<QImage(const std::string& resourceKey)>;

class DecodedImages
{
public:
    void insert(std::string key, QImage image);
    QImage find(const std::string& key) const;
    std::size_t size() const noexcept;
    // Recherche dans ces images ; `this` doit vivre aussi longtemps que la fonction rendue.
    ImageLookup lookup() const;

private:
    std::unordered_map<std::string, QImage> m_images;
};

// Décode toutes les ressources que QImage sait lire (la signature fait foi, pas l'extension) ; à appeler sur
// un fil secondaire pour un gros document. `maxSide` > 0 : chaque image est décodée réduite pour que son plus
// grand côté n'en dépasse pas la valeur (miniatures), jamais agrandie.
DecodedImages decodeImages(const core::Document& document, int maxSide = 0);

// Texte de haut en bas (H4-3, D9) : l'unique mise en page de la colonne d'un caractère, celle du peintre, que
// l'éditeur de l'interface réutilise pour son cadre et sa mesure. `layOutVerticalColumn` met le document en
// colonne (retour à la ligne partout, lignes centrées, un séparateur de ligne entre deux graphèmes voisins,
// largeur = `verticalColumnWidth` mesurée avant la coupe) ; sa largeur est ensuite `textWidth()`, sa hauteur
// `size().height()`. `verticalColumnWidth` : plus grande avance d'un graphème, chacun à sa police.
void layOutVerticalColumn(QTextDocument& document);
double verticalColumnWidth(const QTextDocument& document);

struct PaintOptions
{
    bool background = true;
    bool grid = true;
    bool thumbnail = false; // grille lissée à 0,7 px, échelle de miniature
};

// Objet en coordonnées de l'objet, sans sa transformation ni sa translucidité (le moteur peint de la scène
// les pose par ses nœuds) ; rien pour un groupe.
void paintObjectLocal(QPainter& painter, const core::PageObject& object, const ImageLookup& images);
// Objet en coordonnées de page : visibilité, translucidité, transformation, puis paintObjectLocal.
void paintObject(QPainter& painter, const core::PageObject& object, const ImageLookup& images);
// Fond de page (couleur, dégradé, motif, image) sur (0, 0, largeur, hauteur).
void paintBackground(QPainter& painter, const core::Page& page, const ImageLookup& images);
// Grille visible de la page (F-621) ; traits de 1 px écran.
void paintGrid(QPainter& painter, const core::Page& page, bool thumbnail);
// Page entière en coordonnées de page : fond, grille dessous (ou dessus si drawnAbove), objets dans l'ordre
// du vecteur.
void paintPage(QPainter& painter, const core::Page& page, const ImageLookup& images,
               const PaintOptions& options = {});
// Image de la page à l'échelle `scale` : fond blanc, 96 ppp, lissage.
QImage renderPage(const core::Page& page, const ImageLookup& images, double scale,
                  const PaintOptions& options = {});
// Miniature PNG de kThumbnailWidth pixels de large (hauteur selon la page, kThumbnailMaxHeight au plus),
// comme les .thm d'origine ; page de taille nulle, négative ou non finie : miniature blanche de 320 × 240.
QByteArray thumbnailPng(const core::Page& page, const ImageLookup& images);

} // namespace actcraft::render
