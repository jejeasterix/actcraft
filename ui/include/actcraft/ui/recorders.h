#pragma once
#include <actcraft/media/displayrecorder.h>
#include <actcraft/media/soundrecorder.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/mediainsert.h>
#include <actcraft/ui/preferences.h>

#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QRectF>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Enregistreurs de son (F-293) et d'écran (F-292) côté interface : relie les commandes `record.*` aux deux
// enregistreurs de media/, tient l'état que montrent la palette et l'indicateur, la mention de première
// utilisation et les réglages de la section « Enregistrements ». Aucun enregistrement ne part sans une
// commande de l'utilisateur ; l'indicateur est visible de l'appui jusqu'à la fermeture du fichier.
class Recorders : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(Commands* commands READ commands WRITE setCommands NOTIFY commandsChanged)
    Q_PROPERTY(DocumentController* document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(MediaInsert* mediaInsert READ mediaInsert WRITE setMediaInsert NOTIFY mediaInsertChanged)
    Q_PROPERTY(Preferences* preferences READ preferences WRITE setPreferences NOTIFY preferencesChanged)
    Q_PROPERTY(bool recording READ isRecording NOTIFY stateChanged)
    Q_PROPERTY(bool paused READ isPaused NOTIFY stateChanged)
    Q_PROPERTY(QString kind READ kind NOTIFY stateChanged) // « sound », « screen » ou vide
    Q_PROPERTY(bool indicatorVisible READ indicatorVisible NOTIFY stateChanged)
    Q_PROPERTY(bool separateIndicator READ separateIndicator NOTIFY stateChanged)
    Q_PROPERTY(bool selectingArea READ isSelectingArea NOTIFY selectingAreaChanged)
    Q_PROPERTY(bool paletteVisible READ paletteVisible NOTIFY paletteVisibleChanged)
    Q_PROPERTY(bool noticeVisible READ noticeVisible NOTIFY noticeChanged)
    Q_PROPERTY(QString notice READ notice CONSTANT)
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)
    Q_PROPERTY(bool screenAvailable READ screenAvailable CONSTANT)
    Q_PROPERTY(QString screenUnavailableReason READ screenUnavailableReason CONSTANT)
    Q_PROPERTY(qint64 lastDurationMs READ lastDurationMs NOTIFY lastDurationChanged)
    Q_PROPERTY(bool lastDurationOk READ lastDurationOk NOTIFY lastDurationChanged)
    Q_PROPERTY(QString folder READ folder NOTIFY stateChanged)
    Q_PROPERTY(QString soundFormat READ soundFormat NOTIFY stateChanged)
    Q_PROPERTY(QString screenFormat READ screenFormat NOTIFY stateChanged)
public:
    static constexpr const char* kFolderKey = "recordings/folder";
    static constexpr const char* kSoundFormatKey = "recordings/sound-format";
    static constexpr const char* kScreenFormatKey = "recordings/screen-format";
    static constexpr const char* kNoticeShownKey = "recordings/notice-shown";

    explicit Recorders(QObject* parent = nullptr);

    Commands* commands() const;
    void setCommands(Commands* commands);
    DocumentController* document() const;
    void setDocument(DocumentController* document);
    MediaInsert* mediaInsert() const;
    void setMediaInsert(MediaInsert* insert);
    Preferences* preferences() const;
    void setPreferences(Preferences* preferences);

    // Tests : remplace les enregistreurs par ceux-ci (non possédés).
    void useRecorders(media::SoundRecorder* sound, media::DisplayRecorder* display);

    bool isRecording() const;
    bool isPaused() const;
    QString kind() const;
    bool indicatorVisible() const;
    // Vrai pendant un enregistrement d'écran : l'indicateur vit alors dans une fenêtre à part.
    bool separateIndicator() const;
    // Vrai entre la commande « zone de l'écran » et le tracé (ou l'abandon) de la zone.
    bool isSelectingArea() const;
    bool paletteVisible() const;
    bool noticeVisible() const;
    QString notice() const;
    QString message() const;
    bool screenAvailable() const;
    QString screenUnavailableReason() const;
    qint64 lastDurationMs() const;
    bool lastDurationOk() const;
    // Réglages de la section « Enregistrements » (vide : dossier temporaire du document).
    QString folder() const;
    QString soundFormat() const;
    QString screenFormat() const;

    // Zone d'écran en pixels de l'image capturée : coordonnées logiques de l'écran (QML), origine de l'écran
    // principal soustraite, rapport de pixels appliqué, arrondie vers l'extérieur.
    static QRect pixelArea(const QRectF& logical, qreal pixelRatio, const QPointF& screenOrigin = {});

    Q_INVOKABLE void toggleSound();
    Q_INVOKABLE void toggleScreen(bool area);
    Q_INVOKABLE void chooseScreenArea(qreal x, qreal y, qreal width, qreal height, qreal pixelRatio,
                                      qreal originX = 0, qreal originY = 0);
    // Fin du tracé : la zone (coordonnées logiques de l'écran) est convertie, puis l'enregistrement part.
    Q_INVOKABLE void finishAreaSelection(qreal x, qreal y, qreal width, qreal height, qreal pixelRatio,
                                         qreal originX = 0, qreal originY = 0);
    // Échap pendant le tracé : rien ne part.
    Q_INVOKABLE void cancelAreaSelection();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void hidePalette();
    Q_INVOKABLE void showPalette();
    Q_INVOKABLE void dismissNotice();

signals:
    void commandsChanged();
    void documentChanged();
    void mediaInsertChanged();
    void preferencesChanged();
    void stateChanged();
    // L'interface doit proposer le tracé d'une zone (calque plein écran) ; réponse : finishAreaSelection
    // ou cancelAreaSelection.
    void areaSelectionRequested();
    void selectingAreaChanged();
    void paletteVisibleChanged();
    void noticeChanged();
    void messageChanged();
    void lastDurationChanged();

private:
    void wire();
    void bindCommands();
    void unbindCommands();
    void begin(const char* kind);
    void insert(const QString& path);
    void setMessage(const QString& message);
    void onRecordingChanged();
    void onPausedChanged();
    void applyFolders();
    void showNoticeOnce();
    media::SoundRecorder* activeSound() const;

    QPointer<Commands> m_commands;
    QPointer<DocumentController> m_document;
    QPointer<MediaInsert> m_insert;
    QPointer<Preferences> m_preferences;
    QPointer<media::SoundRecorder> m_sound;
    QPointer<media::DisplayRecorder> m_display;
    bool m_ownRecorders = true;
    QString m_kind;
    QString m_message;
    bool m_paletteVisible = true;
    bool m_noticeVisible = false;
    bool m_noticeShownInSession = false;
    bool m_wasRecording = false;
    qint64 m_lastDurationMs = 0;
    bool m_lastDurationOk = false;
    qint64 m_recordedAtStop = 0;
    QElapsedTimer m_wall;
    QElapsedTimer m_pauseClock;
    qint64 m_pausedMs = 0;
};

} // namespace actcraft::ui
