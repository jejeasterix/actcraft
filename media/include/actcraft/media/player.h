#pragma once
#include <QObject>
#include <QString>

#include <array>
#include <functional>

class QAudioOutput;
class QMediaPlayer;

namespace actcraft::media {

// Lecteur intégré (F-552) : deux pistes simultanées ; un son posé sur une piste remplace celui qu'elle
// jouait. La lecture passe par Qt Multimedia, qui décode et joue sur ses propres fils ; le lecteur vit sur le
// fil qui l'a créé (le fil principal, jamais celui du rendu) et y émet tous ses signaux et rappels. Rien
// n'attend le chargement : un format illisible se découvre par signal, puis est confié au rappel de repli
// posé par ui/ (LinkLauncher::SystemMedia, qui demande confirmation, N2-9). media/ n'ouvre jamais rien
// lui-même et n'écrit aucun fichier.
class Player : public QObject
{
    Q_OBJECT
public:
    enum Track
    {
        First = 0,
        Second = 1
    };
    explicit Player(QObject* parent = nullptr);
    ~Player() override;

    void setFallback(std::function<void(const QString& path)> fallback); // ui/ : LinkLauncher::SystemMedia
    // Rend la main tout de suite ; aucun signal n'est émis avant ce retour. Seul un format illisible est
    // confié au système (fellBack) ; chemin réseau, fichier absent ou illisible faute de droits, autre erreur
    // de lecture : failed, sans repli.
    void play(const QString& path, Track track, bool loop);
    void pause(Track track);
    void stopAll(); // fermeture du document : les deux pistes s'arrêtent, aucun repli en attente ne part
    qint64 position(Track track) const;
    bool isPlaying(Track track) const;

signals:
    void positionChanged(int track, qint64 position);
    void finished(int track);
    void fellBack(const QString& path);
    void failed(int track, const QString& reason);

private:
    struct Channel
    {
        QMediaPlayer* player = nullptr;
        QAudioOutput* output = nullptr;
        QString path;           // fichier en cours ; vide : rien (arrêté, absent ou déjà confié au système)
        bool loaded = false;    // chargé par Qt Multimedia : une erreur ultérieure ne bascule plus
        quint64 generation = 0; // change à chaque play() et stopAll() : un signal différé périmé se tait
    };
    void settle(int track, int error); // fin de chargement en échec : repli ou erreur signalée
    void fallBack(int track);
    void fail(int track, const QString& reason);
    void failLater(int track, const QString& reason);

    std::array<Channel, 2> m_channels;
    std::function<void(const QString& path)> m_fallback;
};

} // namespace actcraft::media
