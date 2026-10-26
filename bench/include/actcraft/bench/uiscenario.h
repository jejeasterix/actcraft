#pragma once

#include <QColor>
#include <QEvent>
#include <QHash>
#include <QPointF>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

class QObject;
class QPointingDevice;
class QQuickWindow;
class QWindow;

namespace actcraft::ui {
class PageView;
}

namespace actcraft::bench {

class EventLoopProbe;
class Injector;

enum class UiStepKind : std::uint8_t
{
    Command,    // {"command": "page.next", "argument": …}
    Stroke,     // {"stroke": [[x, y], …], "pressure": 0.5} — coordonnées de page
    Key,        // {"key": "F5"} — texte de QKeySequence
    Wait,       // {"wait": 300} — millisecondes
    WaitReady,  // {"waitReady": true} — page fixe prête et cache à jour (10 s au plus)
    Capture,    // {"capture": "nom"}
    CheckPixel, // {"checkPixel": "nom", "at": [x, y], "color": "#rrggbb", "tolerance": 48}
    Expect,     // {"expect": "objet.propriété", "value": …}
    // Ajoutées au lot 2 (L2-T20, S5), à la fin : les rangs ci-dessus ne changent pas.
    Fixture, // {"fixture": "trois-pages", "path": "${SORTIE}/a.actcraft"} — sous le dossier de sortie
    WaitFor, // {"waitFor": "objet.propriété", "value": …, "timeoutMs": 10000} — 1 ms à 2 min
    // Fenêtre de boucle : de « watchEventLoop » à « maxEventLoopGapMs ». Plus long intervalle au-dessus du
    // seuil : la fenêtre est rejouée une fois, seuil entier, et n'échoue (ECART) que si l'écart se reproduit.
    // La charge de la machine ne se reproduit pas ; un blocage du fil principal par l'étape jouée (E/S
    // synchrone), si (L10-C2). Les étapes de la fenêtre doivent donc pouvoir être rejouées.
    WatchEventLoop,  // {"watchEventLoop": true} — démarre la sonde de la boucle du fil principal
    MaxEventLoopGap, // {"maxEventLoopGapMs": 25} — arrête la sonde, compare le plus long intervalle ;
                     // "informative": true — l'écart est écrit (« HORS SEUIL … (informatif) »), jamais jugé
                     // ni rejoué (S5, L3-C4 : une preuve sans chronomètre garde l'invariant, tst_ui_files)
    Remember,        // {"remember": "objet.propriété", "as": "nom"} — valeur de ${nom} ensuite
    ArchiveContains, // {"archive": "<chemin>", "contains": ["document.json", "resources/", …]} — préfixes
    // Ajoutée au lot 10 (L10-T5, S33), à la fin.
    CheckEdge, // {"checkEdge": "nom", "from": [x, y], "to": [x, y], "minIntermediate": 2, "maxIntermediate":
               // 0, "minInk": 1}
    // Ajoutées par le socle des lots 3 et 7 (L0-S2), à la fin, dans cet ordre.
    Touch,       // {"touch": [[x, y], …]} — un doigt, coordonnées de page
    Pinch,       // {"pinch": [cx, cy], "from": 100, "to": 200, "steps": 12} — écarts en pixels de vue
    Pointer,     // {"pointer": [[x, y], …], "button": "left", "holdMs": 0, "space": "page"}
    DoubleClick, // {"doubleClick": [x, y], "space": "page"}
    // Ajoutée par L4-T14, à la fin.
    TypeText, // {"typeText": "Bonjour"} — un QKeyEvent par caractère, avec son texte (dans `value`)
};

struct UiStep
{
    UiStepKind kind = UiStepKind::Wait;
    QString command;
    QVariant argument; // invalide : commande sans argument
    std::vector<QPointF> points;
    double pressure = 0.5;
    QString key;
    int ms = 0;
    QString name;
    QPointF at;
    QColor color;
    int tolerance = 48; // par canal
    QString target;     // objectName, ou « window » pour la fenêtre racine
    QString property;
    QString value;      // attendue, en texte (propertyText)
    QString path;       // Fixture, ArchiveContains
    QStringList values; // ArchiveContains : préfixes d'entrées attendus
    QPointF to;         // CheckEdge : fin du segment (début : at)
    int minIntermediate = 0;
    int maxIntermediate = -1; // -1 : sans plafond
    int minInk = 0;           // pixels d'encre exigés sur le segment (contrôle positif d'un contrôle négatif)
    // Ajoutés par L0-S2. Pinch : centre dans `at`. Pointer : points dans `points`. DoubleClick : point dans
    // `at`.
    double pinchFrom = 0;                    // Pinch : écart initial des deux doigts, pixels de vue (> 0)
    double pinchTo = 0;                      // Pinch : écart final (> 0)
    int pinchSteps = 12;                     // Pinch : pas d'une image (1 à 1000)
    Qt::MouseButton button = Qt::LeftButton; // Pointer
    int holdMs = 0;                          // Pointer : maintien au dernier point avant le relâché
    bool windowSpace = false;                // Pointer, DoubleClick : points en fenêtre plutôt qu'en page
    std::optional<double>
        valueTolerance; // Expect : écart admis, valeur numérique ; absent : comparaison exacte
    // Ajouté par L3-C4. MaxEventLoopGap : mesure écrite, l'écart au seuil ne fait pas échouer le scénario.
    bool informative = false;
};

struct UiScenario
{
    QString name;
    QString description;
    std::vector<UiStep> steps;
};

// Fichier « actcraft-scenario/1 » ; tout ou rien, `error` nomme l'étape fautive (« étape 3 : … »).
bool parseUiScenario(const QByteArray& json, UiScenario& out, QString& error);
// « objet.propriété=valeur » (premier point, premier signe =) → étape Expect ; faux si une partie manque.
bool parseReadSpec(const QString& text, UiStep& out);
// Valeur d'une propriété en texte : booléen « true »/« false », réel sans zéros inutiles, couleur
// « #rrggbb », « <absente> » si invalide, « <non textuel : Type> » si elle ne se convertit pas en texte.
QString propertyText(const QVariant& value);

// Joue un scénario sur une fenêtre Qt Quick (DA8). Une ligne de rapport par étape : COMMANDE, TRAIT, TOUCHE,
// ATTENTE, PRET, CAPTURE, PIXEL, LU, FIXTURE, BOUCLE, MEMO, ARCHIVE, BORD, DOIGT, PINCEMENT, POINTEUR,
// DOUBLECLIC (ces quatre : événements livrés à la fenêtre, comptés par un témoin). Chaque étape d'action
// contrôle son résultat, jamais « OK » sur le seul envoi : COMMANDE exige le retour vrai de runCommand ;
// TRAIT, postés à la fenêtre (le filtre de la PageView les y attend, variante B), exige un trait figé par la
// vue ; TOUCHE exige l'appui accepté par un raccourci non ambigu ou par un élément actif (repli sinon : la
// commande nommée équivalente) ; CAPTURE refuse une image vide ou uniforme ; FIXTURE n'écrit que sous le
// dossier de sortie. Lignes terminées par OK, ECHEC ou ECART. Aucune attente à durée fixe hors de l'étape «
// wait » : conditions avec plafond (ATTENTE d'une propriété comprise). Variables, remplacées en une passe
// dans l'argument textuel d'une commande, les chemins et les valeurs attendues : ${SORTIE} (dossier de
// sortie), ${SAMPLES} (ACTCRAFT_SAMPLES_DIR ; absente, l'étape échoue),
// ${<nom>} (mémorisé par « remember ») ; une variable inconnue ou non fermée fait échouer l'étape.
// Sonde de boucle (BOUCLE, écart 1 du plan 2b) : une minuterie de 5 ms sur le fil principal relève le plus
// long intervalle entre deux de ses tours, intervalle en cours à l'arrêt compris. Elle tient hors écran, où
// les images ne sont produites qu'à la demande : un blocage sans image la dépasse aussi. Son démarrage attend
// d'abord que l'image en attente soit rendue (une image demandée, son frameSwapped, 1 s au plus ; sinon «
// BOUCLE sonde démarrée (aucune image rendue en 1000 ms) ») : elle mesure la boucle pendant les étapes
// suivantes (l'E/S de S5), pas le rendu laissé par l'étape précédente (trait), qui prend à lui seul 11 à 147
// ms sur le fil principal en rendu logiciel debug (mesuré, L2-T20). DURÉE DE VIE : le joueur possède le
// stylet synthétique ; il doit être détruit APRÈS la fenêtre (le déclarer avant le moteur QML).
class UiScenarioPlayer
{
public:
    explicit UiScenarioPlayer(QQuickWindow* window);
    ~UiScenarioPlayer();
    UiScenarioPlayer(const UiScenarioPlayer&) = delete;
    UiScenarioPlayer& operator=(const UiScenarioPlayer&) = delete;

    using LineSink = std::function<void(const QString&)>;
    // Reçoit chaque ligne du rapport dès qu'elle est écrite (sortie au fil de l'eau d'actcraft-captures).
    void setLineSink(LineSink sink);

    // Livraison d'un appui ou d'un relâché à la fenêtre ; vrai si la touche a été acceptée. Par défaut
    // QWindowSystemInterface::handleKeyEvent, synchrone : le chemin d'une vraie touche après la plateforme.
    // Remplaçable pour les tests seulement, pour simuler une plateforme dont la livraison ne consulte pas la
    // carte des raccourcis (macOS, voir key()).
    using KeyDelivery = std::function<bool(QWindow* window, ulong timestamp, QEvent::Type type, int key,
                                           Qt::KeyboardModifiers modifiers, const QString& text)>;
    void setKeyDelivery(KeyDelivery delivery);

    // S'arrête à la première étape en échec. Captures et index.md dans `outputDir` (créé).
    bool play(const UiScenario& scenario, const QString& outputDir, QStringList& report);
    // Lecture d'une propriété (étape Expect) : « LU cible.propriété=valeur OK » ou « … ECART (attendu …) ».
    bool expect(const UiStep& step, QStringList& report);

private:
    bool runStep(const UiStep& step, QStringList& report);
    bool read(const UiStep& step, QStringList& report); // expect() sans sortie au fil de l'eau
    void emitFrom(const QStringList& report, qsizetype first) const;
    QObject* find(const QString& target) const;
    ui::PageView* view() const;
    bool isReady() const;
    // Conventions d'une vraie tablette (comme l'injecteur du banc) : bouton seulement à l'appui et au
    // relâché.
    void postTablet(QEvent::Type type, QPointF page, double pressure, Qt::MouseButton button,
                    Qt::MouseButtons buttons);
    bool stroke(const UiStep& step, QStringList& report);
    bool key(const UiStep& step, QStringList& report);
    bool checkPixel(const UiStep& step, QStringList& report);
    bool checkEdge(const UiStep& step, QStringList& report);
    bool touch(const UiStep& step, QStringList& report);
    bool pinch(const UiStep& step, QStringList& report);
    bool pointer(const UiStep& step, QStringList& report);
    bool doubleClick(const UiStep& step, QStringList& report);
    bool typeText(const UiStep& step, QStringList& report);
    Injector& gestures(); // injecteur des gestes directs, créé à la première étape qui en a besoin
    bool fixture(const UiStep& step, QStringList& report);
    bool waitFor(const UiStep& step, QStringList& report);
    bool watchEventLoop(QStringList& report);
    bool maxEventLoopGap(const UiStep& step, QStringList& report);
    bool remember(const UiStep& step, QStringList& report);
    bool archiveContains(const UiStep& step, QStringList& report);
    // Remplace les variables de `text` en une passe (les valeurs insérées ne sont pas relues) ; faux et
    // `why` si une variable est inconnue, non fermée, ou ${SAMPLES} sans ACTCRAFT_SAMPLES_DIR.
    bool expand(const QString& text, QString& out, QString& why) const;
    // `expand`, sinon la ligne « <tête> ECHEC (raison) » est ajoutée au compte rendu.
    bool expandOrFail(const QString& text, const QString& head, QString& out, QStringList& report) const;

    QQuickWindow* m_window = nullptr;
    std::unique_ptr<QPointingDevice> m_stylus;
    std::unique_ptr<Injector> m_gestures; // doigt et souris synthétiques (L0-S2)
    QString m_scenarioName;
    QString m_outputDir;
    QStringList m_index;
    int m_captureIndex = 0;
    LineSink m_sink;
    KeyDelivery m_keyDelivery;
    QHash<QString, QString> m_memory;            // « remember » : nom → valeur
    std::unique_ptr<EventLoopProbe> m_loopProbe; // sonde de boucle armée par « watchEventLoop »
    bool m_loopRetryAvailable = false;           // fenêtre de boucle pas encore rejouée (play)
    bool m_loopRetryAsked = false;               // écart à la première mesure : play rejoue la fenêtre
};

} // namespace actcraft::bench
