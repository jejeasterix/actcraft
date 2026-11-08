#include <actcraft/features/recognitionsession.h>

#include <QtTest>

using namespace actcraft::features;

class TstFeaturesRecognitionSession : public QObject
{
    Q_OBJECT

private slots:
    void sessionFiresAfterTheDelay()
    {
        RecognitionSession s(5000); // délai en ms
        s.addStroke(11, 0);         // identifiant, instant (ns)
        QVERIFY(s.due(4'999'000'000).empty());
        QCOMPARE(s.pending().size(), std::size_t{1}); // pas encore mûr : toujours en attente
        const auto due = s.due(5'000'000'000);
        QCOMPARE(due.size(), std::size_t{1});
        QCOMPARE(due.front(), actcraft::core::ObjectId{11});
        QVERIFY(s.pending().empty());          // rendu, donc oublié
        QVERIFY(s.due(6'000'000'000).empty()); // une seule fois
    }
    void newStrokeRestartsTheDelay()
    {
        RecognitionSession s(5000);
        s.addStroke(1, 0);
        s.addStroke(2, 3'000'000'000);
        QVERIFY(s.due(5'500'000'000).empty());
        QCOMPARE(s.due(8'000'000'000).size(), std::size_t{2}); // les deux traits ensemble (écriture : un mot)
    }
    // Relecture du lot 7 : retirer un trait (converti par la sélection) laisse l'autre en attente, avec son
    // échéance : celle du dernier trait ajouté (3 s + 5 s), ni avancée, ni effacée.
    void removeKeepsTheOtherStrokeAndItsDeadline()
    {
        RecognitionSession s(5000);
        s.addStroke(1, 0);
        s.addStroke(2, 3'000'000'000);
        s.remove({1});
        QCOMPARE(s.pending().size(), std::size_t{1});
        QCOMPARE(s.pending().front(), actcraft::core::ObjectId{2});
        QVERIFY(s.due(7'999'000'000).empty()); // pas avant 3 s + 5 s
        QCOMPARE(s.pending().size(), std::size_t{1});
        const auto due = s.due(8'000'000'000);
        QCOMPARE(due.size(), std::size_t{1});
        QCOMPARE(due.front(), actcraft::core::ObjectId{2});
        s.addStroke(3, 9'000'000'000);
        s.remove({7}); // identifiant inconnu : rien ne change
        QCOMPARE(s.pending().size(), std::size_t{1});
    }
    void zeroDelayIsImmediate()
    {
        RecognitionSession s(0);
        s.addStroke(5, 100);
        QCOMPARE(s.due(100).size(), std::size_t{1});
        s.addStroke(6, 200);
        s.clear(); // trait abandonné : même sans délai, rien n'est rendu
        QVERIFY(s.pending().empty());
        QVERIFY(s.due(200).empty());
    }
    void defaultDelayIsOneSecond()
    {
        // L7-T0 (docs/verifications/2026-10-18-lot7-principes.md) : délai par défaut 1 s, bornes 0 à 5 s.
        const RecognitionSession s;
        QCOMPARE(s.delayMs(), 1000);
    }
    void delayClampedTo0To5000()
    {
        RecognitionSession s(9000);
        QCOMPARE(s.delayMs(), 5000);
        s.setDelayMs(-3);
        QCOMPARE(s.delayMs(), 0);
    }
};

QTEST_GUILESS_MAIN(TstFeaturesRecognitionSession)
#include "tst_features_recognitionsession.moc"
