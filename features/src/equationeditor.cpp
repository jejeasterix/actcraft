#include <actcraft/features/equationeditor.h>

#include <QChar>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <string_view>
#include <utility>

namespace actcraft::features {

namespace {

using Node = core::EquationNode;
using Kind = core::EquationNode::Kind;
using Path = std::vector<int>;

constexpr std::string_view kCursorClass = "actcraft-cursor";

struct TemplateRow
{
    const char16_t* id;
    const char16_t* label;
    const char16_t* mathml;
};

// Les 17 modèles de L7-T24 (formes de tst_core_equation, templatesRoundTrip), puis ceux que les palettes
// ajoutent. Libellés et choix : les nôtres ; les cases vides s'écrivent <mrow/> ou <mtd/>.
constexpr TemplateRow kTemplates[] = {
    {u"fraction", u"Fraction", u"<math><mfrac><mrow/><mrow/></mfrac></math>"},
    {u"power", u"Exposant", u"<math><msup><mrow/><mrow/></msup></math>"},
    {u"subscript", u"Indice", u"<math><msub><mrow/><mrow/></msub></math>"},
    {u"square-root", u"Racine carrée", u"<math><msqrt><mrow/></msqrt></math>"},
    {u"nth-root", u"Racine n-ième", u"<math><mroot><mrow/><mrow/></mroot></math>"},
    {u"parentheses", u"Parenthèses", u"<math><mrow><mo>(</mo><mrow/><mo>)</mo></mrow></math>"},
    {u"sum", u"Somme", u"<math><munderover><mo>∑</mo><mrow/><mrow/></munderover><mrow/></math>"},
    {u"integral", u"Intégrale", u"<math><msubsup><mo>∫</mo><mrow/><mrow/></msubsup><mrow/></math>"},
    {u"matrix-2x2", u"Matrice 2 × 2",
     u"<math><mrow><mo>(</mo><mtable><mtr><mtd/><mtd/></mtr><mtr><mtd/><mtd/></mtr></mtable><mo>)</mo></mrow>"
     u"</math>"},
    {u"greek", u"Quatre lettres grecques", u"<math><mi>α</mi><mi>ω</mi><mi>Δ</mi><mi>Ω</mi></math>"},
    {u"absolute-value", u"Valeur absolue", u"<math><mrow><mo>|</mo><mrow/><mo>|</mo></mrow></math>"},
    {u"matrix-1x2", u"Matrice ligne 1 × 2",
     u"<math><mrow><mo>(</mo><mtable><mtr><mtd/><mtd/></mtr></mtable><mo>)</mo></mrow></math>"},
    {u"matrix-2x1", u"Matrice colonne 2 × 1",
     u"<math><mrow><mo>(</mo><mtable><mtr><mtd/></mtr><mtr><mtd/></mtr></mtable><mo>)</mo></mrow></math>"},
    {u"matrix-3x3", u"Matrice 3 × 3",
     u"<math><mrow><mo>(</mo><mtable><mtr><mtd/><mtd/><mtd/></mtr><mtr><mtd/><mtd/><mtd/></mtr><mtr><mtd/>"
     u"<mtd/><mtd/></mtr></mtable><mo>)</mo></mrow></math>"},
    {u"double-integral", u"Intégrale double", u"<math><msub><mo>∬</mo><mrow/></msub><mrow/></math>"},
    {u"triple-integral", u"Intégrale triple", u"<math><msub><mo>∭</mo><mrow/></msub><mrow/></math>"},
    {u"trig-function", u"Sinus", u"<math><mrow><mi>sin</mi><mo>(</mo><mrow/><mo>)</mo></mrow></math>"},

    {u"reciprocal", u"Un sur…", u"<math><mfrac><mn>1</mn><mrow/></mfrac></math>"},
    {u"cube-root", u"Racine cubique", u"<math><mroot><mrow/><mn>3</mn></mroot></math>"},
    {u"square", u"Carré", u"<math><msup><mrow/><mn>2</mn></msup></math>"},
    {u"sub-sup", u"Indice et exposant", u"<math><msubsup><mrow/><mrow/><mrow/></msubsup></math>"},
    {u"over", u"Au-dessus", u"<math><mover><mrow/><mrow/></mover></math>"},
    {u"under", u"Au-dessous", u"<math><munder><mrow/><mrow/></munder></math>"},
    {u"under-over", u"Au-dessus et au-dessous",
     u"<math><munderover><mrow/><mrow/><mrow/></munderover></math>"},
    {u"vector", u"Vecteur", u"<math><mover><mrow/><mo>→</mo></mover></math>"},
    {u"bar", u"Barre", u"<math><mover><mrow/><mo>¯</mo></mover></math>"},
    {u"hat", u"Chapeau", u"<math><mover><mrow/><mo>^</mo></mover></math>"},
    {u"brackets", u"Crochets", u"<math><mrow><mo>[</mo><mrow/><mo>]</mo></mrow></math>"},
    {u"braces", u"Accolades", u"<math><mrow><mo>{</mo><mrow/><mo>}</mo></mrow></math>"},
    {u"product", u"Produit", u"<math><munderover><mo>∏</mo><mrow/><mrow/></munderover><mrow/></math>"},
    {u"limit", u"Limite", u"<math><munder><mi>lim</mi><mrow/></munder><mrow/></math>"},
    {u"contour-integral", u"Intégrale sur un contour", u"<math><msub><mo>∮</mo><mrow/></msub><mrow/></math>"},
    {u"cosine", u"Cosinus", u"<math><mrow><mi>cos</mi><mo>(</mo><mrow/><mo>)</mo></mrow></math>"},
    {u"tangent", u"Tangente", u"<math><mrow><mi>tan</mi><mo>(</mo><mrow/><mo>)</mo></mrow></math>"},
    {u"logarithm", u"Logarithme", u"<math><mrow><mi>log</mi><mo>(</mo><mrow/><mo>)</mo></mrow></math>"},
    {u"exponential", u"Exponentielle", u"<math><msup><mi>e</mi><mrow/></msup></math>"},
    {u"plus-minus", u"Plus ou moins", u"<math><mo>±</mo></math>"},
    {u"times", u"Multiplié par", u"<math><mo>×</mo></math>"},
    {u"divided", u"Divisé par", u"<math><mo>÷</mo></math>"},
    {u"less-equal", u"Inférieur ou égal", u"<math><mo>≤</mo></math>"},
    {u"greater-equal", u"Supérieur ou égal", u"<math><mo>≥</mo></math>"},
    {u"not-equal", u"Différent de", u"<math><mo>≠</mo></math>"},
    {u"approximately", u"Environ égal", u"<math><mo>≈</mo></math>"},
    {u"infinity", u"Infini", u"<math><mi>∞</mi></math>"},
    {u"alpha", u"alpha", u"<math><mi>α</mi></math>"},
    {u"beta", u"bêta", u"<math><mi>β</mi></math>"},
    {u"gamma", u"gamma", u"<math><mi>γ</mi></math>"},
    {u"delta", u"delta", u"<math><mi>δ</mi></math>"},
    {u"theta", u"thêta", u"<math><mi>θ</mi></math>"},
    {u"lambda", u"lambda", u"<math><mi>λ</mi></math>"},
    {u"mu", u"mu", u"<math><mi>μ</mi></math>"},
    {u"pi", u"pi", u"<math><mi>π</mi></math>"},
    {u"sigma", u"sigma", u"<math><mi>σ</mi></math>"},
    {u"phi", u"phi", u"<math><mi>φ</mi></math>"},
    {u"omega", u"oméga", u"<math><mi>ω</mi></math>"},
    {u"capital-delta", u"Delta majuscule", u"<math><mi>Δ</mi></math>"},
    {u"capital-sigma", u"Sigma majuscule", u"<math><mi>Σ</mi></math>"},
    {u"capital-omega", u"Oméga majuscule", u"<math><mi>Ω</mi></math>"},
};

struct PaletteRow
{
    const char16_t* id;
    const char16_t* label;
    std::vector<const char16_t*> entries;
};

Node placeholder()
{
    Node node;
    node.kind = Kind::Placeholder;
    return node;
}

Node emptyMath()
{
    Node node;
    node.kind = Kind::Math;
    node.children.push_back(placeholder());
    return node;
}

std::size_t at(int i)
{
    return static_cast<std::size_t>(i);
}

// Suites : enfants lus l'un après l'autre (mrow implicite de MathML), en nombre libre
// (core/src/equation.cpp).
bool isSequence(Kind kind)
{
    return kind == Kind::Math || kind == Kind::Row || kind == Kind::Sqrt || kind == Kind::TableCell;
}

// Case où le curseur se pose : une feuille (case vide, jeton, élément inconnu) ou une suite vide.
bool isSlot(const Node& node)
{
    return node.children.empty() && node.kind != Kind::Math && node.kind != Kind::Table &&
           node.kind != Kind::TableRow;
}

template <typename N> N* walk(N& root, const Path& path)
{
    N* node = &root;
    for (const int i : path) {
        if (i < 0 || at(i) >= node->children.size()) {
            return nullptr;
        }
        node = &node->children[at(i)];
    }
    return node;
}

void collectSlots(const Node& node, Path& path, std::vector<Path>& out)
{
    if (!path.empty() && isSlot(node)) {
        out.push_back(path);
        return;
    }
    for (std::size_t i = 0; i < node.children.size(); ++i) {
        path.push_back(static_cast<int>(i));
        collectSlots(node.children[i], path, out);
        path.pop_back();
    }
}

// Cases du sous-arbre de `prefix`, dans l'ordre du texte.
std::vector<Path> slotsOf(const Node& root, Path prefix)
{
    std::vector<Path> out;
    if (const Node* node = walk(root, prefix)) {
        collectSlots(*node, prefix, out);
    }
    return out;
}

Path firstSlotIn(const Node& root, const Path& prefix)
{
    const std::vector<Path> slots = slotsOf(root, prefix);
    return slots.empty() ? prefix : slots.front();
}

Path lastSlotIn(const Node& root, const Path& prefix)
{
    const std::vector<Path> slots = slotsOf(root, prefix);
    return slots.empty() ? prefix : slots.back();
}

bool containsPlaceholder(const Node& node)
{
    return node.kind == Kind::Placeholder ||
           std::any_of(node.children.begin(), node.children.end(), containsPlaceholder);
}

bool findPlaceholder(const Node& node, Path& path)
{
    if (node.kind == Kind::Placeholder) {
        return true;
    }
    for (std::size_t i = 0; i < node.children.size(); ++i) {
        path.push_back(static_cast<int>(i));
        if (findPlaceholder(node.children[i], path)) {
            return true;
        }
        path.pop_back();
    }
    return false;
}

// Cases vides d'un modèle : un <mrow/> sans attribut devient une case vide, une cellule vide en reçoit une.
void markEmptySlots(Node& node)
{
    for (Node& child : node.children) {
        if (child.kind == Kind::Row && child.children.empty() && child.attributes.empty()) {
            child = placeholder();
        } else if (child.kind == Kind::TableCell && child.children.empty()) {
            child.children.push_back(placeholder());
        } else {
            markEmptySlots(child);
        }
    }
}

// Retire le jeton actcraft-cursor des listes class (l'attribut disparaît s'il ne reste rien) ; une liste sans
// ce jeton reste telle quelle.
void dropCursorMark(Node& node)
{
    auto& attributes = node.attributes;
    for (auto it = attributes.begin(); it != attributes.end();) {
        if (it->first != "class") {
            ++it;
            continue;
        }
        const std::string& value = it->second;
        std::string kept;
        bool found = false;
        std::size_t position = 0;
        while (position < value.size()) {
            const std::size_t start = value.find_first_not_of(" \t\r\n", position);
            if (start == std::string::npos) {
                break;
            }
            const std::size_t end = std::min(value.find_first_of(" \t\r\n", start), value.size());
            const std::string_view token(value.data() + start, end - start);
            if (token == kCursorClass) {
                found = true;
            } else {
                if (!kept.empty()) {
                    kept += ' ';
                }
                kept.append(token);
            }
            position = end;
        }
        if (!found) {
            ++it;
        } else if (kept.empty()) {
            it = attributes.erase(it);
        } else {
            it->second = std::move(kept);
            ++it;
        }
    }
    for (Node& child : node.children) {
        dropCursorMark(child);
    }
}

void addCursorMark(Node& node)
{
    for (auto& [name, value] : node.attributes) {
        if (name == "class") {
            if (!value.empty()) {
                value += ' ';
            }
            value += kCursorClass;
            return;
        }
    }
    node.attributes.emplace_back("class", std::string(kCursorClass));
}

// Case atteinte par haut (`up`) ou bas depuis l'enfant `from` d'un élément à deux dimensions ; -1 : aucune.
// Enfants : mfrac (numérateur, dénominateur), msub et munder (base, dessous), msup, mover et mroot (base,
// dessus), msubsup et munderover (base, dessous, dessus).
int verticalStep(Kind kind, int from, bool up)
{
    switch (kind) {
    case Kind::Fraction:
    case Kind::Sub:
    case Kind::Under:
        return up ? (from == 1 ? 0 : -1) : (from == 0 ? 1 : -1);
    case Kind::Sup:
    case Kind::Over:
    case Kind::Root:
        return up ? (from == 0 ? 1 : -1) : (from == 1 ? 0 : -1);
    case Kind::SubSup:
    case Kind::UnderOver:
        if (up) {
            return from == 0 ? 2 : (from == 1 ? 0 : -1);
        }
        return from == 0 ? 1 : (from == 2 ? 0 : -1);
    default:
        return -1;
    }
}

} // namespace

EquationEditor::EquationEditor()
{
    clear();
}

const std::vector<EquationTemplate>& EquationEditor::templates()
{
    static const std::vector<EquationTemplate> list = [] {
        std::vector<EquationTemplate> out;
        out.reserve(std::size(kTemplates));
        for (const TemplateRow& row : kTemplates) {
            out.push_back({QString::fromUtf16(row.id), QString::fromUtf16(row.label),
                           QString::fromUtf16(row.mathml).toStdString()});
        }
        return out;
    }();
    return list;
}

const std::vector<EquationPalette>& EquationEditor::palettes()
{
    // Les douze catégories du rapport L7-T0, en nos mots.
    static const std::vector<EquationPalette> list = [] {
        const PaletteRow rows[] = {
            {u"fractions", u"Fractions", {u"fraction", u"reciprocal"}},
            {u"roots", u"Racines", {u"square-root", u"nth-root", u"cube-root"}},
            {u"scripts", u"Indices et exposants", {u"power", u"subscript", u"sub-sup", u"square"}},
            {u"underover", u"Au-dessus et au-dessous", {u"over", u"under", u"under-over"}},
            {u"accents", u"Accents", {u"vector", u"bar", u"hat"}},
            {u"delimiters", u"Délimiteurs", {u"parentheses", u"absolute-value", u"brackets", u"braces"}},
            {u"operators", u"Grands opérateurs", {u"sum", u"product", u"limit"}},
            {u"integrals",
             u"Intégrales",
             {u"integral", u"double-integral", u"triple-integral", u"contour-integral"}},
            {u"matrices", u"Matrices", {u"matrix-1x2", u"matrix-2x1", u"matrix-2x2", u"matrix-3x3"}},
            {u"functions",
             u"Fonctions",
             {u"trig-function", u"cosine", u"tangent", u"logarithm", u"exponential"}},
            {u"relations",
             u"Relations et symboles",
             {u"plus-minus", u"times", u"divided", u"less-equal", u"greater-equal", u"not-equal",
              u"approximately", u"infinity"}},
            {u"greek",
             u"Lettres grecques",
             {u"alpha", u"beta", u"gamma", u"delta", u"theta", u"lambda", u"mu", u"pi", u"sigma", u"phi",
              u"omega", u"capital-delta", u"capital-sigma", u"capital-omega", u"greek"}},
        };
        std::vector<EquationPalette> out;
        for (const PaletteRow& row : rows) {
            EquationPalette palette{QString::fromUtf16(row.id), QString::fromUtf16(row.label), {}};
            for (const char16_t* entry : row.entries) {
                palette.entries.push_back(QString::fromUtf16(entry));
            }
            out.push_back(std::move(palette));
        }
        return out;
    }();
    return list;
}

std::string EquationEditor::templateMathml(const QString& id)
{
    EquationEditor editor;
    return editor.insertTemplate(id) ? editor.mathml() : std::string();
}

void EquationEditor::clear()
{
    m_root = emptyMath();
    m_cursor = {0};
}

bool EquationEditor::load(const std::string& mathml, QString& error)
{
    std::optional<Node> tree = core::parseMathml(mathml, error);
    if (!tree) {
        return false;
    }
    dropCursorMark(*tree);
    if (tree->children.empty()) {
        tree->children.push_back(placeholder());
    }
    m_root = std::move(*tree);
    const std::vector<Path> slots = slotsOf(m_root, {});
    m_cursor = slots.empty() ? Path{static_cast<int>(m_root.children.size()) - 1} : slots.front();
    return true;
}

// Pose `nodes` au curseur : à la place de la case vide désignée, dans la suite vide désignée, sinon après le
// nœud désigné. Rend les chemins des nœuds posés (vide : rien posé).
std::vector<std::vector<int>> EquationEditor::insertNodes(std::vector<Node> nodes)
{
    std::vector<Path> placed;
    Node* current = walk(m_root, m_cursor);
    if (!current || m_cursor.empty() || nodes.empty()) {
        return placed;
    }
    const int count = static_cast<int>(nodes.size());
    if (isSequence(current->kind) && current->children.empty()) {
        for (int i = 0; i < count; ++i) {
            Path path = m_cursor;
            path.push_back(i);
            placed.push_back(std::move(path));
        }
        current->children = std::move(nodes);
        return placed;
    }
    const bool replace = current->kind == Kind::Placeholder;
    const Path parentPath(m_cursor.begin(), m_cursor.end() - 1);
    Node& parent = *walk(m_root, parentPath);
    const int index = m_cursor.back();
    if (isSequence(parent.kind)) {
        const int first = replace ? index : index + 1;
        if (replace) {
            parent.children.erase(parent.children.begin() + index);
        }
        parent.children.insert(parent.children.begin() + first, std::make_move_iterator(nodes.begin()),
                               std::make_move_iterator(nodes.end()));
        for (int i = 0; i < count; ++i) {
            Path path = parentPath;
            path.push_back(first + i);
            placed.push_back(std::move(path));
        }
        return placed;
    }
    // Élément d'arité fixe : la case reçoit le nœud seul, ou un mrow (nœud désigné, puis les nœuds posés).
    if (replace && count == 1) {
        parent.children[at(index)] = std::move(nodes.front());
        placed.push_back(m_cursor);
        return placed;
    }
    Node row;
    row.kind = Kind::Row;
    if (!replace) {
        row.children.push_back(std::move(parent.children[at(index)]));
    }
    const int offset = replace ? 0 : 1;
    row.children.insert(row.children.end(), std::make_move_iterator(nodes.begin()),
                        std::make_move_iterator(nodes.end()));
    parent.children[at(index)] = std::move(row);
    for (int i = 0; i < count; ++i) {
        Path path = m_cursor;
        path.push_back(offset + i);
        placed.push_back(std::move(path));
    }
    return placed;
}

bool EquationEditor::insertTemplate(const QString& id)
{
    const auto& list = templates();
    const auto found =
        std::find_if(list.begin(), list.end(), [&](const EquationTemplate& t) { return t.id == id; });
    if (found == list.end()) {
        return false;
    }
    QString error;
    std::optional<Node> tree = core::parseMathml(found->mathml, error);
    if (!tree || tree->children.empty()) {
        return false;
    }
    markEmptySlots(*tree);
    const std::vector<Path> placed = insertNodes(std::move(tree->children));
    if (placed.empty()) {
        return false;
    }
    for (const Path& start : placed) {
        Path path = start;
        if (const Node* node = walk(m_root, start); node && findPlaceholder(*node, path)) {
            m_cursor = path;
            return true;
        }
    }
    m_cursor = lastSlotIn(m_root, placed.back());
    return true;
}

void EquationEditor::type(const QString& text)
{
    for (const char32_t c : text.toUcs4()) {
        typeCodePoint(c);
    }
}

void EquationEditor::typeCodePoint(char32_t c)
{
    if (c < 0x20 || c == 0x7F || QChar::isSpace(c)) {
        return;
    }
    const bool digit = c >= U'0' && c <= U'9';
    const bool separator = c == U'.' || c == U',';
    Node* current = walk(m_root, m_cursor);
    if (current && current->kind == Kind::Number && (digit || separator)) {
        current->text += static_cast<char>(c); // ASCII
        return;
    }
    Node token;
    token.kind = digit ? Kind::Number : (QChar::isLetter(c) ? Kind::Identifier : Kind::Operator);
    char32_t written = c;
    if (c == U'-') {
        written = U'−'; // signe moins
    } else if (c == U'*') {
        written = U'×'; // multiplié par
    }
    token.text = QString::fromUcs4(&written, 1).toStdString();
    const std::vector<Path> placed = insertNodes({std::move(token)});
    if (!placed.empty()) {
        m_cursor = placed.back();
    }
}

void EquationEditor::backspace()
{
    Node* current = walk(m_root, m_cursor);
    if (!current || m_cursor.empty()) {
        return;
    }
    if (current->kind == Kind::Number && current->text.size() > 1) {
        current->text.pop_back(); // chiffres et séparateurs : ASCII
        return;
    }
    const Path parentPath(m_cursor.begin(), m_cursor.end() - 1);
    Node& parent = *walk(m_root, parentPath);
    const int index = m_cursor.back();
    const bool removable = isSequence(parent.kind) && parent.children.size() > 1;
    if (current->kind == Kind::Placeholder && !removable) {
        // Modèle dont toutes les cases sont vides : il s'efface, une case vide le remplace ; sinon le curseur
        // recule.
        const bool allEmpty = std::all_of(parent.children.begin(), parent.children.end(),
                                          [](const Node& n) { return n.kind == Kind::Placeholder; });
        if (!isSequence(parent.kind) && !parentPath.empty() && allEmpty) {
            parent = placeholder();
            m_cursor = parentPath;
        } else {
            moveLeft();
        }
        return;
    }
    if (!removable) {
        parent.children[at(index)] = placeholder(); // l'arité reste juste
        return;
    }
    parent.children.erase(parent.children.begin() + index);
    // Un mrow d'enveloppe réduit à un enfant sous un élément d'arité fixe redevient cet enfant.
    if (parent.kind == Kind::Row && parent.children.size() == 1 && parent.attributes.empty() &&
        !parentPath.empty()) {
        const Path grandPath(parentPath.begin(), parentPath.end() - 1);
        Node& grand = *walk(m_root, grandPath);
        if (!isSequence(grand.kind)) {
            Node only = std::move(parent.children.front());
            grand.children[at(parentPath.back())] = std::move(only);
            m_cursor = lastSlotIn(m_root, parentPath);
            return;
        }
    }
    Path target = parentPath;
    if (index > 0) {
        target.push_back(index - 1);
        m_cursor = lastSlotIn(m_root, target);
    } else {
        target.push_back(0);
        m_cursor = firstSlotIn(m_root, target);
    }
}

void EquationEditor::moveLeft()
{
    const std::vector<Path> slots = slotsOf(m_root, {});
    for (auto it = slots.rbegin(); it != slots.rend(); ++it) {
        if (*it < m_cursor) {
            m_cursor = *it;
            return;
        }
    }
}

void EquationEditor::moveRight()
{
    const std::vector<Path> slots = slotsOf(m_root, {});
    for (const Path& slot : slots) {
        if (m_cursor < slot) {
            m_cursor = slot;
            return;
        }
    }
}

void EquationEditor::moveUp()
{
    moveVertically(true);
}

void EquationEditor::moveDown()
{
    moveVertically(false);
}

// Remonte du curseur vers la racine jusqu'au premier élément à deux dimensions qui a une case dans la
// direction demandée (tableau : la ligne voisine, même colonne si elle existe).
void EquationEditor::moveVertically(bool up)
{
    for (std::size_t depth = m_cursor.size(); depth-- > 0;) {
        const Path ancestorPath(m_cursor.begin(), m_cursor.begin() + static_cast<std::ptrdiff_t>(depth));
        const Node* ancestor = walk(m_root, ancestorPath);
        if (!ancestor) {
            return;
        }
        const int from = m_cursor[depth];
        const int to = ancestor->kind == Kind::Table ? (up ? from - 1 : from + 1)
                                                     : verticalStep(ancestor->kind, from, up);
        if (to < 0 || at(to) >= ancestor->children.size()) {
            continue;
        }
        Path target = ancestorPath;
        target.push_back(to);
        if (ancestor->kind == Kind::Table && depth + 1 < m_cursor.size()) {
            const Node& row = ancestor->children[at(to)];
            if (!row.children.empty()) {
                target.push_back(std::min(m_cursor[depth + 1], static_cast<int>(row.children.size()) - 1));
            }
        }
        m_cursor = firstSlotIn(m_root, target);
        return;
    }
}

std::string EquationEditor::mathml() const
{
    return core::toMathml(m_root);
}

std::string EquationEditor::editorMathml() const
{
    Node copy = m_root;
    if (Node* node = walk(copy, m_cursor); node && !m_cursor.empty()) {
        addCursorMark(*node);
    }
    return core::toMathml(copy);
}

std::optional<std::string> EquationEditor::documentMathml() const
{
    if (hasPlaceholder()) {
        return std::nullopt;
    }
    std::string written = core::toMathml(m_root);
    if (written.empty()) {
        return std::nullopt;
    }
    return written;
}

bool EquationEditor::hasPlaceholder() const
{
    return containsPlaceholder(m_root);
}

std::vector<int> EquationEditor::cursorPath() const
{
    return m_cursor;
}

} // namespace actcraft::features
