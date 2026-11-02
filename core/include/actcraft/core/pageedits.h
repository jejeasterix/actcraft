#pragma once

#include <actcraft/core/document.h>
#include <actcraft/core/undo.h>

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace actcraft::core {

// Ressources que citent `pages` (fond, image, vignette et son ou vidéo d'un média, fichier d'une action),
// copiées de `document` ; une clé vide ou inconnue du document est omise. Les octets sont partagés
// implicitement (QByteArray) : la copie ne duplique rien.
std::map<std::string, Resource> pageResources(const Document& document, const std::vector<Page>& pages);

// Copie de `page` dont chaque objet reçoit un identifiant neuf du document ; membres de groupes, extrémités
// de connecteurs et cibles d'actions qui visent un objet de la page suivent leur nouvel identifiant ; une
// référence hors de la page, ou à 0 (« aucun »), reste telle quelle. Identifiants épuisés (allocateId rend
// 0) : les objets restants gardent 0, et la copie n'est pas à insérer.
Page withFreshIds(Document& document, Page page);

// Indices des pages déplacées après MovePagesCommand(pages, before) (F-802 : elles restent sélectionnées).
std::vector<std::size_t> positionsAfterMove(std::size_t pageCount, const std::vector<std::size_t>& pages,
                                            std::size_t before);

// Réordonner (F-802) : `pages` (triés, uniques) retirées puis insérées dans leur ordre devant la page qui
// était à l'indice `before` (pageCount : à la fin). Changes : PageRemoved puis PageInserted par page. Sans
// effet si rien ne bouge.
class MovePagesCommand final : public UndoCommand
{
public:
    MovePagesCommand(std::vector<std::size_t> pages, std::size_t before);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::vector<std::size_t> m_pages;
    std::size_t m_before;
};

// Supprimer, couper (F-601) : refusée (aucun effet) si elle retirerait toutes les pages. Changes :
// PageRemoved.
class RemovePagesCommand final : public UndoCommand
{
public:
    explicit RemovePagesCommand(std::vector<std::size_t> pages);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::vector<std::size_t> m_pages;
    std::vector<Page> m_removed;
};

// Coller, dupliquer, insérer (F-600, F-601) : copies à identifiants neufs (withFreshIds au premier redo,
// gardés ensuite) insérées à `index`. Changes : PageInserted. Identifiants épuisés : rien n'est inséré, aucun
// changement rendu (comme AddObjectCommand). `resources` (pageResources des pages, pour un collage venu d'un
// autre document) : ajoutées au document à chaque redo qui insère ; une clé déjà présente garde la sienne.
// L'annulation ne les retire pas : comme après la suppression d'une page, une ressource que plus rien ne cite
// reste au document.
class InsertPagesCommand final : public UndoCommand
{
public:
    InsertPagesCommand(std::size_t index, std::vector<Page> pages,
                       std::map<std::string, Resource> resources = {});
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::size_t m_index;
    std::vector<Page> m_pages;
    std::map<std::string, Resource> m_resources;
    bool m_fresh = false;
};

} // namespace actcraft::core
