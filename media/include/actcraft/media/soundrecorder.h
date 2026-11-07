#pragma once

#include <QFile>
#include <QMediaFormat>
#include <QObject>
#include <QString>
#include <QTimer>

#include <functional>
#include <memory>

namespace actcraft::media {

// Moteur d'un enregistrement de son. En production : session de capture, entrée audio et enregistreur de
// Qt Multimedia, au format WAV (createCaptureEngine) ; dans les tests : une source simulée. Contrat de Qt
// Multimedia : start(), pause(), resume() et stop() rendent la main sur-le-champ ; le fichier n'est fermé et
// complet qu'à l'émission de stopped(), plus tard, sur le fil de l'enregistreur.
class RecordingEngine : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;
    ~RecordingEngine() override = default;

    // Fichier imposé : jamais l'emplacement par défaut du moteur (dossier Musique de l'utilisateur).
    virtual void start(const QString& path) = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    virtual void stop() = 0;
    // Durée enregistrée en millisecondes, pauses exclues.
    virtual qint64 duration() const = 0;
    // Format du fichier écrit (vide pour une source simulée).
    virtual QMediaFormat mediaFormat() const { return {}; }

    // Délai de garde d'un abandon : un moteur qui ne ferme jamais son fichier est libéré après lui.
    void setAbandonTimeout(int milliseconds) { m_abandonTimeoutMs = milliseconds; }

    // Fin confiée au moteur (document fermé, panne), règle de DisplayEngine::abandon : effacer le fichier
    // avant sa fermeture échoue (fichier ouvert sous Windows) ou le manque (écrit après coup). Le moteur
    // demande l'arrêt, attend stopped() (ou le délai de garde), efface alors le fichier partiel, puis se
    // libère par deleteLater. À appeler une fois, par l'enregistreur seulement, après l'avoir déconnecté ;
    // path est l'emplacement demandé, effacé si le moteur n'a rien dit à l'expiration du délai.
    void abandon(const QString& path)
    {
        if (m_abandoned)
            return;
        m_abandoned = true;
        setParent(nullptr); // plus rien ne le détruit à la place du moteur
        connect(this, &RecordingEngine::stopped, this, [this](const QString& closed) { discard(closed); });
        auto* guard = new QTimer(this);
        guard->setSingleShot(true);
        connect(guard, &QTimer::timeout, this, [this, path] { discard(path); });
        guard->start(m_abandonTimeoutMs);
        stop();
    }

signals:
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

// F-293 : enregistreur de son. Premier appui : l'enregistrement commence (permission du micro demandée à la
// première utilisation) ; appuis suivants : pause, puis reprise. À l'arrêt, le fichier WAV, resté dans le
// dossier fixé par setFolder (dossier temporaire du document), est confié une seule fois au rappel
// d'insertion posé par ui/, qui en fait une ressource et pose l'icône de lecture sur la page en une étape :
// media/ ne touche jamais au document. Rien n'est attendu sur le fil principal.
class SoundRecorder : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool recording READ isRecording NOTIFY recordingChanged)
    Q_PROPERTY(bool paused READ isPaused NOTIFY pausedChanged)
    Q_PROPERTY(bool available READ isAvailable NOTIFY availabilityChanged)
    Q_PROPERTY(QString message READ message NOTIFY availabilityChanged)

public:
    using EngineFactory = std::function<std::unique_ptr<RecordingEngine>()>;
    // Demande la permission du micro ; la réponse peut arriver plus tard (fenêtre du système).
    using PermissionRequest = std::function<void(std::function<void(bool granted)> answer)>;

    // Moteur de Qt Multimedia et permission du système.
    explicit SoundRecorder(QObject* parent = nullptr);
    SoundRecorder(EngineFactory engines, PermissionRequest permission, QObject* parent = nullptr);
    ~SoundRecorder() override;

    static std::unique_ptr<RecordingEngine> createCaptureEngine();
    static void requestMicrophonePermission(std::function<void(bool granted)> answer);

    void setFolder(const QString& folder);
    QString folder() const;
    void setInsertHandler(std::function<void(const QString& path)> handler);

    // Enregistrer, puis pause, puis reprise.
    void toggle();
    // Arrêt : le rappel d'insertion part quand le moteur a fermé le fichier.
    void stop();
    // Abandon : le fichier est effacé quand le moteur l'a fermé ; aucun rappel.
    void cancel();

    // Vrai de l'appui jusqu'à la fermeture du fichier, pause comprise : l'indicateur suit.
    bool isRecording() const;
    bool isPaused() const;
    // Faux après un refus de la permission : la commande se désactive, message() dit pourquoi.
    bool isAvailable() const;
    QString message() const;
    qint64 duration() const;

signals:
    void recordingChanged();
    void pausedChanged();
    void availabilityChanged();
    void finished(const QString& path);
    void failed(const QString& reason);

private:
    enum class State
    {
        Idle,
        Asking,
        Recording,
        Paused,
        Finishing
    };

    void answered(quint64 generation, bool granted);
    void begin();
    void engineStopped(const QString& path);
    void engineFailed(const QString& reason);
    void retireEngine(bool removeFile);
    void end();
    void setPaused(bool paused);
    QString nextPath() const;

    EngineFactory m_engines;
    PermissionRequest m_permission;
    std::function<void(const QString& path)> m_insert;
    std::unique_ptr<RecordingEngine> m_engine;
    QString m_folder;
    QString m_path;
    QString m_message;
    State m_state = State::Idle;
    quint64 m_generation = 0;
    bool m_granted = false;
    bool m_available = true;
    bool m_cancelled = false;
    bool m_paused = false;
};

} // namespace actcraft::media
