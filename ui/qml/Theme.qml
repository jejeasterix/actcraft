pragma Singleton
import QtQuick

// Jetons de l'interface (décision 23 du cadrage livraison-complete) : couleurs, espacements, tailles,
// typographie. Valeurs posées par la direction visuelle (docs/design/2026-10-14-direction-visuelle.md, critique
// du skill impeccable et contrastes mesurés, L1-C5) ; les noms sont figés : les composants ne lisent jamais une
// couleur ou une taille ailleurs qu'ici. Toute paire premier plan / fond rendue est vérifiée par tst_ui_theme.
QtObject {
    // Couleurs
    readonly property color fond: "#4A6A8A"           // autour de la page (F-020 : fond bleu par défaut)
    readonly property color barre: "#20242B"          // ligne supérieure ; fond des info-bulles
    readonly property color palette: "#2B2F36"        // palette d'outils, menus, dialogues (repère de l'essai Android, S3)
    readonly property color surface: "#363B44"        // bouton au repos, zoom, bouton du dialogue
    readonly property color surfaceSurvol: "#424955"  // survol ; entrée surlignée (avec filet d'accent) ; rainure
    readonly property color surfaceActive: "#8DB3F0"  // accent : coché, appuyé, ouvert, focus, lien, partie remplie
    readonly property color texte: "#E8EAED"          // libellés et icônes
    readonly property color texteDiscret: "#B4BBC5"   // raccourcis des menus
    readonly property color texteSurActive: "#20242B" // libellé ou icône posé sur l'accent
    readonly property color texteDesactive: "#6B7380" // libellé ou icône d'une commande indisponible
    readonly property color contour: "#858D98"        // bord d'une pastille, flèche du zoom, bord des menus
    readonly property color separateur: "#3F444D"     // séparation des groupes (décor, sans seuil)
    readonly property color separationCadre: "#B9C0C8" // séparations des cadres d'une page (décor, sans seuil)
    readonly property color cadreSelection: "#FFFFFF" // pastille de la couleur courante
    readonly property color page: "#FFFFFF"           // page ; puits de l'échantillon d'épaisseur

    // Espacements et tailles, pixels logiques
    readonly property int espace1: 4
    readonly property int espace2: 8
    readonly property int espace3: 12
    readonly property int espace4: 16
    readonly property int rayon: 6
    // Hauteur d'une cible tactile, sans marge : à 52, la page 1024 × 768 à 100 % sortait de la vue en 1280 × 720 et
    // les témoins du banc avec elle (tst_bench_verdict, benchTargetsAreVisibleInTheShell).
    readonly property int hauteurBarre: 44
    readonly property int largeurPalette: 112
    readonly property int tailleBouton: 44      // cible tactile
    readonly property int tailleIcone: 24
    readonly property int taillePastille: 20    // pastilles, préréglages d'épaisseur, poignée de la réglette

    // Typographie : police sans empattement du système (N2-3)
    readonly property int taillePolice: 14
    readonly property int taillePoliceTitre: 16
}
