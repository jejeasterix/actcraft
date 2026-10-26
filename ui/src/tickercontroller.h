#pragma once

#include <actcraft/features/ticker.h>

#include <QColor>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

#include <functional>

namespace actcraft::ui {

// Message défilant (F-291) : réglages (features::TickerSettings) et état du défilement offerts à QML. Le
// contrôleur ne déplace rien et n'a aucune minuterie : la surcouche anime le texte par un XAnimator, sur le
// fil de rendu, et signale la fin d'un passage unique (finished). Aucun lien au document, aucune demande de
// redessin du cache. Réglages par défaut lus dans QSettings (tools/ticker) ; keepAsDefault() les y garde.
// Un texte ou une police dont le bloc gardé dépasserait la taille maximale d'un fichier de message est refusé
// à la saisie : les autres réglages restent relisibles. Fichiers de messages : openUrl, saveToUrl ; le choix
// du fichier passe par chooseOpen / chooseSave, remplaçables par setFileChooser (tests : aucune vraie boîte).
class TickerController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY settingsChanged)
    Q_PROPERTY(
        QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY settingsChanged) // vide : interface
    Q_PROPERTY(qreal fontPointSize READ fontPointSize WRITE setFontPointSize NOTIFY settingsChanged)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY settingsChanged)
    Q_PROPERTY(
        int shadowOffset READ shadowOffset WRITE setShadowOffset NOTIFY settingsChanged) // 0 : sans ombre
    Q_PROPERTY(QColor shadowColor READ shadowColor WRITE setShadowColor NOTIFY settingsChanged)
    // Vrai : fond transparent sur l'écran ; faux : backgroundColor.
    Q_PROPERTY(bool screenBackground READ screenBackground WRITE setScreenBackground NOTIFY settingsChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY settingsChanged)
    Q_PROPERTY(int position READ position WRITE setPosition NOTIFY settingsChanged) // Position
    Q_PROPERTY(bool loop READ loop WRITE setLoop NOTIFY settingsChanged)
    Q_PROPERTY(int speed READ speed WRITE setSpeed NOTIFY settingsChanged) // 1 à 10
    Q_PROPERTY(qreal pixelsPerSecond READ pixelsPerSecond NOTIFY settingsChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(bool paused READ paused NOTIFY pausedChanged)

public:
    enum Position
    {
        Top = 0,
        Center = 1,
        Bottom = 2
    };
    Q_ENUM(Position)

    explicit TickerController(QObject* parent = nullptr);

    QString text() const;
    void setText(const QString& text);
    QString fontFamily() const;
    void setFontFamily(const QString& family);
    qreal fontPointSize() const noexcept;
    void setFontPointSize(qreal size);
    QColor textColor() const noexcept;
    void setTextColor(const QColor& color);
    int shadowOffset() const noexcept;
    void setShadowOffset(int offset);
    QColor shadowColor() const noexcept;
    void setShadowColor(const QColor& color);
    bool screenBackground() const noexcept;
    void setScreenBackground(bool screen);
    QColor backgroundColor() const noexcept;
    void setBackgroundColor(const QColor& color);
    int position() const noexcept;
    void setPosition(int position);
    bool loop() const noexcept;
    void setLoop(bool loop);
    int speed() const noexcept;
    void setSpeed(int speed);
    qreal pixelsPerSecond() const noexcept;
    bool running() const noexcept;
    bool paused() const noexcept;

    const features::TickerSettings& settings() const noexcept;

    // Choix d'un fichier de message : renvoie l'URL choisie (vide : abandon) ; save vrai pour enregistrer.
    // Sans choix posé, le contrôleur demande la boîte de fichiers de l'interface (fileDialogRequested).
    using FileChooser = std::function<QUrl(bool save)>;
    void setFileChooser(FileChooser chooser);

    // Démarre un passage depuis le bord droit (relance s'il défile déjà) ; reprend s'il était en pause.
    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void togglePause(); // sans effet à l'arrêt
    // La surcouche : un passage s'est terminé. Arrête le défilement s'il ne boucle pas.
    Q_INVOKABLE void finished();
    // Durée d'un passage complet (features::TickerRun) : de la largeur de la bande à -textWidth ; 1 ms au
    // moins.
    Q_INVOKABLE int durationMs(qreal screenWidth, qreal textWidth) const;
    // Abscisse du texte elapsedMs après le départ d'un passage complet à speedPx, en boucle ou non au départ
    // (features::TickerRun) ; lue à la pause seulement, jamais à chaque image.
    Q_INVOKABLE qreal positionAt(qreal screenWidth, qreal textWidth, qreal speedPx, bool loop,
                                 double elapsedMs) const;
    // Garde les réglages courants comme défaut (QSettings, tools/ticker).
    Q_INVOKABLE void keepAsDefault() const;
    // Lit un fichier de message local (64 Kio au plus, format actcraft-ticker/1) et le prend comme réglages
    // courants, sans le garder comme défaut ; faux (réglages inchangés, fileFinished) s'il est illisible.
    Q_INVOKABLE bool openUrl(const QUrl& url);
    // Écrit les réglages courants dans un fichier de message local.
    Q_INVOKABLE bool saveToUrl(const QUrl& url);
    Q_INVOKABLE void chooseOpen();
    Q_INVOKABLE void chooseSave();

signals:
    void settingsChanged();
    void runningChanged();
    void pausedChanged();
    // Un fichier de message a été lu ou écrit (ok), ou refusé (message : la raison, en clair).
    void fileFinished(bool ok, const QString& message);
    // Aucun choix de fichier posé : la boîte de fichiers de l'interface doit s'ouvrir.
    void fileDialogRequested(bool save);

private:
    void setPaused(bool paused);
    void adopt(const features::TickerSettings& read);
    void askFile(bool save);
    bool failed(const QString& message);

    FileChooser m_chooser;
    features::TickerSettings m_settings;
    bool m_running = false;
    bool m_paused = false;
};

} // namespace actcraft::ui
