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
    // Dossier des profils de l'utilisateur ; vide : <AppConfigLocation>/profiles. Joueur, banc et tests : un
    // dossier d'essai propre au lancement (L6-T5).
    QString profileRoot;
    // Accueil ouvert au démarrage (si la case de l'accueil le permet) : seul app/main.cpp le met à vrai ; le
    // joueur de captures, le banc et les tests le laissent faux (L6-T23).
    bool showDashboard = false;
    // Dossier de la copie de reprise (L11-T13) ; vide : <AppDataLocation>/reprise. Joueur et tests : un
    // dossier d'essai, jamais le vrai dossier de reprise.
    QString recoveryDir;
    // `--journal-empreinte` (essai sur l'émulateur) : « ACTCRAFT EMPREINTE <h> » au journal à chaque
    // changement du document.
    bool logFingerprint = false;
    // `--essai-cadre-web <adresse ou fichier>` (essai de l'image disque, S4) : un cadre posé au démarrage.
    QString webFrameProbe;
};

// Lit `--sans-cache` (décision 8), `--ouvrir <fichier>`, `--journal-empreinte` et `--essai-cadre-web <adresse
// ou fichier>`.
StartupOptions parseStartupOptions(const QStringList& args);

// `--ouvrir <f>`, sinon premier argument qui n'est ni une option ni la valeur d'une option ; vide sinon.
QString openFileArgument(const QStringList& args);

// Charge ActCraft.Ui/Main avec ces options ; retourne la fenêtre principale, nullptr en cas d'échec.
QQuickWindow* loadMainWindow(QQmlApplicationEngine& engine, const StartupOptions& options);
