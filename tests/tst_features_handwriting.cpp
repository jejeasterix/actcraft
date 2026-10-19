#include <actcraft/features/handwriting.h>

#include <QtTest>

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <objbase.h> // test seulement (pas du code livré) : type d'appartement COM du fil
#endif

using namespace actcraft::features;

namespace {

// Attente maximale du reconnaisseur factice à sa barrière. Large : un poste chargé ne la dépasse pas quand
// l'appelant rend la main aussitôt ; un appelant qui attend la reconnaissance la dépasse toujours.
constexpr std::chrono::seconds kGateTimeout{10};

#ifdef Q_OS_WIN
// Vrai si le fil courant est dans un appartement COM unique (STA).
bool currentThreadIsSta()
{
    APTTYPE type{};
    APTTYPEQUALIFIER qualifier{};
    return SUCCEEDED(CoGetApartmentType(&type, &qualifier)) &&
           (type == APTTYPE_STA || type == APTTYPE_MAINSTA);
}
#endif

// Journal du reconnaisseur factice : écrit sur le fil de la reconnaissance, lu par le test.
struct ThreadLog
{
    std::mutex mutex;
    std::condition_variable gateOpened;
    std::vector<QThread*> threads; // fil de la création, puis de chaque reconnaissance
    std::size_t staCalls = 0;      // appels (création compris) faits dans un appartement unique
    bool staWhenDestroyed = false; // appartement unique encore ouvert à la destruction du reconnaisseur
    bool destroyedOnCreationThread =
        false;                 // détruit sur le fil de sa création, qui n'est pas le fil principal
    bool gateOpen = true;      // faux : la création et chaque reconnaissance attendent que le test l'ouvre
    bool gateTimedOut = false; // vrai si un appel a attendu la barrière en vain
};

// Reconnaisseur factice : note le fil et l'appartement de chaque appel.
class FakeRecognizer final : public HandwritingRecognizer
{
public:
    FakeRecognizer(std::shared_ptr<ThreadLog> log, bool isAvailable)
        : m_log(std::move(log)), m_available(isAvailable), m_createdOn(QThread::currentThread())
    {
        noteCall();
    }
    ~FakeRecognizer() override
    {
        QThread* const here = QThread::currentThread();
        const bool onCreationThread = here == m_createdOn && here != QCoreApplication::instance()->thread();
#ifdef Q_OS_WIN
        const bool inApartment = currentThreadIsSta();
#endif
        const std::lock_guard lock(m_log->mutex);
        m_log->destroyedOnCreationThread = onCreationThread;
#ifdef Q_OS_WIN
        m_log->staWhenDestroyed = inApartment;
#endif
    }
    FakeRecognizer(const FakeRecognizer&) = delete;
    FakeRecognizer& operator=(const FakeRecognizer&) = delete;

    bool available() const override { return m_available; }
    QString status() const override
    {
        return m_available ? QStringLiteral("factice") : QStringLiteral("indisponible : test");
    }
    QStringList recognize(const InkStrokes&) override
    {
        noteCall();
        return {QStringLiteral("bonjour"), QStringLiteral("bonjours")};
    }

private:
    // Note le fil et l'appartement de l'appel, puis attend la barrière (aussitôt passée si elle est ouverte).
    // Le test ne l'ouvre qu'après le retour de ses appels : un appelant qui attendrait le reconnaisseur la
    // trouverait fermée jusqu'au bout du délai.
    void noteCall()
    {
        std::unique_lock lock(m_log->mutex);
        m_log->threads.push_back(QThread::currentThread());
#ifdef Q_OS_WIN
        if (currentThreadIsSta())
            ++m_log->staCalls;
#endif
        if (!m_log->gateOpened.wait_for(lock, kGateTimeout, [this] { return m_log->gateOpen; }))
            m_log->gateTimedOut = true;
    }

    std::shared_ptr<ThreadLog> m_log;
    bool m_available;
    QThread* m_createdOn; // fil de la création
};

} // namespace

class TstFeaturesHandwriting : public QObject
{
    Q_OBJECT

private slots:
    void factoryNeverReturnsNull()
    {
        auto r =
            makeHandwritingRecognizer(); // fil principal, sans COM : retombe sur « indisponible » au besoin
        QVERIFY(r != nullptr);
        if (!r->available())
            QVERIFY(!r->status().isEmpty());
#ifndef Q_OS_WIN
        QVERIFY(!r->available()); // hors de Windows, jamais de moteur (reprend unavailableOffWindows de 7b)
#endif
    }
    void recognitionRunsOnOneDedicatedThread()
    {
        auto log = std::make_shared<ThreadLog>();
        log->gateOpen = false; // fermée avant le démarrage du fil : la création attend aussi
        std::vector<QThread*> callbackThreads;
        int replies = 0;
        // Receveur propre au test, détruit après le fil : une réponse restée en file part avec lui, jamais
        // livrée dans un autre test (les rappels écrivent dans cette pile).
        QObject receiver;
        HandwritingThread thread([log] { return std::make_unique<FakeRecognizer>(log, true); });
        const InkStrokes ink{{{0, 0}, {0, 10}}};
        for (int i = 0; i < 2; ++i)
            thread.recognize(ink, &receiver, [&](const RecognitionReply& reply) {
                QCOMPARE(reply.texts.value(0), QStringLiteral("bonjour"));
                callbackThreads.push_back(QThread::currentThread());
                ++replies;
            });
        // L'appelant a repris la main : la création et les deux reconnaissances attendent la barrière.
        const int repliesBeforeTheLoop = replies;
        {
            const std::lock_guard lock(log->mutex);
            log->gateOpen = true;
        }
        log->gateOpened.notify_all();
        QTRY_COMPARE(replies, 2);
        QCOMPARE(repliesBeforeTheLoop, 0); // réponses rendues en file, jamais pendant l'appel
        const std::lock_guard lock(log->mutex);
        QVERIFY(!log->gateTimedOut); // ni la construction ni recognize() n'ont attendu le reconnaisseur
        QCOMPARE(log->threads.size(), std::size_t{3}); // création + deux reconnaissances
        for (QThread* t : log->threads) {
            QCOMPARE(t, static_cast<QThread*>(&thread)); // toujours le même fil, le fil dédié
            QVERIFY(t != QThread::currentThread());      // jamais le fil principal
        }
        for (QThread* t : callbackThreads)
            QCOMPARE(t, QThread::currentThread()); // réponse rendue sur le fil du receveur
    }
    void recognitionThreadIsSingleThreadedApartment()
    {
#ifdef Q_OS_WIN
        auto log = std::make_shared<ThreadLog>();
        {
            bool done = false;
            QObject receiver; // détruit après le fil
            HandwritingThread thread([log] { return std::make_unique<FakeRecognizer>(log, true); });
            const InkStrokes ink{{ {0, 0}, {5, 5} }};
            thread.recognize(ink, &receiver, [&](const RecognitionReply&) { done = true; });
            QTRY_VERIFY(done);
            const std::lock_guard lock(log->mutex);
            QCOMPARE(log->threads.size(), std::size_t{2}); // création + une reconnaissance
            QCOMPARE(log->staCalls, log->threads.size());  // l'une et l'autre dans l'appartement unique
            QCOMPARE(log->threads.front(), static_cast<QThread*>(&thread)); // créé sur le fil dédié
        }
        // Fil arrêté : le reconnaisseur a été détruit sur le fil de sa création (le fil dédié), avant la
        // sortie de l'appartement.
        const std::lock_guard lock(log->mutex);
        QVERIFY(log->destroyedOnCreationThread);
        QVERIFY(log->staWhenDestroyed);
#else
        QSKIP("Pas de COM hors de Windows : l'appartement ne se vérifie que sous Windows (preuve locale).");
#endif
    }
    void unavailableReplyCarriesTheExplanation()
    {
        auto log = std::make_shared<ThreadLog>();
        std::optional<RecognitionReply> got;
        QObject receiver; // détruit après le fil
        HandwritingThread thread([log] { return std::make_unique<FakeRecognizer>(log, false); });
        thread.probe(&receiver, [&](const RecognitionReply& reply) { got = reply; });
        QTRY_VERIFY(got.has_value());
        QVERIFY(!got->available);
        QVERIFY(!got->status.isEmpty());
        QVERIFY(got->texts.isEmpty());
    }
    void recognizesSyntheticInk()
    {
        std::optional<RecognitionReply> probe;
        std::optional<RecognitionReply> got;
        QObject receiver;         // détruit après le fil
        HandwritingThread thread; // vrai reconnaisseur, sur le fil STA
        thread.probe(&receiver, [&](const RecognitionReply& reply) { probe = reply; });
        QTRY_VERIFY_WITH_TIMEOUT(probe.has_value(), 10000); // premier chargement du moteur d'encre
        if (!probe->available)
            QSKIP(qPrintable(QStringLiteral("reconnaisseur absent : ") + probe->status));
        // « l » : un trait vertical ; le contenu n'est pas comparé (S10), seulement la forme du résultat
        thread.recognize(InkStrokes{{{100, 100}, {100, 140}, {100, 180}}}, &receiver,
                         [&](const RecognitionReply& reply) { got = reply; });
        QTRY_VERIFY_WITH_TIMEOUT(got.has_value(), 10000);
        // Meilleur texte et au moins une alternative (lue par IDispatch) ; 1 + 10 alternatives au plus. Un
        // moteur présent qui ne rend rien échoue ici, sans SKIP : le seul saut permis est « reconnaisseur
        // absent ».
        const QString seen = QStringLiteral("textes : ") + got->texts.join(QStringLiteral(" / "));
        QVERIFY2(got->texts.size() >= 2, qPrintable(seen));
        QVERIFY2(got->texts.size() <= 11, qPrintable(seen));
        QStringList copy = got->texts;
        QCOMPARE(copy.removeDuplicates(), qsizetype{0});
    }
};

QTEST_GUILESS_MAIN(TstFeaturesHandwriting)
#include "tst_features_handwriting.moc"
