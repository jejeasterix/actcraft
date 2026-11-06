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

public:
    enum Form
    {
        TextForm,   // texte portant le nom du fichier ou l'adresse
        IconForm,   // image d'espace réservé
        ActionForm, // objet d'action (rectangle légendé)
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

    // Vrai : lien créé en une étape (created émis). Faux : refused émis avec le motif, document intact.
    Q_INVOKABLE bool createFileLink(const QUrl& file, int form, int storage, quint64 existing = 0);
    Q_INVOKABLE bool createWebsiteLink(const QString& address, int form, quint64 existing = 0);

signals:
    void created(quint64 objectId);
    void refused(const QString& reason);
    void sourcesChanged();
    void argumentsChanged();

private:
    bool refuse(const QString& reason);
    bool locked(const char* command) const;
    bool commit(const QString& label, const QString& catalogId, const QString& parameters, int form,
                quint64 existing, const QByteArray& stored, const QString& extension);

    QPointer<Commands> m_commands;
    QPointer<DocumentController> m_document;
    QPointer<ToolHost> m_tools;
    QPointer<LinkLauncher> m_links;
    QString m_arguments;
};

} // namespace actcraft::ui
