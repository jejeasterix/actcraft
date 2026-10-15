#pragma once

#include <QStringList>

class QQmlApplicationEngine;
class QQuickWindow;

// Chargement commun de l'interface, pour l'application et le banc (DA1).
struct StartupOptions
{
    bool cacheEnabled = true;
    // Banc et tests : la page 1 est la page générée de 300 objets (DA7) ; sinon, page vierge.
    bool generatedPage = false;
};

// Lit `--sans-cache` (décision 8).
StartupOptions parseStartupOptions(const QStringList& args);

// Charge ActCraft.Ui/Main avec ces options ; retourne la fenêtre principale, nullptr en cas d'échec.
QQuickWindow* loadMainWindow(QQmlApplicationEngine& engine, const StartupOptions& options);
