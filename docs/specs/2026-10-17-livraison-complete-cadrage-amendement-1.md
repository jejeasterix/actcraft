# Cadrage — livraison complète, amendement 1 (toutes les fonctionnalités, vidéos à chaque lot)

Amendement du cadrage `docs/specs/2026-10-14-livraison-complete-cadrage.md`, écrit le 2026-10-17 pendant la clôture du
lot 2, révisé après le tour 1 de contre-relecture. Il ne réécrit pas le cadrage : il le complète et **fait autorité
sur lui pour ce qu'il change**. Tout le reste du cadrage reste en vigueur.

Sources : décision de Jérôme du 2026-10-17 (coffre `02_Decisions/0100-actcraft-toutes-les-fonctionnalites-d-activinspire-design-libre`,
fichier d'état `docs/sessions/livraison-complete-etat.md` § « Portes ») ; spécification de l'ancien dépôt
`REA - Activinspire/docs/spec/fonctionnalites.md` (exigences F-xxx) ; synthèse du corpus des vidéos
`REA - Activinspire/docs/videos-tanguay/README.md` (sections « Principes absents » et « Écarts avec la spec ») ;
architecture relevée de l'original `REA - Activinspire/docs/re/architecture.md:103` (imports présents dans le binaire) ;
paquets du lot 1 (`build/paquet/ActCraft-0.1.0-setup.exe` : 26 718 595 octets, soit 25,5 Mio ; APK `arm64-v8a` :
51 Mo).

Références aux vidéos : `Vnn` = numéro de fiche du corpus. La table numéro → vidéo reste dans le dossier du corpus,
hors dépôt (décision 12 du cadrage). Aucun titre, aucune transcription, aucune citation. Les intitulés des fonctions
sont les nôtres ; les numéros F-xxx renvoient à la spécification de l'ancien dépôt.

## 1. Le principe et ce qu'il change

**Principe (décision 0100, Jérôme).** ActCraft reprend **toutes les fonctionnalités d'ActivInspire 3.5.18**. Le design
est libre. Sous Windows, le **comportement et la disposition** d'ActivInspire font référence. Sous Android, l'interface
est libre (ActivInspire n'existe pas sous Android) ; l'émulateur sert quand c'est utile. **La fonction est due, sa
réalisation est libre** : quand une manière plus moderne de réaliser une fonction est pertinente (un format ouvert, un
service du système, une interaction tactile), l'orchestrateur la choisit en autonomie, l'écrit dans le plan du lot et
la signale au rapport de lot.

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
(Windows) ; reconnaissance d'écriture (Windows) ; raccourcis vers des programmes (Windows) ; enregistreur d'écran
(Windows) ; enregistreur de son (Android avec la permission du micro, N1-A2) ; boîte d'impression complète (Windows,
Android par PDF) ; recherche de mises à jour et serveur mandataire (Windows) ; cadre web (Android : contenu local seulement, N1-A3).

**Hypothèse N2-6, amendée.** Les cinq profils fournis par l'original sont repris **dans leur usage et leur
disposition** (au tableau, création, langues, mathématiques, médias : ce que chacun ouvre, épingle et règle), avec nos
noms et nos contenus. Ils sont livrés au lot 6 avec les outils déjà là ; trois d'entre eux épinglent des outils des
lots 7 et 8 (`fonctionnalites.md:825`). Règle : **chaque lot qui livre un outil épinglé par un profil complète ce
profil** (une ligne dans la tâche de l'outil) ; le contrôle de bout en bout est S32 au lot 11.

**§ 6 du cadrage, remplacé pour les exigences.** Restent exclus, comme le dit la décision 0100 :

- le vote sans boîtiers : F-126, F-127, F-1000, F-1010, F-1020, F-1030, les actions de vote du navigateur d'action, la
  section de réglages du vote, la touche de fonction du sondage rapide ;
- l'écriture des `.flipchart`.

**Exclusions supplémentaires à la décision 0100**, à confirmer par Jérôme (N1-A15) :

- la lecture Flash (`.swf`, `.flv`) : technologie morte, plus maintenue par son éditeur depuis 2020 ; un objet Flash
  s'affiche par son espace réservé, comme le prévoit la spécification (`fonctionnalites.md:892`) ;
- les liens vers les services en ligne de Promethean (dans F-090, F-760, F-930) : propriété intellectuelle et services
  d'un tiers ; remplacés par la page du projet.

**En attente** (chiffrés au § 5, aucun lot tant que Jérôme n'a pas tranché) : F-661, F-662, F-663, F-665, F-814, et les
anciens `.flp` de F-655. Les autres exclusions du § 6 du cadrage (paquet macOS, `.aab`, armv7, signature, Linux, iOS,
mesures sur machines de référence, ressources de Promethean, contenu des vidéos) restent valables, ainsi que
**l'encre à faible latence native et la prédiction du stylet** : ce sont des techniques de rendu, pas des fonctions
d'ActivInspire, exclues depuis le prototype (`docs/specs/2026-10-13-prototype-mesure-cadrage.md:104-105,204`) parce
que le banc mesure la latence interne, que la prédiction ne réduit pas ; une mesure du banc peut les rouvrir. Le
réglage du serveur mandataire de F-930, muet jusqu'ici, est **repris** pour le bureau (§ 2.2).

**Contraintes posées aux lots déjà cadrés** (sans tâche de plus pour eux) :

- **Lot 3 (gestes).** Le doigt passe par **une seule politique de routage** remplaçable, et toute entrée est rattachée
  à un utilisateur (DA-A1). Le schéma d'entrée (quels périphériques forment le second utilisateur) est tranché par
  `L3-T0` ; s'il ne peut pas l'être, il devient une hypothèse de niveau 1 du lot 3.
- **Commandes (DA1 du cadrage).** Chaque commande déclare deux attributs : **restreignable** (une diffusion protégée
  peut l'interdire, F-680) et **verrouillable** (le mode classe protégé peut la griser, F-298). Le registre les porte
  dès le lot 6 ; chaque lot les remplit pour ses commandes, et la tâche du lot 6 qui crée l'attribut le remplit pour
  les commandes des lots 1 à 5.
- **Format natif.** Les ajouts de l'amendement (restrictions de diffusion, équation, texte de haut en bas, zones de
  page, page allongée, médias) entrent dans `actcraft-document/1` par des clés nouvelles, tant que la première release
  n'est pas publiée. Règle de compatibilité ascendante : un lecteur qui rencontre une clé inconnue la **conserve et la
  réécrit telle quelle** à l'enregistrement ; si le lecteur du lot 2 la perd, la première tâche qui ajoute une clé le
  corrige, prouvé par un aller-retour avec une clé inconnue.
- **Types d'objet inconnus (lot 2, avant toute release).** Le lecteur du lot 2 saute un objet de type inconnu
  (`formats/src/nativejson.cpp:1166-1168`), et l'équation (lot 7) comme le cadre web (lot 8) n'existent pas dans
  `ObjectKind` (`core/include/actcraft/core/model.h:184-194`) : une 0.2.0 publiée qui ouvrirait puis enregistrerait un
  fichier d'une version plus récente perdrait ces objets. Règle : un objet de type inconnu est **gardé tel quel** (JSON
  brut de l'objet, place dans l'ordre des couches et de profondeur, ressources qu'il référence conservées dans
  l'archive), **affiché par un repère neutre** (cadre de sa boîte englobante, s'il en a une), et **réécrit à
  l'identique**. Tâche corrective **`L2-C8`** (`formats/` et `core/`), faite dans le lot 2 avant toute release ;
  preuve : aller-retour d'un document qui contient un objet de type inconnu et une clé inconnue, octets JSON de
  l'objet identiques à l'entrée, ressource référencée toujours présente (S31).

## 2. Fonctions reprises ou rétablies

Tâches = tâches de plan estimées, en plus du cadrage (estimation brute, voir N1-A1). Risque : F faible, M moyen,
É élevé. Plateformes : W Windows, A Android.

### 2.1 Exclusions reprises (14 exigences)

| Exigence | Intitulé (à nous) | Lot | Plateformes | Tâches | Risque | À élucider (REA / vidéo) |
|---|---|---|---|---|---|---|
| F-301 (a) | Allonger la page : une poignée l'agrandit par pas de 10 % | 4 | W, A | 1 | F | emplacement de la commande (écart E12, V50) ; la poignée reste d'une page à l'autre ; base du pas (fixe ou cumulée) |
| F-301 (b) | Texte de haut en bas, avec sa barre de mise en forme | 4 | W, A | 2 | M | lettres empilées ou ligne tournée (REA sur la 3.5.18) ; lecture des `.flipchart` qui en contiennent |
| F-603 | Découpage de la page en zones horizontales et verticales | 4 | W, A | 2 | É | comportement entier inconnu (spécification : « à vérifier ») : REA d'abord, aucune ligne de code avant |
| F-298 | Mode classe protégé par mot de passe | 6 | W, A | 2 | M | liste des fonctions grisées, non décrite (V20) : REA ; elle couvre des commandes des lots 1 à 11 (attribut « verrouillable », § 1) |
| F-920, F-142 | Raccourcis vers des programmes ou des fichiers, dans la barre de raccourcis | 6 | W | 2 | M | place du bouton d'édition (écart E3) ; suppression d'un raccourci (V94) |
| F-297 | Correcteur d'orthographe du paperboard et d'un texte | 6 | W, A | 3 | M | options de la section de réglages ; parcours des pages ; dictionnaire de l'utilisateur |
| F-291 | Message défilant : texte, police, ombre, fond, position, boucle, vitesse | 7 | W, A | 2 | F | bouton de validation (écart E11) ; un seul passage sans boucle (V131) |
| F-299 | Navigateur web : ouvrir l'adresse d'accueil dans le navigateur du système | 7 | W, A | 1 | F | aucun ; adresse d'accueil à nous (N2-A11) |
| F-301 (c) | Équation : éditeur à modèles, double-clic pour rééditer | 7 | W, A | 4 | É | jeu de modèles et représentation des équations des `.flipchart` (MathML, N1-14) : REA |
| F-680 | Diffusion protégée : copie, enregistrement, impression, caméra interdits, sur notre format | 8 | W, A | 2 | M | messages et refus en mode protégé (V58) ; un nouveau fichier est écrit ; dépend de la caméra (lot 7), des ressources (lot 6) et des actions (lot 8) |
| F-292 | Enregistreur d'écran : plein écran ou zone, pause, arrêt, annulation, indicateur visible | 8 | W | 3 | É | réapparition de sa palette masquée (V43) ; réglages de la section des enregistrements |
| F-293 | Enregistreur de son : fichier son et icône de lecture sur la page, indicateur visible | 8 | W, A (N1-A2) | 2 | M | aucun |
| F-302 | Cadre web sur la page : code HTML saisi par l'utilisateur, affiché dans un cadre redimensionnable | 8 | W, A (local seulement) | 3 | É | capture d'image fixe de la vue, glisser du texte ou d'une image vers la page (V63) ; vue native au-dessus de la scène (N1-A3) |
| F-110, F-303 | Mode à deux utilisateurs : deuxième palette, deux pointeurs, chacun son outil, ses réglages, sa sélection et ses outils de maths ; aucune boîte de dialogue pour le second ; gestes à deux doigts suspendus | 11 | W, A | 4 | É | contenu de la deuxième palette ; objet tenu par un utilisateur inaccessible à l'autre ; schéma d'entrée (`L3-T0`, DA-A1) |

F-302 n'était pas dans la liste de Jérôme : il est repris au nom du principe (N1-A11).

### 2.2 Réductions rétablies et reprises implicites

| Exigence | Intitulé (à nous) | Lot | Plateformes | Tâches | Risque | À élucider (REA / vidéo) |
|---|---|---|---|---|---|---|
| F-670 | Boîte d'impression complète : pages par feuille, plage, copies, marge, gris, notes, aperçu, réglages gardés | 5 | W (A : PDF, inchangé) | 3 | M | page allongée imprimée en entier (V44) ; disposition de la boîte ; feuilles de notes |
| F-552 | Lecteur de médias intégré : sons à deux pistes, vidéos sur la page, lecteur de sons ; **mesuré par le banc** | 8 | W, A | 3 | É | formats lus par Qt Multimedia ; second média placé à côté du premier (V126, à vérifier) ; GIF animé (V57) |
| F-640 | Catalogue d'effets de changement de page, effet par page, durée réglable ; **chaque effet mesuré** | 7 | W, A | 3 | M | liste et comportement des effets de l'original (REA) ; cas où les effets sont désactivés |
| F-930 (toucher) | Quatre modes du toucher : ignoré, comme le stylo, comme le sélecteur, gestion complète | 11 | W, A | 2 | M | outil par défaut du toucher ; mode d'interaction par défaut avec les objets (REA) |
| F-930 (réseau) | Réglages réseau : serveur mandataire, port, compte ; mot de passe rangé dans le trousseau du système | 6 | W (bureau seulement) | 1 | M | aucun ; bibliothèque de trousseau à licence compatible (N1-A10) |
| F-880 | Cinq profils fournis, repris dans leur usage et leur disposition, à nos noms (N2-6 amendée) | 6, complétés aux lots 7 et 8 | W, A | 1 | M (trois profils dépendent d'outils des lots 7 et 8) | ce que chaque profil ouvre, épingle et règle (REA sur les profils livrés) |
| F-760 (aide) | Aide hors ligne écrite par nous, ouverte par F1 | 6 (lecteur), 11 (pages) | W, A | 1 + 2 | F | aucun ; textes à nous seulement |
| F-760, F-930 (mises à jour) | Recherche de mises à jour par les releases du dépôt public | 11 | W | 2 | F | aucun (N1-A12) |
| F-930 (langue) | Textes traduisibles : fichiers de traduction préparés, français livré, réglage de la langue | 6 (réglage), 11 (contrôle) | W, A | 1 + 1 | F | aucun |

### 2.3 Tâches ajoutées par lot (fonctions)

| Lot | Fonctions | Tâches |
|---|---|---|
| 4 | page allongée, texte de haut en bas, zones de page | 5 |
| 5 | impression complète | 3 |
| 6 | mode classe protégé, raccourcis vers des programmes, orthographe, lecteur d'aide, langue, réseau, profils | 11 |
| 7 | message défilant, navigateur web, équation, catalogue d'effets | 10 |
| 8 | lecteur de médias, enregistreurs d'écran et de son, cadre web, diffusion protégée | 13 |
| 11 | deux utilisateurs, modes du toucher, pages d'aide, mises à jour, contrôle des traductions | 11 |
| **Total** | 14 exigences reprises, 7 réductions levées, 2 reprises implicites | **53** |

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
| E3 | Place du bouton d'édition des raccourcis vers des programmes dans la barre de raccourcis | V94 | 6 | F-142 |
| E4 | Taille de page par défaut : 1024 × 768 ou taille de l'écran selon le profil | V144 | 6 | 1024 × 768 (décision 15) |
| E5 | Nombre de dés à l'ouverture | V127 | 7 | F-285 |
| E6 | Valeur « bloqué » du limiteur de dimension | V136 | 8 | F-579 |
| E7 | Question attachée à une page sans contenu | V139 | — | non tranché : vote exclu |
| E8 | Bouton de vote dans la palette par défaut | V65 | — | non tranché : vote exclu, aucun bouton |
| E9 | Actions possibles de l'horloge en fin de délai | V83 | 7 | F-290 |
| E10 | Taille personnalisée d'un paperboard neuf : en pixels, ou en nombre de hauteurs de page | V23 | 4 | F-700 (lot 2) ; corrigé au lot 4 si la 3.5.18 diffère |
| E11 | Bouton de validation du message défilant | V75, V131 | 7 | F-291 |
| E12 | Emplacement de la commande qui allonge la page | V50 | 4 | F-301 |

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
| Actions et navigateurs | 7 | | | | 1 | | 6 | | |
| Outils | 12 | | | | | 11 | 1 | | |
| Multimédia et import | 8 | | | | 2 | | 4 | 1 (PowerPoint) | 1 (Flash) |
| Vote | 18 | | | | | | | | 18 |
| **Total** | **92** | **1** | **8** | **5** | **17** | **23** | **14** | **5** | **19** |

Lots 9, 10 et 11 : aucun constat. Principe contraire à la fluidité, non repris tel quel : l'indexation de la
bibliothèque qui bloque le logiciel plusieurs minutes devient une indexation sur fil secondaire, annulable (si les
packs sont retenus).

**Règle de la tâche d'ouverture (`Lx-T0`), pour les lots 3 à 8 et 11.** Chaque plan de lot commence par elle ; les
autres tâches du lot en dépendent. Elle est menée par un agent `concepteur-critique` qui utilise le skill
`reverse-engineer-anything`.

1. Relever dans le corpus tous les principes des fonctions du lot, ceux de la spécification compris (pour vérifier
   l'accord), et ceux du tableau ci-dessus.
2. Trancher sur la 3.5.18, par REA (méthode du § 3), les principes marqués ambigus, les écarts du lot et les points
   « à élucider » du § 2.
3. Écrire le **relevé brut** (constats REA, noms de l'original, renvois aux fiches) **hors dépôt**, dans
   `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot<x>.md`.
4. Écrire en nos mots les exigences du lot dans le plan du lot (identifiants `Vnn`, aucune citation, aucun nom de
   classe ou de propriété de l'original) ; chaque exigence est rattachée à une tâche et à un scénario.
5. Classer en hypothèses pour Jérôme les principes contraires à la fluidité ou à la propriété intellectuelle, et ceux
   que la 3.5.18 ne confirme pas.
6. Livrer `docs/verifications/<date>-lot<x>-principes.md` (table principe → exigence → tâche → scénario, ou raison de
   l'écart ; chemin du relevé brut cité) et `NOMS OK` ; l'orchestrateur amende ensuite le plan du lot (relecture du
   plan exigée).

Estimation : 1 tâche d'ouverture par lot (7) ; principes qui demandent une tâche à eux : lot 4 + 1, lot 5 + 1, lot 6
+ 2, lot 7 + 3, lot 8 + 1 (les autres entrent dans les tâches des fonctions).

**Lot 9, redéfini.** Passe finale de contrôle sur le produit complet : chaque fiche du corpus est rejouée contre
ActCraft, écarts résiduels corrigés ou rendus en hypothèses ; le rapport et S12 du cadrage restent valables.
Estimation gardée à **8 tâches** : 126 fiches exploitables sur 144 (les 18 autres n'ont que leur titre), soit 5 tâches
de relevé d'environ 25 fiches chacune, déjà confrontées une première fois par les lots, et 3 tâches de corrections
résiduelles.

## 5. Chiffrage des imports (décision de Jérôme ensuite)

Aucun de ces imports n'est affecté à un lot. Tant que Jérôme n'a pas tranché (question posée à la porte de fin du
lot 2), le menu d'import et le tableau de bord n'offrent que le PDF (N1-A8). Aucun fichier d'essai de ces formats
n'est disponible : le jeu d'essai ne contient que des `.flipchart`, et s'en procurer demande l'accord de Jérôme
(téléchargement).

| Import | Format | Faisabilité | Dépendances et licences | Tâches | Risque |
|---|---|---|---|---|---|
| SMART Notebook et éléments de galerie (F-662) | archive zip : pages en SVG et manifeste XML pour les versions récentes ; anciennes versions à élucider | oui : lecture des pages SVG en objets ActCraft (formes, traits, textes, images) par un lecteur à nous dans `formats/` (QtCore) ; l'original traite une vingtaine de genres d'objets (`architecture.md:103`) | aucune bibliothèque ; format d'un tiers lu pour l'interopérabilité, documenté par nous | 8 | É : pas d'oracle, pas de fichier d'essai, fidélité du SVG |
| PowerPoint (F-661) | `.pptx` (norme ECMA-376, publique) ; `.ppt` binaire (spécification publique de Microsoft) | en images : par un convertisseur installé sur le poste (PowerPoint ou LibreOffice) vers PDF, puis l'import PDF du lot 5, chaque diapositive en fond de page ; en objets : lecteur à nous des formes, textes et images du `.pptx` | convertisseur non livré (LibreOffice MPL 2.0, PowerPoint payant) ; Android : objets seulement | 3 (images) + 8 (objets) | M (images), É (objets : ≈ 180 formes prédéfinies, thèmes) |
| Questions XML (F-663) | IMS QTI (spécification publique) ; ExamView XML (non documenté) | QTI : oui, une page par question, énoncé et réponses en objets ; ExamView : après élucidation, sur fichiers d'essai | dépend du gestionnaire de questions, qui relève du vote exclu : sans vote, l'import ne produit que des pages | 4 | M ; faible valeur sans le vote |
| Packs de ressources (F-665, F-814) | archive de l'original (`.as2` selon la spécification, à confirmer par REA) ; export d'un dossier dans **notre** format de pack | oui après élucidation : lecture des objets par le lecteur `.flipchart` du lot 2, chemin d'origine recréé, remplacer / étendre / abandonner, indexation sur fil secondaire | navigateur de ressources (lot 6) ; aucun pack de Promethean livré | 6 | M |
| Format commun des tableaux interactifs `.iwb` (absent de la spécification, présent dans le binaire, `architecture.md:103`) | archive zip, SVG et manifeste (norme publique) | oui, import et export ; partage le lecteur SVG de SMART | aucune | 6 seul, 3 avec SMART | M |
| Anciens paperboards `.flp` v2 et v3 (F-655) | binaire de l'ancien logiciel | à élucider par REA ; l'original a un lecteur dédié | aucune | 6 | É : aucun fichier d'essai, aucun oracle |

Tout retenu (`.iwb` fait avec SMART) : **≈ 38 tâches** de plus, en estimation brute. Recommandation, pour Jérôme :
packs et PowerPoint en images d'abord (≈ 9 tâches, les plus utiles en classe et les moins risqués), puis SMART et
`.iwb` ensemble.

## 6. Table des lots mise à jour

Ordre en vigueur (fichier d'état) : lot 2, lot 10, puis lots 3 à 9 et 11. Les lots 1 et 2 ne changent pas ; leur
nombre réel de tâches est donné pour comparaison ; le lot 2 reçoit la tâche corrective `L2-C8` (§ 1).

| Lot | Titre | Avant | Après | Ajouts | Version |
|---|---|---|---|---|---|
| 1 | Socle ouvrable et paquets | 16 | 16 | — (réel : 25, + 56 %, clos) | 0.1.0 |
| 2 | Document, format natif, lecteur `.flipchart` | 16 | 17 | `L2-C8` : types d'objet inconnus gardés (réel : 30 + 1, + 88 %, en clôture) | 0.2.0 |
| 10 | Cache incrémental | 4 | 4 | — | 0.3.0 |
| 3 | Outils I | 12 | 13 | ouverture 1 ; contraintes du § 1 | 0.4.0 |
| 4 | Outils II : formes, texte, pages, grille | 13 | 20 | ouverture 1, fonctions 5, principes 1 | 0.5.0 |
| 5 | Édition et fichiers | 16 | 21 | ouverture 1, fonctions 3, principes 1 | 0.6.0 |
| 6 | Navigateurs, propriétés, profils | 15 | 29 | ouverture 1, fonctions 11, principes 2 | 0.7.0 |
| 7 | Présentation et outils | 15 | 29 | ouverture 1, fonctions 10, principes 3 | 0.8.0 |
| 8 | Interactivité et multimédia | 11 | 26 | ouverture 1, fonctions 13, principes 1 | 0.9.0 |
| 9 | Passe finale de contrôle (corpus) | 8 | 8 | confrontation avancée dans chaque lot | 0.10.0 |
| 11 | Tablette, toucher, finitions, 1.0 | 8 | 20 | ouverture 1, fonctions 11 | 1.0.0 |
| **Total** | | **134** | **203** | **+ 69** brut (imports : ≈ + 38, non comptés) ; corrigé : ≈ 320 à 375 (N1-A1) | |

Les lots 5, 6, 7 et 8 dépassent 20 tâches : leur plan les découpe en deux sous-lots, comme le lot 2 (une seule PR et
une seule version par lot). Le seuil de 20 se juge aussi sur l'estimation corrigée (× 1,56 à 1,88) : les lots 3, 4 et
11 passent en sous-lots si leur plan dépasse 20 tâches.

**Décision 31 du cadrage, amendée (versions).** Les versions suivent l'ordre de livraison, mineure + 1 par lot : lot 2
→ 0.2.0, lot 10 → 0.3.0, lot 3 → 0.4.0, … lot 8 → 0.9.0, lot 9 → 0.10.0 (SemVer compare les nombres, pas les
chiffres), lot 11 → 1.0.0. `versionCode` Android inchangé dans sa règle (0.10.0 → 1000).

## 7. Scénarios d'acceptation ajoutés

Même règle que S1–S14 : joués par l'agent sans souris réelle (`actcraft-captures`, commandes, ligne de commande,
émulateur), sorties sous `.tmp/` ou `build/`, documents d'essai synthétiques écrits par nous.

- **S15 — Page allongée et zones (lot 4).** Sur un document neuf, trois clics sur la poignée : la hauteur de la page
  suit la règle des 10 % par clic écrite par `L4-T0`, la poignée suit le bas de la page et l'outil courant n'a pas
  changé ; **trois Annuler** (une étape par clic) rendent 768. Deux zones horizontales réglées : comportement conforme
  à l'exigence écrite par `L4-T0` (contrôle de pixels) ; aller-retour `.actcraft` : même empreinte.
- **S16 — Texte de haut en bas (lot 4).** Poser un texte « ABC » de haut en bas par touches injectées : les trois
  lettres sont rendues l'une sous l'autre (contrôle de pixels sur trois bandes horizontales) ; gras appliqué par sa
  barre ; rouvert après enregistrement, identique.
- **S17 — Imprimer (lot 5, Windows).** Un document synthétique de 4 pages, dont la page 3 allongée et des notes sur les
  pages 2 et 4 : `file.print` réglé à 2 pages par feuille, en gris, plage 2 à 4, notes incluses, vers l'imprimante
  virtuelle du système qui écrit un fichier. Le fichier compte les feuilles de pages **plus** les feuilles de notes,
  au nombre calculé par la règle écrite par `L5-T0` ; aucune couleur (contrôle de pixels) ; la page allongée tient
  entière sur sa feuille. L'aperçu avant impression montre les mêmes feuilles. Réglages enregistrés : relancer,
  rouvrir la boîte, ils sont là. Sous Android, la commande produit toujours un PDF.
- **S18 — Diffusion protégée (lot 8).** Protéger un document en interdisant la copie et l'impression : un nouveau
  fichier est écrit, l'original reste intact ; rouvert, copier un objet et imprimer affichent le message de refus,
  glisser un objet vers les ressources est refusé, et une action de page fonctionne toujours.
- **S19 — Mode classe protégé (lot 6, rejoué au lot 11).** Créer un mot de passe, verrouiller : toutes les commandes
  marquées « verrouillables » sont grisées ; un mauvais mot de passe ne déverrouille pas ; le bon, si ; relancer :
  l'état verrouillé est gardé et le mot de passe n'est jamais en clair dans les réglages. Rejoué au lot 11 sur toutes
  les commandes des lots 1 à 11.
- **S20 — Raccourci vers un programme (lot 6, Windows).** Un programme d'essai écrit par nous (dans `tests/`) écrit
  un fichier témoin sous `.tmp/` quand on le lance. Ajouter un raccourci vers lui : il apparaît dans la barre de
  raccourcis avec son info-bulle ; un clic crée le fichier témoin ; le supprimer : il disparaît. Un raccourci lu dans
  un profil copié d'un autre poste (signature qui ne vérifie pas, N1-A6) demande une confirmation : refusée, aucun
  fichier témoin. Sous Android, la commande d'édition est absente (capture).
- **S21 — Orthographe (lot 6).** Un document de 2 pages contient « bonjour mosieur » : la vérification affiche la
  page 1, propose « monsieur », remplacer corrige le texte ; ignorer toujours un autre mot ne le signale plus ;
  message de fin.
- **S22 — Effets de page (lot 7).** Pour chaque effet du catalogue : l'effet choisi comme global, passer de la page 1
  à la page 2, banc en D3D11 plein écran pendant la transition (aucune image au-delà de la limite). Puis un effet
  propre posé sur la **page 1**, différent du global : passer de 1 à 2 joue l'effet de la page 1 (l'effet se joue en
  quittant la page). Navigation par le navigateur de pages : aucun effet joué.
- **S23 — Message défilant (lot 7).** Un message en bas d'écran, sans boucle : il traverse l'écran une fois puis
  disparaît ; en boucle, il recommence ; le banc mesure l'écriture au stylet pendant le défilement (aucune reprise hors
  limite).
- **S24 — Équation (lot 7).** Insérer une fraction depuis les modèles, saisir 1 et 2 : l'objet s'affiche sur la page ;
  double-clic : l'éditeur rouvre la même équation ; aller-retour `.actcraft` identique ; une équation lue d'un
  `.flipchart` synthétique s'affiche.
- **S25 — Médias intégrés (lot 8).** Un son placé sur la page, cliqué : le lecteur de sons le lit (position qui
  avance) ; une vidéo joue dans son cadre pendant qu'on écrit au stylet, banc en D3D11 : aucune régression des
  critères de fluidité ; un GIF animé change d'image.
- **S26 — Enregistreurs (lot 8).** Windows : enregistrement d'écran de 5 s d'une zone : l'indicateur d'enregistrement
  est visible pendant toute la durée (capture), le fichier existe, sa durée est de 5 s ± 0,5, et l'écriture pendant
  l'enregistrement reste dans le budget du banc ; enregistrement de son de 3 s : indicateur visible, fichier présent,
  icône de lecture posée sur la page. Android : permission du micro refusée sur l'émulateur → la commande se désactive
  avec un message, sans plantage ; accordée → même résultat que sous Windows.
- **S27 — Cadre web (lot 8).** Un cadre web avec un HTML local d'essai : le contenu s'affiche, se redimensionne,
  recharge ; un `<img src="file://…">` ou un lien vers un partage réseau du HTML n'est pas chargé (politique de
  N1-A3) ; la capture d'image fixe, par le moyen retenu en `L8-T0`, pose une image sur la page ; enregistré puis
  rouvert, le cadre est là. Rideau ouvert : son comportement au-dessus du cadre est celui écrit par `L8-T0`. Le cadre
  est créé par la saisie d'un code HTML. Un document d'essai « reçu » dont le cadre pointe vers un serveur local de
  test hors du dossier du document : à l'ouverture, aucune requête n'atteint le serveur (journal du serveur vide) et le
  repère de blocage est visible ; déblocage confirmé : le contenu s'affiche et le serveur reçoit la requête ; une autre
  page du même document reste bloquée.
- **S28 — Deux utilisateurs et toucher (lot 11).** Mode à deux utilisateurs activé : la deuxième palette apparaît ;
  deux pointeurs injectés, rattachés à deux utilisateurs, tracent en même temps deux traits de couleurs et d'épaisseurs
  différentes ; chacun garde sa sélection ; une règle ouverte par l'un n'apparaît pas chez l'autre ; une commande qui
  ouvre une boîte de dialogue est sans effet depuis le second ; un pincement ne zoome plus ; un objet tenu par l'un ne
  bouge pas sous l'autre ; mode désactivé : le pincement zoome. Pour chacun des quatre modes du toucher, un doigt
  injecté produit l'effet attendu (rien, trait, sélection, comportement complet).
- **S29 — Aide, langue et mises à jour (lot 11).** F1 ouvre l'aide hors ligne sur la page de l'outil courant, sans
  réseau. Une pseudo-langue d'essai (chaque libellé encadré de marques, générée par nous) chargée par le réglage :
  captures à l'écran de la fenêtre principale, des menus et de trois boîtes de dialogue ; aucune chaîne visible n'est
  restée sans marque (lecture des textes affichés). Windows : la recherche de mises à jour contre une réponse de
  releases simulée annonce la version plus récente, et ne fait aucune requête au démarrage tant que le réglage est
  désactivé.
- **S30 — Tâche d'ouverture (chaque lot 3 à 8 et 11).** Le rapport `docs/verifications/<date>-lot<x>-principes.md`
  existe et cite le chemin du relevé brut `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot<x>.md`, qui existe ;
  chaque constat du lot (§ 4) y a une ligne (exigence, tâche, scénario, ou raison) ; `git grep` des titres de la table
  de correspondance du corpus ne trouve rien ; `git grep` des noms de fichiers du jeu d'essai, **lus dans le dossier
  `samples/` hors dépôt** (jamais écrits dans le dépôt), ne trouve rien ; `NOMS OK`.
- **S31 — Objet de type inconnu (lot 2, `L2-C8`).** Un `.actcraft` synthétique dont une page contient, entre deux
  objets connus, un objet d'un type que la version ne connaît pas (avec une clé inconnue et une ressource image qu'il
  référence) : ouvert, la page s'affiche avec un repère neutre à la place de l'objet et un avertissement de lecture ;
  un trait ajouté, enregistré sous un autre nom : les octets JSON de l'objet inconnu sont identiques à l'entrée, il
  reste à la même place dans l'ordre des couches et de profondeur, la ressource est toujours dans l'archive, et la clé
  inconnue du document est conservée.
- **S32 — Profils fournis (lot 11).** Pour chacun des cinq profils : l'activer depuis un profil vierge ; les outils
  qu'il épingle, ceux des lots 7 et 8 compris, sont tous présents dans la palette et la barre de raccourcis (lecture
  du modèle de disposition et capture) ; les navigateurs et réglages qu'il fixe sont appliqués ; aucune commande
  absente de la plateforme n'y figure (Android).

## 8. Hypothèses à revoir créées par l'amendement

### Niveau 1 — difficiles à défaire ou visibles des utilisateurs

- **N1-A1 — Volume.** Estimation brute : + 69 tâches (134 → 203, dont `L2-C8`), sans les imports. Les lots 1 et 2 ont dépassé leur
  estimation de 56 % (16 → 25) et de 88 % (16 → 30) ; appliqué aux 170 tâches restantes, ce rythme donne 265 à 320
  tâches, soit **≈ 320 à 375 au total** avec les 56 faites ou engagées (lots 1 et 2, `L2-C8` compris). La note 0100 annonçait 20 à 25 tâches de plus, sans
  les tâches d'ouverture, l'équation, les enregistreurs ni le cadre web. Lots 5 à 8 en deux sous-lots.
- **N1-A2 — Permissions Android.** La décision 28 (« aucune permission ») est amendée pour le seul enregistreur de son :
  le micro est demandé à la première utilisation ; refusé, la commande se désactive. L'enregistreur d'écran est
  **Windows seul** (sous Android, il exigerait la capture d'écran du système et un service de premier plan).
- **N1-A3 — Cadre web (F-302).** Sous Windows, un moteur du système par Qt WebView s'il existe pour Qt 6.12, sinon Qt
  WebEngine : ≈ + 150 Mo **installés** (fichiers sur le disque), soit un installateur compressé plusieurs fois plus
  lourd que les 25,5 Mio d'aujourd'hui (jusqu'à ≈ 7 fois si la compression gagne peu) — **non vérifié, à mesurer en
  `L8-T0`** (taille installée et taille de l'installateur, avec et sans le moteur). Qt WebView est une
  vue native posée **au-dessus** de la scène Qt Quick : pas de capture par `grabToImage`, et le rideau, le spot et les
  autres surcouches ne la couvrent pas (non vérifié en 6.12, à vérifier en `L8-T0`). Sous Android, vue web du système,
  contenu local seulement (pas de permission réseau ; un lien distant s'ouvre dans le navigateur). **Politique de
  contenu**, sur les deux plateformes : JavaScript désactivé par défaut ; aucun contenu distant chargé sans une
  adresse saisie par l'utilisateur ; aucun `file://` hors du dossier de ressources du document ; aucun chemin réseau
  (SMB, `file://hôte/…`) — même garde que le `QTextDocument` neutralisé de L2-T11. Un document **reçu** dont un cadre
  web embarque du contenu distant l'ouvre bloqué : le cadre montre un repère et un bouton de déblocage, qui demande
  une confirmation et ne vaut que pour cette page de ce document. **Question posée à Jérôme à la
  porte de fin du lot 2** : reprendre F-302 au prix du poids, ou le réserver à une option de construction.
- **N1-A4 — Restrictions de diffusion.** Sur un format ouvert, elles sont une courtoisie : un utilisateur averti peut
  les retirer en éditant le JSON. Pas de chiffrement.
- **N1-A5 — Mot de passe du mode classe protégé.** Aucun mot de passe par défaut (l'original en a un, connu de tous) :
  on le crée au premier verrouillage. Haché avec sel : PBKDF2-SHA256 par `QPasswordDigestor` (dans QtNetwork, sous la
  même licence que Qt), ou Argon2id par une bibliothèque à licence compatible (par exemple libsodium, ISC) si sa
  revue de licence passe ; choix en `L6-T0`. Oublié : réinitialisation en supprimant le réglage, procédure dans l'aide.
- **N1-A6 — Raccourcis vers des programmes.** Ils lancent un programme du poste : le chemin est affiché à la création.
  Chaque raccourci est signé (HMAC) par une clé propre au poste, créée au premier lancement et rangée dans le trousseau
  du système ; un raccourci dont la signature ne vérifie pas (profil copié d'un autre poste) demande une confirmation
  au premier lancement, puis est signé à nouveau.
- **N1-A8 — Imports en attente.** Tant que Jérôme n'a pas tranché le § 5 (question posée à la porte de fin du lot 2),
  le menu d'import et le tableau de bord n'offrent que le PDF.
- **N1-A10 — Taille et licences des paquets.** Qt Multimedia (et son moteur FFmpeg), Qt Print Support (qui tire
  QtWidgets, Windows seulement), Hunspell et un dictionnaire français, une bibliothèque de trousseau, peut-être Qt
  WebEngine : chacun passe par la liste des bibliothèques autorisées et `THIRD_PARTY_NOTICES.md` ; la croissance de
  l'APK et de l'installateur est rapportée à chaque porte de lot.
- **N1-A11 — F-302 repris sans demande explicite.** Au nom du principe ; retirable par l'option de fonctions
  (`ACTCRAFT_FEATURES`). Question jointe à N1-A3, porte de fin du lot 2.
- **N1-A12 — Mises à jour.** Recherche manuelle par le menu d'aide ; recherche au démarrage désactivée par défaut
  (réseau, vie privée) ; source : les releases du dépôt public ; Android : rien (une mise à jour = un nouvel APK).
- **N1-A14 — Apparence simplifiée de l'original pour les petites classes.** Non reprise (design libre) ; un profil
  fourni par nous en tient lieu. Question pour Jérôme à la porte du lot 6.
- **N1-A15 — Exclusions supplémentaires à la décision 0100.** Flash et liens vers les services de Promethean (§ 1) :
  présentées à Jérôme à la porte de fin du lot 2.
- **N1-A16 — Vie privée des enregistreurs.** Un indicateur reste visible à l'écran pendant tout enregistrement d'écran
  ou de son ; la première utilisation affiche une mention : ce qui est enregistré, où le fichier est rangé, rien
  n'est envoyé.

### Niveau 2 — les autres

N1-A7, N1-A9 et N1-A13 de la première version sont passées au niveau 2 (N2-A9 à N2-A11).

- **N2-A1** Références `Vnn` = numéros des fiches du corpus (au lieu d'une nouvelle numérotation V01… au lot 9).
- **N2-A2** Texte de haut en bas sur les deux plateformes (l'original : Windows seulement).
- **N2-A3** Correcteur : dictionnaire français seulement livré ; autres dictionnaires ajoutables par l'utilisateur.
- **N2-A4** Aide hors ligne en français seulement, écrite par nous, sans capture de l'original.
- **N2-A5** Mode du toucher par défaut : gestion complète (comportement de la décision 25).
- **N2-A6** Noms et dessin des effets de page à nous ; la liste suit celle de l'original élucidée par REA.
- **N2-A7** Deuxième palette non personnalisable, comme l'original.
- **N2-A8** Impression complète derrière une option de construction (`ACTCRAFT_PRINT`), construite sous Windows
  seulement.
- **N2-A9** Formats des enregistreurs : écran en `.mp4` (encodeurs du système par Qt Multimedia) au lieu de `.avi` ;
  son en `.wav` comme l'original.
- **N2-A10** Vidéos de versions anciennes : le corpus montre les versions 1.4 à 1.6 ; un principe non confirmé par la
  3.5.18 n'est pas repris. L'observation de l'original à l'écran prévient Jérôme avant chaque séance.
- **N2-A11** Adresse d'accueil du navigateur web : la page du projet par défaut ; réglable.
- **N2-A12** Écart F-583 (données de page), constaté à la clôture du lot 2. Les cadres de page (F-603) et l'effet de
  changement de page propre à une page (F-640), lus d'un `.flipchart`, ne sont pas dans le modèle de document au lot 2 :
  ils sont donc perdus à l'enregistrement en `.actcraft`. Hypothèse : cette perte est acceptée jusqu'aux lots 4
  (cadres) et 7 (effets), où le lecteur `.flipchart` devra les lire et le format natif les porter. À revoir si un
  paperboard réel les utilise avant.

### Décisions d'architecture de l'amendement

**DA-A1 — Entrées rattachées à un utilisateur ; routage unique du doigt.**
Contexte : quatre modes du toucher et mode à deux utilisateurs (lot 11) ; F-303 veut pour chaque utilisateur son
outil, ses réglages, sa sélection et ses outils de maths, et interdit les boîtes de dialogue au second
(`fonctionnalites.md:385`) ; les gestes arrivent au lot 3.
Décision : au lot 3, chaque pointeur est rattaché à un utilisateur (un seul tant que le mode est éteint) ; l'utilisateur
porte {outil courant, réglages d'outil, sélection, outils de maths ouverts} ; tout événement de doigt passe par une
politique unique (mode courant → outil, sélection, geste ou rien) ; les commandes qui ouvrent une boîte refusent un
utilisateur second. Le schéma d'entrée (quels périphériques font le second utilisateur) est tranché en `L3-T0`.
Conséquences : le lot 11 ajoute un utilisateur et des politiques sans réécrire les outils ; jusque-là, un seul
utilisateur et le comportement de la décision 25.
Réversible : interne à `input/`, `tools/` et au registre de commandes.

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
