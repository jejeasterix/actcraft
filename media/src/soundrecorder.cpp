#include "actcraft/media/soundrecorder.h"

#include <QAudioInput>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMediaCaptureSession>
#include <QMediaFormat>
#include <QMediaRecorder>
#include <QPointer>
#include <QStandardPaths>
#include <QUrl>

#if QT_CONFIG(permissions)
#include <QPermissions>
#endif

namespace actcraft::media {

namespace {

// Chaîne de Qt Multimedia : entrée audio par défaut, enregistreur au format WAV (PCM). L'arrêt est rendu
// quand l'enregistreur repasse à l'état « arrêté », c'est-à-dire fichier fermé.
class CaptureEngine final : public RecordingEngine
{
public:
    CaptureEngine()
    {
        m_session.setAudioInput(&m_input);
        m_session.setRecorder(&m_recorder);
        QMediaFormat format(QMediaFormat::Wave);
        format.setAudioCodec(QMediaFormat::AudioCodec::Wave);
        m_recorder.setMediaFormat(format);
        m_recorder.setQuality(QMediaRecorder::HighQuality);
        connect(&m_recorder, &QMediaRecorder::recorderStateChanged, this,
                [this](QMediaRecorder::RecorderState state) {
                    if (state != QMediaRecorder::StoppedState || !m_active)
                        return;
                    m_active = false;
                    const QUrl actual = m_recorder.actualLocation();
                    emit stopped(actual.isLocalFile() ? actual.toLocalFile() : m_path);
                });
        connect(&m_recorder, &QMediaRecorder::errorOccurred, this,
                [this](QMediaRecorder::Error error, const QString& text) {
                    if (error == QMediaRecorder::NoError)
                        return;
                    m_active = false;
                    emit failed(text.isEmpty() ? tr("Enregistrement du son impossible.") : text);
                });
    }

    void start(const QString& path) override
    {
        m_path = path;
        m_active = true;
        m_recorder.setOutputLocation(QUrl::fromLocalFile(path));
        m_recorder.record(); // rend la main : la capture démarre sur les fils de Qt Multimedia
    }
    void pause() override { m_recorder.pause(); }
    void resume() override { m_recorder.record(); }
    void stop() override { m_recorder.stop(); }
    qint64 duration() const override { return m_recorder.duration(); }
    QMediaFormat mediaFormat() const override { return m_recorder.mediaFormat(); }

private:
    QMediaCaptureSession m_session;
    QAudioInput m_input;
    QMediaRecorder m_recorder;
    QString m_path;
    bool m_active = false;
};

} // namespace

SoundRecorder::SoundRecorder(QObject* parent)
    : SoundRecorder(&SoundRecorder::createCaptureEngine, &SoundRecorder::requestMicrophonePermission, parent)
{}

SoundRecorder::SoundRecorder(EngineFactory engines, PermissionRequest permission, QObject* parent)
    : QObject(parent), m_engines(std::move(engines)), m_permission(std::move(permission)),
      m_folder(QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
               QStringLiteral("/actcraft-recordings"))
{}

SoundRecorder::~SoundRecorder()
{
    if (!m_engine)
        return;
    // Document fermé pendant l'enregistrement : le moteur ferme son fichier, puis l'efface lui-même (abandon)
    // ; il ne rappelle plus un enregistreur à demi détruit.
    const QString path = m_path;
    RecordingEngine* const engine = m_engine.release();
    engine->disconnect(this);
    engine->abandon(path);
}

std::unique_ptr<RecordingEngine> SoundRecorder::createCaptureEngine()
{
    return std::make_unique<CaptureEngine>();
}

// Première utilisation : la fenêtre du système (Android, macOS) répond plus tard ; ailleurs, accordée.
void SoundRecorder::requestMicrophonePermission(std::function<void(bool granted)> answer)
{
#if QT_CONFIG(permissions)
    QCoreApplication* const app = QCoreApplication::instance();
    if (!app) {
        answer(false);
        return;
    }
    const QMicrophonePermission microphone;
    switch (app->checkPermission(microphone)) {
    case Qt::PermissionStatus::Granted:
        answer(true);
        return;
    case Qt::PermissionStatus::Denied:
        answer(false);
        return;
    case Qt::PermissionStatus::Undetermined:
        break;
    }
    app->requestPermission(microphone, app, [answer](const QPermission& permission) {
        answer(permission.status() == Qt::PermissionStatus::Granted);
    });
#else
    answer(true);
#endif
}

void SoundRecorder::setFolder(const QString& folder)
{
    m_folder = folder;
}

QString SoundRecorder::folder() const
{
    return m_folder;
}

void SoundRecorder::setInsertHandler(std::function<void(const QString& path)> handler)
{
    m_insert = std::move(handler);
}

void SoundRecorder::toggle()
{
    switch (m_state) {
    case State::Idle:
        if (!m_available)
            return; // commande désactivée après un refus
        if (m_granted) {
            begin();
            return;
        }
        m_state = State::Asking;
        {
            const quint64 generation = ++m_generation;
            const QPointer<SoundRecorder> self(this);
            m_permission([self, generation](bool granted) {
                if (self)
                    self->answered(generation, granted);
            });
        }
        return;
    case State::Recording:
        m_engine->pause();
        m_state = State::Paused;
        setPaused(true);
        return;
    case State::Paused:
        m_engine->resume();
        m_state = State::Recording;
        setPaused(false);
        return;
    case State::Asking:
    case State::Finishing:
        return; // réponse ou fermeture attendue
    }
}

void SoundRecorder::stop()
{
    switch (m_state) {
    case State::Asking:
        cancel();
        return;
    case State::Recording:
    case State::Paused:
        m_cancelled = false;
        m_state = State::Finishing;
        m_engine->stop(); // rend la main ; engineStopped() suivra, fichier fermé
        return;
    case State::Idle:
    case State::Finishing:
        return;
    }
}

void SoundRecorder::cancel()
{
    switch (m_state) {
    case State::Asking:
        ++m_generation; // la réponse encore attendue ne lance plus rien
        m_state = State::Idle;
        return;
    case State::Recording:
    case State::Paused:
        m_cancelled = true;
        m_state = State::Finishing;
        m_engine->stop(); // le fichier s'efface quand le moteur l'a fermé
        return;
    case State::Finishing:
        m_cancelled = true;
        return;
    case State::Idle:
        return;
    }
}

bool SoundRecorder::isRecording() const
{
    return m_state == State::Recording || m_state == State::Paused || m_state == State::Finishing;
}

bool SoundRecorder::isPaused() const
{
    return m_paused;
}

bool SoundRecorder::isAvailable() const
{
    return m_available;
}

QString SoundRecorder::message() const
{
    return m_message;
}

qint64 SoundRecorder::duration() const
{
    return m_engine && isRecording() ? m_engine->duration() : 0;
}

void SoundRecorder::answered(quint64 generation, bool granted)
{
    if (generation != m_generation || m_state != State::Asking)
        return; // abandonné entre-temps
    m_state = State::Idle;
    if (!granted) {
        m_available = false;
        m_message = tr("Le micro est refusé à ActCraft : l'enregistrement de son est désactivé. Autorisez le "
                       "micro dans les réglages du système, puis relancez ActCraft.");
        emit availabilityChanged();
        emit failed(m_message);
        return;
    }
    m_granted = true;
    begin();
}

void SoundRecorder::begin()
{
    if (!QDir().mkpath(m_folder)) {
        emit failed(tr("Dossier d'enregistrement inaccessible : %1").arg(m_folder));
        return;
    }
    // Un moteur par enregistrement : le micro est relâché entre deux enregistrements.
    m_engine = m_engines ? m_engines() : nullptr;
    if (!m_engine) {
        emit failed(tr("Enregistrement du son impossible."));
        return;
    }
    connect(m_engine.get(), &RecordingEngine::stopped, this, &SoundRecorder::engineStopped);
    connect(m_engine.get(), &RecordingEngine::failed, this, &SoundRecorder::engineFailed);
    ++m_generation;
    m_path = nextPath();
    m_cancelled = false;
    m_state = State::Recording;
    m_engine->start(m_path); // chemin imposé : jamais l'emplacement par défaut du moteur
    emit recordingChanged();
}

void SoundRecorder::engineStopped(const QString& path)
{
    if (m_state != State::Recording && m_state != State::Paused && m_state != State::Finishing)
        return;
    const bool cancelled = m_cancelled;
    retireEngine(false);
    m_path.clear();
    end();
    if (cancelled) {
        QFile::remove(path); // fichier fermé par le moteur : rien ne le réécrira
        return;
    }
    emit finished(path);
    if (m_insert)
        m_insert(path); // ui/ en fait une ressource et pose l'icône de lecture, en une étape
}

void SoundRecorder::engineFailed(const QString& reason)
{
    if (m_state != State::Recording && m_state != State::Paused && m_state != State::Finishing)
        return;
    // Le fichier incomplet s'efface quand le moteur l'a fermé (abandon), jamais avant.
    if (m_engine) {
        RecordingEngine* const engine = m_engine.release();
        engine->disconnect(this);
        engine->abandon(m_path);
    }
    m_path.clear();
    end();
    emit failed(reason);
}

// Le moteur sort par deleteLater (on peut être dans l'un de ses signaux). En échec, son fichier incomplet
// s'efface aussi après sa destruction : un moteur ferme son conteneur en se détruisant.
void SoundRecorder::retireEngine(bool removeFile)
{
    if (!m_engine)
        return;
    RecordingEngine* const engine = m_engine.release();
    engine->disconnect(this);
    if (removeFile) {
        const QString path = m_path;
        connect(engine, &QObject::destroyed, engine, [path] { QFile::remove(path); });
    }
    engine->setParent(this);
    engine->deleteLater();
}

void SoundRecorder::end()
{
    ++m_generation;
    m_state = State::Idle;
    m_cancelled = false;
    setPaused(false);
    emit recordingChanged();
}

void SoundRecorder::setPaused(bool paused)
{
    if (m_paused == paused)
        return;
    m_paused = paused;
    emit pausedChanged();
}

// Nom libre dans le dossier du document : son-1.wav, son-2.wav…
QString SoundRecorder::nextPath() const
{
    const QDir folder(m_folder);
    for (int index = 1;; ++index) {
        const QString path = folder.filePath(QStringLiteral("son-%1.wav").arg(index));
        if (!QFileInfo::exists(path))
            return path;
    }
}

} // namespace actcraft::media
