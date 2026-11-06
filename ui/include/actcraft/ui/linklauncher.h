#pragma once

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

#include <functional>

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

    void setOpener(Opener opener); // tests ; défaut : QDesktopServices::openUrl
    int openedCount() const { return m_openedCount; }

    // Valide la cible ; émet confirmationNeeded (cible montrée, jeton) ou refused (motif). N'ouvre jamais.
    Q_INVOKABLE void request(const QString& target, int kind);
    // Fichier rangé dans le document : octets, nom d'origine, clé du document. Extrait sous un nom assaini
    // dans un dossier temporaire propre au document, puis soumis à la confirmation. N'ouvre jamais.
    void requestStored(const QByteArray& bytes, const QString& originalName, const QString& documentKey);
    Q_INVOKABLE void answer(int token, bool accepted); // seul point qui ouvre ; un jeton ne sert qu'une fois
    static QString refusalFor(const QUrl& url, int kind); // vide : acceptée

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

    Opener m_opener;
    QHash<int, Pending> m_pending;
    int m_nextToken = 1;
    int m_openedCount = 0;
};

} // namespace actcraft::ui
