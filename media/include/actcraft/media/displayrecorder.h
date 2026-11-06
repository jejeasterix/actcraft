#pragma once

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QMediaFormat>
#include <QObject>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QStandardPaths>
#include <QString>
#include <QTimer>
#include <QVideoFrame>
#include <QVideoFrameFormat>

#include <functional>
#include <memory>

namespace actcraft::media {

// Moteur d'un enregistrement d'écran. En production (Windows seulement, DisplayRecorder::createScreenEngine)
// : QScreenCapture de l'écran principal lue par un QVideoSink, et QMediaRecorder en MP4 nourri par un
// QVideoFrameInput ; dans les tests : un écran simulé. Le moteur ne recadre ni n'horodate : il livre l'écran
// entier (frameCaptured) et écrit ce que DisplayRecorder lui rend (encode). Contrat de Qt Multimedia :
// start() et stop() rendent la main sur-le-champ ; le fichier n'est fermé et complet qu'à l'émission de
// stopped(), plus tard, sur le fil de l'enregistreur. Détruire le moteur n'est pas fermer son fichier : la
// finalisation du MP4 survit à QMediaRecorder. Seul stopped() dit que le fichier est libre.
class DisplayEngine : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;
    ~DisplayEngine() override = default;

    // Fichier imposé : jamais l'emplacement par défaut du moteur (dossier Vidéos de l'utilisateur).
    virtual void start(const QString& path) = 0;
    // Image à écrire : zone déjà découpée, horodatée en microsecondes de temps enregistré (pauses exclues).
    virtual void encode(const QVideoFrame& frame) = 0;
    // Demande la fermeture du fichier ; peut être appelé plusieurs fois (une panne, puis un abandon).
    // stopped() suit une seule fois, aussi après une panne (failed() puis stopped()).
    virtual void stop() = 0;
    // Format du fichier écrit (vide pour un écran simulé).
    virtual QMediaFormat mediaFormat() const { return {}; }

    // Délai de garde d'un abandon : un moteur qui ne ferme jamais son fichier est libéré après lui.
    void setAbandonTimeout(int milliseconds) { m_abandonTimeoutMs = milliseconds; }

    // Fin confiée au moteur : l'enregistreur le lâche sans le détruire (document fermé, panne), parce
    // qu'effacer le fichier avant sa fermeture échoue (fichier ouvert sous Windows) ou le manque (écrit après
    // coup). Le moteur demande l'arrêt, attend stopped() (ou le délai de garde), efface alors le fichier
    // partiel, puis se libère par deleteLater. À appeler une fois, par l'enregistreur seulement, sur un
    // moteur dont le fichier n'est pas fermé ; path est l'emplacement demandé, effacé si le moteur n'a rien
    // dit à l'expiration du délai.
    void abandon(const QString& path)
    {
        if (m_abandoned)
            return;
        m_abandoned = true;
        setParent(nullptr); // plus rien ne le détruit à la place du moteur
        connect(this, &DisplayEngine::stopped, this, [this](const QString& closed) { discard(closed); });
        auto* guard = new QTimer(this);
        guard->setSingleShot(true);
        connect(guard, &QTimer::timeout, this, [this, path] { discard(path); });
        guard->start(m_abandonTimeoutMs);
        stop();
    }

signals:
    // Écran principal entier, en pixels de l'image capturée.
    void frameCaptured(const QVideoFrame& frame);
    // Fichier fermé et complet ; path est l'emplacement réel, qui peut différer de celui demandé.
    void stopped(const QString& path);
    void failed(const QString& reason);

private:
    void discard(const QString& path)
    {
        if (m_discarded)
            return;
        m_discarded = true;
        QFile::remove(path);
        deleteLater();
    }

    int m_abandonTimeoutMs = 30'000;
    bool m_abandoned = false;
    bool m_discarded = false;
};

// Zone réellement écrite : coupée aux bords de l'image, côtés ramenés au pair (H.264 en 4:2:0). Zone vide :
// l'image entière.
inline QRect recordedArea(const QRect& area, const QSize& frame)
{
    const QRect whole(QPoint(0, 0), frame);
    QRect rect = area.isEmpty() ? whole : area.intersected(whole);
    rect.setWidth(rect.width() & ~1);
    rect.setHeight(rect.height() & ~1);
    return rect;
}

// Zone d'une image d'écran, sans image de l'écran entier : l'image est mappée en lecture et seules les lignes
// de la zone sont copiées (pixels de 4 octets : BGRA, BGRX, RGBA…). Les autres formats, ou une image qui ne
// se mappe pas, passent par toImage(), le chemin général de Qt. rect est contenu dans l'image (recordedArea).
inline QVideoFrame cropFrame(const QVideoFrame& frame, const QRect& rect)
{
    QVideoFrame source(frame);
    QVideoFrame cropped;
    if (source.map(QVideoFrame::ReadOnly)) {
        const QImage::Format format = QVideoFrameFormat::imageFormatFromPixelFormat(source.pixelFormat());
        constexpr qsizetype bytesPerPixel = 4;
        const qsizetype stride = source.bytesPerLine(0);
        if (format != QImage::Format_Invalid && source.planeCount() == 1 &&
            QImage::toPixelFormat(format).bitsPerPixel() == 8 * bytesPerPixel && source.bits(0) &&
            stride >= qsizetype(frame.width()) * bytesPerPixel) {
            const uchar* origin =
                source.bits(0) + qsizetype(rect.y()) * stride + qsizetype(rect.x()) * bytesPerPixel;
            cropped = QVideoFrame(QImage(origin, rect.width(), rect.height(), stride, format).copy());
        }
        source.unmap();
    }
    return cropped.isValid() ? cropped : QVideoFrame(frame.toImage().copy(rect));
}

// F-292 : enregistreur d'écran (Windows), plein écran ou zone, en MP4. Premier appui : l'enregistrement
// commence ; appuis suivants : pause, puis reprise. Le moteur livre l'écran entier ; la zone en est découpée
// avant l'encodeur (jamais l'écran entier mis à l'échelle de la zone), et chaque image reçoit le temps
// enregistré, pauses exclues (jamais l'heure de capture) : la pause ne laisse ni durée ni trou dans le
// fichier. À l'arrêt, le fichier, resté dans le dossier fixé par setFolder (dossier temporaire du document),
// est confié une seule fois au rappel d'insertion posé par ui/ ; à l'abandon et à la destruction, il est
// effacé après sa fermeture par le moteur. Hors Windows, le module est construit sans moteur d'écran :
// isAvailable() est faux et message() dit pourquoi. Rien n'est attendu sur le fil principal.
class DisplayRecorder : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool recording READ isRecording NOTIFY recordingChanged)
    Q_PROPERTY(bool paused READ isPaused NOTIFY pausedChanged)
    Q_PROPERTY(bool available READ isAvailable CONSTANT)
    Q_PROPERTY(QString message READ message CONSTANT)

public:
    using EngineFactory = std::function<std::unique_ptr<DisplayEngine>()>;

    // Moteur de la plateforme : l'écran principal sous Windows, aucun ailleurs.
    explicit DisplayRecorder(QObject* parent = nullptr)
        : DisplayRecorder(unavailableReason().isEmpty() ? EngineFactory(&DisplayRecorder::createScreenEngine)
                                                        : EngineFactory(),
                          parent)
    {
        m_message = unavailableReason();
    }
    explicit DisplayRecorder(EngineFactory engines, QObject* parent = nullptr)
        : QObject(parent), m_engines(std::move(engines)),
          m_folder(QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
                   QStringLiteral("/actcraft-recordings")),
          m_available(bool(m_engines))
    {}
    ~DisplayRecorder() override
    {
        // Document fermé pendant l'enregistrement : le moteur survit à l'enregistreur, ferme son fichier,
        // l'efface puis se libère. Il ne rappelle plus un enregistreur à demi détruit.
        abandonEngine();
    }

    // Définis par displayrecorder_win.cpp sous Windows, par displayrecorder_other.cpp ailleurs (aucun moteur,
    // raison non vide).
    static std::unique_ptr<DisplayEngine> createScreenEngine();
    static QString unavailableReason();

    void setFolder(const QString& folder) { m_folder = folder; }
    QString folder() const { return m_folder; }
    void setInsertHandler(std::function<void(const QString& path)> handler) { m_insert = std::move(handler); }

    // Zone en pixels de l'image capturée (écran principal) ; vide : plein écran. Prise au début de chaque
    // enregistrement.
    void setArea(const QRect& area) { m_area = area; }
    QRect area() const { return m_area; }

    // Enregistrer, puis pause, puis reprise.
    void toggle()
    {
        switch (m_state) {
        case State::Idle:
            begin();
            return;
        case State::Recording:
            m_pauseClock.start();
            m_state = State::Paused;
            setPaused(true);
            return;
        case State::Paused:
            m_pausedNs += m_pauseClock.nsecsElapsed();
            m_state = State::Recording;
            setPaused(false);
            return;
        case State::Finishing:
            return; // fermeture attendue
        }
    }
    // Arrêt : le rappel d'insertion part quand le moteur a fermé le fichier.
    void stop() { finish(false); }
    // Abandon : le fichier est effacé quand le moteur l'a fermé ; aucun rappel.
    void cancel()
    {
        if (m_state == State::Finishing) {
            m_cancelled = true;
            return;
        }
        finish(true);
    }

    // Vrai de l'appui jusqu'à la fermeture du fichier, pause comprise : l'indicateur suit.
    bool isRecording() const
    {
        return m_state == State::Recording || m_state == State::Paused || m_state == State::Finishing;
    }
    bool isPaused() const { return m_paused; }
    // Faux sans moteur d'écran (hors Windows) : la commande est absente, message() dit pourquoi.
    bool isAvailable() const { return m_available; }
    QString message() const { return m_message; }
    // Durée enregistrée en millisecondes, pauses exclues.
    qint64 duration() const { return isRecording() ? recordedMicroseconds() / 1000 : 0; }

signals:
    void recordingChanged();
    void pausedChanged();
    void finished(const QString& path);
    void failed(const QString& reason);

private:
    enum class State
    {
        Idle,
        Recording,
        Paused,
        Finishing
    };

    // Image de la première seconde : durée d'image de 30 i/s, en attendant l'écart entre deux images.
    static constexpr qint64 firstFrameMicroseconds = 33'333;
    // Cadence écrite au plus de 30 i/s : un écran à 60 ou 144 Hz en livre plus, et chaque image écrite coûte
    // un découpage et un encodage sur le fil principal. Les images à moins de 30 ms de la dernière écrite
    // sont sautées avant tout travail ; 30 ms plutôt que 33,3, pour que la gigue de l'horloge ne fasse pas
    // sauter une image sur deux d'une source déjà à 30 i/s.
    static constexpr qint64 minFrameGapMicroseconds = 30'000;

    void begin()
    {
        if (!m_available)
            return; // commande absente ; même appelée, rien ne démarre
        if (!QDir().mkpath(m_folder)) {
            emit failed(tr("Dossier d'enregistrement inaccessible : %1").arg(m_folder));
            return;
        }
        // Un moteur par enregistrement : la capture de l'écran est relâchée entre deux enregistrements.
        m_engine = m_engines();
        if (!m_engine) {
            emit failed(tr("Enregistrement de l'écran impossible."));
            return;
        }
        connect(m_engine.get(), &DisplayEngine::frameCaptured, this, &DisplayRecorder::engineFrame);
        connect(m_engine.get(), &DisplayEngine::stopped, this, &DisplayRecorder::engineStopped);
        connect(m_engine.get(), &DisplayEngine::failed, this, &DisplayRecorder::engineFailed);
        m_path = nextPath();
        m_recordingArea = m_area;
        m_cancelled = false;
        m_pausedNs = 0;
        m_lastStart = -1;
        m_state = State::Recording;
        m_clock.start();
        m_engine->start(m_path); // chemin imposé : jamais l'emplacement par défaut du moteur
        emit recordingChanged();
    }

    void finish(bool cancelled)
    {
        if (m_state != State::Recording && m_state != State::Paused)
            return;
        m_frozen = recordedMicroseconds();
        m_cancelled = cancelled;
        m_state = State::Finishing;
        m_engine->stop(); // rend la main ; engineStopped() suivra, fichier fermé
    }

    // Temps enregistré : horloge du début, moins les pauses passées et la pause en cours.
    qint64 recordedMicroseconds() const
    {
        if (m_state == State::Finishing)
            return m_frozen;
        qint64 nanoseconds = m_clock.nsecsElapsed() - m_pausedNs;
        if (m_state == State::Paused)
            nanoseconds -= m_pauseClock.nsecsElapsed();
        return nanoseconds / 1000;
    }

    // Écran entier reçu : en pause ou à la fermeture, ni image ni temps ; sinon la zone, au temps enregistré.
    void engineFrame(const QVideoFrame& frame)
    {
        if (m_state != State::Recording || !frame.isValid())
            return;
        const qint64 start = recordedMicroseconds();
        if (m_lastStart >= 0 && start - m_lastStart < minFrameGapMicroseconds)
            return; // trop proche de la dernière image écrite : ni découpée ni encodée
        const QRect whole(QPoint(0, 0), frame.size());
        const QRect rect = recordedArea(m_recordingArea, frame.size());
        if (rect.isEmpty()) {
            engineFailed(tr("La zone choisie est hors de l'écran."));
            return;
        }
        // Plein écran : l'image passe telle quelle ; zone : seules ses lignes sont copiées, jamais mise à
        // l'échelle.
        QVideoFrame out = rect == whole ? frame : cropFrame(frame, rect);
        if (!out.isValid())
            return;
        out.setStartTime(start);
        out.setEndTime(start + (m_lastStart < 0 ? firstFrameMicroseconds : start - m_lastStart));
        m_lastStart = start;
        m_engine->encode(out);
    }

    void engineStopped(const QString& path)
    {
        if (!isRecording())
            return;
        const bool cancelled = m_cancelled;
        retireEngine();
        m_path.clear();
        end();
        if (cancelled) {
            QFile::remove(path); // fichier fermé par le moteur : rien ne le réécrira
            return;
        }
        emit finished(path);
        if (m_insert)
            m_insert(path); // ui/ en fait une ressource et pose le média sur la page, en une étape
    }

    void engineFailed(const QString& reason)
    {
        if (!isRecording())
            return;
        abandonEngine(); // le moteur efface son fichier partiel quand il l'a fermé
        m_path.clear();
        end();
        emit failed(reason);
    }

    // Moteur dont le fichier est fermé (stopped() reçu) : il sort par deleteLater, on peut être dans l'un de
    // ses signaux.
    void retireEngine()
    {
        if (!m_engine)
            return;
        DisplayEngine* const engine = m_engine.release();
        engine->disconnect(this);
        engine->setParent(this);
        engine->deleteLater();
    }

    // Moteur dont le fichier n'est pas fermé (panne, destruction de l'enregistreur) : il finit seul, et
    // efface le fichier partiel après l'avoir fermé. L'effacer d'ici passerait avant la fermeture.
    void abandonEngine()
    {
        if (!m_engine)
            return;
        DisplayEngine* const engine = m_engine.release();
        engine->disconnect(this);
        engine->abandon(m_path);
    }

    void end()
    {
        m_state = State::Idle;
        m_cancelled = false;
        setPaused(false);
        emit recordingChanged();
    }

    void setPaused(bool paused)
    {
        if (m_paused == paused)
            return;
        m_paused = paused;
        emit pausedChanged();
    }

    // Nom libre dans le dossier du document : ecran-1.mp4, ecran-2.mp4…
    QString nextPath() const
    {
        const QDir folder(m_folder);
        for (int index = 1;; ++index) {
            const QString path = folder.filePath(QStringLiteral("ecran-%1.mp4").arg(index));
            if (!QFileInfo::exists(path))
                return path;
        }
    }

    EngineFactory m_engines;
    std::function<void(const QString& path)> m_insert;
    std::unique_ptr<DisplayEngine> m_engine;
    QString m_folder;
    QString m_path;
    QString m_message;
    QRect m_area;
    QRect m_recordingArea;
    QElapsedTimer m_clock;
    QElapsedTimer m_pauseClock;
    qint64 m_pausedNs = 0;
    qint64 m_frozen = 0;
    qint64 m_lastStart = -1;
    State m_state = State::Idle;
    bool m_available = false;
    bool m_cancelled = false;
    bool m_paused = false;
};

} // namespace actcraft::media
