#pragma once

#include <QtGlobal>

// Numéros du format .flipchart lus par ActCraft, écrits à la main d'après notre documentation
// (REA - Activinspire/docs/re/format-flipchart.md § 4 à § 7). Seuls les numéros des genres d'objets et des
// propriétés lus y sont, sous des noms à nous : aucun nom de classe ni de propriété d'Inspire. Les numéros de
// propriétés sont stables entre versions du format (§ 4.2), mais propres à chaque genre au-delà de 36 : 38
// est la ressource d'une forme et la translucidité d'une annotation.

namespace actcraft::formats::flipchart::ids {

// Genres d'objets : numéro u16 après la balise 'O' d'un flux et dans chaque nœud de l'index.
namespace kind {
inline constexpr quint16 kDocumentObject = 1;    // le paperboard lui-même
inline constexpr quint16 kPageObject = 2;        // une page
inline constexpr quint16 kStrokeObject = 3;      // trait d'encre (annotation)
inline constexpr quint16 kLineObject = 5;        // ligne brisée
inline constexpr quint16 kShapeObject = 7;       // forme à polygones
inline constexpr quint16 kImageObject = 9;       // image
inline constexpr quint16 kTextObject = 11;       // texte riche
inline constexpr quint16 kGridObject = 13;       // grille de la page
inline constexpr quint16 kGridLevelObject = 14;  // un niveau de la grille
inline constexpr quint16 kGroupObject = 18;      // groupe par références
inline constexpr quint16 kToolAction = 22;       // action : choisir un outil
inline constexpr quint16 kGoToPageAction = 23;   // action : aller à une page
inline constexpr quint16 kShiftAction = 24;      // action : déplacer
inline constexpr quint16 kProgramAction = 25;    // action : commande de l'application
inline constexpr quint16 kOpenFileAction = 26;   // action : ouvrir un fichier
inline constexpr quint16 kObjectAction = 28;     // action sur un objet de la page
inline constexpr quint16 kBackgroundObject = 29; // fond de la page
inline constexpr quint16 kTextAction = 50;       // action sur un texte
inline constexpr quint16 kConnectorObject = 52;  // connecteur entre deux objets
inline constexpr quint16 kTagObject = 53;        // étiquette (légende) d'un objet
inline constexpr quint16 kMediaObject = 253;     // son ou vidéo
inline constexpr quint16 kLaunchAction = 463;    // action : lancer une mini-application
// Texte de haut en bas (§ 4.2) : son cadre est une propriété commune (§ 7.1), le champ de son image n'est pas
// dans notre documentation ; l'objet est donc déclaré non lu (flipchart.cpp, D9).
inline constexpr quint16 kTopToBottomTextObject = 469;
} // namespace kind

inline constexpr bool isActionKind(quint16 k) noexcept
{
    return k == kind::kToolAction || k == kind::kGoToPageAction || k == kind::kShiftAction ||
           k == kind::kProgramAction || k == kind::kOpenFileAction || k == kind::kObjectAction ||
           k == kind::kTextAction || k == kind::kLaunchAction;
}

// Paperboard (§ 5).
namespace docKeys {
inline constexpr quint16 kFallbackTitle = 2;  // titre de secours, si le titre est vide
inline constexpr quint16 kResourceTable = 7;  // table des ressources (objet embarqué)
inline constexpr quint16 kTitleText = 12;     // titre
inline constexpr quint16 kStartPageSize = 18; // taille des pages qui n'en donnent pas
} // namespace docKeys

// Page (§ 6).
namespace pageKeys {
inline constexpr quint16 kPageTitle = 2;
inline constexpr quint16 kPresenterNotes = 5;
inline constexpr quint16 kSheetSize = 6;
inline constexpr quint16 kShowGuides = 17; // grille affichée
} // namespace pageKeys

// Fond de page (§ 6).
namespace fillKeys {
inline constexpr quint16 kFillSource = 2; // 0 encre, 1 image, 2 ressource
inline constexpr quint16 kFillStyle = 3;  // encre embarquée : couleur ou dégradé
inline constexpr quint16 kPictureRef = 4; // pointeur vers l'objet image
} // namespace fillKeys

// Grille de la page.
namespace guideKeys {
inline constexpr quint16 kPreviewZoom = 13;   // échelle dans les miniatures
inline constexpr quint16 kBaseZoom = 14;      // échelle de départ
inline constexpr quint16 kZoomIncrement = 15; // pas d'échelle
inline constexpr quint16 kSnapEnabled = 16;   // aimantation permise
inline constexpr quint16 kDrawnAbove = 17;    // dessinée par-dessus les objets
} // namespace guideKeys

// Niveau de grille.
namespace guideLevelKeys {
inline constexpr quint16 kMarkStyle = 2;   // tracé : indice + 1 dans core::GridLine
inline constexpr quint16 kLayoutStyle = 3; // disposition : indice + 1 dans core::GridPattern
inline constexpr quint16 kTint = 4;        // couleur
inline constexpr quint16 kShown = 5;
inline constexpr quint16 kAnchor = 6; // point de départ
inline constexpr quint16 kSpacingX = 7;
inline constexpr quint16 kSpacingY = 8;
inline constexpr quint16 kSlant = 9; // angle de la disposition oblique
inline constexpr quint16 kLinesAlongX = 11;
inline constexpr quint16 kLinesAlongY = 12;
inline constexpr quint16 kMagnetic = 13; // aimante les objets
} // namespace guideLevelKeys

// Propriétés communes aux objets affichables (§ 7.1).
namespace itemKeys {
inline constexpr quint16 kPlacement = 2;   // transformation (objet embarqué)
inline constexpr quint16 kItemTitle = 4;   // nom de l'objet
inline constexpr quint16 kShown = 6;       // visible
inline constexpr quint16 kLocalBox = 7;    // rectangle englobant local
inline constexpr quint16 kPlane = 8;       // couche : 0 inférieure, 1 intermédiaire, 2 supérieure
inline constexpr quint16 kStackOrder = 9;  // profondeur dans la couche
inline constexpr quint16 kPenAndFill = 10; // encre (objet embarqué)
inline constexpr quint16 kPivot = 12;      // origine de rotation (objet embarqué)
inline constexpr quint16 kMovable = 13;
inline constexpr quint16 kResizable = 14;
inline constexpr quint16 kRotatable = 15;
inline constexpr quint16 kFrozen = 18;       // verrouillé
inline constexpr quint16 kConnectorTag = 33; // étiquette d'un connecteur (sa 34 est une extrémité)
inline constexpr quint16 kTag = 34;          // étiquette des autres objets
} // namespace itemKeys

namespace strokeKeys {
inline constexpr quint16 kSeeThrough = 38; // translucidité, 0 opaque à 255 invisible
inline constexpr quint16 kPointsRef = 39;  // ressource des points
} // namespace strokeKeys

namespace shapeKeys {
inline constexpr quint16 kOutlineRef = 38;   // ressource des polygones
inline constexpr quint16 kWindingRule = 39;  // 2 : remplissage « winding »
inline constexpr quint16 kUnclosed = 42;     // contour ouvert
inline constexpr quint16 kInlinePoints = 49; // points portés par l'objet, sans ressource
} // namespace shapeKeys

namespace lineKeys {
inline constexpr quint16 kPointsRef = 38;
}

namespace imageKeys {
inline constexpr quint16 kPixelsRef = 38; // ressource de l'image
inline constexpr quint16 kKeyed = 39;     // une couleur est rendue transparente
inline constexpr quint16 kKeyColor = 40;  // cette couleur
} // namespace imageKeys

namespace textKeys {
inline constexpr quint16 kContentRef = 38;
}

namespace mediaKeys {
inline constexpr quint16 kPosterRef = 38; // image affichée à la place du média
inline constexpr quint16 kPlayOnOpen = 41;
inline constexpr quint16 kRepeat = 42;
inline constexpr quint16 kClipRef = 45; // ressource du son ou de la vidéo
} // namespace mediaKeys

namespace connectorKeys {
inline constexpr quint16 kFromItem = 34; // pointeur vers l'objet de départ
inline constexpr quint16 kToItem = 35;   // pointeur vers l'objet d'arrivée
inline constexpr quint16 kFromPoint = 39;
inline constexpr quint16 kToPoint = 40;
} // namespace connectorKeys

// Transformation embarquée (§ 7.4).
namespace placementKeys {
inline constexpr quint16 kOffset = 2;      // translation
inline constexpr quint16 kTurnDegrees = 3; // rotation, degrés, sens horaire
inline constexpr quint16 kAxisFactors = 4; // facteurs d'échelle
inline constexpr quint16 kMirrorX = 5;     // miroir horizontal
inline constexpr quint16 kMirrorY = 6;     // miroir vertical
} // namespace placementKeys

namespace pivotKeys {
inline constexpr quint16 kPivotPoint = 2;
}

// Encre embarquée (§ 7.2) : plume et remplissage.
namespace penKeys {
inline constexpr quint16 kStrokeMode = 2; // 0 stylo, 1 surligneur, 2 encre révélatrice
inline constexpr quint16 kOutlineWidth = 3;
inline constexpr quint16 kDashPattern = 4; // valeurs de Qt::PenStyle
inline constexpr quint16 kEndShape = 5;    // bouts : 0 plat, 16 carré, 32 rond
inline constexpr quint16 kCornerShape = 6; // jointures : 0 angle, 64 biseau, 128 rond
inline constexpr quint16 kOutlineColor = 7;
inline constexpr quint16 kFillPattern = 8;     // 0 aucun, 1 plein, 2 à 14 motifs, 25 dégradé
inline constexpr quint16 kFillTint = 9;        // couleur du remplissage
inline constexpr quint16 kOpaqueHatch = 10;    // fond des motifs opaque
inline constexpr quint16 kHatchBaseColor = 11; // couleur de ce fond
inline constexpr quint16 kSecondaryColor = 13; // seconde couleur du dégradé
inline constexpr quint16 kBlendDirection = 14; // 0 aucun dégradé, 1 à 5 : core::GradientKind
} // namespace penKeys

namespace tagKeys {
inline constexpr quint16 kTagText = 2;
}

// Objets ressources (fichiers .ir, § 5).
namespace resourceKeys {
inline constexpr quint16 kPointList = 4;      // trait d'encre, ligne
inline constexpr quint16 kPathOutlines = 4;   // forme
inline constexpr quint16 kSourceFileName = 3; // image, média : nom du fichier d'origine
inline constexpr quint16 kPixelSize = 5;      // image : taille en pixels
inline constexpr quint16 kLayoutRuns = 4;     // texte : lignes mises en page
inline constexpr quint16 kMarkup = 5;         // texte : HTML
} // namespace resourceKeys

// Ligne de texte mise en page (§ 7.5).
namespace runKeys {
inline constexpr quint16 kBox = 2;
inline constexpr quint16 kBaselinePoint = 3;
inline constexpr quint16 kGlyphColor = 5;
inline constexpr quint16 kCharacters = 6;
inline constexpr quint16 kTypeface = 7;
inline constexpr quint16 kPointHeight = 8;
inline constexpr quint16 kBoldness = 9; // échelle Qt 4 (50 normal, 75 gras)
inline constexpr quint16 kSlanted = 10;
inline constexpr quint16 kUnderlined = 11;
inline constexpr quint16 kStruckThrough = 12;
} // namespace runKeys

// Actions (§ 7.6) : commande et cible selon le genre.
namespace actionKeys {
inline constexpr quint16 kObjectActionTarget = 2;
inline constexpr quint16 kObjectActionCommand = 5;
inline constexpr quint16 kOpenFileResource = 2;
inline constexpr quint16 kOpenFileTarget = 4;
inline constexpr quint16 kOpenFileCommand = 10;
inline constexpr quint16 kGoToPageCommand = 4;
inline constexpr quint16 kToolCommand = 3;
inline constexpr quint16 kShiftCommand = 3;
inline constexpr quint16 kProgramCommand = 4;
inline constexpr quint16 kTextActionTarget = 3;
inline constexpr quint16 kTextActionCommand = 6;
inline constexpr quint16 kLaunchCommand = 4;
} // namespace actionKeys

} // namespace actcraft::formats::flipchart::ids
