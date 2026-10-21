#pragma once

#include <actcraft/features/ticker.h>

#include <QColor>
#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Message défilant (F-291) : réglages (features::TickerSettings) et état du défilement offerts à QML. Le
// contrôleur ne déplace rien et n'a aucune minuterie : la surcouche anime le texte par un XAnimator, sur le
// fil de rendu, et signale la fin d'un passage unique (finished). Aucun lien au document, aucune demande de
// redessin du cache. Réglages par défaut lus dans QSettings (tools/ticker) ; keepAsDefault() les y garde.
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

    // Démarre un passage depuis le bord droit (relance s'il défile déjà) ; reprend s'il était en pause.
    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void togglePause(); // sans effet à l'arrêt
    // La surcouche : un passage s'est terminé. Arrête le défilement s'il ne boucle pas.
    Q_INVOKABLE void finished();
    // Durée d'un passage complet (features::TickerRun) : de la largeur de la bande à -textWidth ; 1 ms au
    // moins.
    Q_INVOKABLE int durationMs(qreal screenWidth, qreal textWidth) const;
    // Garde les réglages courants comme défaut (QSettings, tools/ticker).
    Q_INVOKABLE void keepAsDefault() const;

signals:
    void settingsChanged();
    void runningChanged();
    void pausedChanged();

private:
    void setPaused(bool paused);

    features::TickerSettings m_settings;
    bool m_running = false;
    bool m_paused = false;
};

} // namespace actcraft::ui
