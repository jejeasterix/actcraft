# Lot 3 — principes confrontés (vidéos, ancien dépôt, binaire)

Relevé brut (hors dépôt) : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot3.md`. Références `Vnn` :
numéros des fiches du corpus (table hors dépôt). Constats écrits par nous. Tâche L3-T0, scénario S30.

## Méthode et limites

- **Corpus.** 144 fiches cherchées par mots-clés (sélection, poignée, rotation, translucidité, marqueur, stylo,
  gomme, effacer, annuler, rétablir, corbeille, supprimer, curseur, clic droit, doigt, tactile, pincer, zoom,
  verrouillé, deux utilisateurs) : 52 retenues. Les fiches sont tirées de sous-titres, sur des versions anciennes
  de l'original : elles décrivent des gestes, presque jamais des valeurs. Rien sur le toucher, le lissage, les
  réglages par défaut ni les raccourcis (sauf Suppr).
- **Ancien dépôt.** Outils, sélection, encre, document et fenêtre de l'application de référence en Python ;
  plan comparatif (lignes 7 à 9, 14 à 16, captures mesurées) ; relevés d'interface et de commandes ; spécification
  (exigences F-xxx) ; son graphe (huit requêtes, pistes faibles, complétées par lecture directe).
- **Binaire.** En statique seulement : réglages embarqués décompressés (valeurs par défaut, bornes), chaînes et
  symboles. Le serveur d'analyse habituel n'a pas répondu ; aucun désassemblage, aucune observation à l'écran. Ce
  qui l'exigerait est marqué « non tranché ».
- **Légende.** « observé » : lu dans le code, mesuré sur une capture, lu dans un réglage ou vu dans une fiche ;
  « supposé » : déduit.

## Constats

| # | Principe (nos mots) | Source | Exigence pour ActCraft | Tâche | Scénario | Écart, raison ou lot destinataire |
|---|---|---|---|---|---|---|
| P1 | La gomme n'agit que sur les traits de stylo et de marqueur ; un trait gommé posé sur un mot révèle le mot intact. | V72, V120 ; F-230 ; ancien dépôt (gomme) ; lot 10 P9 | `core::isAnnotation` seul critère ; formes, textes et images jamais touchés | L3-T3, L3-T7 | S6 | — |
| P2 | La gomme retire la partie de la ligne centrale du trait intérieure au disque ; les morceaux deviennent des objets distincts ; les bouts arrondis débordent dans le trou ; le passage est interpolé (pas de max(2, épaisseur/3)). | ancien dépôt (gomme, plan comparatif ligne 9) ; lot 10 P9 | `cutByDisk`, `cutBySweep` sur la ligne centrale ; un disque qui n'effleure que le bord épais n'efface rien | L3-T3, L3-T7 | S6 | constat 1 du plan confirmé |
| P3 | Un trait verrouillé n'est pas gommé. | ancien dépôt (gomme) | la gomme saute les objets verrouillés | L3-T7 | S6 | ajout (D4) |
| P4 | L'épaisseur de la gomme se règle comme celle du stylo ; 50 par défaut, deux préréglages (20 et 50). | V72 ; ancien dépôt (outils, relevé d'interface) | défaut 50 | L3-T6 | S6 | **corrige** la gomme de 20 supposée (D4, D5, H3-9) |
| P5 | Repasser le marqueur au même endroit fonce de plus en plus. C'est le constat « Stylo, formes, couleurs » du § 4 de l'amendement affecté au lot 3. | V72 ; lot 10 P5 | deux traits de marqueur superposés foncent | L3-T11 | S37 | — (D3, D12) |
| P6 | Le marqueur est translucide : opacité 128 sur 255 par défaut, réglable de 20 (transparent) à 255 (plein) ; un même trait ne fonce pas sur lui-même. | binaire (réglage du marqueur) ; F-225 ; ancien dépôt (un seul tracé par trait) ; lot 10 P5 | translucidité 127 dans notre convention (0 opaque … 255 invisible) ; un trait dessiné d'un seul tenant | L3-T6, L3-T11 | S37 | auto-recoupement non observé sur l'original (D3) |
| P7 | Le trait passe par les milieux des points successifs ; bouts et jonctions ronds, pour le stylo comme pour le marqueur, et pour les traits lus d'un fichier. | ancien dépôt (encre, import) ; lot 10 P4 | même forme pour un trait dessiné et un trait lu | L3-T11 | S37 | H3-15 confirmé (D2) |
| P8 | Le menu Effacer propose annotations, objets, grille, fond, page, puis Rétablir la page ; un effacement s'annule en un pas. | V16, V67, V71, V109 ; F-130 ; ancien dépôt (menu, relevé d'interface) ; lot 10 P11, P12 | ordre de `clear.*` puis `page.revert` ; une étape par effacement | L3-T4, L3-T12 | S35 | ordre des données déjà conforme |
| P9 | « Objets » retire images et formes et garde les traits ; « Annotations » retire les traits ; les objets verrouillés sont épargnés. | V109, V71 ; ancien dépôt (document) | `ClearKind::Objects` : tout ce qui n'est pas une annotation, toutes couches ; verrouillés gardés | L3-T4 | S35 | D8 ; H3-13 précisé |
| P10 | « Grille » retire la grille et garde les lignes tracées ; « Fond » retire le fond seul, pas les objets de la couche du fond. | V16, V67, V68 ; ancien dépôt (fenêtre) | `ClearKind::Grid` annulable ; `ClearKind::Background` : fond seul | L3-T4 | S35 | l'ancien dépôt n'annule pas la grille : F-130 l'emporte (D8) |
| P11 | Rétablir la page rend l'état enregistré ; un paperboard jamais enregistré redevient une page blanche. | V41, V109 ; F-131 ; lot 10 P12 | `page.revert` sur l'état de la page au dernier enregistrement | L3-T4, L3-T12 | S35 | D9 |
| P12 | L'original fait de Rétablir la page une étape d'annulation propre à la page ; le document reste modifié ensuite. | binaire (symboles d'annulation de la page) ; ancien dépôt (capture a6 mesurée) | une étape ; marque « modifié » gardée | L3-T4, L3-T12 | S35 | **corrige** le plan comparatif ligne 16 (étape a6) et l'ancien dépôt, qui remonte toute la pile (D9) |
| P13 | Annuler défait une action par pas ; la sélection n'est pas une étape ; Annuler et Rétablir vident la sélection ; Rétablir est grisé après une modification. | V109 ; ancien dépôt (pile, captures du plan comparatif lignes 8 et 16) ; lot 10 P11 | décompte de D10 | L3-T4, L3-T12, L3-T14 | S6 | H3-5 confirmé (D10) |
| P14 | Suppr supprime la sélection ; sans sélection, rien (la page reste). | V109 ; F-541 | `edit.delete` sans effet sans sélection | L3-T12, L3-T14 | S35 | — |
| P15 | Corbeille : sélection puis clic dessus, ou glisser l'objet dessus (flèche au survol) ; elle se montre ou se masque depuis Affichage et depuis le profil. | V71, V106, V25 ; F-050 | `view.trash`, dépôt sur la corbeille, clic avec sélection | L3-T8, L3-T16 | S35 | l'ancien dépôt n'a que le glisser : F-050 et le corpus l'emportent ; case du profil : lot 6 |
| P16 | Jeter un outil de maths ou une icône d'action dans la corbeille le ferme ou le retire. | V34, V35, V78 | — | — | — | lots 7 (outils de maths) et 8 (actions) |
| P17 | Les poignées n'apparaissent qu'avec une sélection ; huit poignées, coins et milieux. | V82 ; ancien dépôt (sélection) ; F-520 | cadre et poignées seulement si sélection | L3-T9, L3-T15 | S34 | constat 2 du plan confirmé |
| P18 | La barre d'objet suit un ordre fixe (déplacer librement, rotation, menu, translucidité, regroupement, avancer, reculer, dupliquer, agrandir, réduire, points de forme), au-dessus de l'objet ; elle pivote avec lui ; masquée pendant un glissement. | V122, V30, V137 ; F-521, F-528 ; ancien dépôt (vue, plan comparatif lignes 14-15) | `objectBar` : sous-ensemble du lot dans cet ordre | L3-T9, L3-T15 | S34 | ordre des données déjà conforme (D7) |
| P19 | La rangée se personnalise (commandes ajoutées, liste vide = aucune icône) et se garde dans le profil. | V30, V36, V136, V137 | — | — | — | lot 6 (profils) |
| P20 | La rotation se fait en glissant le bouton de rotation de la barre, autour du centre ; aucune poignée de rotation sur le cadre ; Maj donne des pas de 15°. | V82, V122, V36 ; ancien dépôt (outil de sélection) | `object.rotate` par la barre seulement | L3-T9 | S34 | D7 |
| P21 | Avec l'origine XY active, la rotation se fait autour d'elle. | V36, V134 | — | — | — | lot 7 (origine) |
| P22 | Agrandir et réduire par la barre, facteur 1,1. | V82 ; ancien dépôt (vue) ; F-523 | `kEnlargeStep` | L3-T9 | S34 | réduire = ÷ 1,1 (− 9,09 %), pas − 10 % |
| P23 | La réglette de translucidité rend l'objet progressivement transparent et révèle ce qui est dessous ; une étape d'annulation à la fermeture de la réglette. | V48, V76, V82, V122 ; F-524 ; ancien dépôt (vue) ; lot 10 P17 | une `SetTranslucencyCommand` par ouverture de la réglette | L3-T4, L3-T15 | S34 | D10 |
| P24 | Le rectangle de sélection attrape tout objet qu'il touche, plusieurs objets superposés compris ; trop grand, il attrape un voisin. | V42, V48, V82 ; F-210 ; ancien dépôt (emprise, épaisseur comprise) | `objectsTouching` par l'emprise | L3-T3, L3-T8 | S34 | H3-4 confirmé (D6) |
| P25 | Sélectionner un objet d'action par le rectangle ne déclenche pas l'action. | V42 | — | — | — | lot 8 (actions) |
| P26 | Maj+clic ou Ctrl+clic ajoute ou retire un objet de la sélection ; avec l'une de ces touches, le rectangle ajoute sans vider. | ancien dépôt (outil de sélection) | bascule par Maj ou Ctrl | L3-T8 | S34 | D6 ; original non relevé |
| P27 | Un objet saisi sans être sélectionné se déplace, puis reste non sélectionné ; le glissement commence au-delà de 2 px d'écran ; un stylet qui lâche pendant un glissement le termine. | ancien dépôt (outil de sélection) ; V100 | seuil 2 px ; relâché = dépôt | L3-T8 | S34 | **corrige** le seuil de 3 px (H3-14) |
| P28 | Un objet verrouillé ne se déplace ni ne pivote ; le mode Création le libère. | V114, V103 ; F-210 | `lockedObjectDoesNotMove` | L3-T8, L3-T9 | S34 | mode Création : lot 6 ou 8 |
| P29 | Un clic sur un objet verrouillé le traverse jusqu'à l'objet dessous. | V28 | — | — | — | constat « Sélection, verrouillage, masquage » du § 4 : lots 5, 7, 8 |
| P30 | Restrictions lues du fichier : déplacer, dimensionner, pivoter ; « peut déplacer » peut contraindre à un axe ou un chemin. | V18, V114, V136 ; F-579 ; ancien dépôt (seule l'interdiction de dimensionner appliquée) | les trois interdictions respectées | L3-T9 | S34 | **corrige** le constat 2 du plan pour l'ancien dépôt ; axe et chemin : lot 8 |
| P31 | Le curseur du stylo se dessine au-dessus (défaut), en dessous ou pas du tout. | V50 ; F-201 ; binaire (réglage, défaut « au-dessus ») ; lot 10 P17 | curseur au-dessus, jamais dans le cache | L3-T16 | S37 | réglage : lot 6 |
| P32 | Le curseur du marqueur est un cercle. | F-201 ; ancien dépôt (carré) | cercle | L3-T16 | S37 | l'ancien dépôt s'écarte de la spécification ; correctif m5 du plan gardé |
| P33 | Le clic droit sur la page dépend de l'outil : stylo → convertir en texte, sélectionner ; sélection → stylo ; toujours coller, fond, grille. | V49, V48, V68, V119 ; F-202 | `contextMenu` : `tool.select`, `tool.pen` | L3-T16 | — | convertir : lot 7 ; coller : lot 5 ; fond, grille : lot 4 |
| P34 | La poignée « menu » de la barre ouvre le menu du clic droit sur l'objet. | V82, V122 | — | — | — | lot 5 (F-770) |
| P35 | Chaque outil garde sa couleur et son épaisseur ; au redémarrage, l'original les reprend du profil. | ancien dépôt (outils, en session seulement) ; plan comparatif ; F-136 | mémoire par outil gardée au redémarrage | L3-T6, L3-T14 | S37 | D5 ; H3-7 |
| P36 | Choisir une couleur quand Sélectionner ou la gomme est actif ramène au dernier outil de dessin. | ancien dépôt (boîte à outils) | `colorGoesToActiveTool` : la couleur va au dernier outil d'encre, qui redevient actif | L3-T14 | S37 | ajout |
| P37 | L'outil de départ est le stylo. | binaire (réglage de l'outil par défaut) ; ancien dépôt | `UserState::currentTool()` = `pen` | L3-T6 | — | — |
| P38 | Un double-clic sur un texte l'ouvre en édition. | V56, V96 | — | — | — | outil texte (hors lot 3) |
| P39 | Deux utilisateurs : une boîte à outils double, chacune avec son ancrage et ses couleurs ; le second utilisateur est un second stylet ou le toucher. | V25 ; F-303 ; binaire (utilisateur double, canal d'entrée) | — | — | — | D1 ; lot 11 |
| P40 | Toucher : cinq façons de traiter le doigt (ignoré, à la place du stylet, comme sélection, toucher complet, périphérique du système, ce dernier par défaut) ; outil du doigt par défaut : sélection ; zoom et défilement au toucher **désactivés** par défaut ; gestes de tourne-page actifs. | binaire (réglages du toucher) ; F-930 | — | L3-T5, L3-T13 | S6 | D12 ; modes et réglages : lot 11 |
| P41 | Un réglage regroupe les annotations successives ; le binaire le met **actif** par défaut. | binaire (réglage des objets) ; lot 10 P10 | — | — | — | lot 6 (H3-8) ; contredit « inactif par défaut » de la spécification relevé au lot 10 |

## Décisions

| # | Question | Décision | Source | Tâche amendée |
|---|---|---|---|---|
| D1 | Schéma d'entrée du second utilisateur | Sur l'original : le second utilisateur est le **second stylet** d'un tableau à deux stylets, ou **le toucher** quand le réglage le dit ; un clic de souris sur la boîte à outils du second utilisateur lui revient ; chaque utilisateur a sa sélection et sa boîte à outils (réduite pour le second : ni annulation ni Effacer). Pour ActCraft (deux stylets, ou souris et stylet, ou toucher multipoint) : **non tranché** → hypothèse de niveau 1 (H3-1), lot 11 ; le lot 3 n'a qu'un utilisateur. | V25 ; F-303 ; binaire (utilisateur double, canal d'entrée, stylets du maître et de l'élève, élève au toucher, souris sur la boîte de l'élève, une sélection par utilisateur) | — (lot 11) |
| D2 | Forme du trait ; traits lus d'un fichier | Bouts et jonctions ronds, courbe par les milieux, stylo et marqueur, **traits lus compris** ; les bouts et jonctions écrits dans le fichier sont ignorés (tous ronds dans les fichiers relevés). | ancien dépôt `canvas/items.py:257-293`, `io/import_flipchart.py:80-89`, `io/flipchart_reader.py:73-74` ; 37 annotations relevées | L3-T11 (H3-15 confirmé) |
| D3 | Marqueur | Opacité 128 sur 255 par défaut, soit **`translucency = 127`** dans `InkSettings` (0 opaque … 255 invisible) ; bornes de l'original : opacité 20 à 255. Un trait ne fonce pas sur lui-même (non observé sur l'original, gardé) ; deux passages foncent. | binaire (réglage du marqueur : 128, 20 à 255) ; F-225 ; V72 ; ancien dépôt `canvas/items.py:291`, `332-337` | L3-T6, L3-T11 |
| D4 | Gomme | Épaisseur **50** ; coupe sur la ligne centrale, interpolation au pas max(2, épaisseur / 3) ; n'agit que sur `isAnnotation` (couche du dessus, encre révélatrice comprise, contrat figé) et **saute les objets verrouillés** ; une annotation déplacée hors de la couche du dessus est intacte. | ancien dépôt `canvas/tools.py:44`, `626-740` ; `docs/re/architecture.md:68` ; binaire (épaisseurs par défaut : stylo 4, marqueur 20, gomme 50) ; F-230 ; V72, V120 | L3-T6 (défaut), L3-T7 (verrouillés) |
| D5 | Réglages par défaut et mémoire | Stylo noir 4 ; marqueur #FFFF00, 20, translucidité 127 ; gomme **50** (pas 20) ; outil de départ : stylo ; mémoire par outil gardée au redémarrage. | ancien dépôt `canvas/tools.py:37-59` ; `docs/re/interface.md:319` ; binaire (outil par défaut, marqueur) ; F-136 | L3-T6 ; H3-9 corrigée |
| D6 | Sélection | Rectangle : **emprise** (épaisseur comprise) qui touche le rectangle, bords compris ; désignation : demi-épaisseur + `kHitToleranceScreen` (6 px d'écran, inchangé) ; **Maj+clic ou Ctrl+clic** bascule un objet ; rectangle avec l'une de ces touches : ajoute sans vider ; glissement au-delà de **2 px** d'écran. | ancien dépôt `canvas/tools.py:190-207`, `259`, `331-346` ; F-210 ; V82 | L3-T8 ; H3-4 confirmée, H3-14 corrigée (2 px) |
| D7 | Barre d'objet et rotation | Ordre de F-521 (données du lot déjà conformes) ; centrée au-dessus du cadre, dessous faute de place, en colonne à droite si le cadre est plus haut que large ; pivote avec le cadre ; sélection multiple : cadre réunion non pivoté, rotation autour du centre commun ; **rotation par le bouton de la barre seulement**, aucune poignée de rotation ; Maj : pas de 15°. Dimensions de la barre : constantes figées gardées. | V122, V30, V82 ; F-521, F-528 ; ancien dépôt `canvas/view.py:92-144`, `616-655`, `canvas/selection.py:113-123`, `canvas/tools.py:213-290` | L3-T9, L3-T15 |
| D8 | Effacer ▸ Objets, Fond | Objets : tout ce qui n'est pas une annotation, **toutes couches**, couche du fond comprise, **sauf les verrouillés** ; Fond : le fond seul (couleur, dégradé, image), les objets de la couche du fond restent ; Grille : annulable (une étape) ; Page : annotations et objets non verrouillés, fond et grille gardés. | V109, V71, V16, V67, V68 ; F-130 ; ancien dépôt `canvas/document.py:132-138`, `ui/main_window.py:768-782` | L3-T4 ; H3-13 précisée |
| D9 | Rétablir la page | **Une étape** d'annulation, propre à la page ; état de référence : la page au dernier enregistrement (pour un fichier ouvert et jamais réenregistré : à l'ouverture) ; jamais enregistré : page vide ; le document reste marqué modifié. | V41, V109 ; F-131 ; binaire (étape d'annulation de remise de la page, dossier de remise par page) ; ancien dépôt capture a6 (`docs/tests/runs/annuler/`) | L3-T4, L3-T12 ; H3-11 confirmée |
| D10 | Décompte des étapes | Trait 1 ; glissement (déplacer, redimensionner, pivoter) 1, seulement au-delà du seuil ; passage de gomme 1, seulement s'il a coupé ; translucidité 1 par ouverture de la réglette ; effacement 1 ; Rétablir la page 1 ; sélection 0 ; Annuler et Rétablir vident la sélection. S6 : **deux** Annuler rendent les deux traits. | ancien dépôt `canvas/document.py:34`, `canvas/tools.py:347-356`, `748-777`, `canvas/view.py:1128-1146`, `ui/main_window.py:1019-1024` ; V109 ; plan comparatif (états a1 à a5 identiques) | L3-T4, L3-T14, L3-T18 inchangée ; H3-5 confirmée |
| D11 | Raccourcis | `edit.redo` **Ctrl+Y** (Ctrl+Maj+Z est le zoom de page de l'original aussi : aucun conflit) ; `tool.select` Échap, `tool.pen` Ctrl+Maj+P, `tool.highlighter` Ctrl+H, `tool.eraser` Ctrl+E, `edit.undo` Ctrl+Z, `edit.delete` Suppr, `edit.select-all` Ctrl+A, `object.reduce` Ctrl+- : conformes ; `object.enlarge` : **Ctrl+=** (pas Ctrl++) ; aucun raccourci pour `clear.*`, `page.revert`, `view.trash`, `object.rotate`, `object.translucency`. | `docs/re/interface.md:88`, `110-111`, `162`, `550` ; `docs/re/commands.json:393`, `1708` ; binaire (table des raccourcis, confirmée) ; F-951 à F-953 | **L3-C0** (`object.enlarge`) ; H3-12 confirmée |
| D12 | Gestes ; constat « Stylo, formes, couleurs » | Seuil du pincement : **non tranché** sur l'original (aucune valeur en statique) → `kPinchDeadZone = 0.03` gardé (H3-14) ; pincement et défilement vont ensemble (un seul réglage de l'original les couvre) ; trait au doigt en cours à l'arrivée du deuxième doigt : **non tranché** sur l'original (un relâchement forcé existe, sens supposé) → trait abandonné, rien de versé (M4 du plan), hypothèse de niveau 1 ; zoom et défilement au toucher actifs par défaut dans ActCraft (décision 6 du cadrage), alors que l'original les désactive. Constat du § 4 : **P5** (deux passages de marqueur foncent, V72). | binaire (réglages du toucher : zoom et défilement inactifs, tourne-page actif) ; V72 ; plan (M4, décision 6 du cadrage) | L3-T5, L3-T13 ; nouvelle hypothèse H3-17 |

## Point tranché par l'orchestrateur (reporté tel quel)

Rien n'est ajouté au socle pour M4 (`routeEvent`, `Sample::event`, doigt passé à `Ignore` quand le stylet appuie).
Cela relève de L3-T5, qui s'arrête et le rapporte si le mécanisme de L0-S7 ne fait pas passer les doigts à `Ignore`.

## Ce qui amende le plan (pour l'orchestrateur)

| Tâche | Ce qui change | Décision |
|---|---|---|
| L3-C0 | `object.enlarge` : raccourci Ctrl+= au lieu de Ctrl++ (`actions/data/commands.json`) ; Ctrl+= n'est pris par aucune commande | D11 |
| L3-T4 | `ClearKind::Objects` et `Page` épargnent les objets verrouillés ; `Grid` annulable ; `Background` : fond seul ; `ReplacePageCommand` : une étape, document marqué modifié | D8, D9 |
| L3-T5 | trait au doigt abandonné à l'arrivée du deuxième doigt (H3-17) ; zone morte inchangée | D12 |
| L3-T6 | `defaultSettings("eraser").width = 50` ; `defaultSettings("highlighter").translucency = 127` ; le commentaire de `tools/include/actcraft/tools/userstate.h` (« 20 », « 128 ») est à corriger par amendement de l'en-tête figé | D3, D4, D5 |
| L3-T7 | la gomme saute les objets verrouillés | D4 |
| L3-T8 | seuil de glissement 2 px d'écran ; Maj+clic ou Ctrl+clic bascule ; rectangle avec modificateur ajoute ; objet saisi sans sélection : déplacé, laissé non sélectionné ; relâché du stylet = dépôt | D6 |
| L3-T9 | rotation par le bouton de la barre seulement (Maj : 15°) ; barre au-dessus, dessous faute de place, à droite si le cadre est plus haut que large ; sélection multiple : cadre réunion non pivoté | D7 |
| L3-T11 | lissage et bouts ronds pour les traits lus aussi (H3-15) ; réglage de bouts du fichier ignoré | D2 |
| L3-T12 | Rétablir la page laisse le document modifié | D9 |
| L3-T14 | Annuler et Rétablir vident la sélection ; une couleur choisie sous Sélectionner ou la gomme va au dernier outil d'encre et le réactive | D10, P36 |
| L3-T18 | aucun changement : deux Annuler rendent les deux traits | D10 |

## Hypothèses nouvelles ou modifiées

- **H3-9** (gomme 20) → **corrigée** : 50.
- **H3-14** (seuil de glissement 3 px) → **corrigée** : 2 px ; tolérance 6 px et zone morte 3 % gardées.
- **H3-17 (niveau 1, nouvelle)** — Trait au doigt en cours à l'arrivée du deuxième doigt : abandonné (rien de versé),
  la référence ne le dit pas en statique ; à revoir avec les modes du toucher du lot 11.
- **H3-18 (niveau 2, nouvelle)** — Zoom et défilement à deux doigts actifs par défaut, contre l'original qui les
  désactive ; un réglage pour les couper viendra avec les réglages du toucher (lots 6 et 11).
