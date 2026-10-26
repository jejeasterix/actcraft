#include <actcraft/bench/contentcheck.h>
#include <actcraft/bench/fixtures.h>
#include <actcraft/bench/injector.h>
#include <actcraft/bench/uiscenario.h>
#include <actcraft/formats/nativefile.h>
#include <actcraft/formats/urlio.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/ui/pageview.h>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeySequence>
#include <QMouseEvent>
#include <QPointer>
#include <QPointingDevice>
#include <QQuickItem>
#include <QQuickWindow>
#include <QShortcutEvent>
#include <QTabletEvent>
#include <QTest>
#include <QTimer>
#include <QUrl>
#include <qpa/qwindowsysteminterface.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <functional>
#include <optional>
#include <utility>

namespace actcraft::bench {

namespace {

constexpr int kStrokeSubdivisions = 16; // déplacements postés entre deux points d'un trait
constexpr int kReadyTimeoutMs = 10000;
constexpr int kStrokeTimeoutMs = 5000;   // trait figé après le relâché
constexpr int kActivateTimeoutMs = 2000; // fenêtre active avant une touche
// Bornes des étapes de L2-T20 (le scénario est une entrée non fiable) : attente d'une propriété de 1 ms à 2
// min, seuil de boucle de 1 ms à 1 min.
constexpr int kWaitForDefaultMs = 10000;
constexpr int kWaitForMaxMs = 120000;
constexpr int kLoopGapMaxMs = 60000;
constexpr int kLoopProbeIntervalMs = 5;
constexpr int kLoopSettleTimeoutMs = 1000; // image en attente rendue avant d'armer la sonde
constexpr int kPinchMaxSteps = 1000;       // pas d'un pincement (entrée non fiable)
constexpr int kHoldMaxMs = 60000;          // maintien du pointeur
constexpr double kToleranceEpsilon = 1e-9; // arrondi des flottants à la borne de la tolérance
// Variables réservées d'un scénario (${SORTIE}, ${SAMPLES}) ; les autres viennent de « remember ».
constexpr QLatin1StringView kOutputVariable("SORTIE");
constexpr QLatin1StringView kSamplesVariable("SAMPLES");

bool readPoint(const QJsonValue& value, QPointF& out)
{
    const QJsonArray array = value.toArray();
    if (array.size() != 2 || !array.at(0).isDouble() || !array.at(1).isDouble()) {
        return false;
    }
    out = QPointF(array.at(0).toDouble(), array.at(1).toDouble());
    return true;
}

QString jsonText(const QJsonValue& value)
{
    if (value.isBool()) {
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    }
    if (value.isDouble()) {
        return propertyText(QVariant(value.toDouble()));
    }
    return value.toString();
}

bool splitTarget(const QString& text, QString& target, QString& property)
{
    const qsizetype dot = text.indexOf(QLatin1Char('.'));
    if (dot <= 0 || dot == text.size() - 1) {
        return false;
    }
    target = text.left(dot);
    property = text.mid(dot + 1);
    return true;
}

// Nom de variable mémorisée : lettres ASCII, chiffres, « _ » et « - » ; SORTIE et SAMPLES sont réservés (une
// fixture écrite sous « ${SORTIE} » n'irait jamais ailleurs).
bool isMemoryName(const QString& name)
{
    if (name.isEmpty() || name == kOutputVariable || name == kSamplesVariable) {
        return false;
    }
    return std::all_of(name.cbegin(), name.cend(), [](QChar c) {
        return (c >= u'a' && c <= u'z') || (c >= u'A' && c <= u'Z') || (c >= u'0' && c <= u'9') ||
               c == u'_' || c == u'-';
    });
}

// Vrai si `path` (relatif : au dossier courant, comme formats::urlFromArgument) désigne un fichier sous
// `dir`, une fois « .. » résolus.
bool isUnder(const QString& dir, const QString& path)
{
    const QDir root(QDir::cleanPath(QDir(dir).absolutePath()));
    const QString file = QDir::cleanPath(QDir::current().absoluteFilePath(path));
    const QString relative = root.relativeFilePath(file);
    return !relative.isEmpty() && relative != QLatin1String(".") && relative != QLatin1String("..") &&
           !relative.startsWith(QLatin1String("../")) && QDir::isRelativePath(relative);
}

// « space » : « page » (défaut) ou « window ».
bool readSpace(const QJsonObject& o, bool& windowSpace, QString& why)
{
    windowSpace = false;
    if (!o.contains(QStringLiteral("space"))) {
        return true;
    }
    const QString text = o.value(QStringLiteral("space")).toString();
    if (text == QLatin1String("page")) {
        return true;
    }
    if (text == QLatin1String("window")) {
        windowSpace = true;
        return true;
    }
    why = QStringLiteral("repère inconnu « %1 » (page ou window)").arg(text);
    return false;
}

bool parsePoints(const QJsonValue& value, std::vector<QPointF>& out)
{
    for (const QJsonValue& item : value.toArray()) {
        QPointF point;
        if (!readPoint(item, point)) {
            return false;
        }
        out.push_back(point);
    }
    return !out.empty();
}

bool parseStep(const QJsonObject& o, UiStep& step, QString& why)
{
    if (o.contains(QStringLiteral("command"))) {
        step.kind = UiStepKind::Command;
        step.command = o.value(QStringLiteral("command")).toString();
        if (o.contains(QStringLiteral("argument"))) {
            step.argument = o.value(QStringLiteral("argument")).toVariant();
        }
        if (step.command.isEmpty()) {
            why = QStringLiteral("commande vide");
            return false;
        }
        return true;
    }
    if (o.contains(QStringLiteral("stroke"))) {
        step.kind = UiStepKind::Stroke;
        const QJsonArray points = o.value(QStringLiteral("stroke")).toArray();
        for (const QJsonValue& value : points) {
            QPointF point;
            if (!readPoint(value, point)) {
                why = QStringLiteral("point de trait invalide");
                return false;
            }
            step.points.push_back(point);
        }
        if (step.points.size() < 2) {
            why = QStringLiteral("trait de moins de 2 points");
            return false;
        }
        step.pressure = o.value(QStringLiteral("pressure")).toDouble(0.5);
        return true;
    }
    if (o.contains(QStringLiteral("key"))) {
        step.kind = UiStepKind::Key;
        step.key = o.value(QStringLiteral("key")).toString();
        if (QKeySequence(step.key).isEmpty()) {
            why = QStringLiteral("touche inconnue « %1 »").arg(step.key);
            return false;
        }
        return true;
    }
    if (o.contains(QStringLiteral("wait"))) {
        step.kind = UiStepKind::Wait;
        step.ms = o.value(QStringLiteral("wait")).toInt(-1);
        if (step.ms < 0) {
            why = QStringLiteral("attente invalide");
            return false;
        }
        return true;
    }
    if (o.contains(QStringLiteral("waitReady"))) {
        step.kind = UiStepKind::WaitReady;
        return true;
    }
    if (o.contains(QStringLiteral("capture"))) {
        step.kind = UiStepKind::Capture;
        step.name = o.value(QStringLiteral("capture")).toString();
        if (step.name.isEmpty()) {
            why = QStringLiteral("capture sans nom");
            return false;
        }
        return true;
    }
    if (o.contains(QStringLiteral("checkPixel"))) {
        step.kind = UiStepKind::CheckPixel;
        step.name = o.value(QStringLiteral("checkPixel")).toString();
        step.color = QColor(o.value(QStringLiteral("color")).toString());
        step.tolerance = o.value(QStringLiteral("tolerance")).toInt(48);
        if (step.name.isEmpty() || !readPoint(o.value(QStringLiteral("at")), step.at) ||
            !step.color.isValid()) {
            why = QStringLiteral("contrôle de pixel invalide (checkPixel, at, color)");
            return false;
        }
        return true;
    }
    if (o.contains(QStringLiteral("checkEdge"))) {
        step.kind = UiStepKind::CheckEdge;
        step.name = o.value(QStringLiteral("checkEdge")).toString();
        step.minIntermediate = o.value(QStringLiteral("minIntermediate")).toInt(0);
        step.maxIntermediate = o.value(QStringLiteral("maxIntermediate")).toInt(-1);
        step.minInk = o.value(QStringLiteral("minInk")).toInt(0);
        if (step.name.isEmpty() || !readPoint(o.value(QStringLiteral("from")), step.at) ||
            !readPoint(o.value(QStringLiteral("to")), step.to)) {
            why = QStringLiteral("contrôle de bord invalide (checkEdge, from, to)");
            return false;
        }
        return true;
    }
    if (o.contains(QStringLiteral("expect"))) {
        step.kind = UiStepKind::Expect;
        if (!splitTarget(o.value(QStringLiteral("expect")).toString(), step.target, step.property) ||
            !o.contains(QStringLiteral("value"))) {
            why = QStringLiteral("lecture invalide (expect « objet.propriété », value)");
            return false;
        }
        step.value = jsonText(o.value(QStringLiteral("value")));
        if (o.contains(QStringLiteral("tolerance"))) {
            const QJsonValue tolerance = o.value(QStringLiteral("tolerance"));
            if (!tolerance.isDouble() || tolerance.toDouble() < 0.0) {
                why = QStringLiteral("tolérance invalide (expect : nombre ≥ 0)");
                return false;
            }
            step.valueTolerance = tolerance.toDouble();
        }
        return true;
    }
    if (o.contains(QStringLiteral("fixture"))) {
        step.kind = UiStepKind::Fixture;
        step.name = o.value(QStringLiteral("fixture")).toString();
        step.path = o.value(QStringLiteral("path")).toString();
        if (step.path.isEmpty() || !fixtureNames().contains(step.name)) {
            why = QStringLiteral("fixture invalide (nom connu et path)");
            return false;
        }
        return true;
    }
    if (o.contains(QStringLiteral("waitFor"))) {
        step.kind = UiStepKind::WaitFor;
        step.ms = o.contains(QStringLiteral("timeoutMs")) ? o.value(QStringLiteral("timeoutMs")).toInt(-1)
                                                          : kWaitForDefaultMs;
        if (!splitTarget(o.value(QStringLiteral("waitFor")).toString(), step.target, step.property) ||
            !o.contains(QStringLiteral("value"))) {
            why = QStringLiteral("attente invalide (waitFor « objet.propriété », value)");
            return false;
        }
        if (step.ms < 1 || step.ms > kWaitForMaxMs) {
            why = QStringLiteral("attente invalide (timeoutMs de 1 à %1)").arg(kWaitForMaxMs);
            return false;
        }
        step.value = jsonText(o.value(QStringLiteral("value")));
        return true;
    }
    if (o.contains(QStringLiteral("watchEventLoop"))) {
        step.kind = UiStepKind::WatchEventLoop;
        return true;
    }
    if (o.contains(QStringLiteral("maxEventLoopGapMs"))) {
        step.kind = UiStepKind::MaxEventLoopGap;
        step.ms = o.value(QStringLiteral("maxEventLoopGapMs")).toInt(-1);
        if (step.ms < 1 || step.ms > kLoopGapMaxMs) {
            why = QStringLiteral("seuil de boucle invalide (de 1 à %1 ms)").arg(kLoopGapMaxMs);
            return false;
        }
        if (o.contains(QStringLiteral("informative")) && !o.value(QStringLiteral("informative")).isBool()) {
            why = QStringLiteral("seuil de boucle invalide (informative : true ou false)");
            return false;
        }
        step.informative = o.value(QStringLiteral("informative")).toBool(false);
        return true;
    }
    if (o.contains(QStringLiteral("remember"))) {
        step.kind = UiStepKind::Remember;
        step.name = o.value(QStringLiteral("as")).toString();
        if (!splitTarget(o.value(QStringLiteral("remember")).toString(), step.target, step.property) ||
            !isMemoryName(step.name)) {
            why = QStringLiteral("mémoire invalide (remember « objet.propriété », as : lettres, chiffres, _ "
                                 "ou -, ni SORTIE ni "
                                 "SAMPLES)");
            return false;
        }
        return true;
    }
    if (o.contains(QStringLiteral("archive"))) {
        step.kind = UiStepKind::ArchiveContains;
        step.path = o.value(QStringLiteral("archive")).toString();
        const QJsonArray contains = o.value(QStringLiteral("contains")).toArray();
        for (const QJsonValue& value : contains) {
            // Un préfixe vide serait trouvé dans toute archive.
            if (!value.isString() || value.toString().isEmpty()) {
                why = QStringLiteral("archive invalide (entrée vide ou non textuelle)");
                return false;
            }
            step.values << value.toString();
        }
        if (step.path.isEmpty() || step.values.isEmpty()) {
            why = QStringLiteral("archive invalide (archive, contains)");
            return false;
        }
        return true;
    }
    if (o.contains(QStringLiteral("touch"))) {
        step.kind = UiStepKind::Touch;
        if (!parsePoints(o.value(QStringLiteral("touch")), step.points)) {
            why = QStringLiteral("doigt invalide (touch : au moins un point [x, y])");
            return false;
        }
        return true;
    }
    if (o.contains(QStringLiteral("pinch"))) {
        step.kind = UiStepKind::Pinch;
        const QJsonValue steps = o.value(QStringLiteral("steps"));
        if (!readPoint(o.value(QStringLiteral("pinch")), step.at) ||
            !o.value(QStringLiteral("from")).isDouble() || !o.value(QStringLiteral("to")).isDouble()) {
            why = QStringLiteral("pincement invalide (pinch [cx, cy], from, to)");
            return false;
        }
        step.pinchFrom = o.value(QStringLiteral("from")).toDouble();
        step.pinchTo = o.value(QStringLiteral("to")).toDouble();
        if (!(step.pinchFrom > 0.0) || !(step.pinchTo > 0.0)) {
            why = QStringLiteral("pincement invalide (from et to doivent être > 0)");
            return false;
        }
        if (o.contains(QStringLiteral("steps"))) {
            step.pinchSteps = steps.isDouble() ? steps.toInt(0) : 0;
            if (step.pinchSteps < 1 || step.pinchSteps > kPinchMaxSteps) {
                why = QStringLiteral("pincement invalide (steps de 1 à %1)").arg(kPinchMaxSteps);
                return false;
            }
        }
        return true;
    }
    if (o.contains(QStringLiteral("pointer"))) {
        step.kind = UiStepKind::Pointer;
        if (!parsePoints(o.value(QStringLiteral("pointer")), step.points)) {
            why = QStringLiteral("pointeur invalide (pointer : au moins un point [x, y])");
            return false;
        }
        if (o.contains(QStringLiteral("button"))) {
            const QString button = o.value(QStringLiteral("button")).toString();
            if (button == QLatin1String("left")) {
                step.button = Qt::LeftButton;
            } else if (button == QLatin1String("right")) {
                step.button = Qt::RightButton;
            } else {
                why = QStringLiteral("bouton inconnu « %1 » (left ou right)").arg(button);
                return false;
            }
        }
        if (o.contains(QStringLiteral("holdMs"))) {
            const QJsonValue hold = o.value(QStringLiteral("holdMs"));
            step.holdMs = hold.isDouble() ? hold.toInt(-1) : -1;
            if (step.holdMs < 0 || step.holdMs > kHoldMaxMs) {
                why = QStringLiteral("maintien invalide (holdMs de 0 à %1)").arg(kHoldMaxMs);
                return false;
            }
        }
        return readSpace(o, step.windowSpace, why);
    }
    if (o.contains(QStringLiteral("doubleClick"))) {
        step.kind = UiStepKind::DoubleClick;
        if (!readPoint(o.value(QStringLiteral("doubleClick")), step.at)) {
            why = QStringLiteral("double-clic invalide (doubleClick [x, y])");
            return false;
        }
        return readSpace(o, step.windowSpace, why);
    }
    if (o.contains(QStringLiteral("typeText"))) {
        step.kind = UiStepKind::TypeText;
        const QJsonValue text = o.value(QStringLiteral("typeText"));
        if (!text.isString() || text.toString().isEmpty()) {
            why = QStringLiteral("texte invalide (typeText : une chaîne non vide)");
            return false;
        }
        step.value = text.toString();
        return true;
    }
    why = QStringLiteral("étape inconnue");
    return false;
}

// Attente d'une condition avec plafond, comme Runner::waitFor : la boucle dort jusqu'au prochain événement,
// un minuteur la réveille toutes les 10 ms. Jamais d'attente à durée fixe pour se synchroniser.
bool waitUntil(const std::function<bool()>& condition, int timeoutMs)
{
    QElapsedTimer clock;
    clock.start();
    QTimer wake;
    wake.start(10);
    while (!condition()) {
        if (clock.elapsed() > timeoutMs) {
            return false;
        }
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
    }
    return true;
}

// Témoin d'une touche (filtre posé sur l'application le temps de l'étape). Le booléen « acceptée » de
// QWindowSystemInterface dit si quelqu'un a pris la touche ; le témoin dit qui : un raccourci joué, un
// raccourci ambigu (plusieurs candidats, aucun joué), ou l'appui livré à un élément. Sans élément ni
// raccourci, un appui reste « accepté » par défaut : le témoin l'empêche de passer pour réussi.
class KeyWitness : public QObject
{
public:
    bool shortcut = false;
    bool ambiguous = false;
    QString receiver; // dernier élément qui a reçu l'appui (propagation vers les parents comprise)

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::Shortcut) {
            if (static_cast<QShortcutEvent*>(event)->isAmbiguous()) {
                ambiguous = true;
            } else {
                shortcut = true;
            }
        } else if (event->type() == QEvent::KeyPress) {
            if (const auto* item = qobject_cast<const QQuickItem*>(watched)) {
                receiver = item->objectName().isEmpty() ? QString::fromLatin1(item->metaObject()->className())
                                                        : item->objectName();
            }
        }
        return QObject::eventFilter(watched, event);
    }
};

// Texte d'une touche imprimable sans autre modificateur que Maj (comme QTest), vide sinon.
QString keyText(QKeyCombination combination)
{
    const int key = static_cast<int>(combination.key());
    const Qt::KeyboardModifiers modifiers = combination.keyboardModifiers();
    if (key < 0x20 || key > 0xff || (modifiers & ~Qt::KeyboardModifiers(Qt::ShiftModifier))) {
        return {};
    }
    const QChar c(key);
    return QString((modifiers & Qt::ShiftModifier) ? c.toUpper() : c.toLower());
}

// Horodatage des touches synthétiques : horloge monotone commune à la recherche de raccourci et à la
// livraison.
ulong keyTimestamp()
{
    static QElapsedTimer clock;
    if (!clock.isValid()) {
        clock.start();
    }
    return static_cast<ulong>(clock.elapsed());
}

bool deliverKey(QWindow* window, ulong timestamp, QEvent::Type type, int key, Qt::KeyboardModifiers modifiers,
                const QString& text)
{
    return QWindowSystemInterface::handleKeyEvent<QWindowSystemInterface::SynchronousDelivery>(
        window, timestamp, type, key, modifiers, text);
}

// Image où tous les pixels sont identiques : rien n'a été dessiné (rendu absent, fenêtre vide).
bool isUniform(const QImage& image)
{
    const QImage rgb = image.convertToFormat(QImage::Format_RGB32);
    const QRgb first = rgb.pixel(0, 0);
    for (int y = 0; y < rgb.height(); ++y) {
        const auto* line = reinterpret_cast<const QRgb*>(rgb.constScanLine(y));
        for (int x = 0; x < rgb.width(); ++x) {
            if (line[x] != first) {
                return false;
            }
        }
    }
    return true;
}

// Les délégués d'un Repeater n'ont pas toujours la fenêtre pour parent QObject : on suit aussi l'arbre des
// éléments.
QQuickItem* findItem(QQuickItem* root, const QString& name)
{
    if (!root) {
        return nullptr;
    }
    if (root->objectName() == name) {
        return root;
    }
    const QList<QQuickItem*> children = root->childItems();
    for (QQuickItem* child : children) {
        if (QQuickItem* found = findItem(child, name)) {
            return found;
        }
    }
    return nullptr;
}

} // namespace

bool parseUiScenario(const QByteArray& json, UiScenario& out, QString& error)
{
    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        error = QStringLiteral("JSON invalide : %1").arg(parseError.errorString());
        return false;
    }
    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("format")).toString() != QLatin1String("actcraft-scenario/1")) {
        error = QStringLiteral("format inconnu (attendu actcraft-scenario/1)");
        return false;
    }
    UiScenario scenario;
    scenario.name = root.value(QStringLiteral("name")).toString();
    scenario.description = root.value(QStringLiteral("description")).toString();
    if (scenario.name.isEmpty()) {
        error = QStringLiteral("nom manquant");
        return false;
    }
    const QJsonArray steps = root.value(QStringLiteral("steps")).toArray();
    if (steps.isEmpty()) {
        error = QStringLiteral("aucune étape");
        return false;
    }
    for (qsizetype i = 0; i < steps.size(); ++i) {
        UiStep step;
        QString why;
        if (!parseStep(steps.at(i).toObject(), step, why)) {
            error = QStringLiteral("étape %1 : %2").arg(i + 1).arg(why);
            return false;
        }
        scenario.steps.push_back(std::move(step));
    }
    out = std::move(scenario);
    return true;
}

bool parseReadSpec(const QString& text, UiStep& out)
{
    const qsizetype equals = text.indexOf(QLatin1Char('='));
    if (equals <= 0) {
        return false;
    }
    UiStep step;
    step.kind = UiStepKind::Expect;
    if (!splitTarget(text.left(equals), step.target, step.property)) {
        return false;
    }
    step.value = text.mid(equals + 1);
    out = std::move(step);
    return true;
}

QString propertyText(const QVariant& value)
{
    if (!value.isValid()) {
        return QStringLiteral("<absente>");
    }
    switch (value.typeId()) {
    case QMetaType::Bool:
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    case QMetaType::Double:
    case QMetaType::Float:
        return QString::number(value.toDouble(), 'g', 12);
    case QMetaType::QColor:
        return value.value<QColor>().name(QColor::HexRgb);
    default:
        if (!value.canConvert<QString>()) {
            // QPointF, QRectF, QObject*… : nommés, jamais lus « "" ».
            return QStringLiteral("<non textuel : %1>").arg(QString::fromLatin1(value.typeName()));
        }
        return value.toString();
    }
}

// Témoin des gestes directs (filtre posé sur l'application le temps de l'étape) : compte les événements
// tactiles et souris qui arrivent à la fenêtre, pour qu'une étape ne soit « OK » que si la fenêtre les a
// reçus.
class GestureWitness : public QObject
{
public:
    explicit GestureWitness(const QObject* window) : m_window(window) {}
    int touchBegin = 0;
    int touchUpdate = 0;
    int touchEnd = 0;
    int press = 0;
    int release = 0;
    int doubleClick = 0;
    Qt::MouseButton pressButton = Qt::NoButton; // bouton du dernier appui

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == m_window) {
            switch (event->type()) {
            case QEvent::TouchBegin:
                ++touchBegin;
                break;
            case QEvent::TouchUpdate:
                ++touchUpdate;
                break;
            case QEvent::TouchEnd:
                ++touchEnd;
                break;
            case QEvent::MouseButtonPress:
                ++press;
                pressButton = static_cast<QMouseEvent*>(event)->button();
                break;
            case QEvent::MouseButtonRelease:
                ++release;
                break;
            case QEvent::MouseButtonDblClick:
                ++doubleClick;
                break;
            default:
                break;
            }
        }
        return false;
    }

private:
    const QObject* m_window;
};

// Sonde de boucle : minuterie du fil principal qui relève, à chaque tour, l'intervalle depuis le précédent.
// Un blocage du fil (écriture synchrone, calcul) l'allonge, qu'une image soit produite ou non. Précise : une
// minuterie grossière peut être servie avec retard (Qt::TimerType), ce qui grossirait les intervalles. Armée
// à la construction.
class EventLoopProbe
{
public:
    EventLoopProbe()
    {
        m_timer.setTimerType(Qt::PreciseTimer);
        m_timer.setInterval(kLoopProbeIntervalMs);
        QObject::connect(&m_timer, &QTimer::timeout, &m_timer, [this] {
            const qint64 now = m_clock.elapsed();
            m_max = std::max(m_max, now - m_last);
            m_last = now;
        });
        m_clock.start();
        m_timer.start();
    }

    // Arrête la sonde et rend le plus long intervalle (ms), intervalle en cours compris : un blocage juste
    // avant l'arrêt, sans tour de boucle, compte aussi.
    qint64 stop()
    {
        m_timer.stop();
        m_max = std::max(m_max, m_clock.elapsed() - m_last);
        return m_max;
    }

private:
    QTimer m_timer;
    QElapsedTimer m_clock;
    qint64 m_last = 0;
    qint64 m_max = 0;
};

UiScenarioPlayer::UiScenarioPlayer(QQuickWindow* window)
    : m_window(window), m_stylus(std::make_unique<QPointingDevice>(
                            QStringLiteral("actcraft-captures-stylet"), 5180,
                            QInputDevice::DeviceType::Stylus, QPointingDevice::PointerType::Pen,
                            QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 3)),
      m_keyDelivery(deliverKey)
{}

UiScenarioPlayer::~UiScenarioPlayer() = default;

void UiScenarioPlayer::setLineSink(LineSink sink)
{
    m_sink = std::move(sink);
}

void UiScenarioPlayer::setKeyDelivery(KeyDelivery delivery)
{
    m_keyDelivery = delivery ? std::move(delivery) : KeyDelivery(deliverKey);
}

QObject* UiScenarioPlayer::find(const QString& target) const
{
    if (!m_window) {
        return nullptr;
    }
    if (target == QLatin1String("window")) {
        return m_window;
    }
    if (QObject* found = m_window->findChild<QObject*>(target)) {
        return found;
    }
    return findItem(m_window->contentItem(), target);
}

ui::PageView* UiScenarioPlayer::view() const
{
    return qobject_cast<ui::PageView*>(find(QStringLiteral("pageView")));
}

bool UiScenarioPlayer::isReady() const
{
    // Comme Runner::run : page fixe prête (images chargées) et cache redessiné au zoom courant.
    const ui::PageView* v = view();
    return v && v->fixedPage() && v->fixedPage()->isReady() && v->cacheReady();
}

void UiScenarioPlayer::postTablet(QEvent::Type type, QPointF page, double pressure, Qt::MouseButton button,
                                  Qt::MouseButtons buttons)
{
    // À la fenêtre, pas à l'élément : Qt Quick n'adresse pas le stylet aux éléments, la PageView le capte par
    // un filtre posé sur la fenêtre (pageview.cpp, itemChange et eventFilter).
    const QPointF scene = view()->pageToScene(page);
    QCoreApplication::postEvent(m_window,
                                new QTabletEvent(type, m_stylus.get(), scene, m_window->mapToGlobal(scene),
                                                 pressure, 0, 0, 0, 0, 0, Qt::NoModifier, button, buttons));
    QCoreApplication::processEvents();
}

void UiScenarioPlayer::emitFrom(const QStringList& report, qsizetype first) const
{
    if (m_sink) {
        for (qsizetype i = first; i < report.size(); ++i) {
            m_sink(report.at(i));
        }
    }
}

bool UiScenarioPlayer::expect(const UiStep& step, QStringList& report)
{
    const qsizetype first = report.size();
    const bool ok = read(step, report);
    emitFrom(report, first);
    return ok;
}

bool UiScenarioPlayer::read(const UiStep& step, QStringList& report)
{
    QString expected;
    if (!expandOrFail(step.value, QStringLiteral("LU %1.%2").arg(step.target, step.property), expected,
                      report)) {
        return false;
    }
    QObject* object = find(step.target);
    // Propriété absente : « <absente> » (propertyText), jamais « "" » qui égalerait une valeur attendue vide.
    const QString actual = object ? propertyText(object->property(step.property.toUtf8().constData()))
                                  : QStringLiteral("<introuvable>");
    bool ok = false;
    QString expectation = expected;
    if (step.valueTolerance) {
        // Lecture numérique : |lu − attendu| ≤ tolérance ; un texte non numérique, d'un côté ou de l'autre,
        // est un écart.
        bool actualIsNumber = false;
        bool expectedIsNumber = false;
        const double actualNumber = actual.toDouble(&actualIsNumber);
        const double expectedNumber = expected.toDouble(&expectedIsNumber);
        ok = object && actualIsNumber && expectedIsNumber &&
             std::abs(actualNumber - expectedNumber) <= *step.valueTolerance + kToleranceEpsilon;
        expectation = QStringLiteral("%1 ± %2").arg(expected, QString::number(*step.valueTolerance, 'g', 12));
    } else {
        ok = object && actual == expected;
    }
    report << (ok ? QStringLiteral("LU %1.%2=%3 OK").arg(step.target, step.property, actual)
                  : QStringLiteral("LU %1.%2=%3 ECART (attendu %4)")
                        .arg(step.target, step.property, actual, expectation));
    return ok;
}

bool UiScenarioPlayer::stroke(const UiStep& step, QStringList& report)
{
    ui::PageView* v = view();
    const QString head = QStringLiteral("TRAIT %1 points").arg(step.points.size());
    if (!v) {
        report << head + QStringLiteral(" ECHEC (vue de page « pageView » introuvable)");
        return false;
    }
    // Contrôle du résultat : le trait doit être figé par la vue (versé à la page fixe), une fois.
    int frozen = 0;
    const auto connection =
        QObject::connect(v, &ui::PageView::strokeFrozen, v, [&frozen] { ++frozen; }, Qt::DirectConnection);
    postTablet(QEvent::TabletPress, step.points.front(), step.pressure, Qt::LeftButton, Qt::LeftButton);
    for (std::size_t i = 1; i < step.points.size(); ++i) {
        const QPointF from = step.points[i - 1];
        const QPointF to = step.points[i];
        for (int k = 1; k <= kStrokeSubdivisions; ++k) {
            postTablet(QEvent::TabletMove,
                       from + (to - from) * (k / static_cast<double>(kStrokeSubdivisions)), step.pressure,
                       Qt::NoButton, Qt::LeftButton);
        }
    }
    postTablet(QEvent::TabletRelease, step.points.back(), 0.0, Qt::LeftButton, Qt::NoButton);
    waitUntil([&frozen] { return frozen > 0; }, kStrokeTimeoutMs);
    QObject::disconnect(connection);
    if (frozen != 1) {
        report << head + QStringLiteral(
                             " ECHEC (%1 trait(s) figé(s), 1 attendu : le stylet n'a pas atteint la vue)")
                             .arg(frozen);
        return false;
    }
    report << head + QStringLiteral(" OK");
    return true;
}

bool UiScenarioPlayer::key(const UiStep& step, QStringList& report)
{
    // Fenêtre active si possible (raccourcis de fenêtre) ; un raccourci d'application agit de toute façon.
    m_window->requestActivate();
    const bool active = QTest::qWaitForWindowActive(m_window, kActivateTimeoutMs);
    // Chemin d'une vraie touche : carte des raccourcis de QGuiApplication d'abord, puis livraison synchrone à
    // la fenêtre (élément actif). Le joueur consulte lui-même la carte : la livraison de Qt ne le fait pas
    // sous macOS (QGuiApplicationPrivate::processKeyEvent, « #if !defined(Q_OS_MACOS) » ; la plateforme Cocoa
    // et QTest, par qt_handleKeyEvent, appellent handleShortcutEvent avant elle). Ailleurs la livraison la
    // consulte une seconde fois, sans effet : une touche que la carte vient de refuser l'est encore. Ni
    // qt_handleKeyEvent ni QTest::keySequence : ils jettent le booléen « acceptée », qu'on garde (un élément
    // qui reçoit la touche sans l'accepter ne la fait pas réussir).
    const QKeySequence sequence(step.key);
    KeyWitness witness;
    QCoreApplication::instance()->installEventFilter(&witness);
    bool accepted = true;
    for (int i = 0; i < sequence.count(); ++i) {
        const QKeyCombination combination = sequence[static_cast<uint>(i)];
        const int code = static_cast<int>(combination.key());
        const Qt::KeyboardModifiers modifiers = combination.keyboardModifiers();
        const QString text = keyText(combination);
        const ulong timestamp = keyTimestamp();
        // Vrai : raccourci joué ou ambigu, ou début d'une séquence à plusieurs touches ; l'appui n'est alors
        // pas livré, comme pour une vraie touche.
        bool pressAccepted =
            QWindowSystemInterface::handleShortcutEvent(m_window, timestamp, code, modifiers, 0, 0, 0, text);
        if (!pressAccepted) {
            pressAccepted = m_keyDelivery(m_window, timestamp, QEvent::KeyPress, code, modifiers, text);
        }
        accepted = pressAccepted && accepted;
        m_keyDelivery(m_window, keyTimestamp(), QEvent::KeyRelease, code, modifiers, text);
    }
    QCoreApplication::processEvents();
    QCoreApplication::instance()->removeEventFilter(&witness);
    if (witness.ambiguous) {
        report << QStringLiteral("TOUCHE %1 ECHEC (raccourci ambigu : plusieurs candidats, aucun joué)")
                      .arg(step.key);
        return false;
    }
    if (accepted && witness.shortcut) {
        report << QStringLiteral("TOUCHE %1 OK (raccourci)").arg(step.key);
        return true;
    }
    if (accepted && !witness.receiver.isEmpty()) {
        report << QStringLiteral("TOUCHE %1 OK (acceptée par %2)").arg(step.key, witness.receiver);
        return true;
    }
    // Repli écrit : sans raccourci ni élément actif qui l'accepte (focus perdu, plateforme sans activation),
    // jouer la commande nommée équivalente ({"command": …}), même chemin que les boutons.
    report << QStringLiteral(
                  "TOUCHE %1 ECHEC (ni raccourci ni élément actif ne l'a acceptée%2 ; repli : commande "
                  "nommée équivalente)")
                  .arg(step.key, active ? QString() : QStringLiteral(", fenêtre inactive"));
    return false;
}

// L4-T14 : un appui et un relâché par caractère, avec son texte, livrés comme une vraie frappe (sans la carte
// des raccourcis : une lettre n'en est pas un). Maj pour une majuscule. Faux si un appui n'est pas accepté.
bool UiScenarioPlayer::typeText(const UiStep& step, QStringList& report)
{
    m_window->requestActivate();
    const bool active = QTest::qWaitForWindowActive(m_window, kActivateTimeoutMs);
    bool accepted = true;
    for (const QChar c : step.value) {
        const QString text(c);
        const ushort unicode = c.toUpper().unicode();
        const int code = unicode >= 0x20 && unicode < 0x7f ? static_cast<int>(unicode) : 0;
        const Qt::KeyboardModifiers modifiers = c.isUpper() ? Qt::ShiftModifier : Qt::NoModifier;
        accepted = m_keyDelivery(m_window, keyTimestamp(), QEvent::KeyPress, code, modifiers, text) && accepted;
        m_keyDelivery(m_window, keyTimestamp(), QEvent::KeyRelease, code, modifiers, text);
    }
    QCoreApplication::processEvents();
    report << QStringLiteral("TEXTE %1 %2%3")
                  .arg(step.value, accepted ? QStringLiteral("OK") : QStringLiteral("ECHEC"),
                       accepted || active ? QString() : QStringLiteral(" (fenêtre inactive)"));
    return accepted;
}

namespace {

// Doigt synthétique de Qt Test, créé une seule fois (Qt le garde enregistré jusqu'à la fin du processus).
QPointingDevice* touchDevice()
{
    static QPointingDevice* device = QTest::createTouchDevice();
    return device;
}

// Un temps du doigt livré par Qt Test : les points passent par le suivi des points actifs de QGuiApplication
// (un QTouchEvent posté à la main n'y passe pas : Qt Quick ne retrouve pas le point). Un identifiant par
// doigt.
void deliverFingers(QWindow* window, GesturePhase phase, const std::vector<QPointF>& fingers)
{
    auto sequence = QTest::touchEvent(window, touchDevice(), false);
    for (std::size_t id = 0; id < fingers.size(); ++id) {
        const QPoint point = fingers[id].toPoint();
        switch (phase) {
        case GesturePhase::Press:
            sequence.press(static_cast<int>(id), point, window);
            break;
        case GesturePhase::Move:
            sequence.move(static_cast<int>(id), point, window);
            break;
        case GesturePhase::Release:
            sequence.release(static_cast<int>(id), point, window);
            break;
        }
    }
    sequence.commit();
}

} // namespace

Injector& UiScenarioPlayer::gestures()
{
    if (!m_gestures) {
        // La conversion page → scène est celle du trait (PageView::pageToScene) ; l'étape vérifie la vue
        // avant tout geste en repère de page.
        m_gestures = std::make_unique<Injector>(m_window, [this](QPointF page) {
            const ui::PageView* v = view();
            return v ? v->pageToScene(page) : page;
        });
    }
    return *m_gestures;
}

bool UiScenarioPlayer::touch(const UiStep& step, QStringList& report)
{
    const QString head = QStringLiteral("DOIGT %1 points").arg(step.points.size());
    if (!view()) {
        report << head + QStringLiteral(" ECHEC (vue de page « pageView » introuvable)");
        return false;
    }
    GestureWitness witness(m_window);
    QCoreApplication::instance()->installEventFilter(&witness);
    const bool sent =
        gestures().touchPath(step.points, [this](GesturePhase phase, const std::vector<QPointF>& f) {
            deliverFingers(m_window, phase, f);
        });
    QCoreApplication::instance()->removeEventFilter(&witness);
    const bool ok = sent && witness.touchBegin == 1 && witness.touchEnd == 1;
    report << (ok ? head + QStringLiteral(" OK")
                  : head + QStringLiteral(" ECHEC (la fenêtre a reçu %1 début(s) et %2 fin(s) tactile(s), 1 "
                                          "et 1 attendus)")
                               .arg(witness.touchBegin)
                               .arg(witness.touchEnd));
    return ok;
}

bool UiScenarioPlayer::pinch(const UiStep& step, QStringList& report)
{
    const QString head = QStringLiteral("PINCEMENT ×%1").arg(step.pinchTo / step.pinchFrom, 0, 'g', 4);
    if (!view()) {
        report << head + QStringLiteral(" ECHEC (vue de page « pageView » introuvable)");
        return false;
    }
    GestureWitness witness(m_window);
    QCoreApplication::instance()->installEventFilter(&witness);
    const bool sent = gestures().pinch(
        step.at, step.pinchFrom, step.pinchTo, step.pinchSteps,
        [this](GesturePhase phase, const std::vector<QPointF>& f) { deliverFingers(m_window, phase, f); });
    QCoreApplication::instance()->removeEventFilter(&witness);
    const bool ok = sent && witness.touchBegin == 1 && witness.touchEnd == 1 && witness.touchUpdate >= 1;
    report << (ok ? head + QStringLiteral(" OK")
                  : head + QStringLiteral(" ECHEC (la fenêtre a reçu %1 début(s), %2 mise(s) à jour et %3 "
                                          "fin(s) tactile(s), 1, au moins %4 et 1 attendus)")
                               .arg(witness.touchBegin)
                               .arg(witness.touchUpdate)
                               .arg(witness.touchEnd)
                               .arg(1));
    return ok;
}

bool UiScenarioPlayer::pointer(const UiStep& step, QStringList& report)
{
    const QString head = QStringLiteral("POINTEUR %1").arg(step.points.size());
    if (!step.windowSpace && !view()) {
        report << head + QStringLiteral(" ECHEC (vue de page « pageView » introuvable)");
        return false;
    }
    GestureWitness witness(m_window);
    QCoreApplication::instance()->installEventFilter(&witness);
    const Qt::MouseButton button = step.button;
    const bool sent = gestures().pointerPath(
        step.points,
        [this, button](GesturePhase phase, QPointF position) {
            const QPoint point = position.toPoint();
            switch (phase) {
            case GesturePhase::Press:
                QTest::mousePress(m_window, button, Qt::NoModifier, point);
                break;
            case GesturePhase::Move:
                QTest::mouseMove(m_window, point);
                break;
            case GesturePhase::Release:
                QTest::mouseRelease(m_window, button, Qt::NoModifier, point);
                break;
            }
        },
        step.holdMs, step.windowSpace ? PointSpace::Window : PointSpace::Page);
    QCoreApplication::instance()->removeEventFilter(&witness);
    const bool ok = sent && witness.press == 1 && witness.release == 1 && witness.pressButton == step.button;
    report << (ok ? head + QStringLiteral(" OK")
                  : head + QStringLiteral(" ECHEC (la fenêtre a reçu %1 appui(s) et %2 relâché(s), 1 et 1 "
                                          "attendus)")
                               .arg(witness.press)
                               .arg(witness.release));
    return ok;
}

bool UiScenarioPlayer::doubleClick(const UiStep& step, QStringList& report)
{
    const QString head = QStringLiteral("DOUBLECLIC");
    if (!step.windowSpace && !view()) {
        report << head + QStringLiteral(" ECHEC (vue de page « pageView » introuvable)");
        return false;
    }
    GestureWitness witness(m_window);
    QCoreApplication::instance()->installEventFilter(&witness);
    // Qt Test : appui, relâché, appui (Qt le livre en double-clic), relâché, comme une vraie souris.
    QTest::mouseDClick(
        m_window, Qt::LeftButton, Qt::NoModifier,
        gestures().scenePoint(step.at, step.windowSpace ? PointSpace::Window : PointSpace::Page).toPoint());
    const bool sent = true;
    QCoreApplication::instance()->removeEventFilter(&witness);
    const bool ok = sent && witness.press >= 1 && witness.doubleClick == 1 && witness.release == 2;
    report << (ok ? head + QStringLiteral(" OK")
                  : head + QStringLiteral(" ECHEC (la fenêtre a reçu %1 appui(s), %2 double-clic(s) et %3 "
                                          "relâché(s), au moins 1, 1 et 2 attendus)")
                               .arg(witness.press)
                               .arg(witness.doubleClick)
                               .arg(witness.release));
    return ok;
}

bool UiScenarioPlayer::checkPixel(const UiStep& step, QStringList& report)
{
    ui::PageView* v = view();
    // Image du moment, jamais la dernière capture : grabWindow() synchronise et dessine la scène courante. Le
    // scénario place « waitReady » avant si le cache doit être à jour (le joueur ne le devine pas).
    const QImage image = m_window->grabWindow();
    QColor actual;
    QString read = QStringLiteral("hors image");
    if (!v) {
        read = QStringLiteral("vue de page introuvable");
    } else if (image.isNull()) {
        read = QStringLiteral("capture vide");
    } else {
        actual = sampleColor(image, sceneToImage(v->pageToScene(step.at), m_window->devicePixelRatio()));
        if (actual.isValid()) {
            read = actual.name();
        }
    }
    const bool ok = actual.isValid() && colorNear(actual, step.color, step.tolerance);
    report << QStringLiteral("PIXEL %1 %2 (lu %3, attendu %4, tolérance %5)")
                  .arg(step.name, ok ? QStringLiteral("OK") : QStringLiteral("ECART"), read,
                       step.color.name())
                  .arg(step.tolerance);
    return ok;
}

// L10-T5 (S33) : pixels intermédiaires, un à un, le long d'un segment de page (fond blanc, encre noire,
// marge 24 par canal) ; image du moment, comme checkPixel. Jamais de moyenne 3 × 3 ici : elle donne du gris
// même sur un bord crénelé (constat L2-C11).
bool UiScenarioPlayer::checkEdge(const UiStep& step, QStringList& report)
{
    constexpr int kEdgeMargin = 24;
    const QColor edgeInk(Qt::black);
    ui::PageView* v = view();
    const QImage image = m_window->grabWindow();
    int count = -1;
    int ink = -1;
    if (v && !image.isNull()) {
        const qreal dpr = m_window->devicePixelRatio();
        const QPoint from = sceneToImage(v->pageToScene(step.at), dpr);
        const QPoint to = sceneToImage(v->pageToScene(step.to), dpr);
        count = countIntermediatePixels(image, from, to, QColor(Qt::white), edgeInk, kEdgeMargin);
        ink = countInkPixels(image, from, to, edgeInk, kEdgeMargin);
    }
    const bool ok = count >= step.minIntermediate &&
                    (step.maxIntermediate < 0 || count <= step.maxIntermediate) && ink >= step.minInk;
    const QString expected =
        step.maxIntermediate < 0
            ? QStringLiteral("≥ %1").arg(step.minIntermediate)
            : QStringLiteral("%1 à %2").arg(step.minIntermediate).arg(step.maxIntermediate);
    report << QStringLiteral("BORD %1 %2 (intermédiaires %3, attendu %4 ; encre %5, au moins %6)")
                  .arg(step.name, ok ? QStringLiteral("OK") : QStringLiteral("ECART"))
                  .arg(count)
                  .arg(expected)
                  .arg(ink)
                  .arg(step.minInk);
    return ok;
}

bool UiScenarioPlayer::expand(const QString& text, QString& out, QString& why) const
{
    QString result;
    qsizetype from = 0;
    for (;;) {
        const qsizetype open = text.indexOf(QLatin1String("${"), from);
        if (open < 0) {
            result += QStringView(text).mid(from);
            break;
        }
        const qsizetype close = text.indexOf(QLatin1Char('}'), open + 2);
        if (close < 0) {
            why = QStringLiteral("variable non fermée");
            return false;
        }
        result += QStringView(text).mid(from, open - from);
        const QString name = text.mid(open + 2, close - open - 2);
        if (name == kOutputVariable) {
            result += m_outputDir;
        } else if (name == kSamplesVariable) {
            const QString samples = qEnvironmentVariable("ACTCRAFT_SAMPLES_DIR");
            if (samples.isEmpty()) {
                why = QStringLiteral("ACTCRAFT_SAMPLES_DIR absent");
                return false;
            }
            result += samples;
        } else {
            const auto found = m_memory.constFind(name);
            if (found == m_memory.cend()) {
                why = QStringLiteral("variable inconnue « %1 »").arg(name);
                return false;
            }
            result += *found; // inséré tel quel : une valeur lue dans l'interface n'est jamais relue
        }
        from = close + 1;
    }
    out = result;
    return true;
}

bool UiScenarioPlayer::expandOrFail(const QString& text, const QString& head, QString& out,
                                    QStringList& report) const
{
    QString why;
    if (expand(text, out, why)) {
        return true;
    }
    report << head + QStringLiteral(" ECHEC (%1)").arg(why);
    return false;
}

bool UiScenarioPlayer::fixture(const UiStep& step, QStringList& report)
{
    QString path;
    if (!expandOrFail(step.path, QStringLiteral("FIXTURE %1").arg(step.name), path, report)) {
        return false;
    }
    // Chemin venu du scénario : la fixture n'écrit que sous le dossier de sortie, jamais à côté.
    if (!isUnder(m_outputDir, path)) {
        report << QStringLiteral("FIXTURE %1 ECHEC (%2 hors du dossier de sortie %3)")
                      .arg(step.name, path, m_outputDir);
        return false;
    }
    QString error;
    const bool ok = QDir().mkpath(QFileInfo(path).absolutePath()) && buildFixture(step.name, path, error);
    report << (ok ? QStringLiteral("FIXTURE %1 %2 OK").arg(step.name, path)
                  : QStringLiteral("FIXTURE %1 ECHEC (%2)")
                        .arg(step.name, error.isEmpty() ? QStringLiteral("dossier impossible") : error));
    return ok;
}

bool UiScenarioPlayer::waitFor(const UiStep& step, QStringList& report)
{
    const QString head = QStringLiteral("ATTENTE %1.%2").arg(step.target, step.property);
    QString expected;
    if (!expandOrFail(step.value, head, expected, report)) {
        return false;
    }
    const QPointer<QObject> target = find(step.target);
    if (!target) {
        report << head + QStringLiteral(" ECHEC (objet %1 introuvable)").arg(step.target);
        return false;
    }
    // Condition avec plafond, jamais une durée fixe ; l'objet peut disparaître pendant l'attente.
    const QByteArray property = step.property.toUtf8();
    QString actual;
    waitUntil(
        [&] {
            if (!target) {
                return true;
            }
            actual = propertyText(target->property(property.constData()));
            return actual == expected;
        },
        step.ms);
    if (!target) {
        report << head + QStringLiteral(" ECHEC (objet détruit pendant l'attente)");
        return false;
    }
    const bool ok = actual == expected;
    report << (ok ? head + QStringLiteral("=%1 OK").arg(expected)
                  : head +
                        QStringLiteral("=%1 ECHEC (lu %2 après %3 ms)").arg(expected, actual).arg(step.ms));
    return ok;
}

bool UiScenarioPlayer::watchEventLoop(QStringList& report)
{
    // Image laissée en attente par l'étape précédente (trait, changement de page) : rendue AVANT d'armer la
    // sonde, qui mesure la boucle pendant ce qui suit (l'E/S), pas ce rendu. Une image est demandée ; son
    // frameSwapped, qui est celui de l'image déjà en attente s'il y en a une, libère l'attente. Attente
    // bornée : fenêtre cachée ou rendu arrêté, aucune image ne vient, la sonde s'arme quand même et le dit.
    QString note;
    auto swapped = std::make_shared<std::atomic<bool>>(false);
    const QMetaObject::Connection connection = QObject::connect(
        m_window, &QQuickWindow::frameSwapped, m_window, [swapped] { swapped->store(true); },
        Qt::DirectConnection); // fil de rendu ou fil principal selon la boucle de rendu
    m_window->update();
    if (!waitUntil([&swapped] { return swapped->load(); }, kLoopSettleTimeoutMs)) {
        note = QStringLiteral(" (aucune image rendue en %1 ms)").arg(kLoopSettleTimeoutMs);
    }
    QObject::disconnect(connection);
    m_loopProbe = std::make_unique<EventLoopProbe>();
    report << QStringLiteral("BOUCLE sonde démarrée") + note;
    return true;
}

bool UiScenarioPlayer::maxEventLoopGap(const UiStep& step, QStringList& report)
{
    if (!m_loopProbe) {
        report << QStringLiteral("BOUCLE ECHEC (sonde non démarrée)");
        return false;
    }
    const qint64 max = m_loopProbe->stop();
    m_loopProbe.reset();
    if (max <= step.ms) {
        report << QStringLiteral("BOUCLE max=%1 ms OK").arg(max);
        return true;
    }
    // Mesure informative (L3-C4, S5) : écrite, ni jugée ni rejouée ; sans « ECART », motif d'échec de ctest.
    // L'invariant qu'elle surveillait (le fil principal n'attend jamais l'écriture) est prouvé sans
    // chronomètre par tst_ui_files ; à l'écran, l'orchestrateur juge la mesure aux clôtures.
    if (step.informative) {
        report
            << QStringLiteral("BOUCLE max=%1 ms HORS SEUIL (seuil %2 ms, informatif)").arg(max).arg(step.ms);
        return true;
    }
    // Reprise unique (L10-C2) : la charge de la machine allonge un intervalle au hasard, un blocage du fil
    // principal par l'étape jouée revient à chaque passage. Le seuil reste entier ; la première mesure
    // au-dessus est écrite sans « ECART » (motif d'échec de ctest), la seconde échoue.
    if (m_loopRetryAvailable) {
        m_loopRetryAsked = true;
        report << QStringLiteral("BOUCLE max=%1 ms au-dessus du seuil (%2 ms) : fenêtre rejouée une fois")
                      .arg(max)
                      .arg(step.ms);
        return false;
    }
    report << QStringLiteral("BOUCLE max=%1 ms ECART (seuil %2 ms)").arg(max).arg(step.ms);
    return false;
}

bool UiScenarioPlayer::remember(const UiStep& step, QStringList& report)
{
    QObject* target = find(step.target);
    if (!target) {
        report << QStringLiteral("MEMO %1 ECHEC (objet %2 introuvable)").arg(step.name, step.target);
        return false;
    }
    // Propriété absente ou sans texte : rien à mémoriser (« <absente> » relu plus tard égalerait une autre
    // propriété absente).
    const QVariant value = target->property(step.property.toUtf8().constData());
    if (!value.isValid() || !value.canConvert<QString>()) {
        report << QStringLiteral("MEMO %1 ECHEC (%2.%3 : %4)")
                      .arg(step.name, step.target, step.property, propertyText(value));
        return false;
    }
    const QString text = propertyText(value);
    m_memory.insert(step.name, text);
    report << QStringLiteral("MEMO %1=%2").arg(step.name, text);
    return true;
}

bool UiScenarioPlayer::archiveContains(const UiStep& step, QStringList& report)
{
    QString path;
    if (!expandOrFail(step.path, QStringLiteral("ARCHIVE %1").arg(step.path), path, report)) {
        return false;
    }
    // Lecture bornée (formats::readUrl, 1 Gio) ; relatif : au dossier courant, comme une commande de fichier.
    const formats::UrlReadResult read =
        formats::readUrl(QUrl::fromLocalFile(QDir::current().absoluteFilePath(path)));
    if (!read.ok) {
        report << QStringLiteral("ARCHIVE %1 ECHEC (%2)").arg(path, read.error);
        return false;
    }
    const QStringList names = formats::archiveEntryNames(read.data);
    QStringList missing;
    for (const QString& wanted : step.values) {
        if (std::none_of(names.cbegin(), names.cend(),
                         [&wanted](const QString& name) { return name.startsWith(wanted); })) {
            missing << wanted;
        }
    }
    report << (missing.isEmpty() ? QStringLiteral("ARCHIVE %1 OK").arg(path)
                                 : QStringLiteral("ARCHIVE %1 ECART (manque %2)")
                                       .arg(path, missing.join(QStringLiteral(", "))));
    return missing.isEmpty();
}

bool UiScenarioPlayer::runStep(const UiStep& step, QStringList& report)
{
    switch (step.kind) {
    case UiStepKind::Command: {
        QVariant argumentValue = step.argument;
        if (argumentValue.typeId() == QMetaType::QString) {
            QString expanded;
            if (!expandOrFail(argumentValue.toString(), QStringLiteral("COMMANDE %1").arg(step.command),
                              expanded, report)) {
                return false;
            }
            argumentValue = expanded;
        }
        QVariant result;
        const bool invoked = QMetaObject::invokeMethod(m_window, "runCommand", Q_RETURN_ARG(QVariant, result),
                                                       Q_ARG(QVariant, QVariant(step.command)),
                                                       Q_ARG(QVariant, argumentValue));
        QCoreApplication::processEvents();
        const bool ok = invoked && result.toBool();
        const QString argument =
            argumentValue.isValid() ? QStringLiteral(" ") + propertyText(argumentValue) : QString();
        report << QStringLiteral("COMMANDE %1%2 %3")
                      .arg(step.command, argument, ok ? QStringLiteral("OK") : QStringLiteral("ECHEC"));
        return ok;
    }
    case UiStepKind::Stroke:
        return stroke(step, report);
    case UiStepKind::Key:
        return key(step, report);
    case UiStepKind::Wait: {
        // Pause demandée par le scénario (la seule attente à durée fixe du joueur).
        QElapsedTimer clock;
        clock.start();
        waitUntil([&clock, &step] { return clock.elapsed() >= step.ms; }, step.ms + 1000);
        report << QStringLiteral("ATTENTE %1 ms OK").arg(step.ms);
        return true;
    }
    case UiStepKind::WaitReady: {
        const bool ok = waitUntil([this] { return isReady(); }, kReadyTimeoutMs);
        report << (ok ? QStringLiteral("PRET OK")
                      : QStringLiteral("PRET ECHEC (page ou cache pas prêts après 10 s)"));
        return ok;
    }
    case UiStepKind::Capture: {
        const QImage image = m_window->grabWindow();
        const QString file = QStringLiteral("%1-%2-%3.png")
                                 .arg(m_scenarioName)
                                 .arg(++m_captureIndex, 2, 10, QLatin1Char('0'))
                                 .arg(step.name);
        const QString path = QDir(m_outputDir).filePath(file);
        QString why;
        if (image.isNull()) {
            why = QStringLiteral("capture vide");
        } else if (isUniform(image)) {
            why = QStringLiteral("image uniforme : rien de dessiné");
        } else if (!image.save(path)) {
            why = QStringLiteral("écriture impossible");
        }
        m_index << QStringLiteral("- `%1` : %2").arg(file, step.name);
        report << (why.isEmpty() ? QStringLiteral("CAPTURE %1 OK").arg(path)
                                 : QStringLiteral("CAPTURE %1 ECHEC (%2)").arg(path, why));
        return why.isEmpty();
    }
    case UiStepKind::CheckPixel:
        return checkPixel(step, report);
    case UiStepKind::CheckEdge:
        return checkEdge(step, report);
    case UiStepKind::Expect:
        return read(step, report);
    case UiStepKind::Fixture:
        return fixture(step, report);
    case UiStepKind::WaitFor:
        return waitFor(step, report);
    case UiStepKind::WatchEventLoop:
        return watchEventLoop(report);
    case UiStepKind::MaxEventLoopGap:
        return maxEventLoopGap(step, report);
    case UiStepKind::Remember:
        return remember(step, report);
    case UiStepKind::ArchiveContains:
        return archiveContains(step, report);
    case UiStepKind::Touch:
        return touch(step, report);
    case UiStepKind::Pinch:
        return pinch(step, report);
    case UiStepKind::Pointer:
        return pointer(step, report);
    case UiStepKind::DoubleClick:
        return doubleClick(step, report);
    case UiStepKind::TypeText:
        return typeText(step, report);
    }
    return false;
}

bool UiScenarioPlayer::play(const UiScenario& scenario, const QString& outputDir, QStringList& report)
{
    const qsizetype first = report.size();
    if (!m_window) {
        report << QStringLiteral("FENETRE absente");
        emitFrom(report, first);
        return false;
    }
    m_scenarioName = scenario.name;
    m_outputDir = outputDir;
    m_captureIndex = 0;
    m_index.clear();
    m_memory.clear();    // variables d'un scénario, pas du suivant
    m_loopProbe.reset(); // sonde d'un scénario précédent jamais arrêtée
    if (!QDir().mkpath(outputDir)) {
        report << QStringLiteral("CAPTURE dossier impossible : %1").arg(outputDir);
        emitFrom(report, first);
        return false;
    }
    bool ok = true;
    std::optional<std::size_t> window; // dernière « watchEventLoop » de la fenêtre de boucle ouverte
    bool retried = false;
    std::size_t i = 0;
    while (i < scenario.steps.size()) {
        const UiStep& step = scenario.steps[i];
        if (step.kind == UiStepKind::WatchEventLoop) {
            window = i;
        }
        m_loopRetryAvailable = window.has_value() && !retried;
        m_loopRetryAsked = false;
        const qsizetype before = report.size();
        const bool stepOk = runStep(step, report);
        emitFrom(report, before); // au fil de l'eau : la ligne sort avant l'étape suivante
        if (m_loopRetryAsked) {
            retried = true;
            i = *window; // la fenêtre est rejouée depuis sa « watchEventLoop », une fois
            continue;
        }
        if (!stepOk) {
            ok = false;
            break;
        }
        if (step.kind == UiStepKind::MaxEventLoopGap) {
            window.reset();
            retried = false;
        }
        ++i;
    }
    QFile index(QDir(outputDir).filePath(QStringLiteral("index.md")));
    if (index.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QString text = QStringLiteral("# Captures — %1\n\n%2\n\n").arg(scenario.name, scenario.description);
        text += m_index.join(QLatin1Char('\n'));
        text += QStringLiteral("\n\nRésultat : %1\n")
                    .arg(ok ? QStringLiteral("réussi") : QStringLiteral("en échec"));
        index.write(text.toUtf8());
    }
    return ok;
}

} // namespace actcraft::bench
