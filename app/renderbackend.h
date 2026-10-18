#pragma once

#include <QString>
#include <QStringList>

// Choix du moteur de rendu, avant la création de la première fenêtre Qt Quick.
// Défaut : Direct3D 11 sous Windows (vieux pilotes des puces intégrées), défaut de Qt ailleurs.
// « logiciel » est le secours obligatoire : pilote absent, bureau à distance, machine virtuelle.
enum class RenderBackend
{
    Auto,
    Direct3D11,
    Software,
};

// L10-T5 : échantillonnage multiple en RHI (lissage des traits, constat L2-C11). Même valeur, à changer
// ensemble : `samples` du cache (ui/qml/PageArea.qml) et l'attente `pageCache.samples` de
// bench/scenarios/anticrenelage.json.
inline constexpr int kMsaaSamples = 4;

// Lit `--rendu <auto|d3d11|logiciel>` dans les arguments ; retourne false si la valeur est inconnue.
bool parseRenderBackend(const QStringList& args, RenderBackend& out, QString& error);

// Applique le choix ; doit être appelé avant toute QQuickWindow.
void applyRenderBackend(RenderBackend backend);
