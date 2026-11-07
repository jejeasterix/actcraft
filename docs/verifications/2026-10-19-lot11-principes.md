# Lot 11 — ouverture (L11-T0) : principes, décisions et API revérifiées

- Tâche : L11-T0 du chantier `livraison-complete`, lot 11 (plan
  `docs/plans/2026-10-18-livraison-complete-lot11-PLAN.md`, sous-lots 11a et 11b).
- Branche : `feat/livraison-complete-l11--L11-T0`, base `feat/livraison-complete-l11` à `c263919d` (lots 1 à 6, 8 et
  10 livrés ; **lot 7 absent**).
- Poste : Mac (Qt Homebrew, `build-mac`, Debug, banc activé), 2026-11-07.
- Relevé brut hors dépôt : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot11.md` — **non écrit** (ancien dépôt
  absent du Mac) : reste du PC (section « Restes pour le PC »).

## Méthode et limites

Le plan a été écrit le 2026-10-18, avant les lots 4 à 8. Le patron écarté est celui qui recopierait la table A du plan
en « conforme » : chaque ligne ci-dessous a été relue sur le code de la tête (`c263919d`), par le graphe du worktree
puis par les en-têtes eux-mêmes ; un nom absent est dit absent, avec la tâche qui doit le créer.

Joué sur le Mac : étapes 4 (API), 5 (N11), 6 (commandes de fichier sans `android`), 7 (ce rapport).

Non joué sur le Mac (décision de l'orchestrateur) : étapes 1 (corpus vidéo), 2 (spécification `fonctionnalites.md`),
3 (REA en statique sur le binaire) et le relevé brut. Chaque point « à élucider » du plan prend ici le défaut du plan,
écrit comme hypothèse H11-n « à confronter au PC ».

## État de ce rapport

Premier jalon : table A partielle. Les lignes marquées « à relire » sont complétées au jalon suivant.

## Table A — API revérifiées sur le code réel (`c263919d`)

| # | Verdict | Signature ou état réel (fichier:ligne) | Écart au plan et section à amender |
|---|---|---|---|
| A1 | conforme | `input/include/actcraft/input/routing.h` : `using UserId = std::uint8_t` (:12) ; `RouterState{… gestureTouches}` (:23-27) ; `Decision{Route route = Route::Ignore; UserId user = kFirstUser; bool endToolContacts}` (:30-35) ; `RoutingPolicy::decide(const Sample& press, const RouterState& state) const = 0` (:44) ; `FullTouchPolicy final` (:50-53) ; `PointerRouter::setPolicy(std::unique_ptr<RoutingPolicy>)` (:60), `route(const Sample&) → Decision` (:66), `gestureTouches() const noexcept` (:67), `reset()` (:68), `setCoverTest(std::function<bool(const Sample&)>)` (:70), `release(std::uint64_t pointer)` (:73) | aucun |
| A2 | écart | `input/include/actcraft/input/sample.h` : `struct Sample` (:23), `timestampNs` (:28), `seq` (:31), `pointer` (:33, stylet et souris 0, doigt identifiant + 1), **`std::uint8_t user = 0` (:34, « rempli par le routeur », DA-A1)** ; `DeviceKind` (:7), `Phase` (:15) ; aucun `deviceId` | le champ `user` existe déjà (le plan ne le cite pas) : L11-T4, L11-T5, L11-T7 s'en servent au lieu d'en créer un ; `deviceId` reste à ajouter par L11-T4 (M5) |
| A3 | écart | `ui/include/actcraft/ui/toolhost.h` : `tools::Tool* activeTool(input::UserId user = input::kFirstUser) const` (:143) ; `tools::UserState& user(input::UserId id = input::kFirstUser)` (:144, « un utilisateur jusqu'au lot 11 (D-S10) ») ; `tools/include/actcraft/tools/userstate.h` : `openMathTools() const noexcept → const std::vector<QString>&` (:53, « réservé au lot 7 »), `setOpenMathTools(std::vector<QString>)` (:54) ; `ToolContext::user` est une `UserState&` (`toolcontext.h:125`) | `activeTool` prend déjà l'utilisateur (le plan l'écrit sans argument) ; `id()` : à relire |
| A4 | à relire | `PageView::contactActive() const noexcept` (`pageview.h:351`) ; `endToolContacts(const input::Decision&)` (`pageview.h:269`) ; commentaire `pageview.h:175` : les échantillons vont à `ToolHost::activeTool(utilisateur du contact)` ; ni `finishContacts`, ni `setRoutingPolicy`, ni `lastContactUser` | `finishContacts`, `setRoutingPolicy`, `lastContactUser` sont à créer par L11-T5 (conforme au plan) |
| A5 | à relire | `actions/include/actcraft/actions/commandregistry.h` : `platforms = kAllPlatforms` (:51), `lockable = true` (:58), `restrictable = false` (:59), `isOwnEnabled` (:86), `setLocked`/`isLocked` (:92-93) ; `ui/include/actcraft/ui/commands.h` : `setNativeHandler(const QString&, CommandRegistry::Handler)` (:74), `setLocked`/`isLocked`/`lockedCount` (:91-93) | — |
| A7 | à relire | `ui::ProfileController::activate(const QString&) → bool` (`profilecontroller.h:70`), `currentId()` (:94) ; `actions::ProfileStore` (`profilestore.h:14`) | — |
| A8 | à relire | `ui::HelpViewer::openFor(QString toolId)` (`helpviewer.h:26`), `follow(QUrl)` (:28) | — |
| A9 | à relire | `static bool ui::Language::install(QCoreApplication&, Preferences&)` (`language.h:35`) | — |
| A12 | à relire | `bench/src/uiscenario.cpp` : `command` (+`argument`), `stroke`, `key`, `wait`, `waitReady`, `capture`, `checkPixel`, `checkEdge`, `expect` (`above`, `tolerance`), `fixture`, `waitFor`, `watchEventLoop`, `maxEventLoopGapMs`, `remember`/`as`, `archive`, `file`, `restart`, `touch`, `pinch`, `pointer` (`button`, `modifiers`, `holdMs`), `doubleClick`, `tap`, `typeText` (:159-474) | étapes ajoutées depuis le plan : à relire |
| A20 | à relire | `app/startup.h` : `struct StartupOptions` (:10), `profileRoot` (:19), `loadMainWindow(QQmlApplicationEngine&, const StartupOptions&)` (:32) ; pas de `recoveryDir` | `recoveryDir` à créer par L11-T13 (M9, conforme) |

## N11

`QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac -N` à `c263919d` : **`Total Tests: 246`** (Mac, Debug, banc
activé). À recompter au PC (`ctest --preset debug -N`) : la valeur de référence du plan est celle du PC.
