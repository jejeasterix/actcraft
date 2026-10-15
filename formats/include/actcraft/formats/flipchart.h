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

// Bornes de lecture d'un fichier non fiable : au-delà, la lecture échoue proprement (jamais d'arrêt du
// processus). Une ressource partagée par mille objets est recopiée mille fois dans le modèle, et une entrée
// désignée par mille nœuds est décompressée mille fois : les deux volumes sont comptés.
struct FlipchartLimits
{
    qint64 maxDecompressedBytes = 2ll * 1024 * 1024 * 1024; // volume décompressé cumulé, relectures comprises
    qint64 maxModelBytes = 1024ll * 1024 * 1024; // points, textes et ressources recopiés dans le modèle
};

// Lit un paperboard .flipchart en lecture seule (DA4, décision 18). Ordre de dessin : couche, puis
// profondeur, puis ordre de l'arbre (format-flipchart.md § 7.3). Échoue si le conteneur, l'index racine ou
// toutes les pages sont illisibles, si une borne de `limits` est dépassée ou si la mémoire manque.
FlipchartReadResult readFlipchart(const QByteArray& data, const FlipchartLimits& limits = {});
// Signature du conteneur (« Bamboo ») : de quoi choisir le lecteur sans lire tout le fichier.
bool looksLikeFlipchart(const QByteArray& head) noexcept;

} // namespace actcraft::formats::flipchart
