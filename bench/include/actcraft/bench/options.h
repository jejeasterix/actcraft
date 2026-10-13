#pragma once

#include <actcraft/bench/scenarios.h>

#include <QSize>
#include <QString>
#include <QStringList>

namespace actcraft::bench {

// Options du banc (décision 21). --rendu est validé par parseRenderBackend (app/renderbackend.cpp).
struct BenchOptions
{
    QString requestedBackend; // valeur de --rendu ; défaut : d3d11 sous Windows, auto ailleurs
    ScenarioSet scenarioSet = ScenarioSet::Complet;
    QString output;           // --sortie : fichier « .json », sinon dossier ; vide = dossier par défaut
    bool cacheEnabled = true; // --sans-cache
    bool fullScreen = true;   // --taille plein-ecran (défaut) | LxH
    QSize windowSize;
    QString capturesDir; // --captures : PNG écrits seulement si non vide
};

bool parseBenchOptions(const QStringList& args, BenchOptions& out, QString& error);
// `output` se terminant par .json : ce fichier ; sinon `output` (ou `defaultDir`) + fileName.
QString resolveOutputPath(const BenchOptions& options, const QString& defaultDir, const QString& fileName);

} // namespace actcraft::bench
