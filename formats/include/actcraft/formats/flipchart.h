#pragma once

#include <actcraft/core/document.h>

#include <QByteArray>
#include <QString>
#include <QStringList>

#include <vector>

namespace actcraft::formats::flipchart {

struct FlipchartReadResult
{
    bool ok = false;
    QString error;        // si !ok
    QStringList warnings; // pages, objets ou ressources illisibles, types non lus ; à afficher sans bloquer
    core::Document document;
    std::vector<QByteArray> thumbnails; // miniature JPEG d'origine de chaque page lue, vide si absente
};

// Lit un paperboard .flipchart en lecture seule (DA4, décision 18). Ordre de dessin : couche, puis
// profondeur, puis ordre de l'arbre (format-flipchart.md § 7.3). Échoue seulement si le conteneur, l'index
// racine ou toutes les pages sont illisibles.
FlipchartReadResult readFlipchart(const QByteArray& data);
// Signature du conteneur (« Bamboo ») : de quoi choisir le lecteur sans lire tout le fichier.
bool looksLikeFlipchart(const QByteArray& head) noexcept;

} // namespace actcraft::formats::flipchart
