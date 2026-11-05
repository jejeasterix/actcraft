#pragma once

#include <QString>
#include <QTextDocument>
#include <QUrl>
#include <QVariant>

#include <functional>
#include <memory>

namespace actcraft::ui {

// Bloquant 5 : tout HTML venu du fichier ouvert (texte, notes de page) se lit dans le document isolé de
// ui/src/textediting.cpp (AnalysisDocument), qui ne charge aucune ressource. Interne à la bibliothèque ui.

// Observateur (tests) : chaque ressource demandée et la réponse vide donnée.
using ResourceObserver = std::function<void(const QUrl&, const QVariant&)>;

// HTML assaini : rien n'est chargé ; images hors `data:`, ancres et images de fond retirées ; texte et mise
// en forme gardés.
QString safeTextHtml(const QString& html, const ResourceObserver& observer);

// Document isolé (marge nulle) : toute ressource reçoit une valeur vide ; images ni lues ni dessinées.
std::unique_ptr<QTextDocument> isolatedTextDocument(ResourceObserver observer);

} // namespace actcraft::ui
