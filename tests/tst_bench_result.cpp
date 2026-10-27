#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSysInfo>
#include <QTest>

// Juge le JSON produit par tst_bench_smoke (fixture ctest bench_smoke_json).
class TstBenchResult : public QObject
{
    Q_OBJECT

private:
    QJsonObject m_root;
    QByteArray m_text;

private slots:
    void initTestCase()
    {
        const QString path = qEnvironmentVariable("ACTCRAFT_BENCH_SMOKE_JSON");
        QVERIFY2(!path.isEmpty(), "ACTCRAFT_BENCH_SMOKE_JSON absent");
        QFile file(path);
        QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(path));
        m_text = file.readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(m_text);
        QVERIFY(doc.isObject());
        m_root = doc.object();
    }

    void formatIsVersioned()
    {
        QCOMPARE(m_root.value("format").toString(), QStringLiteral("actcraft-bench/1"));
    }

    void softwareRenderer()
    {
        QCOMPARE(m_root.value("graphics_api").toString(), QStringLiteral("software"));
        QVERIFY(m_root.value("informative").toBool());
    }

    void validityFollowsBuildType()
    {
        QCOMPARE(m_root.value("valid_for_measurement").toBool(), ACTCRAFT_EXPECT_VALID != 0);
        if constexpr (ACTCRAFT_EXPECT_VALID == 0) {
            const QJsonArray reasons = m_root.value("verdict").toObject().value("invalid_reasons").toArray();
            QVERIFY(reasons.contains(QStringLiteral("build-not-release")));
        }
    }

    // L2-T23 : « page » ; L10-T1 : « pauses » (reprises après le délai d'inactivité du cache) ; L3-T19 :
    // « gommer » et « soulever » ; L4-T14 : « forme-page-annotee » et « grille-dessus » ; L4-T29 :
    // « miniatures », « miniatures-ferme » et « miniatures-pauses ». Remplace lot4ScenariosWithFrames
    // (lui-même remplaçant de sevenScenariosWithFrames) : le banc compte onze scénarios, non plus neuf (les
    // neuf premiers gardent leur rang). Amendement 4 (V9) : « forme-page-annotee » écrit la clé
    // shape_preview, relue ici.
    void lot4bScenariosWithFrames()
    {
        const QJsonArray scenarios = m_root.value("scenarios").toArray();
        QCOMPARE(scenarios.size(), 13); // L4-T28 : « zones-zoom » en dernier (douze avant)
        const QStringList names{QStringLiteral("stroke"),           QStringLiteral("zoom"),
                                QStringLiteral("scroll"),           QStringLiteral("page"),
                                QStringLiteral("pauses"),           QStringLiteral("gommer"),
                                QStringLiteral("soulever"),         QStringLiteral("forme-page-annotee"),
                                QStringLiteral("grille-dessus"),    QStringLiteral("miniatures"),
                                QStringLiteral("miniatures-ferme"), QStringLiteral("miniatures-pauses"),
                                QStringLiteral("zones-zoom")};
        for (qsizetype i = 0; i < 13; ++i) {
            const QJsonObject s = scenarios.at(i).toObject();
            QCOMPARE(s.value("name").toString(), names.at(i));
            QVERIFY(s.value("frames").toObject().value("count").toInt() > 0);
            // « gommer » et « soulever » (L3-T19) : sous charge (suite complète en parallèle, debug
            // logiciel), chaque redessin du cache dure des centaines de ms et le fil principal ne présente
            // que 3 images pendant tout « gommer » : toutes ses entrées tombent avant la première, aucun
            // intervalle n'est actif (relevé le 2026-10-19 : 3 images, classes « II », 136 entrées). Au
            // repos, et dans le banc hors écran de la preuve, ils ont des intervalles actifs ; ici seules les
            // images comptent.
            if (i < 5) {
                QVERIFY(s.value("frames").toObject().value("active").toInt() > 0);
            }
            // La clé n'existe que pour le scénario qui trace des formes.
            QCOMPARE(s.contains("shape_preview"), i == 7);
        }
        const QJsonObject preview = scenarios.at(7).toObject().value("shape_preview").toObject();
        // Sous charge (suite complète en parallèle, debug logiciel : relevé le 2026-10-26, 4 images pendant
        // tout le scénario, toutes hors des glissés) aucune image ne tombe dans un glissé : frames = 0. Ici
        // la clé et ses trois champs comptent ; n >= 1 est vérifié par le banc hors écran de la preuve.
        QVERIFY2(preview.value("frames").toInt(-1) >= 0, "clé shape_preview.frames absente");
        QVERIFY(preview.contains("p95_ms") && preview.value("p95_ms").toDouble(-1) >= 0);
        QVERIFY(preview.contains("max_ms") &&
                preview.value("max_ms").toDouble(-1) >= preview.value("p95_ms").toDouble());
    }

    // L4-T29 (S7.5, amendements 1 et 5) : clé `thumbnails` des deux scénarios de miniatures, absente des
    // autres. Panneau fermé : aucun rendu (L4-T19). Panneau ouvert : le nombre de rendus peut valoir 0 en
    // debug logiciel sous charge (>= 0 ici, >= 1 dans le banc hors écran de la preuve) ; aucun n'est jamais
    // lancé pendant un contact.
    void thumbnailsShowInTheResult()
    {
        const QJsonArray scenarios = m_root.value("scenarios").toArray();
        for (qsizetype i = 0; i < scenarios.size(); ++i) {
            QCOMPARE(scenarios.at(i).toObject().contains("thumbnails"), i >= 9 && i <= 11);
        }
        const QJsonObject open = scenarios.at(9).toObject().value("thumbnails").toObject();
        QVERIFY2(open.value("renders").toInt(-1) >= 0, "thumbnails.renders absent");
        QVERIFY(open.value("max_copy_ms").toDouble(-1) >= 0);
        QCOMPARE(open.value("during_contact").toInt(-1), 0);
        const QJsonObject closed = scenarios.at(10).toObject().value("thumbnails").toObject();
        QCOMPARE(closed.value("renders").toInt(-1), 0);
        QCOMPARE(closed.value("during_contact").toInt(-1), 0);
        QVERIFY(closed.contains("max_copy_ms"));
        // Variante sur « pauses » (amendement 1) : panneau ouvert, mêmes bornes que « miniatures ».
        const QJsonObject pauses = scenarios.at(11).toObject().value("thumbnails").toObject();
        QVERIFY2(pauses.value("renders").toInt(-1) >= 0, "thumbnails.renders absent");
        QVERIFY(pauses.value("max_copy_ms").toDouble(-1) >= 0);
        QCOMPARE(pauses.value("during_contact").toInt(-1), 0);
    }

    // L4-T28 : « zones-zoom » a posé ses 40 x 53 cadres avant la mesure (sinon le banc l'invalide).
    void zonesZoomAppliedItsFrames()
    {
        const QJsonArray reasons = m_root.value("verdict").toObject().value("invalid_reasons").toArray();
        for (const QJsonValue& reason : reasons) {
            QVERIFY2(!reason.toString().contains(QStringLiteral("zones not applied")),
                     qPrintable(reason.toString()));
        }
    }

    // L4-T14 (S7.5) : « forme-page-annotee » pose k >= 10 formes, une insertion hors fin de page redessine le
    // cache une fois chacune (k au repos ; sous charge, des demandes d'un même tour fusionnent, comme
    // « soulever » : borne haute k, l'exactitude est vérifiée par le banc hors écran de la preuve).
    // « grille-dessus » : la grille du dessus ne redessine rien, ses traits restent en attente (aucun
    // versement dans la mesure : 0).
    void shapeAndGridAboveCacheRefreshes()
    {
        const QJsonArray scenarios = m_root.value("scenarios").toArray();
        const QJsonObject shapes = scenarios.at(7).toObject().value("cache").toObject();
        const int k = shapes.value("placed_shapes").toInt(-1);
        QVERIFY2(k >= 10, qPrintable(QString::number(k)));
        const int refreshes = shapes.value("refreshes").toInt(-1);
        QVERIFY2(refreshes >= 1 && refreshes <= k, qPrintable(QString::number(refreshes)));
        const QJsonObject grid = scenarios.at(8).toObject().value("cache").toObject();
        QCOMPARE(grid.value("refreshes").toInt(-1), 0);
        QVERIFY(!grid.contains("placed_shapes"));
    }

    // L3-T19 (S36) : redessins du cache de « gommer » (<= k + 1 + pending_cut) et de « soulever » (2 : un au
    // soulèvement, un au dépôt, les redessins d'un même tour regroupés par PageView, L3-T13).
    void eraseAndLiftCacheRefreshes()
    {
        const QJsonArray scenarios = m_root.value("scenarios").toArray();
        const QJsonObject erase = scenarios.at(5).toObject().value("cache").toObject();
        const int k = erase.value("touched_strokes").toInt(-1);
        QVERIFY2(k >= 20, qPrintable(QString::number(k)));
        QCOMPARE(erase.value("pending_cut").toInt(-1), 0);
        const int refreshes = erase.value("refreshes").toInt(-1);
        QVERIFY2(refreshes >= 1 && refreshes <= k + 1 + erase.value("pending_cut").toInt(),
                 qPrintable(QString::number(refreshes)));
        const QJsonObject lift = scenarios.at(6).toObject().value("cache").toObject();
        // 2 au repos (release : 2 mesurés) ; sous charge (suite complète en parallèle, debug logiciel : un
        // redessin dure environ 300 ms), la demande du dépôt tombe pendant le redessin du soulèvement et est
        // fusionnée avec lui (PageView, m_refreshAgain) : un seul signal de demande. L'attendu de S36 est
        // donc une borne haute (2) ; l'exactitude de 2 est vérifiée par le banc hors écran de la preuve.
        const int liftRefreshes = lift.value("refreshes").toInt(-1);
        QVERIFY2(liftRefreshes >= 1 && liftRefreshes <= 2, qPrintable(QString::number(liftRefreshes)));
        const int lifted = lift.value("lifted_objects").toInt(-1);
        QVERIFY2(lifted >= 2 && lifted <= 8, qPrintable(QString::number(lifted)));
    }

    // L10-T1 : instants des demandes de redessin du cache et de leurs fins, par scénario. Implémentation du
    // lot 2 : au moins un redessin par trait de « pauses » (4 en court).
    void cacheRefreshesAreRecorded()
    {
        const QJsonObject pauses = m_root.value("scenarios").toArray().at(4).toObject();
        const QJsonObject cache = pauses.value("cache").toObject();
        QVERIFY(cache.contains("refreshes"));
        const QJsonObject raw = pauses.value("raw").toObject();
        const QJsonArray requests = raw.value("cache_request_ns").toArray();
        const QJsonArray updates = raw.value("cache_updated_ns").toArray();
        // L10-T4 : versement différé. En court, 4 traits ; la reprise k = 0 tombe à 500 ms tout juste et
        // relance la minuterie, la reprise k = 1 (524 ms) fait verser. En debug, ce redessin du cache en
        // rendu logiciel (environ 300 ms, fumée du 2026-10-18) retarde le relâché du trait suivant et ramène
        // la pause k = 2 (548 ms prévus) sous le délai : seul ce premier versement est garanti. Le garde du
        // différé, sans minutage, est deferredFoldShowsInTheResult.
        // L3-C14 : ce premier versement n'était pas garanti non plus. Sa reprise tombe 24 ms après la fin du
        // délai (relevé sous charge : demande à 1391-1395 ms, appui suivant prévu à 1414 ms) ; un relâché
        // traité 24 ms en retard relance la minuterie, et k = 2 n'a que 48 ms : refreshes = 0. Le joueur
        // attend désormais le versement du dernier trait (plan.awaitFinalFold), que rien ne repousse : au
        // moins un versement est relevé quelle que soit la charge (pausesClosesOnTheFoldOfItsLastStroke).
        QVERIFY2(cache.value("refreshes").toInt() >= 1, "au moins un versement attendu dans pauses");
        QVERIFY(requests.size() >= 1);
        QVERIFY(updates.size() >= 1);
        for (qsizetype i = 1; i < requests.size(); ++i) {
            QVERIFY(requests.at(i).toDouble() >= requests.at(i - 1).toDouble());
        }
    }

    // L3-C14 : la mesure de « pauses » se ferme sur le versement différé de son dernier trait (le joueur
    // l'attend) : la dernière demande de redessin suit la dernière entrée prévue, et sa fin est relevée. Sans
    // cette attente, la mesure finissait 150 ms après la dernière entrée et la dernière demande était celle
    // d'une reprise (k = 1 ou 2), avant le dernier trait, au repos comme sous charge.
    void pausesClosesOnTheFoldOfItsLastStroke()
    {
        const QJsonObject raw = m_root.value("scenarios").toArray().at(4).toObject().value("raw").toObject();
        const QJsonArray inputs = raw.value("input_ns").toArray();
        const QJsonArray requests = raw.value("cache_request_ns").toArray();
        const QJsonArray updates = raw.value("cache_updated_ns").toArray();
        QVERIFY(!inputs.isEmpty());
        QVERIFY(!requests.isEmpty());
        QVERIFY(!updates.isEmpty());
        const double lastInput = inputs.last().toDouble();
        QVERIFY2(requests.last().toDouble() > lastInput,
                 qPrintable(QStringLiteral("dernière demande %1 ms avant la dernière entrée")
                                .arg((lastInput - requests.last().toDouble()) / 1e6)));
        QVERIFY(updates.last().toDouble() >= requests.last().toDouble());
    }

    // L10-T4 : aucun redessin du cache pendant « stroke » (pauses de 60 ms en court, sous le délai
    // d'inactivité), et au moins un objet a attendu au-dessus du cache dans « pauses ».
    void deferredFoldShowsInTheResult()
    {
        const QJsonArray scenarios = m_root.value("scenarios").toArray();
        QCOMPARE(scenarios.at(0).toObject().value("cache").toObject().value("refreshes").toInt(), 0);
        QVERIFY(scenarios.at(4).toObject().value("cache").toObject().value("pending_max").toInt() >= 1);
        // Revue de L10-T4 : « zoom », « scroll » et « page » ne tracent rien ; ce qui attendait à la fin du
        // scénario précédent est versé avant leur mesure (Runner::setLevel) et n'y paraît pas.
        for (qsizetype i = 1; i <= 3; ++i) {
            const QJsonObject scenario = scenarios.at(i).toObject();
            QVERIFY2(scenario.value("cache").toObject().value("pending_max").toInt() == 0,
                     qPrintable(scenario.value("name").toString()));
        }
    }

    // 2e revue L2-T23 : le câblage du runner (marques, prêts relevés, fenêtres) classe actifs les intervalles
    // des changements, et eux seuls. Sans les marques, seul l'intervalle de chaque commande serait actif (au
    // plus 7) ; sans prêt relevé, tout serait actif de la première commande à la fin. Il faut donc un
    // intervalle au repos après la première commande : un changement au moins (le zoom posé a 700 ms) finit
    // avant la commande suivante, même en debug sous charge (relevé : 1 sur 12 intervalles, 111 s de banc).
    // L3-C3 : faux sous une charge plus forte (suites en parallèle, 2026-10-19 : 126 intervalles actifs sur
    // 127, aucun au repos) : chaque changement durait plus que l'écart des commandes et le dernier dépassait
    // la fin de la mesure. Le runner attend désormais le prêt du dernier changement, puis deux images :
    // l'intervalle qui suit est au repos quelle que soit la charge, et reste actif si aucun prêt n'est
    // relevé.
    void pageChangesAreActiveUntilShown()
    {
        const QJsonObject page = m_root.value("scenarios").toArray().at(3).toObject();
        const QJsonObject raw = page.value("raw").toObject();
        const QJsonArray inputs = raw.value("input_ns").toArray();
        const QJsonArray swaps = raw.value("swap_ns").toArray();
        const QString classes = raw.value("interval_class").toString();
        QCOMPARE(inputs.size(), 7);
        QCOMPARE(classes.size(), swaps.size() - 1);
        QVERIFY(page.value("frames").toObject().value("active").toInt() > inputs.size());
        const double first = inputs.first().toDouble();
        qsizetype idleAfter = 0;
        for (qsizetype k = 0; k < classes.size(); ++k) {
            if (swaps.at(k).toDouble() >= first && classes.at(k) == u'I') {
                ++idleAfter;
            }
        }
        QVERIFY(idleAfter > 0);
    }

    void strokeHasLatency()
    {
        const QJsonObject latency =
            m_root.value("scenarios").toArray().at(0).toObject().value("latency").toObject();
        QVERIFY(latency.value("samples").toInt() > 0);
        QVERIFY(latency.value("total_ms").toObject().value("p50").toDouble() > 0.0);
    }

    void contentCheckPasses()
    {
        const QJsonObject check = m_root.value("content_check").toObject();
        QVERIFY(check.value("ok").toBool());
        const QJsonArray checks = check.value("checks").toArray();
        QCOMPARE(checks.size(), 5); // L10-T4 : stroke-folded
        for (const QJsonValue& c : checks) {
            QVERIFY2(c.toObject().value("ok").toBool(), qPrintable(c.toObject().value("name").toString()));
        }
    }

    void verdictHasCriteria()
    {
        QVERIFY(!m_root.value("verdict").toObject().value("criteria").toArray().isEmpty());
    }

    void noHostName()
    {
        const QString host = QSysInfo::machineHostName();
        if (host.size() >= 3) {
            QVERIFY(!QString::fromUtf8(m_text).contains(host, Qt::CaseInsensitive));
        }
    }
};

QTEST_GUILESS_MAIN(TstBenchResult)
#include "tst_bench_result.moc"
