#pragma once

#include <actcraft/ui/documentcontroller.h>

#include <QDateTime>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

#include <functional>

namespace actcraft::ui {

// Commandes simples de la présentation : date et heure sur la page (F-300), navigateur web (F-299).
// La date et l'heure s'écrivent en texte sur la page affichée par DocumentInsertions::insertText, en une
// étape d'annulation ; l'heure est injectable (`nowOverride`, valide = utilisée). Le navigateur ouvre
// l'adresse d'accueil par le système ; seuls les schémas http et https sont ouverts, tout autre est refusé
// avec un message. L'ouvreur est injectable (setUrlOpener) : aucun vrai navigateur dans les tests.
class PresentationCommands : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(actcraft::ui::DocumentController* documentController READ documentController WRITE
                   setDocumentController NOTIFY documentControllerChanged)
    Q_PROPERTY(QDateTime nowOverride READ nowOverride WRITE setNowOverride NOTIFY nowOverrideChanged)
    // Dernier refus (vide si la dernière ouverture a réussi).
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)
    Q_PROPERTY(QString webHome READ webHome CONSTANT)

public:
    using UrlOpener = std::function<bool(const QUrl&)>;

    explicit PresentationCommands(QObject* parent = nullptr);

    DocumentController* documentController() const noexcept;
    void setDocumentController(DocumentController* controller);
    QDateTime nowOverride() const;
    void setNowOverride(const QDateTime& now);
    QString message() const;
    // Adresse d'accueil : réglage tools/webHome, sinon la page du projet.
    QString webHome() const;
    // Remplace l'ouvreur du système (test) ; un ouvreur vide rétablit QDesktopServices::openUrl.
    void setUrlOpener(UrlOpener opener);

    // Date et heure courantes, format long français, près du centre de la page affichée ; faux sans document.
    Q_INVOKABLE bool insertDateTime();
    // Ouvre l'adresse d'accueil ; faux si refusée (schéma) ou si l'ouverture échoue.
    Q_INVOKABLE bool openWebHome();

signals:
    void documentControllerChanged();
    void nowOverrideChanged();
    void messageChanged();

private:
    void setMessage(const QString& message);

    QPointer<DocumentController> m_document;
    QDateTime m_now;
    QString m_message;
    UrlOpener m_opener;
};

} // namespace actcraft::ui
