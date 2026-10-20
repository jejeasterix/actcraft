#include "presentationdocument.h"
#include "support/lot7registry.h"
#include "support/presentationharness.h"

#include <actcraft/features/transitions.h>
#include <actcraft/ui/documentcontroller.h>

#include <QQuickItemGrabResult>
#include <QRandomGenerator>
#include <QTemporaryDir>
#include <QtTest>

#include <algorithm>
#include <utility>

namespace {

// Tout le fichier dans cet espace de noms anonyme (exécutable unique : aucun nom global hors du registre).
using actcraft::ui::DocumentController;
using actcraft::ui::DocumentInsertions;
using presentationharness::MainWindow;
using presentationharness::Pointer;

constexpr int kIdle = 0;
constexpr int kAnimating = 2;

QObject* transitionOf(const MainWindow& main)
{
    return main.window() ? main.window()->findChild<QObject*>(QStringLiteral("transition")) : nullptr;
}

DocumentController* documentOf(const MainWindow& main)
{
    return main.window() ? main.window()->findChild<DocumentController*>(QStringLiteral("documentController"))
                         : nullptr;
}

int phaseOf(const QObject* transition)
{
    return transition->property("phase").toInt();
}

QString playedOf(const QObject* transition)
{
    return transition->property("played").toString();
}

int snapshotsOf(const MainWindow& main)
{
    const QQuickItem* root = main.item(QStringLiteral("transitionRoot"));
    return root ? root->property("snapshotCount").toInt() : -1;
}

void setPageEffect(DocumentController& document, const char* effect)
{
    DocumentInsertions insertions(document);
    actcraft::core::PagePresentation value = insertions.shownPage()->presentation;
    value.transition = effect;
    insertions.setPagePresentation(std::move(value));
}

QString pageEffectOf(DocumentController& document)
{
    const DocumentInsertions insertions(document);
    return QString::fromStdString(insertions.shownPage()->presentation.transition);
}

// Effet fini, page prête.
bool settle(const MainWindow& main, const QObject* transition)
{
    return QTest::qWaitFor([transition] { return phaseOf(transition) == kIdle; }, 8000) && main.waitReady();
}

class TstUiTransitions : public QObject
{
    Q_OBJECT

private slots:
    // Effet « aucun » (défaut) : la page change aussitôt, comme sans enveloppe, et aucun instantané n'est
    // créé.
    void noEffectWhenNone()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QObject* transition = transitionOf(main);
        DocumentController* document = documentOf(main);
        QVERIFY(transition && document);
        QCOMPARE(transition->property("globalEffect").toString(), QStringLiteral("none"));
        QSignalSpy phases(transition, SIGNAL(phaseChanged()));
        QVERIFY(main.run(QStringLiteral("page.next")));
        QCOMPARE(document->currentIndex(), 1);
        QVERIFY(main.run(QStringLiteral("page.previous")));
        QCOMPARE(document->currentIndex(), 0);
        QVERIFY(main.waitReady());
        QTest::qWait(100);
        QCOMPARE(phases.count(), 0);
        QCOMPARE(snapshotsOf(main), 0);
        QCOMPARE(playedOf(transition), QString());
    }

    // Effet global : page.next le joue, la page change à la fin de l'instantané, l'effet se termine.
    void effectPlayedOnNext()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QObject* transition = transitionOf(main);
        DocumentController* document = documentOf(main);
        QVERIFY(transition && document);
        QVERIFY(main.run(QStringLiteral("view.page-effect"), QStringLiteral("dissolve")));
        QVERIFY(main.run(QStringLiteral("page.next")));
        QTRY_COMPARE_WITH_TIMEOUT(phaseOf(transition), kAnimating, 5000);
        QCOMPARE(document->currentIndex(), 1);
        QCOMPARE(playedOf(transition), QStringLiteral("dissolve"));
        QCOMPARE(snapshotsOf(main), 1);
        QVERIFY(settle(main, transition));
        QCOMPARE(document->currentIndex(), 1);
    }

    // Les gestionnaires de page.next et page.previous sont ceux de la présentation après le chargement, par
    // la commande comme par le raccourci.
    void pageNextIsWrapped()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QObject* transition = transitionOf(main);
        DocumentController* document = documentOf(main);
        QVERIFY(transition && document);
        QVERIFY(main.run(QStringLiteral("view.page-effect"), QStringLiteral("slide-right")));
        QVERIFY(main.run(QStringLiteral("page.next")));
        QVERIFY(phaseOf(transition) != kIdle);
        QVERIFY(settle(main, transition));
        QCOMPARE(document->currentIndex(), 1);
        QVERIFY(main.run(QStringLiteral("page.previous")));
        QVERIFY(phaseOf(transition) != kIdle);
        QVERIFY(settle(main, transition));
        QCOMPARE(document->currentIndex(), 0);
        QTest::keyClick(main.window(), Qt::Key_PageDown);
        QVERIFY(phaseOf(transition) != kIdle);
        QVERIFY(settle(main, transition));
        QCOMPARE(document->currentIndex(), 1);
        QCOMPARE(snapshotsOf(main), 3);
    }

    // L'effet est celui de la page QUITTÉE, lu dans PagePresentation::transition (écrit ici dans le document,
    // sans la commande), jamais celui de la page d'arrivée ; effet global « aucun ».
    void pageEffectReadFromThePresentation()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QObject* transition = transitionOf(main);
        DocumentController* document = documentOf(main);
        QVERIFY(transition && document);
        QVERIFY(main.run(QStringLiteral("page.next")));
        QCOMPARE(document->currentIndex(), 1);
        QVERIFY(main.waitReady());
        setPageEffect(*document, "slide-right");
        QVERIFY(main.run(QStringLiteral("page.previous")));
        QTRY_COMPARE_WITH_TIMEOUT(phaseOf(transition), kAnimating, 5000);
        QCOMPARE(playedOf(transition), QStringLiteral("slide-right"));
        QVERIFY(settle(main, transition));
        QCOMPARE(document->currentIndex(), 0);
        setPageEffect(*document, "dissolve");
        QVERIFY(main.run(QStringLiteral("page.next")));
        QTRY_COMPARE_WITH_TIMEOUT(phaseOf(transition), kAnimating, 5000);
        QCOMPARE(playedOf(transition), QStringLiteral("dissolve"));
        QVERIFY(settle(main, transition));
        QCOMPARE(document->currentIndex(), 1);
    }

    // Rideau actif (curtain.active) : aucun effet, la page change aussitôt ; rideau fermé : l'effet revient.
    void noEffectWhileCurtainActive()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QObject* transition = transitionOf(main);
        DocumentController* document = documentOf(main);
        QObject* curtain = main.window()->findChild<QObject*>(QStringLiteral("curtain"));
        QVERIFY(transition && document && curtain);
        QVERIFY(main.run(QStringLiteral("view.page-effect"), QStringLiteral("dissolve")));
        QVERIFY(main.run(QStringLiteral("present.curtain")));
        QVERIFY(curtain->property("active").toBool());
        QVERIFY(main.run(QStringLiteral("page.next")));
        QCOMPARE(document->currentIndex(), 1);
        QCOMPARE(phaseOf(transition), kIdle);
        QVERIFY(main.waitReady());
        QTest::qWait(100);
        QCOMPARE(snapshotsOf(main), 0);
        QCOMPARE(playedOf(transition), QString());

        QVERIFY(main.run(QStringLiteral("present.curtain")));
        QVERIFY(!curtain->property("active").toBool());
        QVERIFY(main.run(QStringLiteral("page.previous")));
        QTRY_COMPARE_WITH_TIMEOUT(phaseOf(transition), kAnimating, 5000);
        QCOMPARE(playedOf(transition), QStringLiteral("dissolve"));
        QVERIFY(settle(main, transition));
    }

    // Le cache n'est redessiné ni pour l'instantané ni par l'effet : aucune demande avant le changement de
    // page, et autant de demandes, effet fini, qu'un changement de page sans effet.
    void transitionDoesNotRefreshTheCacheBeforeTheEnd()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QObject* transition = transitionOf(main);
        DocumentController* document = documentOf(main);
        QVERIFY(transition && document);
        QVERIFY(main.run(QStringLiteral("page.next"))); // deux pages
        QVERIFY(main.run(QStringLiteral("page.previous")));
        QVERIFY(main.waitReady());
        QTest::qWait(200);

        QSignalSpy spy(main.pageView(), &actcraft::ui::PageView::cacheRefreshRequested);
        QVERIFY(spy.isValid());
        QVERIFY(main.run(QStringLiteral("page.next")));
        QVERIFY(main.waitReady());
        QTest::qWait(200);
        const int withoutEffect = spy.count();
        QVERIFY(withoutEffect >= 1);
        QVERIFY(main.run(QStringLiteral("page.previous")));
        QVERIFY(main.waitReady());
        QTest::qWait(200);

        QVERIFY(main.run(QStringLiteral("view.page-effect"), QStringLiteral("dissolve")));
        QVERIFY(main.run(QStringLiteral("view.page-effect-duration"), 10));
        spy.clear();
        int atTurn = -1;
        QObject context;
        // Juste avant le changement de page (la vue demande son redessin dans le changement même).
        QObject::connect(document, &DocumentController::pageAboutToChange, &context, [&] {
            if (atTurn < 0) {
                atTurn = static_cast<int>(spy.count());
            }
        });
        QVERIFY(main.run(QStringLiteral("page.next")));
        QTRY_COMPARE_WITH_TIMEOUT(phaseOf(transition), kAnimating, 5000);
        QCOMPARE(atTurn, 0);
        QVERIFY(settle(main, transition));
        QTest::qWait(200);
        QCOMPARE(static_cast<int>(spy.count()), withoutEffect);
        QCOMPARE(document->currentIndex(), 1);
    }

    // Un trait versé moins de 500 ms avant page.next, encore en attente au-dessus du cache, est dans
    // l'instantané (source : la page entière, pas la page fixe du cache). Image lue par grabToImage ici
    // seulement.
    void snapshotShowsAPendingStroke()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QObject* transition = transitionOf(main);
        QVERIFY(transition != nullptr);
        QVERIFY(main.run(QStringLiteral("view.page-effect"), QStringLiteral("dissolve")));
        QVERIFY(main.run(QStringLiteral("view.page-effect-duration"), 30));
        QVERIFY(main.run(QStringLiteral("pen.width"), 16));
        const int before = main.pageObjectCount();
        actcraft::ui::PageView* view = main.pageView();
        presentationharness::strokeAt(main.window(), Pointer::Stylus, view->pageToScene(QPointF(200, 300)),
                                      view->pageToScene(QPointF(800, 300)));
        QTRY_COMPARE_WITH_TIMEOUT(main.pageObjectCount(), before + 1, 2000);
        QVERIFY(main.fixedPage()->pendingCount() > 0); // pas encore dans le cache
        QVERIFY(main.run(QStringLiteral("page.next")));
        QTRY_COMPARE_WITH_TIMEOUT(phaseOf(transition), kAnimating, 5000);

        QQuickItem* snapshot = main.item(QStringLiteral("transitionSnapshot"));
        QVERIFY(snapshot != nullptr);
        const QSharedPointer<QQuickItemGrabResult> grab = snapshot->grabToImage();
        QVERIFY(grab);
        QSignalSpy ready(grab.data(), &QQuickItemGrabResult::ready);
        QVERIFY(ready.wait(5000));
        const QImage image = grab->image();
        QVERIFY(!image.isNull());
        const auto pixelAt = [&](QPointF pagePoint) {
            const QPointF local = snapshot->mapFromScene(view->pageToScene(pagePoint));
            const qreal ratio = image.width() / snapshot->width();
            return image.pixelColor((local * ratio).toPoint());
        };
        const QColor ink = pixelAt(QPointF(500, 300));
        const QColor blank = pixelAt(QPointF(500, 600));
        QVERIFY2(qGray(blank.rgb()) > 200, qPrintable(blank.name(QColor::HexArgb)));
        QVERIFY2(qGray(ink.rgb()) < 100, qPrintable(ink.name(QColor::HexArgb)));
        QVERIFY(settle(main, transition));
    }

    // Trois page.next pendant un effet de 1,5 s (pendant l'instantané, puis pendant l'effet) : page 4.
    void fastRepeatLosesNoPage()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QObject* transition = transitionOf(main);
        DocumentController* document = documentOf(main);
        QVERIFY(transition && document);
        QVERIFY(main.run(QStringLiteral("view.page-effect"), QStringLiteral("dissolve")));
        QVERIFY(main.run(QStringLiteral("page.next")));
        QVERIFY(phaseOf(transition) != kIdle);
        QVERIFY(main.run(QStringLiteral("page.next"))); // pendant l'instantané
        QTRY_COMPARE_WITH_TIMEOUT(phaseOf(transition), kAnimating, 5000);
        QVERIFY(main.run(QStringLiteral("page.next"))); // pendant l'effet
        QVERIFY(settle(main, transition));
        QCOMPARE(document->currentIndex(), 3);
        QCOMPARE(document->pageCount(), 4);
        QCOMPARE(main.item(QStringLiteral("pageIndicator"))->property("text").toString(),
                 QStringLiteral("4 / 4"));
    }

    // « Au hasard » : un effet du catalogue tiré par features::pickEffect sur le générateur injecté.
    void randomUsesTheInjectedGenerator()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QObject* transition = transitionOf(main);
        QVERIFY(transition != nullptr);
        QVERIFY(QMetaObject::invokeMethod(transition, "seedRandom", Q_ARG(quint32, 42u)));
        QVERIFY(main.run(QStringLiteral("view.page-effect"), QStringLiteral("random")));
        QVERIFY(main.run(QStringLiteral("page.next")));
        QTRY_COMPARE_WITH_TIMEOUT(phaseOf(transition), kAnimating, 5000);
        QRandomGenerator expected(42u);
        const QString id = actcraft::features::pickEffect(QStringLiteral("random"), expected);
        QVERIFY(id != QStringLiteral("random") && id != QStringLiteral("none"));
        QCOMPARE(playedOf(transition), id);
        QVERIFY(settle(main, transition));
    }

    // Chacun des seize mouvements a ses pièces ; « aucun » et « au hasard » n'en ont pas.
    void everyEffectHasAMotion()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QObject* transition = transitionOf(main);
        QVERIFY(transition != nullptr);
        const QVariantList catalogue = transition->property("catalogue").toList();
        QCOMPARE(catalogue.size(), 18);
        for (const QVariant& entry : catalogue) {
            const QString id = entry.toMap().value(QStringLiteral("id")).toString();
            QVariantList pieces;
            QVERIFY(QMetaObject::invokeMethod(transition, "motion", Q_RETURN_ARG(QVariantList, pieces),
                                              Q_ARG(QString, id)));
            if (id == QStringLiteral("none") || id == QStringLiteral("random")) {
                QVERIFY2(pieces.isEmpty(), qPrintable(id));
                continue;
            }
            QVERIFY2(!pieces.isEmpty(), qPrintable(id));
            double longest = 0;
            for (const QVariant& piece : pieces) {
                const QVariantMap p = piece.toMap();
                const double span = p.value(QStringLiteral("span")).toDouble();
                QVERIFY2(span > 0 && span <= 1, qPrintable(id));
                longest = std::max(longest, span);
            }
            QVERIFY2(qFuzzyCompare(longest, 1.0), qPrintable(id));
        }
    }

    // Effet global et durée gardés d'une fenêtre à la suivante ; durée bornée à 5..30 dixièmes.
    void effectSettingsRemembered()
    {
        QTemporaryDir settings;
        QVERIFY(settings.isValid());
        {
            MainWindow first({}, settings.path());
            QVERIFY(first.window() != nullptr);
            QVERIFY(first.run(QStringLiteral("view.page-effect"), QStringLiteral("drapes")));
            QVERIFY(first.run(QStringLiteral("view.page-effect-duration"), 99));
            QCOMPARE(transitionOf(first)->property("durationTenths").toInt(), 30);
            QVERIFY(first.run(QStringLiteral("view.page-effect-duration"), 25));
        }
        MainWindow second({}, settings.path());
        QVERIFY(second.window() != nullptr);
        QCOMPARE(transitionOf(second)->property("globalEffect").toString(), QStringLiteral("drapes"));
        QCOMPARE(transitionOf(second)->property("durationTenths").toInt(), 25);
    }

    // page.effect écrit l'effet de la page affichée en une étape d'annulation.
    void pageEffectIsUndoneAtOnce()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        DocumentController* document = documentOf(main);
        QVERIFY(document != nullptr);
        QVERIFY(!document->canUndo());
        QVERIFY(main.run(QStringLiteral("page.effect"), QStringLiteral("shards")));
        QCOMPARE(pageEffectOf(*document), QStringLiteral("shards"));
        QVERIFY(document->canUndo());
        QVERIFY(main.run(QStringLiteral("edit.undo")));
        QCOMPARE(pageEffectOf(*document), QString());
        QVERIFY(!document->canUndo());
    }
};

} // namespace

ACTCRAFT_LOT7_TEST("transitions", TstUiTransitions)
#include "tst_ui_transitions.moc"
