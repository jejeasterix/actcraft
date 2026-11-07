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
    QString
        output; // --sortie : fichier « .json » ou dossier (toute autre extension est refusée) ; vide = défaut
    bool cacheEnabled = true; // --sans-cache
    bool fullScreen = true;   // --taille plein-ecran (défaut) | LxH
    QSize windowSize;
    QString capturesDir; // --captures : PNG écrits seulement si non vide
    // --retenir-decodage (L6-C11, test de fumée seulement) : la sonde de flash retient le décodage de la page
    // suivante jusqu'à sa capture, pour être exercée même quand il finit avant l'image (release hors écran).
    bool holdDecodeForTests = false;
};

// Plateforme du dossier des résultats (L11-T15) : fonction pure, testable sans Android.
enum class Platform
{
    Desktop,
    Android
};
// Android : `base` = dossier propre externe de l'application (lisible par `adb pull` sans `run-as`),
// résultats dans
// `<base>/bench-results`. Ailleurs : `base` = dossier de l'exécutable, résultats dans
// `<base>/resultats-banc`.
QString resultsDirFor(Platform platform, const QString& base);
// Android passe les options en une seule chaîne (`applicationArguments`) : « prog "--a 1 --b 2" » devient
// « prog --a 1 --b 2 ». Des arguments déjà séparés sont rendus tels quels.
QStringList expandSingleStringArguments(const QStringList& args);

bool parseBenchOptions(const QStringList& args, BenchOptions& out, QString& error);
// `output` se terminant par .json : ce fichier ; sinon `output` (ou `defaultDir`) + fileName.
QString resolveOutputPath(const BenchOptions& options, const QString& defaultDir, const QString& fileName);

} // namespace actcraft::bench
