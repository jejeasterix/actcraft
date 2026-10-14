# Points à traiter

Liste tenue à jour entre les sessions. **À lire en début de session**, après `docs/BRIEF.md`. Cocher ou retirer
un point une fois traité, avec la date et la preuve (commit, PR, mesure).

Ouverte le 2026-10-14 (passage à la licence PolyForm Noncommercial, PR #2).

## Licence et propriété intellectuelle

- [ ] **Description du dépôt GitHub** : elle dit encore « Logiciel libre de tableau blanc interactif ». À
  remplacer à la main dans *Settings → About* (Jérôme), par exemple « Tableau blanc interactif au code source public
  (C++20 / Qt 6 Quick) ». Plus aucun texte ne doit dire « libre » ni « open source » : PolyForm Noncommercial n'est
  pas une licence open source au sens de l'OSI.
- [ ] **Contributions externes : accord de contribution (CLA) avant la première PR d'un tiers.** Sans cession ou
  licence explicite du contributeur, Jérôme ne peut pas accorder d'autorisation commerciale sur le code de ce
  contributeur. Écrire `CONTRIBUTING.md` avec le CLA ; refuser toute PR externe d'ici là.
- [ ] **Validation juridique avant toute diffusion commerciale** : la licence et l'interopérabilité `.flipchart`
  (directive 2009/24/CE, art. 5.3, 6 et 8 ; CPI art. L122-6-1).
- [ ] **Statut de salarié (CPI art. L113-9)** : un logiciel créé « dans l'exercice des fonctions ou d'après les
  instructions de l'employeur » appartient à l'employeur. Garder la preuve qu'ActCraft est développé hors du temps
  de travail, sur du matériel personnel ; ne jamais commiter depuis un poste ou un compte de l'entreprise.
- [ ] **Communication publique** : le dépôt est public et `CLAUDE.md` décrit ActCraft comme « clone d'ActivInspire
  Studio 3.5.18 construit par rétro-ingénierie ». Préférer « compatible avec les fichiers .flipchart » dans tout
  texte public (risque de marque, et de parasitisme si l'interface est trop proche). Décision à prendre par
  Jérôme : reformuler `CLAUDE.md`, ou le garder tel quel.
- [ ] **Oracle de l'étape 3** : les JSON tirés des paperboards de `samples/` dérivent de contenus de tiers. Ne pas
  les commiter dans ce dépôt public ; produire des paperboards d'essai à nous, ou garder l'oracle en local.
- [ ] **Licences des ressources tierces** (icônes, polices) : vérifier qu'elles permettent la redistribution sous
  PolyForm Noncommercial et un usage commercial sous accord écrit (par exemple Lucide sous ISC, Tabler sous MIT).
  Consigner chaque ressource et sa licence dans un fichier `THIRD-PARTY.md`.

## Prototype de mesure (étapes 1 et 2 du BRIEF)

- [ ] **Décision N1, en attente de Jérôme** : 1 figement de 64,4 ms sur 10 traits (seuil < 41,7 ms), quand un
  trait terminé passe dans le cache de la page (`docs/mesures/2026-10-13-prototype-mesure.md`). (a) Tolérer jusqu'à
  4 images par trait terminé et le compter à part, ou (b) garder le seuil et traiter le redessin du cache dans un
  chantier dédié.
- [ ] **Mesurer sur les machines de référence** (Dell Inspiron 3510, MacBook Air M1) avant de trancher N1 :
  donner à Jérôme la procédure exacte (`docs/banc.md`).
- [ ] **Étape 2 : décision go / no-go**, à consigner dans le coffre (`02_Decisions/`).
- [ ] **Stylet réel** : `tablette-element: non` (`docs/verifications/2026-10-13-qt612.md`). La variante B (filtre
  posé sur la fenêtre) n'est vérifiée que pour un événement posté. À confirmer avec un vrai stylet.

## Distribution (.exe, .dmg, .apk, .ipa)

- [ ] **Aucun paquet n'est produit par la CI** (compilation et tests seulement). À prévoir dans un chantier dédié :
  - Windows : `windeployqt` + installateur (Inno Setup, NSIS ou MSIX) ; certificat de signature de code contre
    l'avertissement SmartScreen (dépense : décision de Jérôme).
  - macOS : `macdeployqt -dmg` ; compte Apple Developer (99 $/an), signature Developer ID et notarisation.
  - Android : `androiddeployqt` (APK pour les essais, AAB pour le Play Store) ; SDK, NDK et JDK dans la CI ;
    interface tactile à adapter.
  - iOS : Qt lié statiquement, donc LGPLv3 : fournir de quoi relier l'application à une autre version de Qt
    (fichiers objets), ou prendre une licence commerciale de Qt.

## Entretien

- [ ] **`graphify update .` après le merge de la PR #2** (outil absent de la session qui l'a ouverte), puis
  commit de `graphify-out/`.
- [ ] **Coffre `C:\SecondBrain`** : consigner la décision de licence (`02_Decisions/`, étiquette `projet/actcraft`),
  pas encore fait (coffre inaccessible depuis une session cloud).
