#pragma once

#include <QString>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace actcraft::core {

// Arbre d'équation (F-301) : sous-ensemble de MathML présentation (décision 0100). Le texte gardé reste
// `Equation::mathml` (model.h) : une équation qu'on ne modifie pas n'est jamais réécrite ; l'arbre sert à la mise en
// page et à l'éditeur, qui réécrit sous la forme canonique de toMathml.
struct EquationNode
{
    enum class Kind { Math, Row, Identifier, Number, Operator, Text, Fraction, Sup, Sub, SubSup, Sqrt, Root,
                      UnderOver, Under, Over, Table, TableRow, TableCell, Placeholder, Raw };
    Kind kind = Kind::Row;
    // Identifier, Number, Operator, Text : contenu UTF-8, références de caractères décodées. Raw : texte XML
    // d'origine de l'élément (balises comprises), recopié tel quel.
    std::string text;
    std::vector<EquationNode> children;
    // Attributs d'un élément reconnu (nom qualifié, valeur décodée ; UTF-8), dans l'ordre lu et réécrits dans cet
    // ordre. Un Raw garde les siens dans `text`.
    std::vector<std::pair<std::string, std::string>> attributes;
};

inline constexpr std::size_t kMaxMathmlBytes = 256 * 1024;
inline constexpr int kMaxEquationDepth = 64; // éléments imbriqués, math compris

// Placeholder : case vide de l'éditeur, écrite <mrow class="actcraft-placeholder"/> (ses autres attributs suivent la
// marque) ; à la lecture, un <mrow/> vide dont le seul attribut est cette marque.
inline constexpr std::string_view kPlaceholderClass = "actcraft-placeholder";

// Lit un MathML UTF-8 (racine math). Erreur (nullopt, `error` renseigné) : XML mal formé, UTF-8 invalide, DTD
// (aucune entité externe ni déclarée n'est lue), plus de kMaxMathmlBytes octets ou de kMaxEquationDepth niveaux,
// arité d'un élément reconnu fausse. Un élément inconnu de cette version devient un nœud Raw.
std::optional<EquationNode> parseMathml(const std::string& mathml, QString& error);

// Forme canonique : sans blancs entre éléments ni indentation, références en caractères UTF-8 sauf < > & échappés,
// élément vide en <x/> ; un Raw recopié tel quel.
std::string toMathml(const EquationNode& root);

} // namespace actcraft::core
