# Cadrage — livraison complète, amendement 1 (toutes les fonctionnalités, vidéos à chaque lot)

Amendement du cadrage `docs/specs/2026-10-14-livraison-complete-cadrage.md`, écrit le 2026-10-17 pendant la clôture du
lot 2. Il ne réécrit pas le cadrage : il le complète et **fait autorité sur lui pour ce qu'il change**. Tout le reste
du cadrage reste en vigueur.

Sources : décision de Jérôme du 2026-10-17 (coffre `02_Decisions/0100-actcraft-toutes-les-fonctionnalites-d-activinspire-design-libre`,
fichier d'état `docs/sessions/livraison-complete-etat.md` § « Portes ») ; spécification de l'ancien dépôt
`REA - Activinspire/docs/spec/fonctionnalites.md` (exigences F-xxx) ; synthèse du corpus des vidéos
`REA - Activinspire/docs/videos-tanguay/README.md` (sections « Principes absents » et « Écarts avec la spec ») ;
architecture relevée de l'original `REA - Activinspire/docs/re/architecture.md:103` (imports présents dans le binaire).

Références aux vidéos : `Vnn` = numéro de fiche du corpus. La table numéro → vidéo reste dans le dossier du corpus,
hors dépôt (décision 12 du cadrage). Aucun titre, aucune transcription, aucune citation.

## 1. Le principe et ce qu'il change

**Principe (décision 0100, Jérôme).** ActCraft reprend **toutes les fonctionnalités d'ActivInspire 3.5.18**. Le design
est libre. Sous Windows, le **comportement et la disposition** d'ActivInspire font référence. Sous Android, l'interface
est libre (ActivInspire n'existe pas sous Android) ; l'émulateur sert quand c'est utile.

**Décision 1 du cadrage, amendée.** La référence de comportement n'est plus « ce que fait l'application Python » mais
**ActivInspire 3.5.18**, établi par la spécification, la rétro-ingénierie (skill `reverse-engineer-anything`, REA) et
le corpus des vidéos. L'application Python reste la référence là où elle reproduit déjà l'original (plan comparatif,
oracle du lecteur `.flipchart`). Bilan des 200 exigences : **189 incluses** (175 + 14 reprises), **6 exclues** (vote),
**5 en attente de la décision de Jérôme** (imports chiffrés au § 5).

**Hypothèse N1-1, amendée.** « Aucune fonction à revoir » reste vrai (réponse de Jérôme à la porte du lot 1). S'y
ajoutent : Windows = comportement et disposition de l'original, dessin à nous ; Android = disposition libre, mêmes
commandes et mêmes comportements sur la page.

**Hypothèse N1-2, remplacée.** Les quatre réductions sont levées : impression (N1-15), médias (N2-9), effets de
changement de page (N2-10), modes du toucher (décision 25). Restent propres à une plateforme : fonctions de bureau
(Windows), reconnaissance d'écriture (Windows), boutons définis par l'utilisateur (Windows), enregistreurs (Windows
d'abord).

**§ 6 du cadrage, remplacé pour les exigences.** Restent exclus :

- le vote sans boîtiers : F-126, F-127, F-1000, F-1010, F-1020, F-1030, les actions de vote du navigateur d'action, la
  section de réglages du vote, la touche de fonction du sondage rapide ;
- l'écriture des `.flipchart` ;
- les liens vers les services en ligne de Promethean (dans F-090, F-760, F-930) : remplacés par la page du projet ;
- la lecture Flash : espace réservé, comme le prévoit la spécification (`fonctionnalites.md:892`).

**En attente** (chiffrés au § 5, aucun lot tant que Jérôme n'a pas tranché) : F-661, F-662, F-663, F-665, F-814, et les
anciens `.flp` de F-655. Les autres exclusions du § 6 du cadrage (paquet macOS, `.aab`, armv7, signature, Linux, iOS,
mesures sur machines de référence, ressources de Promethean, contenu des vidéos) restent valables.

**Contraintes posées aux lots déjà cadrés** (sans tâche de plus pour eux) :

- **Lot 3 (gestes).** Le doigt passe par **une seule politique de routage** remplaçable (DA-A1) ; l'état d'un outil
  est porté **par pointeur**, pas par l'outil global, pour que le mode à deux utilisateurs du lot 11 ne réécrive pas
  les outils.
- **Format natif.** Les ajouts de l'amendement (restrictions de publication, équation, texte vertical, cadres,
  extension de page, médias) entrent dans `actcraft-document/1` par des clés nouvelles, tolérées par les lecteurs
  existants (DA3), tant que la première release n'est pas publiée.

## 2. Fonctions reprises ou rétablies

Tâches = tâches de plan estimées, en plus du cadrage. Risque : F faible, M moyen, É élevé. Plateformes : W Windows,
A Android.

### 2.1 Exclusions reprises (14 exigences)

| Exigence | Intitulé (à nous) | Lot | Plateformes | Tâches | Risque | À élucider (REA / vidéo) |
|---|---|---|---|---|---|---|
| F-301 (a) | Extension de page : poignée qui allonge la page par pas de 10 % | 4 | W, A | 1 | F | emplacement de la commande (écart E12, V50) ; l'icône reste d'une page à l'autre |
| F-301 (b) | Texte vertical : zone de texte écrite de haut en bas, sa barre de mise en forme | 4 | W, A | 2 | M | empilement des lettres ou rotation de la ligne (REA sur la 3.5.18) ; lecture des `.flipchart` qui en contiennent |
| F-603 | Cadres de page : division horizontale et verticale de la page | 4 | W, A | 2 | É | comportement entier inconnu (spécification : « à vérifier ») : REA d'abord, aucune ligne de code avant |
| F-680 | Publier : restrictions de copie, d'enregistrement, d'impression, de caméra, sur notre format | 5 | W, A | 2 | M | messages et refus en mode publié (V58) ; « Publier » écrit un nouveau fichier |
| F-298 | Verrouillage du formateur par mot de passe | 6 | W, A | 2 | M | liste des fonctions grisées, non décrite (V20) : REA |
| F-920, F-142 | Boutons définis par l'utilisateur : lancer un programme ou un fichier | 6 | W | 2 | M | place du bouton d'édition (écart E3) ; suppression d'un bouton (V94) |
| F-297 | Correcteur d'orthographe du paperboard et d'un texte | 6 | W, A | 3 | M | options de la section de réglages ; parcours des pages ; dictionnaire de l'utilisateur |
| F-291 | Message défilant : texte, police, ombre, fond, position, boucle, vitesse | 7 | W, A | 2 | F | bouton de validation (écart E11) ; un seul passage sans boucle (V131) |
| F-299 | Navigateur web : ouvrir l'adresse d'accueil dans le navigateur du système | 7 | W, A | 1 | F | aucun ; adresse d'accueil à nous (N1-A13) |
| F-301 (c) | Équation : éditeur à modèles, double-clic pour rééditer | 7 | W, A | 4 | É | jeu de modèles et représentation des équations des `.flipchart` (MathML, N1-14) : REA |
| F-292 | Enregistreur d'écran : plein écran ou zone, pause, arrêt, annulation | 8 | W (A : N1-A2) | 3 | É | réapparition de sa palette masquée (V43) ; réglages de la section « enregistrements » |
| F-293 | Enregistreur de son : fichier son et icône de lecture sur la page | 8 | W (A : N1-A2) | 2 | M | aucun |
| F-302 | HTML incorporé : contenu web dans un cadre de la page | 8 | W, A (N1-A3) | 3 | É | capture d'image fixe de la vue, glisser du texte ou d'une image vers la page (V63) |
| F-110, F-303 | Mode à deux utilisateurs : deuxième palette, deux pointeurs, gestes à deux doigts suspendus | 11 | W, A | 4 | É | contenu de la palette de l'élève ; objet tenu par un pointeur inaccessible à l'autre |

F-302 n'était pas dans la liste de Jérôme : il est repris au nom du principe (N1-A11).

### 2.2 Réductions rétablies

| Exigence | Intitulé (à nous) | Lot | Plateformes | Tâches | Risque | À élucider (REA / vidéo) |
|---|---|---|---|---|---|---|
| F-670 | Boîte d'impression complète : pages par feuille, plage, copies, marge, gris, notes, aperçu, réglages gardés | 5 | W (A : PDF, inchangé) | 3 | M | page allongée imprimée en entier (V44) ; disposition de la boîte |
| F-552 | Lecteur de médias intégré : sons à deux pistes, vidéos sur la page, contrôleur de son ; **mesuré par le banc** | 8 | W, A | 3 | É | formats lus par Qt Multimedia ; second média placé à côté du premier (V126, à vérifier) ; GIF animé (V57) |
| F-640 | Catalogue d'effets de changement de page, effet par page, durée réglable ; **chaque effet mesuré** | 7 | W, A | 3 | M | liste et comportement des effets de l'original (REA) ; cas où les effets sont désactivés |
| F-930 (toucher) | Quatre modes du toucher : ignoré, comme le stylo, comme le sélecteur, gestion complète | 11 | W, A | 2 | M | outil par défaut du toucher ; mode d'interaction par défaut avec les objets (REA) |
| F-760 (aide) | Aide hors ligne écrite par nous, ouverte par F1 | 6 (lecteur), 11 (pages) | W, A | 1 + 2 | F | aucun ; textes à nous seulement |
| F-760, F-930 (mises à jour) | Recherche de mises à jour par les releases du dépôt public | 11 | W | 2 | F | aucun (N1-A12) |
| F-930 (langue) | Textes traduisibles : fichiers de traduction préparés, français livré, réglage de la langue | 6 (réglage), 11 (contrôle) | W, A | 1 + 1 | F | aucun |

### 2.3 Tâches ajoutées par lot (fonctions)

| Lot | Fonctions | Tâches |
|---|---|---|
| 4 | extension de page, texte vertical, cadres | 5 |
| 5 | impression complète, publier | 5 |
| 6 | verrouillage, boutons de l'utilisateur, orthographe, lecteur d'aide, langue | 9 |
| 7 | message défilant, navigateur web, équation, catalogue d'effets | 10 |
| 8 | lecteur de médias, enregistreurs d'écran et de son, HTML incorporé | 11 |
| 11 | deux utilisateurs, modes du toucher, pages d'aide, mises à jour, contrôle des traductions | 11 |
| **Total** | 14 exigences reprises, 7 réductions levées | **51** |

## 3. Les 12 écarts entre les vidéos et la spécification

Les vidéos montrent les versions 1.4 à 1.6 ; la spécification décrit la 3.5.18. **La 3.5.18 fait foi.** Chaque écart
est tranché dans la tâche d'ouverture du lot qui l'accueille (§ 4), par REA sur l'installation de
`C:\Program Files\Activ Software\Inspire\` : d'abord en statique (ressources des boîtes de dialogue, valeurs par défaut
des réglages et des profils, chaînes de l'interface), puis, seulement si le statique ne suffit pas, en observant
l'original à l'écran — après avoir prévenu Jérôme, puisque cela pilote la souris de l'écran principal (`CLAUDE.md`,
mode autonome, point 3). Le résultat est écrit comme exigence du lot, avec nos mots ; en attendant, la spécification
s'applique.

| # | Sujet (à nous) | Vidéos | Lot | Défaut tant que non tranché |
|---|---|---|---|---|
| E1 | Remplissage d'une forme à la création : choix « sans remplissage » dans la palette des formes | V16, V61, V76, V117 | 4 | forme remplie (F-241) et choix « sans remplissage » offert |
| E2 | Validation de la boîte du fond de page : un bouton, ou appliquer puis valider | V68 | 4 | F-610 |
| E3 | Place du bouton d'édition des boutons de l'utilisateur dans la barre de raccourcis | V94 | 6 | F-142 |
| E4 | Taille de page par défaut : 1024 × 768 ou taille de l'écran selon le profil | V144 | 6 | 1024 × 768 (décision 15) |
| E5 | Nombre de dés à l'ouverture | V127 | 7 | F-285 |
| E6 | Valeur « bloqué » du limiteur de dimension | V136 | 8 | F-579 |
| E7 | Question attachée à une page sans contenu | V139 | — | non tranché : vote exclu |
| E8 | Bouton de vote dans la palette par défaut | V65 | — | non tranché : vote exclu, aucun bouton |
| E9 | Actions possibles de l'horloge en fin de délai | V83 | 7 | F-290 |
| E10 | Taille personnalisée d'un nouveau paperboard : en pixels, ou en nombre de hauteurs de page | V23 | 4 | F-700 (lot 2) ; corrigé au lot 4 si la 3.5.18 diffère |
| E11 | Bouton de validation du message défilant | V75, V131 | 7 | F-291 |
| E12 | Emplacement de la commande d'extension de page | V50 | 4 | F-301 |

## 4. Les 137 principes absents de la spécification

La synthèse du corpus regroupe les 137 principes absents en **92 constats** (une entrée ou sous-entrée de la section
« Principes absents »). La répartition se fait par constat ; la tâche d'ouverture de chaque lot revient aux fiches et
recompte les principes eux-mêmes.

| Thème | Constats | Lot 3 | Lot 4 | Lot 5 | Lot 6 | Lot 7 | Lot 8 | Imports (§ 5) | Exclus |
|---|---|---|---|---|---|---|---|---|---|
| Interface et profils | 12 | | 1 | | 7 | 4 | | | |
| Stylo, formes, couleurs | 8 | 1 | 3 | 3 | 1 | | | | |
| Texte et reconnaissance | 5 | | 1 | | | 4 | | | |
| Sélection, verrouillage, masquage | 6 | | | 1 | | 2 | 3 | | |
| Pages, fond, grille | 7 | | 3 | 1 | 1 | 2 | | | |
| Ressources | 9 | | | | 5 | | | 4 (packs) | |
| Actions et navigateurs | 7 | | | 4 | 1 | | 2 | | |
| Outils | 12 | | | | | 11 | 1 | | |
| Multimédia et import | 8 | | | | 2 | | 4 | 1 (PowerPoint) | 1 (Flash) |
| Vote | 18 | | | | | | | | 18 |
| **Total** | **92** | **1** | **8** | **9** | **17** | **23** | **10** | **5** | **19** |

Lots 9, 10 et 11 : aucun constat. Principe contraire à la fluidité, non repris tel quel : l'indexation de la
bibliothèque qui bloque le logiciel plusieurs minutes devient une indexation sur fil secondaire, annulable (si les
packs sont retenus).

**Règle de la tâche d'ouverture (`Lx-T0`), pour les lots 3 à 8 et 11.** Chaque plan de lot commence par elle ; les
autres tâches du lot en dépendent.

1. Relever dans le corpus tous les principes des fonctions du lot, ceux de la spécification compris (pour vérifier
   l'accord), et ceux du tableau ci-dessus.
2. Trancher sur la 3.5.18, par REA (méthode du § 3), les principes marqués ambigus, les écarts du lot et les points
   « à élucider » du § 2.
3. Écrire en nos mots les exigences du lot dans le plan du lot (identifiants `Vnn`, aucune citation, aucun nom de
   classe ou de propriété de l'original) ; chaque exigence est rattachée à une tâche et à un scénario.
4. Classer en hypothèses pour Jérôme les principes contraires à la fluidité ou à la propriété intellectuelle, et ceux
   que la 3.5.18 ne confirme pas.
5. Livrer `docs/verifications/<date>-lot<x>-principes.md` (table principe → exigence → tâche → scénario, ou raison de
   l'écart) et `NOMS OK` ; l'orchestrateur amende ensuite le plan du lot (relecture du plan exigée).

Estimation : 1 tâche d'ouverture par lot (7) ; principes qui demandent une tâche à eux : lot 4 + 1, lot 5 + 1, lot 6
+ 2, lot 7 + 3, lot 8 + 1 (les autres entrent dans les tâches des fonctions).

**Lot 9, redéfini.** Passe finale de contrôle sur le produit complet : chaque fiche du corpus est rejouée contre
ActCraft, écarts résiduels corrigés ou rendus en hypothèses ; le rapport et S12 du cadrage restent valables.

## 5. Chiffrage des imports (décision de Jérôme ensuite)

Aucun de ces imports n'est affecté à un lot. Tant que Jérôme n'a pas tranché, le menu Importer et le tableau de bord
n'offrent que le PDF (N1-A8). Aucun fichier d'essai de ces formats n'est disponible : le jeu d'essai ne contient que
des `.flipchart`, et s'en procurer demande l'accord de Jérôme (téléchargement).

| Import | Format | Faisabilité | Dépendances et licences | Tâches | Risque |
|---|---|---|---|---|---|
| SMART Notebook et éléments de galerie (F-662) | archive zip : pages en SVG et manifeste XML pour les versions récentes ; anciennes versions à élucider | oui : lecture des pages SVG en objets ActCraft (formes, traits, textes, images) par un lecteur à nous dans `formats/` (QtCore) ; l'original traite une vingtaine de genres d'objets (`architecture.md:103`) | aucune bibliothèque ; format d'un tiers lu pour l'interopérabilité, documenté par nous | 8 | É : pas d'oracle, pas de fichier d'essai, fidélité du SVG |
| PowerPoint (F-661) | `.pptx` (norme ECMA-376, publique) ; `.ppt` binaire (spécification publique de Microsoft) | « en tant qu'images » : par un convertisseur installé sur le poste (PowerPoint ou LibreOffice) vers PDF, puis l'import PDF du lot 5 ; « en tant qu'objets » : lecteur à nous des formes, textes et images du `.pptx` | convertisseur non livré (LibreOffice MPL 2.0, PowerPoint payant) ; Android : objets seulement | 3 (images) + 8 (objets) | M (images), É (objets : ≈ 180 formes prédéfinies, thèmes) |
| Questions XML (F-663) | IMS QTI (spécification publique) ; ExamView XML (non documenté) | QTI : oui, une page par question, énoncé et réponses en objets ; ExamView : après élucidation, sur fichiers d'essai | dépend du gestionnaire de questions, qui relève du vote exclu : sans vote, l'import ne produit que des pages | 4 | M ; faible valeur sans le vote |
| Packs de ressources (F-665, F-814) | archive de l'original (`.as2` selon la spécification, à confirmer par REA) ; export d'un dossier dans **notre** format de pack | oui après élucidation : lecture des objets par le lecteur `.flipchart` du lot 2, chemin d'origine recréé, remplacer / étendre / abandonner, indexation sur fil secondaire | navigateur de ressources (lot 6) ; aucun pack de Promethean livré | 6 | M |
| Format commun des tableaux interactifs `.iwb` (absent de la spécification, présent dans le binaire, `architecture.md:103`) | archive zip, SVG et manifeste (norme publique) | oui, import et export ; partage le lecteur SVG de SMART | aucune | 6 seul, 3 avec SMART | M |
| Anciens paperboards `.flp` v2 et v3 (F-655) | binaire de l'ancien logiciel | à élucider par REA ; l'original a un lecteur dédié | aucune | 6 | É : aucun fichier d'essai, aucun oracle |

Tout retenu : ≈ 38 à 41 tâches de plus. Recommandation, pour Jérôme : packs et PowerPoint « en tant qu'images »
d'abord (≈ 9 tâches, les plus utiles en classe et les moins risqués), puis SMART et `.iwb` ensemble.

## 6. Table des lots mise à jour

Ordre en vigueur (fichier d'état) : lot 2, lot 10, puis lots 3 à 9 et 11. Les lots 1 et 2 ne changent pas ; leur
nombre réel de tâches est donné pour comparaison.

| Lot | Titre | Avant | Après | Ajouts |
|---|---|---|---|---|
| 1 | Socle ouvrable et paquets | 16 | 16 | — (réel : 25, clos) |
| 2 | Document, format natif, lecteur `.flipchart` | 16 | 16 | — (réel : 30, en clôture) |
| 10 | Cache incrémental | 4 | 4 | — |
| 3 | Outils I | 12 | 13 | ouverture 1 ; contraintes du § 1 |
| 4 | Outils II : formes, texte, pages, grille | 13 | 20 | ouverture 1, fonctions 5, principes 1 |
| 5 | Édition et fichiers | 16 | 23 | ouverture 1, fonctions 5, principes 1 |
| 6 | Navigateurs, propriétés, profils | 15 | 27 | ouverture 1, fonctions 9, principes 2 |
| 7 | Présentation et outils | 15 | 29 | ouverture 1, fonctions 10, principes 3 |
| 8 | Interactivité et multimédia | 11 | 24 | ouverture 1, fonctions 11, principes 1 |
| 9 | Passe finale de contrôle (corpus) | 8 | 6 | confrontation déplacée dans chaque lot |
| 11 | Tablette, toucher, finitions, 1.0 | 8 | 20 | ouverture 1, fonctions 11 |
| **Total** | | **134** | **198** | **+ 64** (imports : jusqu'à + 41, non comptés) |

Les lots 6, 7 et 8 dépassent 20 tâches : leur plan les découpe en deux sous-lots, comme le lot 2 (une PR par lot).

## 7. Scénarios d'acceptation ajoutés

Même règle que S1–S14 : joués par l'agent sans souris réelle (`actcraft-captures`, commandes, ligne de commande,
émulateur), sorties sous `.tmp/` ou `build/`, documents d'essai synthétiques écrits par nous.

- **S15 — Extension et cadres (lot 4).** Sur un document neuf, trois clics sur la poignée d'extension : la hauteur de
  la page suit la règle des 10 % par clic écrite par `L4-T0` (base fixe ou cumulée), la poignée suit le bas de la page et l'outil courant n'a pas changé ; Annuler rend
  768. Deux cadres horizontaux réglés : comportement de la page conforme à l'exigence écrite par `L4-T0` (contrôle de
  pixels) ; aller-retour `.actcraft` : même empreinte.
- **S16 — Texte vertical (lot 4).** Poser un texte vertical « ABC » par touches injectées : les trois lettres sont
  rendues l'une sous l'autre (contrôle de pixels sur trois bandes horizontales) ; gras appliqué par sa barre ; rouvert
  après enregistrement, identique.
- **S17 — Imprimer (lot 5, Windows).** `file.print` sur un document de 4 pages, réglé à 2 pages par feuille, en
  gris, plage 2 à 4, notes incluses, vers l'imprimante virtuelle du système qui écrit un fichier : le fichier compte
  2 feuilles, sans couleur (contrôle de pixels), et une page allongée y tient entière. L'aperçu avant impression
  montre les mêmes feuilles. Réglages enregistrés : relancer, rouvrir la boîte, ils sont là. Sous Android, la commande
  produit toujours un PDF.
- **S18 — Publier (lot 5).** Publier un document avec « interdire la copie » et « interdire l'impression » : un
  nouveau fichier est écrit, l'original reste intact ; rouvert, copier un objet et imprimer affichent le message de
  refus, et une action de page fonctionne toujours.
- **S19 — Verrouillage du formateur (lot 6).** Créer un mot de passe, verrouiller : les commandes de la liste écrite
  par `L6-T0` sont grisées ; un mauvais mot de passe ne déverrouille pas ; le bon, si ; relancer : l'état verrouillé
  et le mot de passe (haché, jamais en clair dans les réglages) sont gardés.
- **S20 — Bouton défini par l'utilisateur (lot 6, Windows).** Ajouter un bouton qui ouvre un fichier texte d'essai :
  il apparaît dans la barre de raccourcis avec son info-bulle ; un clic ouvre le fichier par l'application du système
  (processus fils relevé) ; le supprimer : il disparaît. Sous Android, la commande d'édition est absente (capture).
- **S21 — Orthographe (lot 6).** Un document de 2 pages contient « bonjour mosieur » : la vérification affiche la
  page 1, propose « monsieur », « remplacer » corrige le texte ; « ignorer toujours » sur un autre mot ne le signale
  plus ; message de fin.
- **S22 — Effets de page (lot 7).** Pour chaque effet du catalogue : passer de la page 1 à la page 2 avec l'effet
  choisi, banc en D3D11 plein écran pendant la transition (aucune image au-delà de la limite) ; un effet propre à la
  page 2 prime sur l'effet global ; navigation par le navigateur de pages : aucun effet joué.
- **S23 — Message défilant (lot 7).** Un message en bas d'écran, sans boucle : il traverse l'écran une fois puis
  disparaît ; en boucle, il recommence ; le banc mesure l'écriture au stylet pendant le défilement (aucune reprise hors
  limite).
- **S24 — Équation (lot 7).** Insérer une fraction depuis les modèles, saisir 1 et 2 : l'objet s'affiche sur la page ;
  double-clic : l'éditeur rouvre la même équation ; aller-retour `.actcraft` identique ; une équation lue d'un
  `.flipchart` synthétique s'affiche.
- **S25 — Médias intégrés (lot 8).** Un son placé sur la page, cliqué : le contrôleur de son le lit (position qui
  avance) ; une vidéo joue dans son cadre pendant qu'on écrit au stylet, banc en D3D11 : aucune régression des
  critères de fluidité ; un GIF animé change d'image.
- **S26 — Enregistreurs (lot 8).** Windows : enregistrement d'écran de 5 s d'une zone : le fichier existe, sa durée
  est de 5 s ± 0,5, et l'écriture pendant l'enregistrement reste dans le budget du banc ; enregistrement de son de
  3 s : le fichier existe et une icône de lecture est posée sur la page. Android : si la permission est refusée sur
  l'émulateur, la commande se désactive avec un message, sans plantage.
- **S27 — HTML incorporé (lot 8).** Un cadre web avec un HTML local d'essai : le contenu s'affiche, se redimensionne,
  recharge ; la capture d'image fixe pose une image sur la page ; enregistré puis rouvert, le cadre est là.
- **S28 — Deux utilisateurs et toucher (lot 11).** Mode à deux utilisateurs activé : la deuxième palette apparaît ;
  deux pointeurs injectés tracent en même temps deux traits de couleurs différentes ; un pincement ne zoome plus ; un
  objet tenu par un pointeur ne bouge pas sous l'autre ; mode désactivé : le pincement zoome. Pour chacun des quatre
  modes du toucher, un doigt injecté produit l'effet attendu (rien, trait, sélection, comportement complet).
- **S29 — Aide, langue et mises à jour (lot 11).** F1 ouvre l'aide hors ligne sur la page de l'outil courant, sans
  réseau ; une langue d'essai chargée par le réglage change les libellés du menu Fichier ; `lupdate` ne trouve aucune
  chaîne hors traduction ; Windows : la recherche de mises à jour contre une réponse de releases simulée annonce la
  version plus récente, et ne fait aucune requête au démarrage tant que le réglage est désactivé.
- **S30 — Tâche d'ouverture (chaque lot 3 à 8 et 11).** Le rapport `docs/verifications/<date>-lot<x>-principes.md`
  existe ; chaque constat du lot (§ 4) y a une ligne (exigence, tâche, scénario, ou raison) ; `git grep` des titres
  de la table de correspondance du corpus ne trouve rien ; `NOMS OK`.

## 8. Hypothèses à revoir créées par l'amendement

### Niveau 1 — difficiles à défaire ou visibles des utilisateurs

- **N1-A1 — Volume.** + 64 tâches (134 → 198), sans les imports ; la note 0100 en annonçait 20 à 25, sans compter les
  tâches d'ouverture, l'équation, les enregistreurs ni le HTML incorporé. Lots 6, 7, 8 en deux sous-lots.
- **N1-A2 — Permissions Android.** La décision 28 (« aucune permission ») est amendée pour le seul enregistreur de son :
  le microphone est demandé à la première utilisation ; refusé, la commande se désactive. L'enregistreur d'écran
  Android n'est livré que si Qt 6.12 le permet sans service de premier plan à nous (non vérifié) ; sinon Windows seul.
- **N1-A3 — Moteur web.** F-302 exige un moteur web : sous Android, la vue web du système, contenu local seulement
  (pas de permission réseau ; un lien distant s'ouvre dans le navigateur) ; sous Windows, un moteur du système par
  Qt WebView s'il existe pour Qt 6.12, sinon Qt WebEngine (≈ 150 Mo de plus dans l'installateur, à mesurer). Choix
  fait par `L8-T0`, poids de l'installateur et licences rapportés à la porte du lot 8.
- **N1-A4 — Restrictions de publication.** Sur un format ouvert, elles sont une courtoisie : un utilisateur averti
  peut les retirer en éditant le JSON. Pas de chiffrement.
- **N1-A5 — Mot de passe du formateur.** Aucun mot de passe par défaut (l'original en a un, connu de tous) : on le
  crée au premier verrouillage. Haché avec sel dans les réglages. Oublié : réinitialisation en supprimant le réglage,
  procédure dans l'aide.
- **N1-A6 — Boutons définis par l'utilisateur.** Ils lancent un programme du poste : le chemin est affiché à la
  création, et un bouton lu dans un profil copié d'un autre poste demande une confirmation au premier lancement.
- **N1-A7 — Formats des enregistreurs.** Écran en `.mp4` (encodeurs du système par Qt Multimedia) au lieu de `.avi` ;
  son en `.wav` comme l'original.
- **N1-A8 — Imports en attente.** Tant que Jérôme n'a pas tranché le § 5, le menu Importer et le tableau de bord
  n'offrent que le PDF.
- **N1-A9 — Vidéos de versions anciennes.** Le corpus montre les versions 1.4 à 1.6 : un principe non confirmé par la
  3.5.18 n'est pas repris. L'observation de l'original à l'écran prévient Jérôme avant chaque séance.
- **N1-A10 — Taille et licences des paquets.** Qt Multimedia (et son moteur FFmpeg), Qt Print Support (qui tire
  QtWidgets, Windows seulement), Hunspell et un dictionnaire français, peut-être Qt WebEngine : chacun passe par la
  liste des bibliothèques autorisées et `THIRD_PARTY_NOTICES.md` ; la croissance de l'APK et de l'installateur est
  rapportée à chaque porte de lot.
- **N1-A11 — F-302 repris sans demande explicite.** Au nom du principe ; retirable par l'option de fonctions
  (`ACTCRAFT_FEATURES`) si Jérôme le refuse.
- **N1-A12 — Mises à jour.** Recherche manuelle par le menu Aide ; recherche au démarrage désactivée par défaut
  (réseau, vie privée) ; source : les releases du dépôt public ; Android : rien (une mise à jour = un nouvel APK).
- **N1-A13 — Adresse d'accueil du navigateur web.** Défaut : la page du projet ; réglable.
- **N1-A14 — Apparence simplifiée de l'original pour les petites classes.** Non reprise (design libre) ; un profil
  fourni par nous en tient lieu. Question pour Jérôme à la porte du lot 6.

### Niveau 2 — les autres

- **N2-A1** Références `Vnn` = numéros des fiches du corpus (au lieu d'une nouvelle numérotation V01… au lot 9).
- **N2-A2** Texte vertical sur les deux plateformes (l'original : Windows seulement).
- **N2-A3** Correcteur : dictionnaire français seulement livré ; autres dictionnaires ajoutables par l'utilisateur.
- **N2-A4** Aide hors ligne en français seulement, écrite par nous, sans capture de l'original.
- **N2-A5** Mode du toucher par défaut : gestion complète (comportement de la décision 25).
- **N2-A6** Noms et dessin des effets de page à nous ; la liste suit celle de l'original élucidée par REA.
- **N2-A7** Palette de l'élève non personnalisable, comme l'original.
- **N2-A8** Impression complète derrière une option de construction (`ACTCRAFT_PRINT`), construite sous Windows
  seulement.

### Décisions d'architecture de l'amendement

**DA-A1 — Routage du doigt et outils par pointeur.**
Contexte : quatre modes du toucher et mode à deux utilisateurs (lot 11) ; les gestes arrivent au lot 3.
Décision : au lot 3, tout événement de doigt passe par une politique unique (mode courant → outil, sélection, geste ou
rien) ; l'état d'un outil en cours (trait, glissement) est indexé par l'identifiant du pointeur.
Conséquences : le lot 11 ajoute des politiques sans réécrire les outils ; un seul pointeur suffit jusque-là.
Réversible : interne à `input/` et `tools/`.

**DA-A2 — Impression complète dans un module à part, Windows seulement.**
Contexte : `QPrinter` et sa boîte sont dans Qt Print Support, qui dépend de QtWidgets et n'existe pas sous Android
(N1-15 du cadrage).
Décision : module `print/` derrière `ACTCRAFT_PRINT`, boîte d'impression en QML à nous, feuilles composées par le
peintre partagé (DA5) ; Android garde « Imprimer » par PDF. La tâche qui le crée vérifie que `QPrinter` marche sous
une application Qt Quick sans `QApplication` (non vérifié), et mesure le démarrage avant et après.
Conséquences : le cœur reste sans QtWidgets ; l'aperçu réutilise le rendu logiciel.
Réversible : le module se retire par l'option.

**DA-A3 — Médias par Qt Multimedia, hors du fil de rendu de la page.**
Contexte : F-552 et les enregistreurs ; la fluidité prime (`docs/BRIEF.md`).
Décision : lecture et enregistrement par Qt Multimedia (LGPLv3) dans un module `media/`, vidéos rendues dans leur
propre élément au-dessus du cache de page ; chaque fonction média a son scénario de banc.
Conséquences : la décision N2-9 du cadrage (médias par l'application du système) devient le repli si Qt Multimedia
manque pour une cible.
Réversible : le module se retire par `ACTCRAFT_FEATURES`.
