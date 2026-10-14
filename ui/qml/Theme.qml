pragma Singleton
import QtQuick

// Jetons de l'interface (décision 23 du cadrage livraison-complete) : couleurs, espacements, tailles,
// typographie. Valeurs posées par la direction visuelle (docs/design/2026-10-14-direction-visuelle.md) ; les
// noms sont figés : les composants ne lisent jamais une couleur ou une taille ailleurs qu'ici.
QtObject {
    // Couleurs
    readonly property color fond: "#4A6A8A"           // autour de la page (F-020 : fond bleu par défaut)
    readonly property color barre: "#20242B"          // ligne supérieure
    readonly property color palette: "#2B2F36"        // palette d'outils (repère de l'essai Android, S3)
    readonly property color surface: "#363B44"        // bouton au repos
    readonly property color surfaceSurvol: "#424955"  // bouton survolé
    readonly property color surfaceActive: "#4E7FD1"  // bouton coché, outil actif, préréglage choisi
    readonly property color texte: "#E8EAED"
    readonly property color texteDiscret: "#A9B0BA"
    readonly property color separateur: "#3F444D"
    readonly property color cadreSelection: "#FFFFFF" // pastille de la couleur courante
    readonly property color page: "#FFFFFF"

    // Espacements et tailles, pixels logiques
    readonly property int espace1: 4
    readonly property int espace2: 8
    readonly property int espace3: 12
    readonly property int espace4: 16
    readonly property int rayon: 6
    readonly property int hauteurBarre: 40
    readonly property int largeurPalette: 112
    readonly property int tailleBouton: 44      // cible tactile
    readonly property int tailleIcone: 24
    readonly property int taillePastille: 20

    // Typographie : police sans empattement du système (N2-3)
    readonly property int taillePolice: 14
    readonly property int taillePoliceTitre: 16
}
