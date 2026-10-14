#include <actcraft/bench/uiscenario.h>

#include <QFile>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

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
    }

    void playsOnARealPageView()
    {
        UiScenario s;
        QString error;
        QVERIFY(parseUiScenario(kAllSteps, s, error));
        Fixture f;
        QVERIFY(load(f));
        QStringList report;
        QVERIFY2(f.player->play(s, f.out.path(), report), qPrintable(report.join(QLatin1Char('\n'))));
        QCOMPARE(f.window->property("dernier").toString(), QStringLiteral("sans.argument"));
        QVERIFY(QFile::exists(f.out.filePath(QStringLiteral("essai-01-fin.png"))));
        QVERIFY(QFile::exists(f.out.filePath(QStringLiteral("index.md"))));
        const QString all = report.join(QLatin1Char('\n'));
        QVERIFY2(all.contains(QLatin1String("TRAIT 2 points OK")), qPrintable(all));
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
};

QTEST_MAIN(TstBenchUiScenario)
#include "tst_bench_uiscenario.moc"
