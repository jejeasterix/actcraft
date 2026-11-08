#pragma once

#include <actcraft/core/equation.h>

#include <QString>

#include <optional>
#include <string>
#include <vector>

namespace actcraft::features {

// Modèle de l'éditeur d'équations (F-301) : identifiant stable, libellé (nos mots), MathML canonique dont les
// cases vides s'écrivent <mrow/> (une cellule de tableau vide : <mtd/>), comme les modèles de L7-T24.
struct EquationTemplate
{
    QString id;
    QString label;
    std::string mathml;
};

// Palette : une catégorie de modèles (rapport L7-T0), dans l'ordre de présentation.
struct EquationPalette
{
    QString id;
    QString label;
    std::vector<QString> entries; // identifiants de modèles
};

// Éditeur d'équations à modèles (F-301) sur l'arbre de core::EquationNode. Le curseur désigne un nœud par son
// chemin d'indices depuis <math> : une case vide, un jeton (mi, mn, mo, mtext), un élément inconnu gardé tel
// quel ou une suite vide. La saisie remplace la case vide désignée, sinon s'ajoute après le nœud désigné dans
// sa suite (math, mrow, msqrt, mtd) ; sous un élément d'arité fixe (mfrac, msup…), le nœud et la saisie sont
// réunis dans un mrow. Chiffres groupés en un mn, lettres en mi, le reste en mo.
//
// Trois écritures : mathml() (cases vides marquées, sans curseur), editorMathml() (en plus, la marque
// actcraft-cursor sur le nœud du curseur : l'aperçu seul la reçoit) et documentMathml() (ce qui entre dans le
// document : aucune marque ; rien tant qu'une case reste vide).
class EquationEditor
{
public:
    EquationEditor();

    static const std::vector<EquationTemplate>& templates();
    static const std::vector<EquationPalette>& palettes();
    // MathML d'un modèle tel que l'éditeur l'insère (cases vides marquées, sans curseur) ; vide s'il est
    // inconnu. Sert au dessin des boutons de palette.
    static std::string templateMathml(const QString& id);

    void clear(); // une case vide
    // Remplace l'équation ; la marque du curseur lue est retirée ; curseur sur la première case. Faux (rien
    // ne change) si le MathML ne se lit pas.
    bool load(const std::string& mathml, QString& error);
    bool insertTemplate(const QString& id); // faux si le modèle est inconnu
    void type(const QString& text);
    void backspace();
    void moveLeft();
    void moveRight();
    void moveUp();
    void moveDown();

    std::string mathml() const;
    std::string editorMathml() const;
    std::optional<std::string> documentMathml() const;
    bool hasPlaceholder() const;
    std::vector<int> cursorPath() const;

private:
    std::vector<std::vector<int>> insertNodes(std::vector<core::EquationNode> nodes);
    void moveVertically(bool up);
    void typeCodePoint(char32_t c);

    core::EquationNode m_root;
    std::vector<int> m_cursor;
};

} // namespace actcraft::features
