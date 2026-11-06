#include <actcraft/ui/recorders.h>

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QGuiApplication>
#include <QPointer>
#include <QScreen>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>

#include <algorithm>
#include <cmath>

namespace actcraft::ui {

namespace {

constexpr qint64 kDurationToleranceMs = 500;

void putLe(QByteArray& out, quint32 value, int bytes)
{
    for (int i = 0; i < bytes; ++i)
        out.append(static_cast<char>((value >> (8 * i)) & 0xFF));
}

// Moteur de son simulé (plateforme « offscreen » : aucun micro, aucune fenêtre de permission) : un WAV
// silencieux de la durée enregistrée, pauses exclues, écrit à la fermeture.
class SilentEngine final : public media::RecordingEngine
{
public:
    void start(const QString& path) override
    {
        m_path = path;
        m_clock.start();
    }
    void pause() override
    {
        m_recorded += m_clock.elapsed();
        m_running = false;
    }
    void resume() override
    {
        m_clock.restart();
        m_running = true;
    }
    void stop() override
    {
        if (m_running)
            m_recorded += m_clock.elapsed();
        m_running = false;
        const QString path = m_path;
        const quint32 samples = static_cast<quint32>(m_recorded * 8); // 8 kHz
        QByteArray wav("RIFF");
        putLe(wav, 36 + samples * 2, 4);
        wav += "WAVEfmt ";
        putLe(wav, 16, 4);
        putLe(wav, 1, 2);
        putLe(wav, 1, 2);
        putLe(wav, 8000, 4);
        putLe(wav, 16000, 4);
        putLe(wav, 2, 2);
        putLe(wav, 16, 2);
        wav += "data";
        putLe(wav, samples * 2, 4);
        wav += QByteArray(static_cast<qsizetype>(samples) * 2, '\0');
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(wav) != wav.size()) {
            QTimer::singleShot(0, this, [this] { emit failed(tr("Fichier d'enregistrement illisible.")); });
            return;
        }
        file.close();
        QTimer::singleShot(0, this, [this, path] { emit stopped(path); });
    }
    qint64 duration() const override { return m_recorded + (m_running ? m_clock.elapsed() : 0); }

private:
    QString m_path;
    QElapsedTimer m_clock;
    qint64 m_recorded = 0;
    bool m_running = true;
};

bool simulatedPlatform()
{
    return QGuiApplication::platformName() == QLatin1String("offscreen");
}

media::SoundRecorder* makeSoundRecorder(QObject* parent)
{
    if (!simulatedPlatform())
        return new media::SoundRecorder(parent);
    return new media::SoundRecorder([] { return std::unique_ptr<media::RecordingEngine>(new SilentEngine); },
                                    [](std::function<void(bool)> answer) { answer(true); }, parent);
}

} // namespace

Recorders::Recorders(QObject* parent)
    : QObject(parent), m_sound(makeSoundRecorder(this)), m_display(new media::DisplayRecorder(this))
{
    wire();
}

void Recorders::wire()
{
    if (m_sound) {
        connect(m_sound, &media::SoundRecorder::recordingChanged, this, &Recorders::onRecordingChanged);
        connect(m_sound, &media::SoundRecorder::pausedChanged, this, &Recorders::onPausedChanged);
        connect(m_sound, &media::SoundRecorder::failed, this, &Recorders::setMessage);
        connect(m_sound, &media::SoundRecorder::availabilityChanged, this,
                [this] { setMessage(m_sound->message()); });
        m_sound->setInsertHandler([this](const QString& path) { insert(path); });
    }
    if (m_display) {
        connect(m_display, &media::DisplayRecorder::recordingChanged, this, &Recorders::onRecordingChanged);
        connect(m_display, &media::DisplayRecorder::pausedChanged, this, &Recorders::onPausedChanged);
        connect(m_display, &media::DisplayRecorder::failed, this, &Recorders::setMessage);
        m_display->setInsertHandler([this](const QString& path) { insert(path); });
    }
    applyFolders();
}

void Recorders::useRecorders(media::SoundRecorder* sound, media::DisplayRecorder* display)
{
    if (m_ownRecorders) {
        delete m_sound.data();
        delete m_display.data();
        m_ownRecorders = false;
    } else {
        if (m_sound)
            m_sound->disconnect(this);
        if (m_display)
            m_display->disconnect(this);
    }
    m_sound = sound;
    m_display = display;
    wire();
    emit stateChanged();
}

Commands* Recorders::commands() const
{
    return m_commands;
}

void Recorders::setCommands(Commands* commands)
{
    if (m_commands == commands)
        return;
    unbindCommands();
    m_commands = commands;
    bindCommands();
    emit commandsChanged();
}

DocumentController* Recorders::document() const
{
    return m_document;
}

void Recorders::setDocument(DocumentController* document)
{
    if (m_document == document)
        return;
    m_document = document;
    emit documentChanged();
}

MediaInsert* Recorders::mediaInsert() const
{
    return m_insert;
}

void Recorders::setMediaInsert(MediaInsert* insert)
{
    if (m_insert == insert)
        return;
    if (m_insert)
        m_insert->disconnect(this);
    m_insert = insert;
    if (m_insert)
        connect(m_insert, &MediaInsert::failed, this, &Recorders::setMessage);
    emit mediaInsertChanged();
}

Preferences* Recorders::preferences() const
{
    return m_preferences;
}

void Recorders::setPreferences(Preferences* preferences)
{
    if (m_preferences == preferences)
        return;
    m_preferences = preferences;
    applyFolders();
    emit preferencesChanged();
    emit stateChanged();
}

void Recorders::bindCommands()
{
    if (!m_commands)
        return;
    m_commands->setNativeHandler(QStringLiteral("record.sound"), [this](const QVariant&) { toggleSound(); });
    m_commands->setNativeHandler(QStringLiteral("record.pause"), [this](const QVariant&) { pause(); });
    m_commands->setNativeHandler(QStringLiteral("record.stop"), [this](const QVariant&) { stop(); });
    m_commands->setNativeHandler(QStringLiteral("record.cancel"), [this](const QVariant&) { cancel(); });
    m_commands->setNativeHandler(QStringLiteral("record.screen-full"),
                                 [this](const QVariant&) { toggleScreen(false); });
    m_commands->setNativeHandler(QStringLiteral("record.screen-area"), [this](const QVariant& argument) {
        const QVariantMap area = argument.toMap();
        if (area.contains(QStringLiteral("width"))) {
            chooseScreenArea(
                area.value(QStringLiteral("x")).toReal(), area.value(QStringLiteral("y")).toReal(),
                area.value(QStringLiteral("width")).toReal(), area.value(QStringLiteral("height")).toReal(),
                area.value(QStringLiteral("ratio"), 1.0).toReal());
        }
        toggleScreen(true);
    });
}

void Recorders::unbindCommands()
{
    if (!m_commands)
        return;
    for (const char* id : {"record.sound", "record.pause", "record.stop", "record.cancel",
                           "record.screen-full", "record.screen-area"}) {
        m_commands->setNativeHandler(QString::fromLatin1(id), {});
    }
}

media::SoundRecorder* Recorders::activeSound() const
{
    return m_kind == QLatin1String("sound") ? m_sound.data() : nullptr;
}

bool Recorders::isRecording() const
{
    return (m_sound && m_sound->isRecording()) || (m_display && m_display->isRecording());
}

bool Recorders::isPaused() const
{
    return (m_sound && m_sound->isRecording() && m_sound->isPaused()) ||
           (m_display && m_display->isRecording() && m_display->isPaused());
}

QString Recorders::kind() const
{
    if (m_sound && m_sound->isRecording())
        return QStringLiteral("sound");
    if (m_display && m_display->isRecording())
        return QStringLiteral("screen");
    return {};
}

bool Recorders::indicatorVisible() const
{
    return isRecording();
}

bool Recorders::paletteVisible() const
{
    return m_paletteVisible;
}

bool Recorders::noticeVisible() const
{
    return m_noticeVisible;
}

QString Recorders::notice() const
{
    return tr("Ce qui est enregistré : le son du micro ou l'image de l'écran, pendant que l'indicateur est "
              "affiché. Le fichier est rangé dans le dossier des enregistrements (réglages), puis ajouté au "
              "paperboard. Rien n'est envoyé.");
}

QString Recorders::message() const
{
    return m_message;
}

bool Recorders::screenAvailable() const
{
    return m_display && m_display->isAvailable();
}

QString Recorders::screenUnavailableReason() const
{
    return m_display ? m_display->message() : QString();
}

qint64 Recorders::lastDurationMs() const
{
    return m_lastDurationMs;
}

bool Recorders::lastDurationOk() const
{
    return m_lastDurationOk;
}

QString Recorders::folder() const
{
    return m_preferences ? m_preferences->value(QString::fromLatin1(kFolderKey)).toString() : QString();
}

QString Recorders::soundFormat() const
{
    return m_preferences
               ? m_preferences->value(QString::fromLatin1(kSoundFormatKey), QStringLiteral("wav")).toString()
               : QStringLiteral("wav");
}

QString Recorders::screenFormat() const
{
    return m_preferences
               ? m_preferences->value(QString::fromLatin1(kScreenFormatKey), QStringLiteral("mp4")).toString()
               : QStringLiteral("mp4");
}

QRect Recorders::pixelArea(const QRectF& logical, qreal pixelRatio, const QPointF& screenOrigin)
{
    if (!(pixelRatio > 0) || logical.isEmpty())
        return {};
    const QRectF local = logical.translated(-screenOrigin);
    const int left = static_cast<int>(std::floor(local.left() * pixelRatio));
    const int top = static_cast<int>(std::floor(local.top() * pixelRatio));
    const int right = static_cast<int>(std::ceil(local.right() * pixelRatio));
    const int bottom = static_cast<int>(std::ceil(local.bottom() * pixelRatio));
    return QRect(QPoint(left, top), QPoint(right - 1, bottom - 1));
}

void Recorders::applyFolders()
{
    const QString chosen = folder();
    if (chosen.isEmpty())
        return;
    if (m_sound)
        m_sound->setFolder(chosen);
    if (m_display)
        m_display->setFolder(chosen);
}

void Recorders::showNoticeOnce()
{
    const QString key = QString::fromLatin1(kNoticeShownKey);
    const bool shown = m_preferences ? m_preferences->value(key, false).toBool() : m_noticeShownInSession;
    if (shown)
        return;
    m_noticeShownInSession = true;
    if (m_preferences)
        m_preferences->setValue(key, true);
    m_noticeVisible = true;
    emit noticeChanged();
}

void Recorders::dismissNotice()
{
    if (!m_noticeVisible)
        return;
    m_noticeVisible = false;
    emit noticeChanged();
}

void Recorders::begin(const char* kindName)
{
    const bool wasIdle = !isRecording();
    applyFolders();
    setMessage({});
    if (wasIdle) {
        showNoticeOnce();
        m_wall.start();
        m_pausedMs = 0;
        m_kind = QString::fromLatin1(kindName);
    }
    // V43 : relancer un enregistrement fait réapparaître une palette masquée.
    showPalette();
}

void Recorders::toggleSound()
{
    if (!m_sound || (isRecording() && !m_sound->isRecording()))
        return; // un seul enregistrement à la fois
    if (!isRecording())
        begin("sound");
    m_sound->toggle();
}

void Recorders::toggleScreen(bool area)
{
    Q_UNUSED(area)
    if (!m_display || !m_display->isAvailable()) {
        setMessage(m_display ? m_display->message() : tr("Enregistrement de l'écran indisponible."));
        return;
    }
    if (isRecording() && !m_display->isRecording())
        return;
    if (!isRecording()) {
        if (area && m_display->area().isEmpty()) {
            // Sans zone choisie : la moitié centrale de l'écran principal.
            if (QScreen* screen = QGuiApplication::primaryScreen()) {
                const QRectF geometry = screen->geometry();
                chooseScreenArea(geometry.width() / 4, geometry.height() / 4, geometry.width() / 2,
                                 geometry.height() / 2, screen->devicePixelRatio(), geometry.x(),
                                 geometry.y());
            }
        } else if (!area) {
            m_display->setArea({});
        }
        begin("screen");
    }
    m_display->toggle();
}

void Recorders::chooseScreenArea(qreal x, qreal y, qreal width, qreal height, qreal pixelRatio, qreal originX,
                                 qreal originY)
{
    if (m_display)
        m_display->setArea(pixelArea(QRectF(x, y, width, height), pixelRatio, QPointF(originX, originY)));
}

void Recorders::pause()
{
    if (media::SoundRecorder* sound = activeSound())
        sound->toggle();
    else if (m_display && m_display->isRecording())
        m_display->toggle();
}

void Recorders::stop()
{
    if (!isRecording())
        return;
    const bool paused = isPaused();
    if (m_sound && m_sound->isRecording()) {
        m_recordedAtStop = m_sound->duration();
        m_sound->stop();
    } else if (m_display && m_display->isRecording()) {
        m_recordedAtStop = m_display->duration();
        m_display->stop();
    }
    Q_UNUSED(paused)
}

void Recorders::cancel()
{
    if (m_sound && m_sound->isRecording())
        m_sound->cancel();
    else if (m_display && m_display->isRecording())
        m_display->cancel();
}

void Recorders::hidePalette()
{
    if (!m_paletteVisible)
        return;
    m_paletteVisible = false;
    emit paletteVisibleChanged();
}

void Recorders::showPalette()
{
    if (m_paletteVisible)
        return;
    m_paletteVisible = true;
    emit paletteVisibleChanged();
}

void Recorders::insert(const QString& path)
{
    // La durée enregistrée est comparée au temps écoulé, pauses exclues, avant toute insertion.
    const qint64 wall = m_wall.isValid() ? m_wall.elapsed() - m_pausedMs : 0;
    m_lastDurationMs = m_recordedAtStop;
    m_lastDurationOk = m_recordedAtStop > 0 && std::llabs(m_recordedAtStop - wall) <= kDurationToleranceMs;
    emit lastDurationChanged();
    if (m_insert)
        m_insert->insertFile(QUrl::fromLocalFile(path));
    else
        setMessage(tr("Enregistrement rangé : %1").arg(path));
}

void Recorders::setMessage(const QString& message)
{
    if (m_message == message)
        return;
    m_message = message;
    emit messageChanged();
}

void Recorders::onPausedChanged()
{
    if (isPaused())
        m_pauseClock.start();
    else if (m_pauseClock.isValid()) {
        m_pausedMs += m_pauseClock.elapsed();
        m_pauseClock.invalidate();
    }
    emit stateChanged();
}

void Recorders::onRecordingChanged()
{
    const bool now = isRecording();
    if (!now)
        m_kind.clear();
    m_wasRecording = now;
    emit stateChanged();
}

} // namespace actcraft::ui
