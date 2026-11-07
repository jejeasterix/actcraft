#pragma once

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

#include <functional>
#include <map>
#include <memory>

class QTemporaryDir;

class QThreadPool;

namespace actcraft::ui {

// Seul chemin vers QDesktopServices::openUrl pour le contenu d'un document (D8-4) : la cible est validée,
// montrée en entier, puis ouverte après « oui » seulement. Un refus n'ouvre rien ; une demande sans réponse
// n'ouvre rien. Fichiers : chemin local existant, jamais réseau, extension de la liste blanche fermée,
// comparée sans casse après retrait des points et espaces finaux du nom.
class LinkLauncher : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int openedCount READ openedCount NOTIFY opened) // nombre d'ouvertures (scénarios, L8-T19)

public:
    enum Kind
    {
        Website,
        File,
        StoredFile,
        SystemMedia
    };
    Q_ENUM(Kind)
    using Opener = std::function<bool(const QUrl&)>;

    explicit LinkLauncher(QObject* parent = nullptr);
    ~LinkLauncher() override; // retire les dossiers d'extraction

    void setOpener(Opener opener); // tests ; défaut : QDesktopServices::openUrl
    // Sonde de test, appelée sur le fil qui écrit le fichier extrait (jamais le fil de l'interface).
    static void setWriteProbe(std::function<void()> probe);
    int openedCount() const { return m_openedCount; }

    // Valide la cible ; émet confirmationNeeded (cible montrée, jeton) ou refused (motif). N'ouvre jamais.
    Q_INVOKABLE void request(const QString& target, int kind);
    // Fichier rangé dans le document : octets, nom d'origine, clé du document. Extrait sous un nom assaini
    // dans un dossier temporaire propre au document (nom imprévisible, 0700), marqué « venu d'ailleurs »,
    // puis soumis à la confirmation. N'ouvre jamais. L'écriture se fait hors du fil de l'interface, dans
    // l'ordre des demandes : confirmationNeeded ou refused arrive ensuite, et rien n'arrive si le lanceur a
    // disparu entre-temps.
    void requestStored(const QByteArray& bytes, const QString& originalName, const QString& documentKey);
    Q_INVOKABLE void answer(int token, bool accepted); // seul point qui ouvre ; un jeton ne sert qu'une fois
    static QString refusalFor(const QUrl& url, int kind); // vide : acceptée
    // Accès au disque faits par refusalFor depuis le démarrage (tests : un chemin réseau n'en fait aucun).
    static int diskQueries();

signals:
    void confirmationNeeded(const QString& shownTarget, int token);
    void refused(const QString& reason);
    void opened(const QUrl& url);

private:
    struct Pending
    {
        QUrl url;
        int kind = Website;
    };
    void ask(const QUrl& url, int kind);
    QString extractionDir(const QString& documentKey); // vide : création impossible

    Opener m_opener;
    QHash<int, Pending> m_pending;
    int m_nextToken = 1;
    int m_openedCount = 0;
    std::map<QString, std::unique_ptr<QTemporaryDir>> m_extractionDirs; // par clé de document
    // Un seul fil d'écriture (ordre des demandes) ; déclaré après les dossiers : détruit, donc terminé, avant
    // eux.
    std::unique_ptr<QThreadPool> m_writers;
};

} // namespace actcraft::ui
