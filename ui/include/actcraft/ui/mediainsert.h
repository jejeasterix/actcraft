#pragma once
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>

#include <QImage>
#include <QObject>
#include <QPointF>
#include <QPointer>
#include <QQmlEngine>
#include <QString>
#include <QUrl>

#include <functional>
#include <optional>

namespace actcraft::ui {

// Insertion d'un son ou d'une vidéo (F-552, S25, lot 8) : un fichier ou une ressource du document devient un
// `core::Media` à image d'espace réservé, portant l'action « jouer un son ». Aucune lecture ici (le lecteur
// est dans media/). Une insertion = une étape d'historique. Le décodage de la première image d'une vidéo se
// fait sur le pool de fils, jamais sur le fil principal.
class MediaInsert : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(Commands* commands READ commands WRITE setCommands NOTIFY commandsChanged)
    Q_PROPERTY(DocumentController* document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)
public:
    // Première image d'une vidéo, appelée sur un fil du pool avec le chemin du fichier local ; image nulle :
    // espace réservé par défaut (16:9).
    using FrameProbe = std::function<QImage(const QString& path)>;

    explicit MediaInsert(QObject* parent = nullptr);

    Commands* commands() const;
    void setCommands(Commands* commands);
    DocumentController* document() const;
    void setDocument(DocumentController* document);
    bool isBusy() const;
    void setFrameProbe(FrameProbe probe);

    // Fichier local ou content:// ; refusé sous le verrou de classe (« insert.media » est verrouillable).
    Q_INVOKABLE void insertFile(const QUrl& url);
    // Même insertion, centrée sur `center` (coordonnées de page) : fichier déposé sur la page.
    void insertFileAt(const QUrl& url, QPointF center);
    // Ressource déjà dans le document (glissée depuis la bibliothèque).
    Q_INVOKABLE void insertResource(const QString& key);

signals:
    void commandsChanged();
    void documentChanged();
    void busyChanged();
    void inserted(int count);
    void failed(QString message);

private:
    bool allowed();
    void startFile(const QUrl& url, std::optional<QPointF> center);
    QPointer<Commands> m_commands;
    QPointer<DocumentController> m_document;
    FrameProbe m_probe;
    int m_pending = 0;
};

} // namespace actcraft::ui
