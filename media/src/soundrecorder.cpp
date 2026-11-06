#include "actcraft/media/soundrecorder.h"

// Talon du test rouge (L8-T26) : l'API existe, rien ne s'enregistre encore.

namespace actcraft::media {

SoundRecorder::SoundRecorder(QObject* parent) : QObject(parent) {}

SoundRecorder::SoundRecorder(EngineFactory engines, PermissionRequest permission, QObject* parent)
    : QObject(parent), m_engines(std::move(engines)), m_permission(std::move(permission))
{}

SoundRecorder::~SoundRecorder() = default;

std::unique_ptr<RecordingEngine> SoundRecorder::createCaptureEngine()
{
    return {};
}

void SoundRecorder::requestMicrophonePermission(std::function<void(bool granted)> answer)
{
    answer(false);
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

void SoundRecorder::toggle() {}
void SoundRecorder::stop() {}
void SoundRecorder::cancel() {}

bool SoundRecorder::isRecording() const
{
    return m_state == State::Recording || m_state == State::Paused;
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
    return 0;
}

void SoundRecorder::answered(quint64 generation, bool granted)
{
    m_granted = generation == m_generation && granted;
    m_cancelled = false;
}
void SoundRecorder::begin() {}
void SoundRecorder::engineStopped(const QString&) {}
void SoundRecorder::engineFailed(const QString&) {}
void SoundRecorder::end() {}
void SoundRecorder::setPaused(bool) {}

QString SoundRecorder::nextPath() const
{
    return {};
}

} // namespace actcraft::media
