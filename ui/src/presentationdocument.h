#pragma once

#include <actcraft/core/model.h>
#include <actcraft/ui/documentcontroller.h>

#include <QByteArray>
#include <QRectF>
#include <QString>

#include <cstddef>
#include <string>
#include <vector>

namespace actcraft::ui {

// Insertions du lot 7 (F-285, F-295, F-296, F-300, F-301, F-275) : par la pile du contrôleur (execute), donc
// par l'interface de cache figée. Hors de documentcontroller.* (socle, D-S8). Fil principal.
class DocumentInsertions
{
public:
    explicit DocumentInsertions(DocumentController& controller);
    core::ObjectId insertObject(core::PageObject object); // page affichée, une étape ; 0 si refusé
    core::ObjectId
    insertObjectOnNewPage(core::PageObject object); // page ajoutée en fin, affichée ; une étape
    core::ObjectId insertText(const QString& plainText,
                              const QRectF& bounds); // couche intermédiaire, une étape
    // Image d'une ressource neuve (caméra, capture du spot) : ressource ajoutée et objet inséré en une étape
    // (une commande locale dont redo() appelle core::Document::addResource, puis insère l'objet).
    core::ObjectId insertImage(QByteArray data, std::string extension, core::ImageObject image,
                               bool onNewPage);
    // core::ReplaceObjectsCommand, un groupe ; identifiants des ajouts.
    std::vector<core::ObjectId> replaceObjects(std::vector<core::ObjectId> removed,
                                               std::vector<core::PageObject> added);
    void setPagePresentation(core::PagePresentation value); // core::SetPagePresentationCommand, page affichée
    const core::Page* shownPage() const noexcept;
    std::size_t shownIndex() const noexcept;

private:
    DocumentController& m_controller;
};

} // namespace actcraft::ui
