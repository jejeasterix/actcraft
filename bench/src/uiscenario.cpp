#include <actcraft/bench/contentcheck.h>
#include <actcraft/bench/uiscenario.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/ui/pageview.h>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeySequence>
#include <QPointingDevice>
#include <QQuickItem>
#include <QQuickWindow>
#include <QShortcutEvent>
#include <QTabletEvent>
#include <QTest>
#include <QTimer>
#include <qpa/qwindowsysteminterface.h>

#include <functional>
#include <utility>

namespace actcraft::bench {

namespace {

constexpr int kStrokeSubdivisions = 16; // déplacements postés entre deux points d'un trait
constexpr int kReadyTimeoutMs = 10000;
constexpr int kStrokeTimeoutMs = 5000;   // trait figé après le relâché
constexpr int kActivateTimeoutMs = 2000; // fenêtre active avant une touche

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
    if (o.contains(QStringLiteral("expect"))) {
        step.kind = UiStepKind::Expect;
        if (!splitTarget(o.value(QStringLiteral("expect")).toString(), step.target, step.property) ||
            !o.contains(QStringLiteral("value"))) {
            why = QStringLiteral("lecture invalide (expect « objet.propriété », value)");
            return false;
        }
        step.value = jsonText(o.value(QStringLiteral("value")));
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

UiScenarioPlayer::UiScenarioPlayer(QQuickWindow* window)
    : m_window(window), m_stylus(std::make_unique<QPointingDevice>(
                            QStringLiteral("actcraft-captures-stylet"), 5180,
                            QInputDevice::DeviceType::Stylus, QPointingDevice::PointerType::Pen,
                            QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 3))
{}

UiScenarioPlayer::~UiScenarioPlayer() = default;

void UiScenarioPlayer::setLineSink(LineSink sink)
{
    m_sink = std::move(sink);
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
    QObject* object = find(step.target);
    // Propriété absente : « <absente> » (propertyText), jamais « "" » qui égalerait une valeur attendue vide.
    const QString actual = object ? propertyText(object->property(step.property.toUtf8().constData()))
                                  : QStringLiteral("<introuvable>");
    const bool ok = object && actual == step.value;
    report << (ok ? QStringLiteral("LU %1.%2=%3 OK").arg(step.target, step.property, actual)
                  : QStringLiteral("LU %1.%2=%3 ECART (attendu %4)")
                        .arg(step.target, step.property, actual, step.value));
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
    // Même chemin qu'une vraie touche et que QTest::keySequence (qt_handleKeyEvent) : QWindowSystemInterface
    // en livraison synchrone, carte des raccourcis de QGuiApplication puis élément actif. QTest jette le
    // booléen « acceptée » ; on le garde : un élément qui reçoit la touche sans l'accepter ne la fait pas
    // réussir.
    const QKeySequence sequence(step.key);
    KeyWitness witness;
    QCoreApplication::instance()->installEventFilter(&witness);
    bool accepted = true;
    for (int i = 0; i < sequence.count(); ++i) {
        const QKeyCombination combination = sequence[static_cast<uint>(i)];
        const int code = static_cast<int>(combination.key());
        const Qt::KeyboardModifiers modifiers = combination.keyboardModifiers();
        const QString text = keyText(combination);
        accepted = QWindowSystemInterface::handleKeyEvent<QWindowSystemInterface::SynchronousDelivery>(
                       m_window, QEvent::KeyPress, code, modifiers, text) &&
                   accepted;
        QWindowSystemInterface::handleKeyEvent<QWindowSystemInterface::SynchronousDelivery>(
            m_window, QEvent::KeyRelease, code, modifiers, text);
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

bool UiScenarioPlayer::runStep(const UiStep& step, QStringList& report)
{
    switch (step.kind) {
    case UiStepKind::Command: {
        QVariant result;
        const bool invoked = QMetaObject::invokeMethod(m_window, "runCommand", Q_RETURN_ARG(QVariant, result),
                                                       Q_ARG(QVariant, QVariant(step.command)),
                                                       Q_ARG(QVariant, step.argument));
        QCoreApplication::processEvents();
        const bool ok = invoked && result.toBool();
        const QString argument =
            step.argument.isValid() ? QStringLiteral(" ") + propertyText(step.argument) : QString();
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
    case UiStepKind::Expect:
        return read(step, report);
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
    if (!QDir().mkpath(outputDir)) {
        report << QStringLiteral("CAPTURE dossier impossible : %1").arg(outputDir);
        emitFrom(report, first);
        return false;
    }
    bool ok = true;
    for (const UiStep& step : scenario.steps) {
        const qsizetype before = report.size();
        const bool stepOk = runStep(step, report);
        emitFrom(report, before); // au fil de l'eau : la ligne sort avant l'étape suivante
        if (!stepOk) {
            ok = false;
            break;
        }
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
