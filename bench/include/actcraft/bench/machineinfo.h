#pragma once

#include <QString>

namespace actcraft::bench {

// Métadonnées du passage (décision 27). Jamais le nom d'hôte ni le nom d'utilisateur.
struct MachineInfo
{
    QString model; // modèle matériel (Windows : fabricant + produit du BIOS ; macOS : hw.model)
    QString cpu;
    QString os;
    QString kernel;
    QString qtVersion;
    QString commit;
    bool commitDirty = false;
    QString buildType;
    QString compilerFlags;
};

MachineInfo readMachineInfo();

// Lettres ASCII et chiffres ; toute autre suite de caractères devient « - » ; vide → « inconnu ».
QString sanitizeForFileName(const QString& text);

} // namespace actcraft::bench
