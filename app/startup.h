#pragma once

#include <QStringList>
#include <QUrl>

class QQmlApplicationEngine;
class QQuickWindow;

// Chargement commun de l'interface, pour l'application et le banc (DA1).
struct StartupOptions
{
    bool cacheEnabled = true;
    // Banc et tests : la page 1 est la page générée de 300 objets (DA7) ; sinon, page vierge.
    bool generatedPage = false;
    // Fichier à ouvrir au lancement, vide sinon.
    QUrl openUrl;
};

// Lit `--sans-cache` (décision 8) et `--ouvrir <fichier>`.
StartupOptions parseStartupOptions(const QStringList& args);

// `--ouvrir <f>`, sinon premier argument qui n'est ni une option ni la valeur d'une option ; vide sinon.
QString openFileArgument(const QStringList& args);

// Charge ActCraft.Ui/Main avec ces options ; retourne la fenêtre principale, nullptr en cas d'échec.
QQuickWindow* loadMainWindow(QQmlApplicationEngine& engine, const StartupOptions& options);
