#pragma once

#include <cstddef>
#include <limits>
#include <vector>

namespace actcraft::ui {

// Historique des pages vues (D8-12) : précédente et suivante dans l'ordre des visites, comme un navigateur.
// Pile bornée à 100 entrées (les plus anciennes tombent) ; une visite après un retour efface la suite ; une
// navigation par l'historique n'est pas réinscrite (la page atteinte est déjà l'entrée courante).
class PageHistory
{
public:
    static constexpr std::size_t kLimit = 100;

    void visited(int page); // ignorée si c'est déjà l'entrée courante
    // Page précédente (suivante) encore dans le document, d'indice inférieur à `pageCount` : les entrées hors
    // du document sont sautées ; s'il n'y en a aucune, −1 et la position ne bouge pas (rien n'est consommé).
    int back(int pageCount = std::numeric_limits<int>::max());
    int forward(int pageCount = std::numeric_limits<int>::max());
    // Recalage sur les pages ajoutées ou retirées, un changement à la fois avec l'indice du moment (lecture
    // de core::applyToCache, revue L8-C3) : une page insérée fait reculer d'un rang les entrées à partir de
    // son indice ; une page retirée perd ses entrées, les suivantes avancent d'un rang, deux entrées devenues
    // voisines et égales n'en font plus qu'une. Un déplacement (retrait puis insertion) perd les entrées de
    // la page déplacée, comme la page affichée déplacée est relue à son indice (DocumentController::settle).
    void pageInserted(int index);
    void pageRemoved(int index);
    void clear();
    std::size_t size() const noexcept { return m_pages.size(); }

private:
    std::vector<int> m_pages;
    std::size_t m_position = 0; // entrée courante ; sans objet si m_pages est vide
};

} // namespace actcraft::ui
