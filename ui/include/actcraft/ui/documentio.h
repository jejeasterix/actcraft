#pragma once

#include <actcraft/core/document.h>

#include <QString>
#include <QStringList>
#include <QUrl>

#include <cstdint>

namespace actcraft::ui {

// Lecture et écriture d'un document par son URL (décision 17), à appeler sur un fil secondaire : aucune
// lecture, aucun décodage, aucune compression, aucune écriture sur le fil principal (règle de fluidité 1).
struct LoadOutcome
{
    bool ok = false;
    QString error;
    QStringList warnings;
    core::Document document;
    bool native = false;        // .actcraft (sinon .flipchart : document sans fichier, décision 18)
    QString title;              // nom du fichier sans extension
    bool offMainThread = false; // relevé par la lecture elle-même (preuve de S5, point d'injection de test)
};

// La signature choisit le lecteur (« PK\3\4 » : archive .actcraft ; « Bamboo » : .flipchart), jamais
// l'extension ; sinon erreur.
LoadOutcome loadDocument(const QUrl& url);

struct SaveOutcome
{
    bool ok = false;
    QString error;
    std::uint64_t fingerprint = 0; // empreinte du document enregistré
    bool offMainThread = false;    // relevé par l'écriture elle-même
};

// Miniatures (peintre partagé, 320 px de large, images décodées réduites), archive .actcraft, écriture
// atomique (fichier local) ou en une passe (content://). `snapshot` est une copie à soi : jamais le document
// affiché.
SaveOutcome saveDocument(const core::Document& snapshot, const QUrl& url);
// Nom du fichier sans extension ; pour content://, dernier segment décodé.
QString titleFromUrl(const QUrl& url);
// F-657, N2-4 : Documents/ActCraft (créé s'il manque) ; sous Android, dossier de l'application. `documents`
// non vide remplace le dossier Documents du système (tests : jamais le profil de l'utilisateur).
QString defaultDocumentsFolder(const QString& documents = QString());

} // namespace actcraft::ui
