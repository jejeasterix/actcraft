#include "presentationdocument.h"
#include "support/lot7registry.h"
#include "support/presentationharness.h"

#include <actcraft/features/transitions.h>
#include <actcraft/ui/documentcontroller.h>

#include <QHash>
#include <QQuickItemGrabResult>
#include <QRandomGenerator>
#include <QStringList>
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

// Morceaux de l'instantané encore présents.
int snapshotPieces(const MainWindow& main)
{
    const QQuickItem* frame = main.item(QStringLiteral("transitionSnapshot"));
    return frame ? static_cast<int>(frame->childItems().size()) : -1;
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

// Navigateur d'effets (L7-T8) : ouvert par la commande, racine « effectsRoot » visible.
QQuickItem* openPicker(const MainWindow& main)
{
    if (!main.run(QStringLiteral("view.page-effects"))) {
        return nullptr;
    }
    QQuickItem* picker = main.item(QStringLiteral("effectsRoot"));
    return picker && QTest::qWaitFor([picker] { return picker->isVisible(); }, 3000) ? picker : nullptr;
}

// Noms des délégués « effect-<id> » sous un élément.
void collectDelegates(const QQuickItem* item, QStringList& names)
{
    const QList<QQuickItem*> children = item->childItems();
    for (const QQuickItem* child : children) {
        if (child->objectName().startsWith(QStringLiteral("effect-"))) {
            names << child->objectName();
        }
        collectDelegates(child, names);
    }
}

QPoint centerOf(const QQuickItem* item)
{
    return item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint();
}

// Appui bref au stylet sur un bouton ou un délégué. Qt (QGuiApplication) envoie l'événement de tablette non
// accepté ; si la fenêtre ne l'accepte pas, il le rejoue en souris
// (Qt::AA_SynthesizeMouseForUnhandledTabletEvents, actif par défaut) : le rejeu est refait ici à la même
// condition, et à elle seule. Renvoie vrai si la tablette a pris l'appui.
bool stylusTap(QQuickWindow* window, QPoint at)
{
    const auto send = [&](QEvent::Type type) {
        const bool down = type == QEvent::TabletPress;
        QTabletEvent event(type, presentationharness::stylusDevice(), QPointF(at),
                           window->mapToGlobal(QPointF(at)), down ? 0.5 : 0.0, 0, 0, 0, 0, 0, Qt::NoModifier,
                           Qt::LeftButton, down ? Qt::LeftButton : Qt::NoButton);
        event.setAccepted(false); // comme QGuiApplication avant l'envoi
        QCoreApplication::sendEvent(window, &event);
        return event.isAccepted();
    };
    const bool taken = send(QEvent::TabletPress);
    if (!taken) {
        QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, at);
    }
    send(QEvent::TabletRelease);
    if (!taken) {
        QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, at);
    }
    QCoreApplication::processEvents();
    return taken;
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
        QTRY_COMPARE(snapshotPieces(main), 0); // instantané retiré (destroy() est différé)
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

    void noEffectWhileCurtainActive_data()
    {
        QTest::addColumn<QString>("command");
        QTest::addColumn<QString>("control");
        QTest::newRow("rideau") << QStringLiteral("present.curtain") << QStringLiteral("curtain");
        // noEffectWhileSpotlightActive : même règle pour le spot (7a, « rideau ou spot actif »).
        QTest::newRow("spot") << QStringLiteral("present.spotlight-circle") << QStringLiteral("spotlight");
    }

    // Rideau ou spot actif (curtain.active, spotlight.active) : aucun effet, la page change aussitôt ; fermé
    // : l'effet revient.
    void noEffectWhileCurtainActive()
    {
        QFETCH(QString, command);
        QFETCH(QString, control);
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QObject* transition = transitionOf(main);
        DocumentController* document = documentOf(main);
        QObject* blocker = main.window()->findChild<QObject*>(control);
        QVERIFY(transition && document && blocker);
        QVERIFY(main.run(QStringLiteral("view.page-effect"), QStringLiteral("dissolve")));
        QVERIFY(main.run(command));
        QVERIFY(blocker->property("active").toBool());
        QVERIFY(main.run(QStringLiteral("page.next")));
        QCOMPARE(document->currentIndex(), 1);
        QCOMPARE(phaseOf(transition), kIdle);
        QVERIFY(main.waitReady());
        QTest::qWait(100);
        QCOMPARE(snapshotsOf(main), 0);
        QCOMPARE(playedOf(transition), QString());

        QVERIFY(main.run(command));
        QVERIFY(!blocker->property("active").toBool());
        QVERIFY(main.run(QStringLiteral("page.previous")));
        QTRY_COMPARE_WITH_TIMEOUT(phaseOf(transition), kAnimating, 5000);
        QCOMPARE(playedOf(transition), QStringLiteral("dissolve"));
        QVERIFY(settle(main, transition));
    }

    // S22 (F-640 : effets désactivés pour la navigation par le navigateur de page) : page.goto (lot 4) change
    // de page aussitôt, sans instantané ni effet, même avec un effet global. Mutant : page.goto enveloppé
    // comme page.next.
    void pageNavigatorPlaysNoEffect()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QObject* transition = transitionOf(main);
        DocumentController* document = documentOf(main);
        QVERIFY(transition && document);
        QVERIFY(main.run(QStringLiteral("page.next"))); // deux pages, sans effet
        QCOMPARE(document->currentIndex(), 1);
        QVERIFY(main.waitReady());
        QVERIFY(main.run(QStringLiteral("view.page-effect"), QStringLiteral("dissolve")));
        QVERIFY(main.run(QStringLiteral("page.goto"), 1)); // numéro de page, depuis 1
        QCOMPARE(document->currentIndex(), 0);
        QCOMPARE(phaseOf(transition), kIdle);
        QCOMPARE(snapshotsOf(main), 0);
        QCOMPARE(playedOf(transition), QString());
        // Témoin : page.next joue l'effet.
        QVERIFY(main.waitReady());
        QVERIFY(main.run(QStringLiteral("page.next")));
        QTRY_COMPARE_WITH_TIMEOUT(playedOf(transition), QStringLiteral("dissolve"), 5000);
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
        const int withoutEffect = static_cast<int>(spy.count());
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
        QTRY_COMPARE(snapshotPieces(main), 0); // les instantanés interrompus aussi
    }

    // Aucune image rendue (fenêtre cachée) : l'instantané n'arrive jamais ; passé le repli, la page change
    // sans effet au lieu de rester en attente.
    void pageTurnsWhenNoFrameIsRendered()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QObject* transition = transitionOf(main);
        DocumentController* document = documentOf(main);
        QVERIFY(transition && document);
        QVERIFY(main.run(QStringLiteral("view.page-effect"), QStringLiteral("dissolve")));
        main.window()->hide();
        QCoreApplication::processEvents();
        QVERIFY(main.run(QStringLiteral("page.next")));
        QCOMPARE(phaseOf(transition), 1); // instantané demandé
        QCOMPARE(document->currentIndex(), 0);
        QTRY_COMPARE_WITH_TIMEOUT(document->currentIndex(), 1, 6000);
        QCOMPARE(phaseOf(transition), kIdle);
        QCOMPARE(playedOf(transition), QString());
        QTRY_COMPARE(snapshotPieces(main), 0);
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

    // Chacun des seize mouvements a ses pièces, chaque pièce bouge, et deux effets n'ont jamais le même
    // mouvement ; « aucun » et « au hasard » n'en ont pas.
    void everyEffectHasAMotion()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QObject* transition = transitionOf(main);
        QVERIFY(transition != nullptr);
        const QVariantList catalogue = transition->property("catalogue").toList();
        QCOMPARE(catalogue.size(), 18);
        QHash<QString, QString> owners; // signature du mouvement -> effet
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
            QStringList signature;
            for (const QVariant& piece : pieces) {
                const QVariantMap p = piece.toMap();
                const double span = p.value(QStringLiteral("span")).toDouble();
                QVERIFY2(span > 0 && span <= 1, qPrintable(id));
                longest = std::max(longest, span);
                const bool moves = p.value(QStringLiteral("dx")).toDouble() != 0 ||
                                   p.value(QStringLiteral("dy")).toDouble() != 0 ||
                                   p.value(QStringLiteral("scale")).toDouble() != 1 ||
                                   p.value(QStringLiteral("opacity")).toDouble() != 1;
                QVERIFY2(moves, qPrintable(id));
                QStringList values;
                for (const char* key :
                     {"x", "y", "width", "height", "dx", "dy", "scale", "opacity", "origin", "span"}) {
                    values << QString::number(p.value(QString::fromLatin1(key)).toDouble(), 'g', 6);
                }
                signature << values.join(QLatin1Char(','));
            }
            QVERIFY2(qFuzzyCompare(longest, 1.0), qPrintable(id));
            const QString key = signature.join(QLatin1Char(';'));
            QVERIFY2(!owners.contains(key), qPrintable(id + QStringLiteral(" = ") + owners.value(key)));
            owners.insert(key, id);
        }
        QCOMPARE(owners.size(), 16);
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

    // La commande view.page-effects ouvre le navigateur : une vignette (délégué « effect-<id> ») par entrée
    // du catalogue, dix-huit, et pas une de plus.
    void browserListsEveryEffect()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QObject* transition = transitionOf(main);
        QVERIFY(transition != nullptr);
        const QQuickItem* hidden = main.item(QStringLiteral("effectsRoot"));
        QVERIFY(hidden != nullptr);
        QVERIFY2(!hidden->isVisible(), "fermé tant qu'on ne l'a pas demandé");
        const QQuickItem* picker = openPicker(main);
        QVERIFY(picker != nullptr);
        const QVariantList catalogue = transition->property("catalogue").toList();
        QCOMPARE(catalogue.size(), 18);
        QStringList names;
        collectDelegates(picker, names);
        QCOMPARE(names.size(), 18);
        for (const QVariant& entry : catalogue) {
            const QString id = entry.toMap().value(QStringLiteral("id")).toString();
            QVERIFY2(names.contains(QStringLiteral("effect-") + id), qPrintable(id));
        }
    }

    // Un appui sur la deuxième vignette choisit l'effet global ; « Terminé » ferme sans le défaire.
    void browserChoiceSetsTheGlobalEffect()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QObject* transition = transitionOf(main);
        QVERIFY(transition != nullptr);
        QQuickItem* picker = openPicker(main);
        QVERIFY(picker != nullptr);
        const QVariantList catalogue = transition->property("catalogue").toList();
        QVERIFY(catalogue.size() > 2);
        const QString second = catalogue.at(1).toMap().value(QStringLiteral("id")).toString();
        const QString third = catalogue.at(2).toMap().value(QStringLiteral("id")).toString();
        QCOMPARE(transition->property("globalEffect").toString(), QStringLiteral("none"));
        QQuickItem* cell = main.item(QStringLiteral("effect-") + second);
        QVERIFY(cell != nullptr);
        QVERIFY(main.window()->contentItem()->contains(
            cell->mapToItem(main.window()->contentItem(), QPointF(5, 5))));
        QTest::mouseClick(main.window(), Qt::LeftButton, Qt::NoModifier, centerOf(cell));
        QTRY_COMPARE(transition->property("globalEffect").toString(), second);
        QQuickItem* other = main.item(QStringLiteral("effect-") + third);
        QVERIFY(other != nullptr);
        QTest::mouseClick(main.window(), Qt::LeftButton, Qt::NoModifier, centerOf(other));
        QTRY_COMPARE(transition->property("globalEffect").toString(), third);
        QQuickItem* done = main.item(QStringLiteral("effectsDone"));
        QVERIFY(done != nullptr);
        QTest::mouseClick(main.window(), Qt::LeftButton, Qt::NoModifier, centerOf(done));
        QTRY_VERIFY(!picker->isVisible());
        QCOMPARE(transition->property("globalEffect").toString(), third);
    }

    // Le même choix, à l'appui bref du stylet sur une vignette.
    void browserChoiceWorksWithTheStylus()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QObject* transition = transitionOf(main);
        QVERIFY(transition != nullptr);
        QVERIFY(openPicker(main) != nullptr);
        QQuickItem* cell = main.item(QStringLiteral("effect-dissolve"));
        QVERIFY(cell != nullptr);
        stylusTap(main.window(), centerOf(cell));
        QTRY_COMPARE(transition->property("globalEffect").toString(), QStringLiteral("dissolve"));
    }

    // Avec la case « pour cette page seulement », le choix s'écrit dans la page affichée (page.effect) et
    // laisse l'effet global.
    void browserPageOnlyChoiceWritesThePageEffect()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QObject* transition = transitionOf(main);
        DocumentController* document = documentOf(main);
        QVERIFY(transition && document);
        QVERIFY(openPicker(main) != nullptr);
        QQuickItem* box = main.item(QStringLiteral("effectsPageOnly"));
        QQuickItem* cell = main.item(QStringLiteral("effect-shards"));
        QVERIFY(box && cell);
        QTest::mouseClick(main.window(), Qt::LeftButton, Qt::NoModifier, centerOf(box));
        QTRY_VERIFY(box->property("checked").toBool());
        QTest::mouseClick(main.window(), Qt::LeftButton, Qt::NoModifier, centerOf(cell));
        QTRY_COMPARE(pageEffectOf(*document), QStringLiteral("shards"));
        QCOMPARE(transition->property("globalEffect").toString(), QStringLiteral("none"));
    }

    void pageEffectsPickerBlocksTheStylus_data() { presentationharness::addPointerRows(); }

    // Ouvert, le navigateur couvre la page : un trait posé sur lui n'en verse aucun, au stylet comme au doigt
    // ; le même geste, panneau fermé, verse un trait. À la souris, l'appui gauche n'atteint pas la vue même
    // sans couverture (constaté en L7-T9) : le clic droit, que la vue teste par sa couverture (menu
    // contextuel), discrimine : aucun menu sur le panneau, un menu une fois fermé.
    void pageEffectsPickerBlocksTheStylus()
    {
        QFETCH(Pointer, pointer);
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QQuickItem* picker = openPicker(main);
        QQuickItem* hint = main.item(QStringLiteral("effectsHint"));
        QQuickItem* done = main.item(QStringLiteral("effectsDone"));
        QVERIFY(picker && hint && done);
        // Un tracé sur l'astuce, texte sans gestionnaire, qui reste dans la page.
        const QPointF from = hint->mapToScene(QPointF(10, hint->height() / 2));
        const QPointF to = hint->mapToScene(QPointF(hint->width() - 10, hint->height() / 2));
        const QRectF pageZone = main.pageView()->mapRectToScene(main.pageView()->boundingRect());
        QVERIFY2(pageZone.contains(from) && pageZone.contains(to), "le tracé d'essai est sur la page");
        const int before = main.pageObjectCount();
        QSignalSpy menu(main.pageView(), &actcraft::ui::PageView::contextMenuRequested);

        presentationharness::strokeAt(main.window(), pointer, from, to);
        QTest::qWait(50);
        QCOMPARE(main.pageObjectCount(), before);
        QCOMPARE(main.pageView()->ink()->pointCount(), std::size_t{0});
        if (pointer == Pointer::Mouse) {
            QTest::mouseClick(main.window(), Qt::RightButton, Qt::NoModifier, from.toPoint());
            QTest::qWait(50);
            QCOMPARE(menu.count(), 0);
        }

        QTest::mouseClick(main.window(), Qt::LeftButton, Qt::NoModifier, centerOf(done));
        QTRY_VERIFY(!picker->isVisible());
        presentationharness::strokeAt(main.window(), pointer, from, to);
        if (pointer == Pointer::Mouse) {
            QTest::mouseClick(main.window(), Qt::RightButton, Qt::NoModifier, from.toPoint());
            QTRY_COMPARE(menu.count(), 1);
            QTest::keyClick(main.window(), Qt::Key_Escape);
        } else {
            QTRY_COMPARE(main.pageObjectCount(), before + 1);
        }
    }
};

} // namespace

ACTCRAFT_LOT7_TEST("transitions", TstUiTransitions)
#include "tst_ui_transitions.moc"
