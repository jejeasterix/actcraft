#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Permissions que le système garde sur les URL content:// rendues par son sélecteur (Android :
// ContentResolver). Ailleurs, aucune permission : prendre et rendre sont sans effet, une URL content:// est
// tenue. Remplacée dans les tests par une factice.
class PermissionBackend
{
public:
    virtual ~PermissionBackend() = default;
    virtual bool take(const QUrl& url) = 0;        // lecture et écriture gardées au-delà de la session
    virtual bool holds(const QUrl& url) const = 0; // permission encore valide (le fournisseur peut se taire)
    virtual bool reachable(const QUrl& url) const = 0; // le fournisseur rend une ligne pour le document
    virtual QString
    displayName(const QUrl& url) const = 0; // nom donné par le fournisseur ; vide s'il se tait
    virtual void release(const QUrl& url) = 0;
};

// Portail de documents (D11-9) : un document content:// ouvert ou enregistré par le sélecteur du système
// garde sa permission (`keep`) ; il se rouvre tant qu'elle vaut (`canOpen`) ; `forget` la rend. Une URL
// content:// vaut par sa permission, jamais par un chemin tiré de l'URL ; un fichier local, par son
// existence, sans permission. Insérer une image et importer un PDF passent par la même garde (`admit`).
class DocumentPortal : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit DocumentPortal(QObject* parent = nullptr);

    // Celui de la plateforme si nul. Non possédé (tests).
    void setBackend(PermissionBackend* backend) { m_backend = backend; }

    static bool isContent(const QUrl& url);
    // Repli du nom : nom du fichier local, ou dernier segment décodé du chemin d'une URL content:// (comme
    // documentio.cpp `targetName`) ; « 13 » pour `…downloads.documents/document/13`.
    static QString fallbackName(const QUrl& url);
    // Nom affiché : celui du fournisseur (OpenableColumns.DISPLAY_NAME sous Android), sinon `fallbackName` ;
    // jamais l'URL.
    QString displayName(const QUrl& url) const;
    // content:// : permission encore valide ; ailleurs : vrai.
    bool permitted(const QUrl& url) const;
    // content:// : le fournisseur répond ; fichier local : il existe ; autre URL : faux.
    bool reachable(const QUrl& url) const;

    // content:// : permission de lecture et d'écriture gardée ; ailleurs : vrai, sans effet.
    Q_INVOKABLE bool keep(const QUrl& url);
    // `permitted` et `reachable`.
    Q_INVOKABLE bool canOpen(const QUrl& url) const;
    // content:// : permission rendue ; ailleurs : sans effet.
    Q_INVOKABLE void forget(const QUrl& url);
    // Garde d'une URL lue une fois (image insérée, PDF importé) : vrai si elle s'ouvre ; sinon `refused`
    // avec le nom affiché, et faux. Aucune permission gardée.
    Q_INVOKABLE bool admit(const QUrl& url);

signals:
    void refused(const QString& name);

private:
    PermissionBackend& backend() const;

    PermissionBackend* m_backend = nullptr;
};

} // namespace actcraft::ui
