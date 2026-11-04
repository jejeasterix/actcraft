#pragma once

#include <actcraft/actions/profile.h>

#include <QString>
#include <QStringList>

#include <optional>

namespace actcraft::actions {

// Profils fournis (lecture seule) et de l'utilisateur (D6-1). Une copie utilisateur masque le profil fourni
// de même identifiant. Tout identifiant hors `^[a-z0-9-]+$` est refusé (aucune lecture, aucune écriture).
class ProfileStore
{
public:
    // `userRoot` : dossier des profils de l'utilisateur (créé à la première écriture) ;
    // `builtinRoot` : dossier des profils fournis (défaut : « :/actcraft/actions/profiles ») ;
    // `builtinOrder` : ordre d'affichage des profils fournis.
    ProfileStore(QString userRoot, QString builtinRoot, QStringList builtinOrder);

    QStringList ids() const; // fournis dans `builtinOrder`, puis ceux de l'utilisateur seulement, triés
    bool isBuiltin(const QString& id) const;
    bool hasUserCopy(const QString& id) const;
    std::optional<Profile> load(const QString& id, QString& error) const;
    // Dossier propre au profil (disposition éditée, F-910) : <userRoot>/<id>.
    QString directory(const QString& id) const;

    bool save(const Profile& profile, QString& error); // QSaveFile, écriture atomique
    bool saveAs(const Profile& profile, const QString& newId, const QString& label, QString& error);
    bool remove(const QString& id, QString& error); // refuse un profil fourni
    bool reset(const QString& id, QString& error);  // retire la copie utilisateur d'un profil fourni
    // Un seul fichier JSON : { "format": "actcraft-profile-export/1", "profile": {…}, "layout": {…} | null }.
    bool exportTo(const QString& id, const QString& filePath, QString& error) const;
    // Rend l'identifiant créé (jamais un identifiant existant : suffixe -2, -3…).
    std::optional<QString> importFrom(const QString& filePath, QString& error);

private:
    QString userFile(const QString& id) const;
    QString m_userRoot;
    QString m_builtinRoot;
    QStringList m_builtinOrder;
};

} // namespace actcraft::actions
