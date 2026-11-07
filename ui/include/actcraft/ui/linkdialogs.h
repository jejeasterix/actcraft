#pragma once

#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/linklauncher.h>
#include <actcraft/ui/toolhost.h>

#include <QObject>
#include <QPointer>
#include <QString>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

#include <functional>
#include <string>

namespace actcraft::ui {

// Création des liens vers un fichier ou un site (F-550, F-551), en une seule étape d'annulation : la boîte
// LinkDialog.qml appelle createFileLink ou createWebsiteLink. Un lien est une action d'objet du catalogue
// (`document.open-file`, `document.open-website`) portée par un texte, une icône, un objet d'action neufs ou
// par un objet existant de la page. Création et ouverture suivent les mêmes règles : la cible passe par
// LinkLauncher::refusalFor, avec le même motif, avant toute écriture dans le document. Fermé par défaut :
// sans lanceur, sans document ou sous le verrou de classe, rien n'est créé. Rien ne s'ouvre ici (jamais
// d'ouverture, de QProcess ni d'argument de ligne de commande passé) ; l'ouverture est celle de ActionRunner
// puis de LinkLauncher, après confirmation.
class LinkDialogs : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(actcraft::ui::Commands* commands READ commands WRITE setCommands NOTIFY sourcesChanged)
    Q_PROPERTY(
        actcraft::ui::DocumentController* document READ document WRITE setDocument NOTIFY sourcesChanged)
    Q_PROPERTY(actcraft::ui::ToolHost* toolHost READ toolHost WRITE setToolHost NOTIFY sourcesChanged)
    Q_PROPERTY(actcraft::ui::LinkLauncher* links READ links WRITE setLinks NOTIFY sourcesChanged)
    // Paramètres de ligne de commande de la référence : gardés dans les paramètres de l'action d'un lien
    // vers un fichier, jamais passés à l'ouverture.
    Q_PROPERTY(QString arguments READ arguments WRITE setArguments NOTIFY argumentsChanged)
    // Vrai tant qu'un fichier à ranger dans le document est lu et haché hors du fil de l'interface.
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    enum Form
    {
        TextForm,    // texte portant le nom du fichier ou l'adresse
        IconForm,    // image d'espace réservé
        ActionForm,  // objet d'action (rectangle légendé)
        ExistingForm // objet existant de la page (identifiant donné, sinon l'unique objet sélectionné)
    };
    Q_ENUM(Form)
    enum Storage
    {
        ExternalStorage, // chemin gardé tel quel
        DocumentStorage  // octets rangés dans le document, à leur SHA-256
    };
    Q_ENUM(Storage)

    explicit LinkDialogs(QObject* parent = nullptr);
    ~LinkDialogs() override;

    Commands* commands() const noexcept { return m_commands.data(); }
    void setCommands(Commands* commands);
    DocumentController* document() const noexcept { return m_document.data(); }
    void setDocument(DocumentController* document);
    ToolHost* toolHost() const noexcept { return m_tools.data(); }
    void setToolHost(ToolHost* tools);
    LinkLauncher* links() const noexcept { return m_links.data(); }
    void setLinks(LinkLauncher* links);
    QString arguments() const { return m_arguments; }
    void setArguments(const QString& arguments);
    bool busy() const noexcept { return m_busy; }

    // Sondes de test, sur le modèle de LinkLauncher::setOpener. La sonde de lecture est appelée sur le fil
    // qui lit et hache le fichier rangé ; le compteur donne le nombre de rangements de ressource faits par
    // les commandes de lien (un par étape créée, jamais un par rejeu).
    static void setReadProbe(std::function<void()> probe);
    static int resourceInsertions() noexcept;

    // Vrai : lien créé en une étape (created émis), ou, pour un fichier rangé dans le document, lecture
    // acceptée : le fichier est lu et haché hors du fil de l'interface, puis created (ou refused) arrive au
    // retour, et rien n'est posé si le document ou l'objet visé a disparu entre-temps (busy vrai entre les
    // deux). Faux : refused émis avec le motif, document intact.
    // `path` : chemin local en texte, tel que le champ de la boîte le garde ; `#`, `?` et `%` y sont des
    // caractères du nom, jamais de la syntaxe d'URL. Un texte à schéma d'au moins deux lettres (file:, http:)
    // reste une adresse, jugée par LinkLauncher::refusalFor.
    Q_INVOKABLE bool createFileLink(const QString& path, int form, int storage, quint64 existing = 0);
    // Chemin local d'une URL de fichier (rendu du sélecteur « Parcourir… »), décodé ; une URL qui n'est pas
    // un fichier local est rendue telle quelle.
    Q_INVOKABLE QString localPath(const QUrl& url) const;
    Q_INVOKABLE bool createWebsiteLink(const QString& address, int form, quint64 existing = 0);

signals:
    void created(quint64 objectId);
    void refused(const QString& reason);
    void sourcesChanged();
    void argumentsChanged();
    void busyChanged();

private:
    bool refuse(const QString& reason);
    bool locked(const char* command) const;
    void setBusy(bool busy);
    void storeFile(const QString& path, const QString& name, const QString& parameters, int form,
                   quint64 existing);
    bool commit(const QString& label, const QString& catalogId, const QString& parameters, int form,
                quint64 existing, const QByteArray& stored, const QString& extension,
                std::string storedKey = {});

    QPointer<Commands> m_commands;
    QPointer<DocumentController> m_document;
    QPointer<ToolHost> m_tools;
    QPointer<LinkLauncher> m_links;
    QString m_arguments;
    bool m_busy = false;
};

} // namespace actcraft::ui
