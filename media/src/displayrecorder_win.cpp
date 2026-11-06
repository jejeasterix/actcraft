#include "actcraft/media/displayrecorder.h"

#include <QGuiApplication>
#include <QMediaCaptureSession>
#include <QMediaFormat>
#include <QMediaRecorder>
#include <QScreen>
#include <QScreenCapture>
#include <QUrl>
#include <QVideoFrame>
#include <QVideoFrameInput>
#include <QVideoSink>

namespace actcraft::media {

namespace {

// Chaîne de Qt Multimedia, en deux sessions : la première capture l'écran principal et le livre à un puits
// d'images (frameCaptured) ; la seconde reçoit, par une entrée d'images, ce que DisplayRecorder en a tiré
// (zone découpée, temps hors pause) et l'écrit en MP4 (H.264). L'arrêt est rendu quand l'enregistreur repasse
// à l'état « arrêté », c'est-à-dire fichier fermé.
class ScreenEngine final : public DisplayEngine
{
public:
    ScreenEngine()
    {
        m_screen.setScreen(QGuiApplication::primaryScreen());
        m_capture.setScreenCapture(&m_screen);
        m_capture.setVideoSink(&m_sink);
        connect(&m_sink, &QVideoSink::videoFrameChanged, this, [this](const QVideoFrame& frame) {
            if (m_open && frame.isValid())
                emit frameCaptured(frame);
        });
        connect(&m_screen, &QScreenCapture::errorOccurred, this,
                [this](QScreenCapture::Error error, const QString& text) {
                    if (error != QScreenCapture::NoError)
                        fail(text.isEmpty() ? tr("Capture de l'écran impossible.") : text);
                });

        m_recording.setVideoFrameInput(&m_input);
        m_recording.setRecorder(&m_recorder);
        QMediaFormat format(QMediaFormat::MPEG4);
        format.setVideoCodec(QMediaFormat::VideoCodec::H264);
        m_recorder.setMediaFormat(format);
        m_recorder.setQuality(QMediaRecorder::HighQuality);
        connect(&m_recorder, &QMediaRecorder::recorderStateChanged, this,
                [this](QMediaRecorder::RecorderState state) {
                    if (state != QMediaRecorder::StoppedState || !m_open)
                        return;
                    m_open = false;
                    const QUrl actual = m_recorder.actualLocation();
                    emit stopped(actual.isLocalFile() ? actual.toLocalFile() : m_path);
                });
        connect(&m_recorder, &QMediaRecorder::errorOccurred, this,
                [this](QMediaRecorder::Error error, const QString& text) {
                    if (error != QMediaRecorder::NoError)
                        fail(text.isEmpty() ? tr("Enregistrement de l'écran impossible.") : text);
                });
    }
    ~ScreenEngine() override { m_screen.setActive(false); }

    void start(const QString& path) override
    {
        m_path = path;
        m_open = true;
        m_recorder.setOutputLocation(QUrl::fromLocalFile(path));
        m_recorder.record(); // rend la main ; l'encodeur prend la taille de la première image reçue
        m_screen.setActive(true);
    }
    void encode(const QVideoFrame& frame) override
    {
        if (m_open)
            m_input.sendVideoFrame(frame); // file pleine : l'image est perdue, jamais attendue
    }
    void stop() override
    {
        m_screen.setActive(false);
        m_recorder.stop(); // rend la main : stopped() quand le fichier est fermé
    }
    QMediaFormat mediaFormat() const override { return m_recorder.mediaFormat(); }

private:
    void fail(const QString& reason)
    {
        if (!m_open)
            return;
        m_open = false;
        m_screen.setActive(false);
        emit failed(reason);
    }

    QMediaCaptureSession m_capture;
    QScreenCapture m_screen;
    QVideoSink m_sink;
    QMediaCaptureSession m_recording;
    QVideoFrameInput m_input;
    QMediaRecorder m_recorder;
    QString m_path;
    bool m_open = false;
};

} // namespace

std::unique_ptr<DisplayEngine> DisplayRecorder::createScreenEngine()
{
    return std::make_unique<ScreenEngine>();
}

QString DisplayRecorder::unavailableReason()
{
    return {};
}

} // namespace actcraft::media
