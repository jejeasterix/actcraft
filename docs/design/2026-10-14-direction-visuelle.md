# Direction visuelle d'ActCraft — 2026-10-14, révisée le 2026-10-15

Décision 23 et hypothèse N1-13 du cadrage `docs/specs/2026-10-14-livraison-complete-cadrage.md`. Disposition de
la référence (menus en haut, palette à droite, page au centre, pas de barre d'état) ; dessin entièrement à nous.

## Comment la direction a été posée

- **L1-T7 (2026-10-14)** a écrit le thème `Theme.qml`, les dix icônes et ce document avec les valeurs de départ du
  plan, **sans invoquer le skill `impeccable`** ; les contrastes n'avaient pas été mesurés.
- **L1-C5 (2026-10-15)** a invoqué le skill `impeccable` (version 4.5.0) en mode **critique + audit** sur
  l'interface réelle : `ui/qml/*.qml`, `Theme.qml`, `ui/icons/*.svg` et des captures hors écran (scénario
  `premiers-pas`, menus, dialogue « À propos »). Deux évaluations isolées, comme le skill l'exige :
  - **A, revue de design** (heuristiques de Nielsen, charge cognitive, personas) : 21/36, « acceptable » ;
  - **B, détecteur et audit technique natif** : 11/20, « acceptable ». Le détecteur du skill rend 0 constat,
    parce que ses règles visent le web et ne lisent pas le QML (essai témoin fait) : ce zéro ne prouve rien.
- Les recommandations (pensées pour le web) ont été traduites en jetons `Theme` et en QML, puis chaque paire de
  couleurs a été **mesurée par calcul** (WCAG 2.1) ; `tests/tst_ui_theme.cpp` refait ce calcul à chaque essai.

## Intention

Un tableau blanc de classe, vu de loin et touché du doigt : la page blanche domine, l'interface s'efface autour
d'elle. Barre et palette sombres et sobres, fond bleu calme autour de la page. **Une seule couleur d'accent, claire,
portant un texte sombre** : elle marque ce qui est coché, appuyé, ouvert, le focus clavier, le lien et la partie
parcourue de la réglette ; on la voit du fond de la classe. Les contrôles que nous ne dessinons pas (menus, zoom,
dialogue, info-bulles) prennent la même palette. Aucune ombre calculée, aucun flou, aucune transparence empilée
(règle de fluidité 4).

## Jetons (ui/qml/Theme.qml)

| Jeton | Valeur | Rôle | Pourquoi |
|---|---|---|---|
| `fond` | `#4A6A8A` | autour de la page | inchangé ; page à 5,65:1 contre lui |
| `barre` | `#20242B` | ligne supérieure, info-bulles, pouce des barres de défilement | inchangé |
| `palette` | `#2B2F36` | palette, menus, dialogues | inchangé (repère de `scripts/essai-android.ps1`) |
| `surface` | `#363B44` | bouton au repos, zoom | inchangé |
| `surfaceSurvol` | `#424955` | survol, entrée de menu surlignée, rainure de la réglette | inchangé |
| `surfaceActive` | `#8DB3F0` (était `#4E7FD1`) | accent | l'ancien bleu portait le texte clair à 3,30:1 ; aucun bleu ne tient à la fois 4,5:1 sous un texte clair et 3:1 contre la palette : l'accent devient clair et porte un texte sombre |
| `texte` | `#E8EAED` | libellés et icônes | inchangé |
| `texteDiscret` | `#B4BBC5` (était `#A9B0BA`) | raccourcis des menus | 4,15:1 seulement sur une entrée surlignée |
| `texteSurActive` | `#20242B` (nouveau) | libellé ou icône sur l'accent | 7,30:1 |
| `texteDesactive` | `#6B7380` (nouveau) | commande indisponible | nettement plus terne que `texte` (2,35:1 ; les contrôles inactifs sont exemptés du seuil) |
| `contour` | `#858D98` (nouveau) | bord des pastilles, flèche du zoom, bord des menus, filet de l'indicateur de largeur | les pastilles sombres (noir, `#424242`) tombaient à 1,34-1,56:1 contre la palette |
| `separateur` | `#3F444D` | séparation des groupes | décor, sans seuil |
| `cadreSelection`, `page` | `#FFFFFF` | pastille choisie ; page et puits de l'échantillon | inchangés |
| `hauteurBarre` | 44 (était 40) | barre | la hauteur d'une cible tactile : le bouton plein écran (44) et le zoom débordaient. La critique proposait 52 (deux marges de 4) : refusé, la page à 100 % sortait alors de la vue en 1280 × 720 avec les témoins du banc (`tst_bench_verdict`, `benchTargetsAreVisibleInTheShell`) |
| `tailleBouton`, `tailleIcone`, `taillePastille` | 44, 24, 20 | cibles, icônes, pastilles | inchangés ; les préréglages d'épaisseur et la poignée de la réglette passent à `taillePastille` |
| `largeurPalette`, `espace1…4`, `rayon`, polices | 112 ; 4, 8, 12, 16 ; 6 ; 14 et 16 | gabarit | inchangés |

## Règles d'état (QML)

- **Bouton de commande** : appuyé ou coché → fond `surfaceActive`, icône `texteSurActive` ; survolé →
  `surfaceSurvol` ; indisponible → icône `texteDesactive`. L'appui se voit sans survol (doigt).
- **Menus de la barre** : ouvert ou appuyé → accent et texte sombre ; focus clavier (Tab) → filet de 2 px
  `surfaceActive`.
- **Menus surgissants** : fond `palette`, texte `texte`, raccourcis `texteDiscret` (plus d'opacité), entrée
  indisponible `texteDesactive`, hauteur 44, colonne de la coche réservée (la coche recouvrait « Stylo »).
- **Pastilles et préréglages** : bord `contour` ; choisie → cadre blanc de 2 px doublé d'un filet `palette`, visible
  aussi sur la pastille blanche ; appui → réduction à 90 % ; zone de toucher étendue au pas de la grille (24 px).
- **Réglette** : rainure `surfaceSurvol`, partie parcourue `surfaceActive`, poignée `texte` (accent quand appuyée).
- **Échantillon d'épaisseur** : posé sur un puits couleur de page ; l'encre noire par défaut y est lisible.
- **Barres de défilement** : pouce `barre` cerclé de `texte`, lisible sur la page comme sur le fond.
- **Indicateur de largeur sous le pointeur** : cercle de la couleur de l'encre et filet extérieur `contour`.
- **Fenêtre** : `palette` de la `Window` (`Main.qml`) tirée de `Theme` pour tous les contrôles du style Basic.
- **« À propos »** : lien en texte stylé (couleur `surfaceActive`) ; notices sur un puits couleur de page en texte
  `barre` (le Markdown colore ses liens d'après la palette de l'application, lisible sur fond clair).

## Icônes (ui/icons/)

Grille de 24 px, trait de 2, extrémités et jonctions rondes, formes géométriques simples, une seule couleur.
Dessinées pour ActCraft ; aucune ne reprend celles d'ActivInspire ni celles du clone Python, qui les imitaient.
Depuis L1-C5, la couleur affichée vient de `Theme` (teinte selon l'état) ; la couleur écrite dans le fichier ne sert
plus. Aucune icône n'a été jugée illisible : aucune n'a été redessinée.

## Contrastes mesurés (WCAG 2.1 AA : texte 4,5:1, icône et indicateur d'état 3:1)

| Paire rendue | Avant | Après |
|---|---|---|
| Libellé de menu ouvert sur l'accent | 3,30 (`#E8EAED` / `#4E7FD1`) | 7,30 (`#20242B` / `#8DB3F0`) |
| Icône cochée ou appuyée sur l'accent | 3,30 | 7,30 |
| Accent contre la palette / la barre / `surface` / `surfaceSurvol` | 3,38 / 3,92 / 2,83 / 2,28 | 6,30 / 7,30 / 5,27 / 4,25 |
| Raccourci de menu | 4,07 (opacité 0,6 sur blanc) | 6,95 sur la palette, 4,69 surligné |
| Entrée de menu surlignée (clavier) contre le menu | 1,08 (`#F6F6F6` / blanc) | 1,48, texte à 7,52 |
| Réglette : partie parcourue contre la palette | 1,11 | 6,30 (4,25 contre la rainure) |
| Échantillon noir (couleur par défaut) | 1,56 sur la palette | 21 sur le puits blanc |
| Pastilles sombres contre la palette | 1,34 à 1,77 | bord `contour` à 4,01 |
| Cadre de la pastille blanche choisie | 1,00 | cadre blanc + filet `palette` à 13,44 |
| Pouce de défilement sur la page / sur le fond | 1,57 / 2,34 | 15,57 / bord clair à 4,68 |
| Indicateur de largeur, encre blanche sur la page | 1,00 | filet `contour` à 3,35 |
| Lien de « À propos » | 8,59 (bleu sur dialogue clair) | 6,30 (accent sur dialogue sombre) |
| Texte sur barre / palette / `surface` / `surfaceSurvol` | 12,92 / 11,15 / 9,34 / 7,52 | inchangés |
| Flèche du zoom sur `surface` | 9,17 (style clair) | 3,36 |
| Notices de « À propos » | 12,11 | 15,57 (`#20242B` sur le puits blanc) |

`tst_ui_theme` (`themePairsMeetWcagAa`) vérifie 20 paires du thème et la formule elle-même ; il a été vu rouge sur
le thème de L1-T7 (accent contre `surface`, `texteDiscret` surligné, jetons absents) et sur une paire rendue mauvaise
exprès.

## Cibles tactiles

Sur un TNI de 65 à 86 pouces en 1080p, 1 px logique vaut 0,8 à 0,9 mm : 20 px font 17 mm, assez pour un doigt.
Sur tablette Android, 1 px logique vaut 1 dp (0,16 mm) : les pastilles et préréglages (20 dp), la réglette et les
barres de défilement sont trop petits (48 dp attendus). L1-C5 porte la barre, les menus et la réglette à 44 et
étend la zone de toucher des pastilles au pas de la grille (24, minimum de WCAG 2.2) ; atteindre 44-48 dp sur la
palette demande une autre mise en page (deux colonnes ou palette élargie), prévue au lot 11 (« mise en page étroite,
palette repliable »).

## Écarté

- **Info-bulle à l'appui long** (critique A et B) : au doigt, les icônes n'ont aucun libellé. Écarté ici : brancher
  `pressAndHold` empêche le clic après un appui long, changement de comportement à décider et à tester.
- **Taille de texte du système** (B) : les tailles restent en pixels logiques fixes (`taillePolice`).
- **Palette ancrée en bas, icône « Outils » ambiguë** (A) : refonte de disposition ou nouvelle icône, hors de
  cette correction (voir ci-dessous).
- **Confirmation avant de quitter, F1 vers un site souvent filtré en classe** (A) : comportement, pas design.
- **Persistance du rapport de critique** dans `.impeccable/critique/` : hors des fichiers de la tâche ; les constats
  sont dans ce document.

## À signaler à Jérôme (porte de fin du lot 1)

1. **Accent clair** `#8DB3F0` avec texte et icône sombres sur l'outil actif et le menu ouvert : choix fait pour le
   contraste et la lecture à distance ; à confirmer à l'œil sur un TNI.
2. **Barre de 44 px** au lieu de 40, sans marge autour des boutons : la page perd 4 px de hauteur. Plus d'air
   (52 px) demanderait de revoir la position des témoins du banc en 1280 × 720.
3. **Cibles sur tablette** : 44-48 dp sur la palette imposent deux colonnes de pastilles ou une palette plus large ;
   à trancher pour le lot 11.
4. **Libellés au doigt** : sans survol, aucune info-bulle ; un appui long qui l'affiche changerait le clic.
5. **Atteinte au TNI** (critique A) : sur un grand tableau, le haut de la palette est hors de portée d'un élève ;
   la moitié basse est vide. Ancrer le contenu en bas serait une décision de disposition.
6. **Icône « Outils »** (quatre carrés) lue comme une grille d'applications ; garder ou redessiner.
7. Les enseignants habitués aux icônes d'origine devront apprendre celles-ci (N1-13). Captures : scénario
   `premiers-pas` (`scripts/captures.ps1`).
