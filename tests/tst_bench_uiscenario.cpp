#include <actcraft/bench/fixtures.h>
#include <actcraft/bench/uiscenario.h>
#include <actcraft/formats/nativefile.h>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>
#include <QTimer>
#include <QtQml/qqmlextensionplugin.h>
#include <qpa/qwindowsysteminterface.h>

#include <memory>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft::bench;

namespace {

const QByteArray kAllSteps =
    "{\"format\": \"actcraft-scenario/1\","
    " \"name\": \"essai\","
    " \"description\": \"Toutes les sortes d'étapes.\","
    " \"steps\": ["
    "  {\"command\": \"essai.commande\", \"argument\": 3},"
    "  {\"command\": \"sans.argument\"},"
    "  {\"stroke\": [[100, 100], [300, 100]], \"pressure\": 1.0},"
    "  {\"key\": \"F5\"},"
    "  {\"wait\": 10},"
    "  {\"waitReady\": true},"
    "  {\"capture\": \"fin\"},"
    "  {\"checkPixel\": \"trait\", \"at\": [200, 100], \"color\": \"#000000\", \"tolerance\": 60},"
    "  {\"expect\": \"window.compteur\", \"value\": 12}"
    "]}";

// Fenêtre d'essai à vraie PageView. Le joueur est déclaré AVANT le moteur : détruit après la fenêtre, qui
// garde son stylet synthétique.
struct Fixture
{
    std::unique_ptr<UiScenarioPlayer> player;
    QQmlApplicationEngine engine;
    QQuickWindow* window = nullptr;
    QTemporaryDir out;
};

bool load(Fixture& f)
{
    f.engine.load(QUrl::fromLocalFile(QStringLiteral(ACTCRAFT_TESTS_DIR "/data/uiscenario/fenetre.qml")));
    if (f.engine.rootObjects().size() != 1) {
        return false;
    }
    f.window = qobject_cast<QQuickWindow*>(f.engine.rootObjects().constFirst());
    if (!f.window || !QTest::qWaitForWindowExposed(f.window)) {
        return false;
    }
    f.player = std::make_unique<UiScenarioPlayer>(f.window);
    return f.out.isValid();
}

UiScenario scenario(const char* name, const char* steps)
{
    UiScenario s;
    QString error;
    const QByteArray json = QByteArrayLiteral("{\"format\":\"actcraft-scenario/1\",\"name\":\"") + name +
                            QByteArrayLiteral("\",\"steps\":") + steps + '}';
    if (!parseUiScenario(json, s, error)) {
        qWarning("%s", qPrintable(error));
    }
    return s;
}

// Avale le stylet avant la vue (installé après son filtre, donc appelé avant lui) et compte ce qu'il voit :
// un trait posté à la fenêtre qui n'atteint jamais la vue.
class StylusSwallower : public QObject
{
public:
    int presses = 0;
    int moves = 0;
    int releases = 0;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        switch (event->type()) {
        case QEvent::TabletPress:
            ++presses;
            return true;
        case QEvent::TabletMove:
            ++moves;
            return true;
        case QEvent::TabletRelease:
            ++releases;
            return true;
        default:
            return QObject::eventFilter(watched, event);
        }
    }
};

// Accepte tout ShortcutOverride : QWindowSystemInterface::handleShortcutEvent conclut alors que la touche est
// reprise par l'élément actif et ne consulte pas la carte des raccourcis.
class ShortcutMapBypass : public QObject
{
protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::ShortcutOverride) {
            event->accept();
            return true;
        }
        return QObject::eventFilter(watched, event);
    }
};

// Livraison d'une plateforme qui ne consulte pas la carte des raccourcis, comme Qt sous macOS :
// QGuiApplicationPrivate::processKeyEvent n'y appelle pas handleShortcutEvent (« #if !defined(Q_OS_MACOS) »),
// la plateforme Cocoa et QTest (qt_handleKeyEvent) le font avant elle. Reproduite sur toute plateforme par le
// vrai chemin (appui spontané, élément actif), carte neutralisée le temps de la livraison. Pas de
// QCoreApplication::sendEvent : un appui non spontané fait consulter la carte par Qt Quick
// (QQuickDeliveryAgentPrivate::deliverKeyEvent), le test ne distinguerait plus rien.
bool deliverWithoutShortcutMap(QWindow* window, ulong timestamp, QEvent::Type type, int key,
                               Qt::KeyboardModifiers modifiers, const QString& text)
{
    ShortcutMapBypass bypass;
    QCoreApplication::instance()->installEventFilter(&bypass);
    const bool accepted = QWindowSystemInterface::handleKeyEvent<QWindowSystemInterface::SynchronousDelivery>(
        window, timestamp, type, key, modifiers, text);
    QCoreApplication::instance()->removeEventFilter(&bypass);
    return accepted;
}

// Plus long intervalle lu sur une ligne « BOUCLE max=<x> ms … » ; -1 si la ligne n'a pas cette forme.
int loopMax(const QString& line)
{
    static const QRegularExpression pattern(QStringLiteral("^BOUCLE max=(\\d+) ms "));
    const QRegularExpressionMatch match = pattern.match(line);
    return match.hasMatch() ? match.captured(1).toInt() : -1;
}

// Variable d'environnement mise de côté le temps d'un test, puis remise.
class EnvironmentGuard
{
public:
    explicit EnvironmentGuard(const char* name)
        : m_name(name), m_had(qEnvironmentVariableIsSet(name)), m_value(qgetenv(name))
    {}
    ~EnvironmentGuard()
    {
        if (m_had) {
            qputenv(m_name, m_value);
        } else {
            qunsetenv(m_name);
        }
    }
    EnvironmentGuard(const EnvironmentGuard&) = delete;
    EnvironmentGuard& operator=(const EnvironmentGuard&) = delete;

private:
    const char* m_name;
    bool m_had;
    QByteArray m_value;
};

} // namespace

// Joueur de scénarios d'interface (DA8). Pas de chaînes brutes R"(…)" dans ce fichier : moc 6.12 s'y perd et
// n'engendre rien (« No relevant classes found »).
class TstBenchUiScenario : public QObject
{
    Q_OBJECT

private slots:
    void parsesEveryStepKind()
    {
        UiScenario s;
        QString error;
        QVERIFY2(parseUiScenario(kAllSteps, s, error), qPrintable(error));
        QCOMPARE(s.name, QStringLiteral("essai"));
        QCOMPARE(s.steps.size(), std::size_t{9});
        QVERIFY(s.steps[0].kind == UiStepKind::Command);
        QCOMPARE(s.steps[0].argument.toDouble(), 3.0);
        QVERIFY(!s.steps[1].argument.isValid());
        QVERIFY(s.steps[2].kind == UiStepKind::Stroke);
        QCOMPARE(s.steps[2].points.size(), std::size_t{2});
        QCOMPARE(s.steps[2].pressure, 1.0);
        QVERIFY(s.steps[3].kind == UiStepKind::Key);
        QCOMPARE(s.steps[4].ms, 10);
        QVERIFY(s.steps[5].kind == UiStepKind::WaitReady);
        QCOMPARE(s.steps[6].name, QStringLiteral("fin"));
        QCOMPARE(s.steps[7].color, QColor(Qt::black));
        QCOMPARE(s.steps[7].tolerance, 60);
        QCOMPARE(s.steps[8].target, QStringLiteral("window"));
        QCOMPARE(s.steps[8].property, QStringLiteral("compteur"));
        QCOMPARE(s.steps[8].value, QStringLiteral("12"));
    }

    void rejectsBadScenarios_data()
    {
        QTest::addColumn<QByteArray>("json");
        QTest::addColumn<QString>("expected");
        QTest::newRow("format") << QByteArray(
                                       "{\"format\":\"autre/1\",\"name\":\"x\",\"steps\":[{\"wait\":1}]}")
                                << QStringLiteral("format");
        QTest::newRow("vide") << QByteArray(
                                     "{\"format\":\"actcraft-scenario/1\",\"name\":\"x\",\"steps\":[]}")
                              << QStringLiteral("aucune étape");
        QTest::newRow("inconnue")
            << QByteArray("{\"format\":\"actcraft-scenario/1\",\"name\":\"x\",\"steps\":[{\"danse\":1}]}")
            << QStringLiteral("étape 1");
        QTest::newRow("trait")
            << QByteArray(
                   "{\"format\":\"actcraft-scenario/1\",\"name\":\"x\",\"steps\":[{\"stroke\":[[1,2]]}]}")
            << QStringLiteral("moins de 2 points");
        QTest::newRow("lecture") << QByteArray(
                                        "{\"format\":\"actcraft-scenario/"
                                        "1\",\"name\":\"x\",\"steps\":[{\"expect\":\"titre\",\"value\":1}]}")
                                 << QStringLiteral("objet.propriété");
    }

    void rejectsBadScenarios()
    {
        QFETCH(QByteArray, json);
        QFETCH(QString, expected);
        UiScenario s;
        QString error;
        QVERIFY(!parseUiScenario(json, s, error));
        QVERIFY2(error.contains(expected), qPrintable(error));
    }

    void readSpecSplitsAtFirstDotAndEquals()
    {
        UiStep s;
        QVERIFY(parseReadSpec(QStringLiteral("pageIndicator.text=2 / 2"), s));
        QCOMPARE(s.target, QStringLiteral("pageIndicator"));
        QCOMPARE(s.property, QStringLiteral("text"));
        QCOMPARE(s.value, QStringLiteral("2 / 2"));
        QVERIFY(parseReadSpec(QStringLiteral("a.b=x=y"), s));
        QCOMPARE(s.value, QStringLiteral("x=y"));
        QVERIFY(!parseReadSpec(QStringLiteral("sanspoint=1"), s));
        QVERIFY(!parseReadSpec(QStringLiteral("a.b"), s));
    }

    void propertyTextIsStable()
    {
        QCOMPARE(propertyText(true), QStringLiteral("true"));
        QCOMPARE(propertyText(8.0), QStringLiteral("8"));
        QCOMPARE(propertyText(0.5), QStringLiteral("0.5"));
        QCOMPARE(propertyText(QColor(QStringLiteral("#E53935"))), QStringLiteral("#e53935"));
        QCOMPARE(propertyText(QStringLiteral("2 / 2")), QStringLiteral("2 / 2"));
        // Types sans texte : nommés, jamais lus « "" » (qui égalerait une valeur attendue vide).
        QCOMPARE(propertyText(QPointF(1, 2)), QStringLiteral("<non textuel : QPointF>"));
        QCOMPARE(propertyText(QRectF(0, 0, 1, 1)), QStringLiteral("<non textuel : QRectF>"));
        QObject object;
        QCOMPARE(propertyText(QVariant::fromValue(&object)), QStringLiteral("<non textuel : QObject*>"));
        QCOMPARE(propertyText(QVariant()), QStringLiteral("<absente>"));
    }

    void playsOnARealPageView()
    {
        UiScenario s;
        QString error;
        QVERIFY(parseUiScenario(kAllSteps, s, error));
        Fixture f;
        QVERIFY(load(f));
        QStringList report;
        QStringList streamed; // lignes données au fil de l'eau, avant la fin du scénario
        f.player->setLineSink([&streamed](const QString& line) { streamed << line; });
        QVERIFY2(f.player->play(s, f.out.path(), report), qPrintable(report.join(QLatin1Char('\n'))));
        QCOMPARE(streamed, report);
        QCOMPARE(f.window->property("dernier").toString(), QStringLiteral("sans.argument"));
        QVERIFY(QFile::exists(f.out.filePath(QStringLiteral("essai-01-fin.png"))));
        QVERIFY(QFile::exists(f.out.filePath(QStringLiteral("index.md"))));
        const QString all = report.join(QLatin1Char('\n'));
        QVERIFY2(all.contains(QLatin1String("TRAIT 2 points OK")), qPrintable(all));
        QVERIFY2(all.contains(QLatin1String("ATTENTE 10 ms OK")), qPrintable(all));
        QVERIFY2(all.contains(QLatin1String("CAPTURE ") + f.out.filePath(QStringLiteral("essai-01-fin.png")) +
                              QLatin1String(" OK")),
                 qPrintable(all));
        // QTest::keySequence hors écran : le raccourci d'application a bien été déclenché (compteur 12, pas
        // 2).
        QVERIFY2(all.contains(QLatin1String("TOUCHE F5 OK (raccourci)")), qPrintable(all));
        QVERIFY2(all.contains(QLatin1String("PIXEL trait OK")), qPrintable(all));
        QVERIFY2(all.contains(QLatin1String("LU window.compteur=12 OK")), qPrintable(all));
        UiStep wrong;
        QVERIFY(parseReadSpec(QStringLiteral("window.compteur=99"), wrong));
        QVERIFY(!f.player->expect(wrong, report));
        QVERIFY(report.last().contains(QLatin1String("ECART")));
        UiStep missing;
        QVERIFY(parseReadSpec(QStringLiteral("absent.text=x"), missing));
        QVERIFY(!f.player->expect(missing, report));
        // Propriété absente attendue vide : une valeur invalide lue « "" » passerait à tort.
        UiStep absentProperty;
        QVERIFY(parseReadSpec(QStringLiteral("window.absente="), absentProperty));
        QVERIFY(!f.player->expect(absentProperty, report));
        QVERIFY2(report.last().contains(QLatin1String("ECART")), qPrintable(report.last()));
    }

    // Discriminant : un trait posté à la fenêtre mais jamais reçu par la vue doit faire échouer l'étape. Un
    // joueur qui écrit « TRAIT OK » dès que les événements sont postés passerait ici.
    void strokeThatNeverReachesTheViewFails()
    {
        Fixture f;
        QVERIFY(load(f));
        StylusSwallower swallower;
        f.window->installEventFilter(&swallower); // après la vue : appelé avant son filtre
        const UiScenario s = scenario(
            "perdu", "[{\"waitReady\":true},{\"stroke\":[[100,100],[300,100]]},{\"command\":\"apres\"}]");
        QCOMPARE(s.steps.size(), std::size_t{3});
        QStringList report;
        QVERIFY2(!f.player->play(s, f.out.path(), report), qPrintable(report.join(QLatin1Char('\n'))));
        QVERIFY2(report.last().startsWith(QLatin1String("TRAIT")) &&
                     report.last().contains(QLatin1String("ECHEC")),
                 qPrintable(report.last()));
        // Postés à la fenêtre (là où le filtre de la vue les attend), pas à l'élément.
        QCOMPARE(swallower.presses, 1);
        QVERIFY(swallower.moves > 0);
        QCOMPARE(swallower.releases, 1);
        QVERIFY(f.window->property("dernier").toString().isEmpty()); // arrêt à l'étape en échec
    }

    // Discriminant : une touche qui n'atteint ni raccourci ni élément actif fait échouer l'étape.
    void keyThatReachesNothingFails()
    {
        Fixture f;
        QVERIFY(load(f));
        const UiScenario s = scenario("touche", "[{\"key\":\"F6\"}]");
        QCOMPARE(s.steps.size(), std::size_t{1});
        QStringList report;
        QVERIFY2(!f.player->play(s, f.out.path(), report), qPrintable(report.join(QLatin1Char('\n'))));
        QVERIFY2(report.last().startsWith(QLatin1String("TOUCHE F6 ECHEC")), qPrintable(report.last()));
        QCOMPARE(f.window->property("compteur").toInt(), 0);
    }

    // Discriminant : l'élément « piege » a le focus et reçoit F6 sans l'accepter ; F7 est un raccourci
    // ambigu. Recevoir n'est pas accepter, et un raccourci ambigu ne joue rien.
    void keyReceivedButIgnoredOrAmbiguousFails()
    {
        Fixture f;
        QVERIFY(load(f));
        auto* trap = f.window->findChild<QQuickItem*>(QStringLiteral("piege"));
        QVERIFY(trap);
        f.window->requestActivate();
        QTRY_VERIFY(trap->hasActiveFocus()); // piège armé : F6 lui est bien livrée
        QStringList report;
        QVERIFY(!f.player->play(scenario("ignoree", "[{\"key\":\"F6\"}]"), f.out.path(), report));
        QVERIFY2(report.last().startsWith(QLatin1String("TOUCHE F6 ECHEC")), qPrintable(report.last()));
        QVERIFY(!f.player->play(scenario("ambigue", "[{\"key\":\"F7\"}]"), f.out.path(), report));
        QVERIFY2(report.last().startsWith(QLatin1String("TOUCHE F7 ECHEC")) &&
                     report.last().contains(QLatin1String("ambigu")),
                 qPrintable(report.last()));
        QCOMPARE(f.window->property("compteur").toInt(), 0);
    }

    // Discriminant : un raccourci est joué même quand la livraison de la plateforme ne consulte pas la carte
    // des raccourcis (Qt sous macOS). Un joueur qui s'en remet à QWindowSystemInterface::handleKeyEvent passe
    // sous Windows et échoue sous macOS (run 38690918062 : « TOUCHE F5 ECHEC (ni raccourci ni élément actif…)
    // »).
    void keyReachesShortcutsWhenDeliveryIgnoresThem()
    {
        Fixture f;
        QVERIFY(load(f));
        f.window->requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(f.window));
        // Prémisse : cette livraison seule n'atteint pas le raccourci F5, comme sous macOS.
        deliverWithoutShortcutMap(f.window, 0, QEvent::KeyPress, Qt::Key_F5, Qt::NoModifier, QString());
        deliverWithoutShortcutMap(f.window, 0, QEvent::KeyRelease, Qt::Key_F5, Qt::NoModifier, QString());
        QCOMPARE(f.window->property("compteur").toInt(), 0);

        f.player->setKeyDelivery(deliverWithoutShortcutMap);
        QStringList report;
        QVERIFY2(f.player->play(scenario("raccourci", "[{\"key\":\"F5\"}]"), f.out.path(), report),
                 qPrintable(report.join(QLatin1Char('\n'))));
        QCOMPARE(report.last(), QStringLiteral("TOUCHE F5 OK (raccourci)"));
        QCOMPARE(f.window->property("compteur").toInt(), 10);
        // Le contrôle « acceptée » tient sur ce chemin : ambiguë, et reçue sans être acceptée, échouent.
        QVERIFY(!f.player->play(scenario("ambigue", "[{\"key\":\"F7\"}]"), f.out.path(), report));
        QVERIFY2(report.last().startsWith(QLatin1String("TOUCHE F7 ECHEC")) &&
                     report.last().contains(QLatin1String("ambigu")),
                 qPrintable(report.last()));
        QVERIFY(!f.player->play(scenario("ignoree", "[{\"key\":\"F6\"}]"), f.out.path(), report));
        QVERIFY2(report.last().startsWith(QLatin1String("TOUCHE F6 ECHEC")), qPrintable(report.last()));
        QCOMPARE(f.window->property("compteur").toInt(), 10);
    }

    // Discriminant : une capture uniforme (rien de dessiné) est refusée.
    void uniformCaptureFails()
    {
        Fixture f;
        QVERIFY(load(f));
        auto* view = f.window->findChild<QQuickItem*>(QStringLiteral("pageView"));
        QVERIFY(view);
        view->setVisible(false); // reste le seul fond de la fenêtre
        QStringList report;
        QVERIFY(!f.player->play(scenario("uniforme", "[{\"capture\":\"vide\"}]"), f.out.path(), report));
        QVERIFY2(report.last().startsWith(QLatin1String("CAPTURE")) &&
                     report.last().contains(QLatin1String("ECHEC")),
                 qPrintable(report.last()));
    }

    // Discriminant : le contrôle de pixel lit l'image du moment (pas la dernière capture) et peut échouer.
    void pixelCheckReadsTheCurrentImage()
    {
        Fixture f;
        QVERIFY(load(f));
        const UiScenario s = scenario(
            "frais",
            "[{\"waitReady\":true},"
            " {\"capture\":\"avant\"},"
            " {\"checkPixel\":\"blanc-avant\",\"at\":[200,400],\"color\":\"#ffffff\",\"tolerance\":24},"
            " {\"stroke\":[[100,400],[300,400]],\"pressure\":1.0},"
            " {\"waitReady\":true},"
            " {\"checkPixel\":\"noir-apres\",\"at\":[200,400],\"color\":\"#000000\",\"tolerance\":60}]");
        QCOMPARE(s.steps.size(), std::size_t{6});
        QStringList report;
        QVERIFY2(f.player->play(s, f.out.path(), report), qPrintable(report.join(QLatin1Char('\n'))));

        const UiScenario blank = scenario(
            "blanc",
            "[{\"checkPixel\":\"noir-ailleurs\",\"at\":[600,600],\"color\":\"#000000\",\"tolerance\":60}]");
        QVERIFY(!f.player->play(blank, f.out.path(), report));
        QVERIFY2(report.last().startsWith(QLatin1String("PIXEL noir-ailleurs ECART")),
                 qPrintable(report.last()));

        const UiScenario outside = scenario(
            "dehors",
            "[{\"checkPixel\":\"dehors\",\"at\":[5000,5000],\"color\":\"#ffffff\",\"tolerance\":255}]");
        QVERIFY(!f.player->play(outside, f.out.path(), report));
        QVERIFY2(report.last().contains(QLatin1String("hors image")), qPrintable(report.last()));
    }

    // Une étape en échec arrête le scénario ; l'index le dit.
    void stopsAtFirstFailingStep()
    {
        Fixture f;
        QVERIFY(load(f));
        const UiScenario s = scenario("arret", "[{\"command\":\"inconnue\"},{\"command\":\"apres\"}]");
        QCOMPARE(s.steps.size(), std::size_t{2});
        QStringList report;
        QVERIFY(!f.player->play(s, f.out.path(), report));
        QCOMPARE(report.size(), qsizetype{1});
        QCOMPARE(report.last(), QStringLiteral("COMMANDE inconnue ECHEC"));
        QVERIFY(f.window->property("dernier").toString().isEmpty());
        QFile index(f.out.filePath(QStringLiteral("index.md")));
        QVERIFY(index.open(QIODevice::ReadOnly));
        QVERIFY(QString::fromUtf8(index.readAll()).contains(QStringLiteral("en échec")));
    }

    // Étapes de L2-T20 (S5) : fixture, attente d'une propriété, sonde de boucle, mémoire, archive.
    void newStepsAreParsed()
    {
        UiScenario s;
        QString error;
        QVERIFY2(
            parseUiScenario(
                "{\"format\":\"actcraft-scenario/1\",\"name\":\"x\",\"steps\":["
                "{\"fixture\":\"trois-pages\",\"path\":\"${SORTIE}/a.actcraft\"},"
                "{\"waitFor\":\"documentController.busy\",\"value\":false,\"timeoutMs\":5000},"
                "{\"watchEventLoop\":true},"
                "{\"maxEventLoopGapMs\":25},"
                "{\"remember\":\"documentController.fingerprint\",\"as\":\"avant\"},"
                "{\"archive\":\"${SORTIE}/a.actcraft\",\"contains\":[\"document.json\",\"thumbnails/\"]},"
                "{\"waitFor\":\"a.b\",\"value\":\"x\"}]}",
                s, error),
            qPrintable(error));
        QCOMPARE(s.steps.size(), std::size_t{7});
        QVERIFY(s.steps[0].kind == UiStepKind::Fixture);
        QCOMPARE(s.steps[0].name, QStringLiteral("trois-pages"));
        QCOMPARE(s.steps[0].path, QStringLiteral("${SORTIE}/a.actcraft"));
        QVERIFY(s.steps[1].kind == UiStepKind::WaitFor);
        QCOMPARE(s.steps[1].target, QStringLiteral("documentController"));
        QCOMPARE(s.steps[1].property, QStringLiteral("busy"));
        QCOMPARE(s.steps[1].value, QStringLiteral("false"));
        QCOMPARE(s.steps[1].ms, 5000);
        QVERIFY(s.steps[2].kind == UiStepKind::WatchEventLoop);
        QVERIFY(s.steps[3].kind == UiStepKind::MaxEventLoopGap);
        QCOMPARE(s.steps[3].ms, 25);
        QVERIFY(s.steps[4].kind == UiStepKind::Remember);
        QCOMPARE(s.steps[4].name, QStringLiteral("avant"));
        QCOMPARE(s.steps[4].property, QStringLiteral("fingerprint"));
        QVERIFY(s.steps[5].kind == UiStepKind::ArchiveContains);
        QCOMPARE(s.steps[5].path, QStringLiteral("${SORTIE}/a.actcraft"));
        QCOMPARE(s.steps[5].values,
                 (QStringList{QStringLiteral("document.json"), QStringLiteral("thumbnails/")}));
        QCOMPARE(s.steps[6].ms, 10000); // attente par défaut : 10 s
        // Contrat du lot 1 : les nouvelles sortes s'ajoutent à la fin, les anciennes gardent leur rang.
        QCOMPARE(static_cast<int>(UiStepKind::Expect), 7);
        QCOMPARE(static_cast<int>(UiStepKind::Fixture), 8);
    }

    void badNewStepsAreRefused_data()
    {
        QTest::addColumn<QByteArray>("step");
        QTest::addColumn<QString>("expected"); // motif propre à la ligne
        QTest::newRow("fixture sans chemin")
            << QByteArray("{\"fixture\":\"trois-pages\"}") << QStringLiteral("fixture invalide");
        QTest::newRow("fixture inconnue")
            << QByteArray("{\"fixture\":\"inconnue\",\"path\":\"x\"}") << QStringLiteral("fixture invalide");
        QTest::newRow("waitFor sans valeur")
            << QByteArray("{\"waitFor\":\"a.b\"}") << QStringLiteral("attente invalide (waitFor");
        QTest::newRow("waitFor sans point")
            << QByteArray("{\"waitFor\":\"ab\",\"value\":1}") << QStringLiteral("attente invalide (waitFor");
        // Bornes d'attente (entrée non fiable) : ni nulle, ni négative, ni sans fin.
        QTest::newRow("attente nulle") << QByteArray("{\"waitFor\":\"a.b\",\"value\":1,\"timeoutMs\":0}")
                                       << QStringLiteral("timeoutMs de 1 à 120000");
        QTest::newRow("attente négative") << QByteArray("{\"waitFor\":\"a.b\",\"value\":1,\"timeoutMs\":-5}")
                                          << QStringLiteral("timeoutMs de 1 à 120000");
        QTest::newRow("attente sans fin")
            << QByteArray("{\"waitFor\":\"a.b\",\"value\":1,\"timeoutMs\":2000000000}")
            << QStringLiteral("timeoutMs de 1 à 120000");
        QTest::newRow("attente non numérique")
            << QByteArray("{\"waitFor\":\"a.b\",\"value\":1,\"timeoutMs\":\"long\"}")
            << QStringLiteral("timeoutMs de 1 à 120000");
        QTest::newRow("seuil de boucle nul")
            << QByteArray("{\"maxEventLoopGapMs\":0}") << QStringLiteral("seuil de boucle invalide");
        QTest::newRow("seuil de boucle démesuré")
            << QByteArray("{\"maxEventLoopGapMs\":2000000000}") << QStringLiteral("seuil de boucle invalide");
        QTest::newRow("remember sans nom")
            << QByteArray("{\"remember\":\"a.b\"}") << QStringLiteral("mémoire invalide");
        // Noms réservés : ${SORTIE} et ${SAMPLES} ne se redéfinissent pas (une fixture irait ailleurs).
        QTest::newRow("remember SORTIE")
            << QByteArray("{\"remember\":\"a.b\",\"as\":\"SORTIE\"}") << QStringLiteral("mémoire invalide");
        QTest::newRow("remember SAMPLES")
            << QByteArray("{\"remember\":\"a.b\",\"as\":\"SAMPLES\"}") << QStringLiteral("mémoire invalide");
        QTest::newRow("remember nom invalide")
            << QByteArray("{\"remember\":\"a.b\",\"as\":\"a}b\"}") << QStringLiteral("mémoire invalide");
        QTest::newRow("archive sans liste")
            << QByteArray("{\"archive\":\"x\"}") << QStringLiteral("archive invalide (archive, contains)");
        QTest::newRow("archive liste vide") << QByteArray("{\"archive\":\"x\",\"contains\":[]}")
                                            << QStringLiteral("archive invalide (archive, contains)");
        // Un préfixe vide serait trouvé dans toute archive : refusé, comme une entrée non textuelle.
        QTest::newRow("archive entrée vide") << QByteArray("{\"archive\":\"x\",\"contains\":[\"\"]}")
                                             << QStringLiteral("entrée vide ou non textuelle");
        QTest::newRow("archive entrée nombre") << QByteArray("{\"archive\":\"x\",\"contains\":[3]}")
                                               << QStringLiteral("entrée vide ou non textuelle");
    }

    void badNewStepsAreRefused()
    {
        QFETCH(QByteArray, step);
        QFETCH(QString, expected);
        UiScenario s;
        QString error;
        QVERIFY(!parseUiScenario(
            "{\"format\":\"actcraft-scenario/1\",\"name\":\"x\",\"steps\":[" + step + "]}", s, error));
        QVERIFY2(error.startsWith(QStringLiteral("étape 1")) && error.contains(expected), qPrintable(error));
    }

    void fixtureWritesAThreePageDocument()
    {
        QTemporaryDir dir;
        QString error;
        const QString path = dir.filePath(QStringLiteral("f.actcraft"));
        QVERIFY2(buildFixture(QStringLiteral("trois-pages"), path, error), qPrintable(error));
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const actcraft::formats::DecodeResult r = actcraft::formats::decodeActcraft(file.readAll());
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.document.pageCount(), std::size_t{3});
        QCOMPARE(r.document.resources().size(), std::size_t{1});
        QVERIFY(r.document.page(0).objects.empty()); // le trait y est ajouté au stylet
        QCOMPARE(r.document.page(1).objects.size(), std::size_t{2});
        QVERIFY(std::get_if<actcraft::core::Shape>(&r.document.page(1).objects[0]));
        QVERIFY(std::get_if<actcraft::core::Text>(&r.document.page(1).objects[1]));
        QCOMPARE(r.document.page(2).objects.size(), std::size_t{1});
        const auto* image = std::get_if<actcraft::core::ImageObject>(&r.document.page(2).objects[0]);
        QVERIFY(image);
        QVERIFY(r.document.resource(image->resource));
        QVERIFY(!buildFixture(QStringLiteral("inconnue"), path, error));
        QVERIFY(error.contains(QStringLiteral("inconnue")));
        QVERIFY(fixtureNames().contains(QStringLiteral("trois-pages")));
    }

    void rememberThenExpect()
    {
        Fixture f;
        QVERIFY(load(f));
        QStringList report;
        QVERIFY2(f.player->play(scenario("memoire", "[{\"remember\":\"window.width\",\"as\":\"l\"},"
                                                    " {\"expect\":\"window.width\",\"value\":\"${l}\"}]"),
                                f.out.path(), report),
                 qPrintable(report.join(QLatin1Char('\n'))));
        QCOMPARE(report.at(0), QStringLiteral("MEMO l=800"));
        QCOMPARE(report.at(1), QStringLiteral("LU window.width=800 OK"));
        // Objet absent : rien n'est mémorisé, l'étape échoue.
        QVERIFY(!f.player->play(scenario("absent", "[{\"remember\":\"absent.text\",\"as\":\"m\"}]"),
                                f.out.path(), report));
        QVERIFY2(report.last().startsWith(QLatin1String("MEMO m ECHEC")), qPrintable(report.last()));
        // Propriété absente : « <absente> » mémorisé égalerait plus tard une autre propriété absente.
        QVERIFY(!f.player->play(scenario("propriete", "[{\"remember\":\"window.absente\",\"as\":\"m\"},"
                                                      " {\"expect\":\"window.autre\",\"value\":\"${m}\"}]"),
                                f.out.path(), report));
        QCOMPARE(report.last(), QStringLiteral("MEMO m ECHEC (window.absente : <absente>)"));
    }

    // Les variables sont remplacées dans l'argument d'une commande et la valeur attendue, en une passe : une
    // valeur mémorisée qui contient « ${SORTIE} » reste telle quelle (elle vient de l'interface, pas du
    // scénario).
    void variablesAreExpandedOnce()
    {
        Fixture f;
        QVERIFY(load(f));
        QStringList report;
        QVERIFY2(f.player->play(
                     scenario("variables",
                              "[{\"command\":\"essai\",\"argument\":\"${SORTIE}/x.actcraft\"},"
                              " {\"expect\":\"window.dernier\",\"value\":\"essai:${SORTIE}/x.actcraft\"}]"),
                     f.out.path(), report),
                 qPrintable(report.join(QLatin1Char('\n'))));
        QCOMPARE(f.window->property("dernier").toString(),
                 QStringLiteral("essai:") + f.out.path() + QStringLiteral("/x.actcraft"));
        QCOMPARE(report.at(0),
                 QStringLiteral("COMMANDE essai ") + f.out.path() + QStringLiteral("/x.actcraft OK"));

        f.window->setProperty("dernier", QStringLiteral("${SORTIE}"));
        QVERIFY2(f.player->play(scenario("une-passe", "[{\"remember\":\"window.dernier\",\"as\":\"v\"},"
                                                      " {\"expect\":\"window.dernier\",\"value\":\"${v}\"}]"),
                                f.out.path(), report),
                 qPrintable(report.join(QLatin1Char('\n'))));
        QCOMPARE(report.last(), QStringLiteral("LU window.dernier=${SORTIE} OK"));
    }

    // Variable inconnue ou mal fermée : l'étape échoue, rien n'est joué avec le texte brut.
    void unknownVariableFails_data()
    {
        QTest::addColumn<QByteArray>("steps");
        QTest::addColumn<QString>("expected");
        QTest::newRow("lecture inconnue")
            << QByteArray("[{\"expect\":\"window.width\",\"value\":\"${absente}\"}]")
            << QStringLiteral("LU window.width ECHEC (variable inconnue « absente »)");
        QTest::newRow("lecture non fermée")
            << QByteArray("[{\"expect\":\"window.width\",\"value\":\"${SORTIE\"}]")
            << QStringLiteral("LU window.width ECHEC (variable non fermée)");
        QTest::newRow("commande inconnue")
            << QByteArray("[{\"command\":\"essai\",\"argument\":\"${absente}/x\"}]")
            << QStringLiteral("COMMANDE essai ECHEC (variable inconnue « absente »)");
        QTest::newRow("attente inconnue")
            << QByteArray("[{\"waitFor\":\"window.width\",\"value\":\"${absente}\",\"timeoutMs\":100}]")
            << QStringLiteral("ATTENTE window.width ECHEC (variable inconnue « absente »)");
    }

    void unknownVariableFails()
    {
        QFETCH(QByteArray, steps);
        QFETCH(QString, expected);
        Fixture f;
        QVERIFY(load(f));
        QStringList report;
        QVERIFY(!f.player->play(scenario("inconnue", steps.constData()), f.out.path(), report));
        QCOMPARE(report.last(), expected);
        QVERIFY(f.window->property("dernier").toString().isEmpty()); // commande jamais jouée
    }

    // ${SAMPLES} vient d'ACTCRAFT_SAMPLES_DIR ; absente, l'étape échoue au lieu de chercher « /… ».
    void samplesVariableNeedsTheEnvironment()
    {
        EnvironmentGuard guard("ACTCRAFT_SAMPLES_DIR");
        Fixture f;
        QVERIFY(load(f));
        qunsetenv("ACTCRAFT_SAMPLES_DIR");
        const UiScenario s =
            scenario("samples", "[{\"command\":\"essai\",\"argument\":\"${SAMPLES}/a.flipchart\"}]");
        QStringList report;
        QVERIFY(!f.player->play(s, f.out.path(), report));
        QCOMPARE(report.last(), QStringLiteral("COMMANDE essai ECHEC (ACTCRAFT_SAMPLES_DIR absent)"));
        QVERIFY(f.window->property("dernier").toString().isEmpty());
        qputenv("ACTCRAFT_SAMPLES_DIR", QByteArrayLiteral("C:/jeux"));
        QVERIFY2(f.player->play(s, f.out.path(), report), qPrintable(report.last()));
        QCOMPARE(f.window->property("dernier").toString(), QStringLiteral("essai:C:/jeux/a.flipchart"));
    }

    // Fixture puis archive : l'archive écrite contient ses entrées ; une entrée qui manque est nommée.
    void fixtureStepThenArchive()
    {
        Fixture f;
        QVERIFY(load(f));
        const QString path = f.out.filePath(QStringLiteral("a.actcraft"));
        QStringList report;
        QVERIFY2(f.player->play(scenario("archive",
                                         "[{\"fixture\":\"trois-pages\",\"path\":\"${SORTIE}/a.actcraft\"},"
                                         " {\"archive\":\"${SORTIE}/a.actcraft\","
                                         "  \"contains\":[\"document.json\",\"resources/\"]}]"),
                                f.out.path(), report),
                 qPrintable(report.join(QLatin1Char('\n'))));
        QCOMPARE(report.at(0), QStringLiteral("FIXTURE trois-pages ") + path + QStringLiteral(" OK"));
        QCOMPARE(report.at(1), QStringLiteral("ARCHIVE ") + path + QStringLiteral(" OK"));
        QVERIFY(QFile::exists(path));

        QVERIFY(!f.player->play(scenario("manque",
                                         "[{\"archive\":\"${SORTIE}/a.actcraft\","
                                         " \"contains\":[\"document.json\",\"thumbnails/\",\"absent\"]}]"),
                                f.out.path(), report));
        QCOMPARE(report.last(),
                 QStringLiteral("ARCHIVE ") + path + QStringLiteral(" ECART (manque thumbnails/, absent)"));
        QVERIFY(!f.player->play(scenario("introuvable", "[{\"archive\":\"${SORTIE}/rien.actcraft\","
                                                        " \"contains\":[\"document.json\"]}]"),
                                f.out.path(), report));
        QVERIFY2(report.last().startsWith(QStringLiteral("ARCHIVE ") +
                                          f.out.filePath(QStringLiteral("rien.actcraft")) +
                                          QStringLiteral(" ECHEC")),
                 qPrintable(report.last()));
    }

    // Le chemin d'une fixture vient du scénario (entrée non fiable) : elle n'écrit que sous le dossier de
    // sortie, jamais à côté ni ailleurs.
    void fixtureOutsideTheOutputDirIsRefused()
    {
        Fixture f;
        QVERIFY(load(f));
        QTemporaryDir elsewhere;
        QVERIFY(elsewhere.isValid());
        const QString outside = elsewhere.filePath(QStringLiteral("ailleurs.actcraft"));
        const QString sibling = QDir::cleanPath(f.out.path() + QStringLiteral("/../voisin-L2-T20.actcraft"));
        QFile::remove(sibling); // reste d'un passage précédent (mutation)
        const QList<QString> paths{outside, QStringLiteral("${SORTIE}/../voisin-L2-T20.actcraft"),
                                   QStringLiteral("${SORTIE}/sous/../../voisin-L2-T20.actcraft")};
        for (const QString& path : paths) {
            QStringList report;
            const QByteArray steps = "[{\"fixture\":\"trois-pages\",\"path\":\"" + path.toUtf8() + "\"}]";
            QVERIFY2(!f.player->play(scenario("dehors", steps.constData()), f.out.path(), report),
                     qPrintable(report.join(QLatin1Char('\n'))));
            QVERIFY2(report.last().startsWith(QLatin1String("FIXTURE trois-pages ECHEC")) &&
                         report.last().contains(QLatin1String("hors du dossier de sortie")),
                     qPrintable(report.last()));
        }
        QVERIFY(!QFile::exists(outside));
        QVERIFY(!QFile::exists(sibling));
    }

    // Attente d'une propriété : elle suit la valeur qui change plus tard (pas une lecture unique), et échoue
    // à son plafond sans le dépasser de beaucoup.
    void waitForFollowsTheProperty()
    {
        Fixture f;
        QVERIFY(load(f));
        QTimer::singleShot(200, f.window, [&f] { f.window->setProperty("compteur", 7); });
        QStringList report;
        QVERIFY2(f.player->play(scenario("attente", "[{\"waitFor\":\"window.compteur\",\"value\":7}]"),
                                f.out.path(), report),
                 qPrintable(report.join(QLatin1Char('\n'))));
        QCOMPARE(report.last(), QStringLiteral("ATTENTE window.compteur=7 OK"));

        QElapsedTimer clock;
        clock.start();
        QVERIFY(!f.player->play(
            scenario("plafond", "[{\"waitFor\":\"window.compteur\",\"value\":99,\"timeoutMs\":200}]"),
            f.out.path(), report));
        const qint64 elapsed = clock.elapsed();
        QCOMPARE(report.last(), QStringLiteral("ATTENTE window.compteur=99 ECHEC (lu 7 après 200 ms)"));
        QVERIFY2(elapsed >= 200 && elapsed < 5000, qPrintable(QString::number(elapsed)));
        QVERIFY(
            !f.player->play(scenario("objet", "[{\"waitFor\":\"absent.x\",\"value\":1,\"timeoutMs\":100}]"),
                            f.out.path(), report));
        QVERIFY2(report.last().startsWith(QLatin1String("ATTENTE absent.x ECHEC")),
                 qPrintable(report.last()));
    }

    // Sonde au repos : le plus long intervalle est mesuré et reste sous un seuil large.
    void eventLoopProbeMeasures()
    {
        Fixture f;
        QVERIFY(load(f));
        QStringList report;
        QVERIFY2(f.player->play(scenario("sonde", "[{\"watchEventLoop\":true},{\"wait\":300},"
                                                  " {\"maxEventLoopGapMs\":100}]"),
                                f.out.path(), report),
                 qPrintable(report.join(QLatin1Char('\n'))));
        QCOMPARE(report.at(0), QStringLiteral("BOUCLE sonde démarrée"));
        QVERIFY2(loopMax(report.last()) >= 0 && report.last().endsWith(QLatin1String(" ms OK")),
                 qPrintable(report.last()));
        // Arrêtée : une seconde comparaison sans nouvelle sonde échoue.
        QVERIFY(!f.player->play(scenario("arretee", "[{\"maxEventLoopGapMs\":100}]"), f.out.path(), report));
        QCOMPARE(report.last(), QStringLiteral("BOUCLE ECHEC (sonde non démarrée)"));
    }

    // Discriminant (écart 1 du plan) : la sonde voit un fil principal bloqué alors qu'aucune image n'est
    // produite (hors écran, les images ne viennent qu'à la demande). Une mesure de la durée des images
    // (frameSwapped, patron du banc) ne verrait rien ici et dirait « OK » ; une mesure de l'écart entre deux
    // images échouerait au repos (eventLoopProbeMeasures).
    void eventLoopProbeSeesABlockedMainThread()
    {
        Fixture f;
        QVERIFY(load(f));
        // Blocage armé une fois la sonde démarrée (son démarrage attend une image, de durée variable).
        f.player->setLineSink([&f](const QString& line) {
            if (line.startsWith(QStringLiteral("BOUCLE sonde démarrée"))) {
                QTimer::singleShot(100, f.window, [] { QThread::msleep(250); });
            }
        });
        QStringList report;
        QVERIFY2(!f.player->play(scenario("bloque", "[{\"watchEventLoop\":true},{\"wait\":600},"
                                                    " {\"maxEventLoopGapMs\":100}]"),
                                 f.out.path(), report),
                 qPrintable(report.join(QLatin1Char('\n'))));
        QVERIFY2(loopMax(report.last()) >= 250 &&
                     report.last().contains(QLatin1String("ECART (seuil 100 ms)")),
                 qPrintable(report.join(QLatin1Char('\n'))));
    }

    // Discriminant : un blocage juste avant la comparaison, sans tour de boucle entre les deux (ici, la
    // sortie au fil de l'eau qui bloque), compte aussi : l'intervalle en cours à l'arrêt de la sonde est
    // mesuré.
    void eventLoopProbeCountsTheLastInterval()
    {
        Fixture f;
        QVERIFY(load(f));
        f.player->setLineSink([](const QString& line) {
            if (line == QLatin1String("ATTENTE 50 ms OK")) {
                QThread::msleep(250);
            }
        });
        QStringList report;
        QVERIFY(!f.player->play(scenario("fin", "[{\"watchEventLoop\":true},{\"wait\":50},"
                                                " {\"maxEventLoopGapMs\":100}]"),
                                f.out.path(), report));
        QVERIFY2(loopMax(report.last()) >= 250 && report.last().contains(QLatin1String("ECART")),
                 qPrintable(report.join(QLatin1Char('\n'))));
    }

    // Discriminant : une image en attente au démarrage de la sonde (ici, une image dont la préparation bloque
    // le fil principal 250 ms, comme le rendu logiciel d'un trait en debug) est rendue AVANT que la sonde
    // s'arme. Armée tout de suite, la sonde mesurerait ce rendu (ECART) au lieu de la boucle qui suit.
    void eventLoopProbeStartsAfterThePendingFrame()
    {
        Fixture f;
        QVERIFY(load(f));
        int frames = 0;
        const auto slow = QObject::connect(
            f.window, &QQuickWindow::afterAnimating, f.window,
            [&frames] {
                if (++frames == 1) {
                    QThread::msleep(250);
                }
            },
            Qt::DirectConnection);
        f.window->update(); // image en attente, pas encore rendue (aucun tour de boucle avant play)
        QStringList report;
        QVERIFY2(f.player->play(scenario("attente-image", "[{\"watchEventLoop\":true},{\"wait\":100},"
                                                          " {\"maxEventLoopGapMs\":100}]"),
                                f.out.path(), report),
                 qPrintable(report.join(QLatin1Char('\n'))));
        QObject::disconnect(slow);
        QVERIFY(frames >= 1); // l'image lente a bien été rendue, avant la sonde
        QCOMPARE(report.at(0), QStringLiteral("BOUCLE sonde démarrée"));
    }

    // L'attente de l'image est bornée : fenêtre cachée, aucune image ne vient, la sonde s'arme après 1 s et
    // le dit, sans bloquer le scénario.
    void eventLoopProbeStartIsBounded()
    {
        Fixture f;
        QVERIFY(load(f));
        f.window->hide();
        QTRY_VERIFY(!f.window->isExposed());
        QElapsedTimer clock;
        clock.start();
        QStringList report;
        QVERIFY2(f.player->play(
                     scenario("sans-image", "[{\"watchEventLoop\":true},{\"maxEventLoopGapMs\":60000}]"),
                     f.out.path(), report),
                 qPrintable(report.join(QLatin1Char('\n'))));
        const qint64 elapsed = clock.elapsed();
        QCOMPARE(report.at(0), QStringLiteral("BOUCLE sonde démarrée (aucune image rendue en 1000 ms)"));
        QVERIFY2(elapsed >= 1000 && elapsed < 5000, qPrintable(QString::number(elapsed)));
    }
};

QTEST_MAIN(TstBenchUiScenario)
#include "tst_bench_uiscenario.moc"
