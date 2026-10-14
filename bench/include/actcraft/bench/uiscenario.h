#pragma once

#include <QColor>
#include <QEvent>
#include <QPointF>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

class QObject;
class QPointingDevice;
class QQuickWindow;

namespace actcraft::ui {
class PageView;
}

namespace actcraft::bench {

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
    QString value; // attendue, en texte (propertyText)
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
// ATTENTE, PRET, CAPTURE, PIXEL, LU. Chaque étape d'action contrôle son résultat, jamais « OK » sur le seul
// envoi : COMMANDE exige le retour vrai de runCommand ; TRAIT, postés à la fenêtre (le filtre de la PageView
// les y attend, variante B), exige un trait figé par la vue ; TOUCHE exige l'appui accepté par un raccourci
// non ambigu ou par un élément actif (repli sinon : la commande nommée équivalente) ; CAPTURE refuse une
// image vide ou uniforme. Lignes terminées par OK, ECHEC ou ECART. Aucune attente à durée fixe hors de
// l'étape « wait » : conditions avec plafond. DURÉE DE VIE : le joueur possède le stylet synthétique ; il
// doit être détruit APRÈS la fenêtre (le déclarer avant le moteur QML).
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

    QQuickWindow* m_window = nullptr;
    std::unique_ptr<QPointingDevice> m_stylus;
    QString m_scenarioName;
    QString m_outputDir;
    QStringList m_index;
    int m_captureIndex = 0;
    LineSink m_sink;
};

} // namespace actcraft::bench
