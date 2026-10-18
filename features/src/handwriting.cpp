#include <actcraft/features/handwriting.h>

#include <QMetaObject>

#include <utility>

namespace actcraft::features {

namespace {

class UnavailableRecognizer final : public HandwritingRecognizer
{
public:
    explicit UnavailableRecognizer(QString why) : m_why(std::move(why)) {}

    bool available() const override { return false; }
    QString status() const override { return m_why; }
    QStringList recognize(const InkStrokes&) override { return {}; }

private:
    QString m_why;
};

QString unavailableText()
{
    return QStringLiteral("La reconnaissance d'écriture utilise le moteur d'encre de Windows ; il n'est pas "
                          "disponible sur ce système.");
}

} // namespace

std::unique_ptr<HandwritingRecognizer> makeHandwritingRecognizer()
{
    auto platform = makePlatformHandwritingRecognizer();
    if (platform)
        return platform;
    return std::make_unique<UnavailableRecognizer>(unavailableText());
}

// Un seul fil, le même pour toutes les demandes : un objet COM d'appartement unique ne sert que sur le fil
// qui l'a créé, et CoUninitialize doit suivre CoInitializeEx sur ce même fil. Un pool (QThreadPool,
// QtConcurrent) ne le garantit pas d'une demande à l'autre.
HandwritingThread::HandwritingThread(Factory factory) : m_factory(std::move(factory)), m_context(new QObject)
{
    m_context->moveToThread(this); // les demandes attendent dans la file jusqu'à exec()
    start();
}

HandwritingThread::~HandwritingThread()
{
    quit();
    wait();
    delete m_context; // nul si run() a tourné ; sinon (fil jamais démarré) libéré ici
}

void HandwritingThread::run()
{
    const bool apartment = enterRecognitionApartment();
    m_recognizer = m_factory ? m_factory() : nullptr; // créé dans l'appartement, sur ce fil
    if (!m_recognizer)
        m_recognizer = std::make_unique<UnavailableRecognizer>(unavailableText());
    exec(); // boucle d'événements : sert les demandes et fait passer les messages de l'appartement
    m_recognizer.reset(); // détruit sur ce fil, avant de sortir de l'appartement
    delete m_context;     // emporte les demandes restées en file
    m_context = nullptr;
    if (apartment)
        leaveRecognitionApartment();
}

void HandwritingThread::recognize(InkStrokes strokes, QObject* receiver, Callback done)
{
    post(receiver, std::move(done), std::move(strokes));
}

void HandwritingThread::probe(QObject* receiver, Callback done)
{
    post(receiver, std::move(done), std::nullopt);
}

void HandwritingThread::post(QObject* receiver, Callback done, std::optional<InkStrokes> strokes)
{
    // En file vers m_context, qui vit sur ce fil : l'appelant reprend la main aussitôt.
    QMetaObject::invokeMethod(
        m_context,
        [this, receiver, done = std::move(done), strokes = std::move(strokes)]() {
            RecognitionReply reply{m_recognizer->available(), m_recognizer->status(), {}};
            if (reply.available && strokes)
                reply.texts = m_recognizer->recognize(*strokes);
            // Réponse en file vers le fil du receveur : jamais appelée ici.
            QMetaObject::invokeMethod(receiver, [done, reply]() { done(reply); }, Qt::QueuedConnection);
        },
        Qt::QueuedConnection);
}

} // namespace actcraft::features
