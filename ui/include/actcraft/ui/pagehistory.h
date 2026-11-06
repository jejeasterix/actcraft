#pragma once

#include <cstddef>
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
    int back();             // page précédente, −1 si rien
    int forward();          // page suivante, −1 si rien
    void clear();
    std::size_t size() const noexcept { return m_pages.size(); }

private:
    std::vector<int> m_pages;
    std::size_t m_position = 0; // entrée courante ; sans objet si m_pages est vide
};

} // namespace actcraft::ui
