#pragma once
#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>

#include <memory>
#include <vector>

namespace actcraft::core {

inline constexpr double kPasteOffset = 20.0;     // D1, unités de page
inline constexpr double kDuplicateOffset = 10.0; // D2

// Objets à copier, fermés : un groupe emporte ses membres (récursivement), dans l'ordre de la page ; un
// connecteur garde le lien vers un objet emporté, perd l'autre (objectA ou objectB remis à 0).
std::vector<PageObject> closureForCopy(const Page& page, const std::vector<ObjectId>& ids);
// Copies prêtes à coller : nom vidé, déverrouillées, translatées de (dx, dy), ordonnées membres avant
// groupes, connecteurs en dernier ; identifiants remplacés par des rangs provisoires 1..n (références
// remappées).
std::vector<PageObject> preparePaste(std::vector<PageObject> objects, double dx, double dy);
// Coller sur l'objet (F-540) : centre de l'ensemble sur le centre de `target`, couche de `target`.
std::vector<PageObject> preparePasteOnto(std::vector<PageObject> objects, const PageObject& target);
// Même chose, `target` lu sur `page` : l'emprise d'un groupe est celle de ses membres (L5-C17, M6).
std::vector<PageObject> preparePasteOnto(std::vector<PageObject> objects, const Page& page,
                                         const PageObject& target);

// Ajout de copies préparées en une étape : un ReplaceObjectsCommand (A6) puis, au redo, références de groupe
// et de connecteur passées des rangs provisoires aux identifiants tirés (addedIds(), même ordre). Aucun
// Change de plus.
class PasteObjectsCommand final : public UndoCommand
{
public:
    PasteObjectsCommand(std::size_t page, std::vector<PageObject> prepared);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;
    const std::vector<ObjectId>& addedIds() const noexcept;

private:
    std::unique_ptr<UndoCommand> m_inner; // ReplaceObjectsCommand
    std::size_t m_page;
    std::vector<ObjectId> m_ids;
};

std::unique_ptr<UndoCommand> makeCutCommand(const Page& page, std::size_t pageIndex,
                                            const std::vector<ObjectId>& ids);
std::unique_ptr<PasteObjectsCommand> makeDuplicateCommand(const Page& page, std::size_t pageIndex,
                                                          const std::vector<ObjectId>& ids);

} // namespace actcraft::core
