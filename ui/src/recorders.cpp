#include <actcraft/ui/recorders.h>

#include <QDir>
#include <QPointer>
#include <QStandardPaths>
#include <QUrl>

#include <algorithm>
#include <array>
#include <cmath>

namespace actcraft::ui {

namespace {

constexpr qint64 kDurationToleranceMs = 500;
// Côté minimal, en pixels logiques, d'une zone tracée : en dessous, c'est un clic, pas une zone.
constexpr qreal kMinimumAreaSide = 8;
constexpr qreal kIndicatorMargin = 12;

Recorders::Engines& injectedEngines()
{
    static Recorders::Engines engines;
    return engines;
}

media::SoundRecorder* makeSoundRecorder(QObject* parent)
{
    const Recorders::Engines& engines = injectedEngines();
    if (!engines.sound)
        return new media::SoundRecorder(parent);
    return new media::SoundRecorder(
        engines.sound,
        engines.permission
            ? engines.permission
            : media::SoundRecorder::PermissionRequest(&media::SoundRecorder::requestMicrophonePermission),
        parent);
}

media::DisplayRecorder* makeDisplayRecorder(QObject* parent)
{
    const Recorders::Engines& engines = injectedEngines();
    if (!engines.display)
        return new media::DisplayRecorder(parent);
    return new media::DisplayRecorder(engines.display, parent);
}

} // namespace

void Recorders::setDefaultEngines(Engines engines)
{
    injectedEngines() = std::move(engines);
}

void Recorders::resetDefaultEngines()
{
    injectedEngines() = {};
}

Recorders::Recorders(QObject* parent)
    : QObject(parent), m_sound(makeSoundRecorder(this)), m_display(makeDisplayRecorder(this))
{
    wire();
}

void Recorders::wire()
{
    if (m_sound) {
        connect(m_sound, &media::SoundRecorder::recordingChanged, this, &Recorders::onRecordingChanged);
        connect(m_sound, &media::SoundRecorder::pausedChanged, this, &Recorders::onPausedChanged);
        connect(m_sound, &media::SoundRecorder::failed, this, &Recorders::setMessage);
        // Permission du micro refusée : la commande se désactive (S26), et le message dit pourquoi.
        connect(m_sound, &media::SoundRecorder::availabilityChanged, this, [this] {
            setMessage(m_sound->message());
            syncCommandState();
        });
        m_sound->setInsertHandler([this](const QString& path) { insert(path); });
    }
    if (m_display) {
        connect(m_display, &media::DisplayRecorder::recordingChanged, this, &Recorders::onRecordingChanged);
        connect(m_display, &media::DisplayRecorder::pausedChanged, this, &Recorders::onPausedChanged);
        connect(m_display, &media::DisplayRecorder::failed, this, &Recorders::setMessage);
        m_display->setInsertHandler([this](const QString& path) { insert(path); });
    }
    m_defaultSoundFolder = m_sound ? m_sound->folder() : QString();
    m_defaultDisplayFolder = m_display ? m_display->folder() : QString();
    applyFolders();
    syncCommandState();
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
        if (!area.contains(QStringLiteral("width"))) {
            toggleScreen(true); // sans zone donnée : l'utilisateur la trace
            return;
        }
        // Zone donnée par l'appelant (script, essai) : pas de tracé.
        chooseScreenArea(area.value(QStringLiteral("x")).toReal(), area.value(QStringLiteral("y")).toReal(),
                         area.value(QStringLiteral("width")).toReal(),
                         area.value(QStringLiteral("height")).toReal(),
                         area.value(QStringLiteral("ratio"), 1.0).toReal());
        if (isRecording())
            toggleScreen(true); // pause, reprise
        else if (screenReady())
            startScreen();
    });
    syncCommandState();
}

void Recorders::unbindCommands()
{
    if (!m_commands)
        return;
    for (const char* id : {"record.sound", "record.pause", "record.stop", "record.cancel",
                           "record.screen-full", "record.screen-area"}) {
        m_commands->setNativeHandler(QString::fromLatin1(id), {});
    }
    m_commands->setEnabled(QStringLiteral("record.sound"), true);
}

// La commande « enregistrer le son » suit l'enregistreur : désactivée tant que la permission du micro est
// refusée, réactivée si l'enregistreur redevient disponible.
void Recorders::syncCommandState()
{
    if (m_commands)
        m_commands->setEnabled(QStringLiteral("record.sound"), !m_sound || m_sound->isAvailable());
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

bool Recorders::separateIndicator() const
{
    return m_display && m_display->isRecording();
}

bool Recorders::isSelectingArea() const
{
    return m_selectingArea;
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

QPointF Recorders::indicatorSpot(const QRectF& screen, const QSizeF& size) const
{
    const std::array<QPointF, 4> corners = {
        QPointF(screen.right() - size.width() - kIndicatorMargin, screen.top() + kIndicatorMargin),
        QPointF(screen.left() + kIndicatorMargin, screen.top() + kIndicatorMargin),
        QPointF(screen.right() - size.width() - kIndicatorMargin,
                screen.bottom() - size.height() - kIndicatorMargin),
        QPointF(screen.left() + kIndicatorMargin, screen.bottom() - size.height() - kIndicatorMargin)};
    for (const QPointF& corner : corners) {
        if (!m_drawnArea.intersects(QRectF(corner, size)))
            return corner;
    }
    return corners[0];
}

void Recorders::applyFolders()
{
    // Réglage vidé : chaque enregistreur retrouve son dossier par défaut, pas le dernier dossier choisi.
    const QString chosen = folder();
    if (m_sound)
        m_sound->setFolder(chosen.isEmpty() ? m_defaultSoundFolder : chosen);
    if (m_display)
        m_display->setFolder(chosen.isEmpty() ? m_defaultDisplayFolder : chosen);
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

void Recorders::begin()
{
    const bool wasIdle = !isRecording();
    applyFolders();
    setMessage({});
    if (wasIdle) {
        showNoticeOnce();
        m_wall.start();
        m_pausedMs = 0;
    }
    // V43 : relancer un enregistrement fait réapparaître une palette masquée.
    showPalette();
}

void Recorders::toggleSound()
{
    if (!m_sound || (isRecording() && !m_sound->isRecording()))
        return; // un seul enregistrement à la fois
    if (!isRecording())
        begin();
    m_sound->toggle();
}

// Faux, avec le motif en message, quand l'écran n'est pas enregistrable (aucun moteur d'écran hors Windows).
bool Recorders::screenReady()
{
    if (m_display && m_display->isAvailable())
        return true;
    setMessage(m_display ? m_display->message() : tr("Enregistrement de l'écran indisponible."));
    return false;
}

void Recorders::startScreen()
{
    begin();
    m_display->toggle();
}

void Recorders::toggleScreen(bool area)
{
    if (!screenReady())
        return;
    if (isRecording()) {
        if (m_display->isRecording())
            m_display->toggle(); // pause, reprise
        return;                  // un seul enregistrement à la fois
    }
    if (area) {
        requestAreaSelection(); // F-292 « zone » : l'utilisateur la trace, aucune zone par défaut
        return;
    }
    m_drawnArea = {};
    m_display->setArea({});
    startScreen();
}

void Recorders::requestAreaSelection()
{
    if (m_selectingArea)
        return; // le tracé est déjà proposé
    setSelectingArea(true);
    emit areaSelectionRequested();
}

void Recorders::setSelectingArea(bool selecting)
{
    if (m_selectingArea == selecting)
        return;
    m_selectingArea = selecting;
    emit selectingAreaChanged();
}

void Recorders::finishAreaSelection(qreal x, qreal y, qreal width, qreal height, qreal pixelRatio,
                                    qreal originX, qreal originY)
{
    if (!m_selectingArea)
        return; // aucun tracé demandé : une réponse perdue ne lance rien
    setSelectingArea(false);
    if (width < kMinimumAreaSide || height < kMinimumAreaSide) {
        // Une zone vide voudrait dire « plein écran » : un clic sans glisser ne lance rien.
        setMessage(tr("Zone trop petite : tracez un rectangle pour enregistrer une zone de l'écran."));
        return;
    }
    if (!screenReady() || isRecording())
        return;
    chooseScreenArea(x, y, width, height, pixelRatio, originX, originY);
    startScreen();
}

void Recorders::cancelAreaSelection()
{
    setSelectingArea(false);
}

void Recorders::chooseScreenArea(qreal x, qreal y, qreal width, qreal height, qreal pixelRatio, qreal originX,
                                 qreal originY)
{
    m_drawnArea = QRectF(x, y, width, height);
    if (m_display)
        m_display->setArea(pixelArea(m_drawnArea, pixelRatio, QPointF(originX, originY)));
}

void Recorders::pause()
{
    if (m_sound && m_sound->isRecording())
        m_sound->toggle();
    else if (m_display && m_display->isRecording())
        m_display->toggle();
}

void Recorders::stop()
{
    if (m_sound && m_sound->isRecording()) {
        m_recordedAtStop = m_sound->duration();
        m_sound->stop();
    } else if (m_display && m_display->isRecording()) {
        m_recordedAtStop = m_display->duration();
        m_display->stop();
    }
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
    emit stateChanged();
}

} // namespace actcraft::ui
