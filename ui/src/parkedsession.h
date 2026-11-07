#pragma once

#include <actcraft/core/document.h>
#include <actcraft/core/undo.h>
#include <actcraft/ui/documentcontroller.h>

#include <QString>
#include <QStringList>
#include <QUrl>

#include <cstddef>
#include <memory>

namespace actcraft::ui {

// Document garé par un onglet (L5-T20). Tout ce qui appartient au document part avec lui (déplacé, jamais
// copié) ; ce qui appartient au contrôleur (page fixe, observateurs, relais d'écriture) reste. Un soulèvement
// est déposé avant le garage : il n'y en a jamais ici.
struct DocumentController::ParkedSession
{
    core::Document document;
    core::UndoStack undo;
    std::size_t current = 0; // page affichée
    QUrl fileUrl;
    QString title; // vide : « Sans titre » à la reprise
    bool modifiedOutsideStack = false;
    std::uint64_t documentKey = 0; // 0 : session neuve, la reprise lui en donne une
    QStringList warnings;
    // Document tel qu'ouvert ou enregistré en dernier (page de référence de « Rétablir la page ») ; nullptr :
    // aucune.
    std::shared_ptr<const core::Document> reference;

    bool isModified() const noexcept { return modifiedOutsideStack || !undo.isClean(); }
};

} // namespace actcraft::ui
