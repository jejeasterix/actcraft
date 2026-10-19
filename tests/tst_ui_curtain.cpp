#include "support/lot7registry.h"
#include "support/presentationharness.h"

#include <QtTest>

namespace {

using presentationharness::MainWindow;
using presentationharness::Pointer;

// Tout le fichier dans cet espace de noms anonyme (exécutable unique : aucun nom global hors du registre).
QObject* curtainOf(const MainWindow& main)
{
    return main.window() ? main.window()->findChild<QObject*>(QStringLiteral("curtain")) : nullptr;
}

QPointF inWindow(const MainWindow& main, QPointF pagePoint)
{
    return main.pageView()->pageToScene(pagePoint);
}

bool coverageIs(const QObject* curtain, double expected)
{
    return qAbs(curtain->property("coverage").toDouble() - expected) <= 0.02;
}

class TstUiCurtain : public QObject
{
    Q_OBJECT

private slots:
    // Surcouche : aucune demande de redessin du cache (ouverture, fermeture, tirage complet) ; le stylet
    // écrit ensuite dans la partie découverte.
    void curtainNeverRefreshesTheCache()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QObject* curtain = curtainOf(main);
        QVERIFY(curtain != nullptr);
        QSignalSpy spy(main.pageView(), &actcraft::ui::PageView::cacheRefreshRequested);
        QVERIFY(spy.isValid());
        const int before = main.pageObjectCount();

        QVERIFY(main.run(QStringLiteral("present.curtain")));
        QVERIFY(curtain->property("active").toBool());
        QVERIFY(main.run(QStringLiteral("present.curtain"))); // fermé puis rouvert : un nouveau store plein
        QVERIFY(main.run(QStringLiteral("present.curtain")));
        QVERIFY(curtain->property("active").toBool());
        QVERIFY(coverageIs(curtain, 1.0));

        presentationharness::strokeAt(main.window(), Pointer::Mouse, inWindow(main, QPointF(512, 2)),
                                      inWindow(main, QPointF(512, 384)));
        QTest::qWait(100);
        QVERIFY2(coverageIs(curtain, 0.5), qPrintable(curtain->property("coverage").toString()));
        QCOMPARE(spy.count(), 0);
        QCOMPARE(main.pageObjectCount(), before);

        // La partie découverte reste la page : un trait au stylet y est versé.
        presentationharness::strokeAt(main.window(), Pointer::Stylus, inWindow(main, QPointF(300, 100)),
                                      inWindow(main, QPointF(700, 100)));
        QTRY_COMPARE_WITH_TIMEOUT(main.pageObjectCount(), before + 1, 5000);
    }

    void curtainBlocksTheStylus_data() { presentationharness::addPointerRows(); }

    // Le store est une couverture (addInputCover) : rien n'est versé dessous, quel que soit le périphérique ;
    // le bord se tire (effet du gestionnaire lu) ; la partie découverte reçoit le trait.
    void curtainBlocksTheStylus()
    {
        QFETCH(Pointer, pointer);
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QObject* curtain = curtainOf(main);
        QVERIFY(curtain != nullptr);
        const int before = main.pageObjectCount();
        QVERIFY(main.run(QStringLiteral("present.curtain")));

        // Rideau fermé, trait au milieu du store : aucun trait, le store n'a pas bougé.
        presentationharness::strokeAt(main.window(), pointer, inWindow(main, QPointF(300, 400)),
                                      inWindow(main, QPointF(700, 400)));
        QTest::qWait(150);
        QCOMPARE(main.pageObjectCount(), before);
        QVERIFY(coverageIs(curtain, 1.0));

        // Bord haut tiré jusqu'au milieu : la moitié est découverte, toujours aucun trait.
        presentationharness::strokeAt(main.window(), pointer, inWindow(main, QPointF(512, 2)),
                                      inWindow(main, QPointF(512, 384)));
        QTest::qWait(150);
        QVERIFY2(coverageIs(curtain, 0.5), qPrintable(curtain->property("coverage").toString()));
        QCOMPARE(main.pageObjectCount(), before);

        // Trait dans la moitié découverte : versé, et le store n'a pas bougé.
        presentationharness::strokeAt(main.window(), pointer, inWindow(main, QPointF(300, 100)),
                                      inWindow(main, QPointF(700, 100)));
        QTRY_COMPARE_WITH_TIMEOUT(main.pageObjectCount(), before + 1, 5000);
        QVERIFY(coverageIs(curtain, 0.5));
    }
};

} // namespace

ACTCRAFT_LOT7_TEST("curtain", TstUiCurtain)
#include "tst_ui_curtain.moc"
