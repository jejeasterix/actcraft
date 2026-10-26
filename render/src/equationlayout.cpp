#include "equationlayout.h"

#include <QColor>
#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPathStroker>
#include <QPen>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QTransform>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace actcraft::render {

namespace {

using Kind = core::EquationNode::Kind;

// Règles de mise en page (les nôtres, F-301), en cadratins : em = taille en pixels de la police au niveau
// courant. Les glyphes sont tirés en contours à kReferencePixels puis mis à l'échelle : aucune taille
// arrondie au pixel, même rendu sur une image transparente (texture) ou blanche (page logicielle).
constexpr int kReferencePixels = 100;
constexpr double kFractionScale = 0.8; // numérateur et dénominateur
constexpr double kScriptScale = 0.7;   // exposant, indice, bornes
constexpr double kIndexScale = 0.6;    // indice d'une racine n-ième
constexpr double kMinimumScale = 0.3;  // plancher des réductions successives, rapporté à la base
constexpr double kLargeOperatorScale = 1.4;
constexpr double kTokenAscent = 0.7; // hauteur minimale d'un jeton au-dessus de la ligne de base
constexpr double kTokenDescent = 0.2;
constexpr double kEmptyWidth = 0.6; // case vide
constexpr double kRule = 0.06;      // barre de fraction, trait de la racine
constexpr double kGap = 0.12;       // entre une barre et ce qu'elle sépare
constexpr double kAxis = 0.28; // axe des fractions et des grands opérateurs, au-dessus de la ligne de base
constexpr double kFractionPad = 0.1;
constexpr double kOperatorSpace = 0.2;
constexpr double kSeparatorSpace = 0.15;
constexpr double kScriptGap = 0.05;
constexpr double kSign = 0.6; // largeur du signe de la racine
constexpr double kRootPad = 0.1;
constexpr double kLimitGap = 0.1;
constexpr double kColumnGap = 0.8;
constexpr double kRowGap = 0.35;
constexpr double kFenceMargin = 0.05;
constexpr std::size_t kCacheEntries = 64;

constexpr std::string_view kCursorClass = "actcraft-cursor";

struct Context
{
    QFont font;    // famille et style, à kReferencePixels
    double baseEm; // taille de base, pixels
};

bool hasClass(const core::EquationNode& node, std::string_view token)
{
    for (const auto& [name, value] : node.attributes) {
        if (name != "class") {
            continue;
        }
        std::size_t at = 0;
        while (at < value.size()) {
            const std::size_t begin = value.find_first_not_of(" \t\r\n", at);
            if (begin == std::string::npos) {
                break;
            }
            std::size_t end = value.find_first_of(" \t\r\n", begin);
            if (end == std::string::npos) {
                end = value.size();
            }
            if (std::string_view(value).substr(begin, end - begin) == token) {
                return true;
            }
            at = end;
        }
    }
    return false;
}

bool oneOf(const std::string& text, std::initializer_list<std::string_view> set)
{
    return std::find(set.begin(), set.end(), std::string_view(text)) != set.end();
}

bool isFence(const core::EquationNode& node)
{
    return node.kind == Kind::Operator &&
           oneOf(node.text, {"(", ")", "[", "]", "{", "}", "|", "‖", "⟨", "⟩", "⌈", "⌉", "⌊", "⌋"});
}

bool isLargeOperator(const std::string& text)
{
    return oneOf(text, {"∑", "∏", "∐", "∫", "∬", "∭", "∮", "⋃", "⋂", "⋀", "⋁"});
}

bool isSpacedOperator(const std::string& text)
{
    return oneOf(text, {"+", "-", "−", "=", "<", ">", "≤", "≥", "≠", "≈", "≡", "×", "÷",
                        "±", "∓", "·", "→", "←", "⇒", "⇔", "∈", "∉", "⊂", "⊆", "∪", "∩"});
}

double reduced(const Context& ctx, double em, double factor)
{
    return std::max(em * factor, ctx.baseEm * kMinimumScale);
}

EquationBox emptyBox(double em)
{
    EquationBox box;
    box.width = kEmptyWidth * em;
    box.ascent = kTokenAscent * em;
    box.descent = kTokenDescent * em;
    return box;
}

void fitInk(EquationBox& box, double em)
{
    const QRectF ink = box.ink.boundingRect();
    box.ascent = std::max(-ink.top(), kTokenAscent * em);
    box.descent = std::max(ink.bottom(), kTokenDescent * em);
}

// Jeton : contours des glyphes (police du système, repli de Qt pour les signes absents), avance de la police.
EquationBox token(const Context& ctx, const std::string& text, double em, bool italic, double size = 1.0)
{
    EquationBox box;
    QFont font = ctx.font;
    font.setItalic(italic);
    const QString string = QString::fromStdString(text);
    const double k = em * size / kReferencePixels;
    QPainterPath path;
    path.addText(0, 0, font, string);
    box.ink = QTransform::fromScale(k, k).map(path);
    const double advance = QFontMetricsF(font).horizontalAdvance(string) * k;
    box.width = std::max(advance, box.ink.isEmpty() ? 0.0 : box.ink.boundingRect().right());
    fitInk(box, em);
    return box;
}

EquationBox layoutNode(const Context& ctx, const core::EquationNode& node, double em);

// Parenthèse étirée sur [-ascent, descent] si son glyphe est plus court.
void stretchFence(EquationBox& fence, double ascent, double descent)
{
    const QRectF ink = fence.ink.boundingRect();
    if (ink.isEmpty() || ascent + descent <= ink.height() * 1.05) {
        return;
    }
    QTransform t;
    t.translate(0, -ascent);
    t.scale(1, (ascent + descent) / ink.height());
    t.translate(0, -ink.top());
    fence.ink = t.map(fence.ink);
    fence.ascent = std::max(fence.ascent, ascent);
    fence.descent = std::max(fence.descent, descent);
}

// Suite d'éléments sur une même ligne de base ; vide : une case vide (rien ne s'y dessine en mode Page).
EquationBox row(const Context& ctx, const std::vector<core::EquationNode>& nodes, double em)
{
    if (nodes.empty()) {
        return emptyBox(em);
    }
    EquationBox box;
    double innerAscent = 0;
    double innerDescent = 0;
    bool hasInner = false;
    for (const core::EquationNode& node : nodes) {
        box.children.push_back(layoutNode(ctx, node, em));
        if (!isFence(node)) {
            innerAscent = std::max(innerAscent, box.children.back().ascent);
            innerDescent = std::max(innerDescent, box.children.back().descent);
            hasInner = true;
        }
    }
    if (hasInner) {
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            if (isFence(nodes[i])) {
                stretchFence(box.children[i], innerAscent + kFenceMargin * em,
                             innerDescent + kFenceMargin * em);
            }
        }
    }
    double x = 0;
    for (EquationBox& child : box.children) {
        child.x = x;
        child.baseline = 0;
        x += child.width;
        box.ascent = std::max(box.ascent, child.ascent);
        box.descent = std::max(box.descent, child.descent);
    }
    box.width = x;
    return box;
}

EquationBox operatorToken(const Context& ctx, const std::string& text, double em)
{
    const bool large = isLargeOperator(text);
    EquationBox box = token(ctx, text, em, false, large ? kLargeOperatorScale : 1.0);
    if (large) {
        // Grand opérateur centré sur l'axe.
        const QRectF ink = box.ink.boundingRect();
        box.ink.translate(0, -kAxis * em - ink.center().y());
        fitInk(box, em);
    }
    double left = 0;
    double right = 0;
    if (isSpacedOperator(text)) {
        left = right = kOperatorSpace * em;
    } else if (text == "," || text == ";") {
        right = kSeparatorSpace * em;
    }
    if (left > 0) {
        box.ink.translate(left, 0);
    }
    box.width += left + right;
    return box;
}

EquationBox fraction(const Context& ctx, const core::EquationNode& node, double em)
{
    const double small = reduced(ctx, em, kFractionScale);
    EquationBox num = layoutNode(ctx, node.children[0], small);
    EquationBox den = layoutNode(ctx, node.children[1], small);
    const double rule = kRule * em;
    const double gap = kGap * em;
    const double axis = kAxis * em;
    const double pad = kFractionPad * em;
    EquationBox box;
    box.width = std::max(num.width, den.width) + 2 * pad;
    num.x = (box.width - num.width) / 2;
    num.baseline = -(axis + rule / 2 + gap + num.descent);
    den.x = (box.width - den.width) / 2;
    den.baseline = -(axis - rule / 2) + gap + den.ascent;
    box.ascent = -(num.baseline - num.ascent);
    box.descent = den.baseline + den.descent;
    box.ink.addRect(QRectF(pad / 2, -(axis + rule / 2), box.width - pad, rule));
    box.children.push_back(std::move(num));
    box.children.push_back(std::move(den));
    return box;
}

// Exposant et indice à droite de la base : sup (ou nullptr), sub (ou nullptr).
EquationBox scripts(const Context& ctx, const core::EquationNode& baseNode, const core::EquationNode* subNode,
                    const core::EquationNode* supNode, double em)
{
    const double small = reduced(ctx, em, kScriptScale);
    EquationBox base = layoutNode(ctx, baseNode, em);
    EquationBox box;
    box.ascent = base.ascent;
    box.descent = base.descent;
    const double at = base.width + kScriptGap * em;
    double scriptsWidth = 0;
    std::optional<EquationBox> sub;
    std::optional<EquationBox> sup;
    if (supNode) {
        sup = layoutNode(ctx, *supNode, small);
        sup->x = at;
        sup->baseline = -std::max(0.4 * em, base.ascent - 0.5 * sup->ascent);
        scriptsWidth = std::max(scriptsWidth, sup->width);
    }
    if (subNode) {
        sub = layoutNode(ctx, *subNode, small);
        sub->x = at;
        double down = std::max(0.25 * em, base.descent);
        if (sup) {
            // Au moins 0,1 em entre le bas de l'exposant et le haut de l'indice.
            down = std::max(down, sup->baseline + sup->descent + 0.1 * em + sub->ascent);
        }
        sub->baseline = down;
        scriptsWidth = std::max(scriptsWidth, sub->width);
    }
    box.width = at + scriptsWidth;
    box.children.push_back(std::move(base));
    if (sub) {
        box.ascent = std::max(box.ascent, -(sub->baseline - sub->ascent));
        box.descent = std::max(box.descent, sub->baseline + sub->descent);
        box.children.push_back(std::move(*sub));
    }
    if (sup) {
        box.ascent = std::max(box.ascent, -(sup->baseline - sup->ascent));
        box.descent = std::max(box.descent, sup->baseline + sup->descent);
        box.children.push_back(std::move(*sup));
    }
    return box;
}

// Racine : signe tracé (petit trait montant, descente, montée jusqu'à la barre), barre au-dessus du contenu,
// indice éventuel au-dessus du petit trait.
EquationBox radical(const Context& ctx, EquationBox content, const core::EquationNode* indexNode, double em)
{
    const double rule = kRule * em;
    const double top = -(content.ascent + kGap * em + rule);
    const double bottom = content.descent + 0.05 * em;
    const double height = bottom - top;
    const double tickY = bottom - std::min(0.42 * height, 0.5 * em);
    const double valley = 0.3 * em;
    std::optional<EquationBox> index;
    double shift = 0;
    if (indexNode) {
        index = layoutNode(ctx, *indexNode, reduced(ctx, em, kIndexScale));
        shift = std::max(0.0, index->width + 0.05 * em - valley);
        index->x = shift + valley - 0.05 * em - index->width;
        index->baseline = tickY - 0.1 * em - index->descent;
    }
    EquationBox box;
    content.x = shift + kSign * em;
    content.baseline = 0;
    box.width = content.x + content.width + kRootPad * em;
    QPainterPath line;
    line.moveTo(shift + 0.02 * em, tickY);
    line.lineTo(shift + 0.14 * em, tickY - 0.07 * em);
    line.lineTo(shift + valley, bottom - rule / 2);
    line.lineTo(shift + kSign * em - 0.02 * em, top + rule / 2);
    line.lineTo(box.width - kRootPad * em / 2, top + rule / 2);
    QPainterPathStroker stroker;
    stroker.setWidth(rule);
    stroker.setCapStyle(Qt::FlatCap);
    stroker.setJoinStyle(Qt::MiterJoin);
    box.ink = stroker.createStroke(line);
    box.ascent = -top;
    box.descent = bottom;
    box.children.push_back(std::move(content));
    if (index) {
        box.ascent = std::max(box.ascent, -(index->baseline - index->ascent));
        box.children.push_back(std::move(*index));
    }
    return box;
}

// Bornes au-dessous et au-dessus de la base, centrées (under ou over : nullptr si absente).
EquationBox limits(const Context& ctx, const core::EquationNode& baseNode,
                   const core::EquationNode* underNode, const core::EquationNode* overNode, double em)
{
    const double small = reduced(ctx, em, kScriptScale);
    const double gap = kLimitGap * em;
    EquationBox base = layoutNode(ctx, baseNode, em);
    std::optional<EquationBox> under;
    std::optional<EquationBox> over;
    EquationBox box;
    box.width = base.width;
    if (underNode) {
        under = layoutNode(ctx, *underNode, small);
        box.width = std::max(box.width, under->width);
    }
    if (overNode) {
        over = layoutNode(ctx, *overNode, small);
        box.width = std::max(box.width, over->width);
    }
    base.x = (box.width - base.width) / 2;
    box.ascent = base.ascent;
    box.descent = base.descent;
    if (under) {
        under->x = (box.width - under->width) / 2;
        under->baseline = base.descent + gap + under->ascent;
        box.descent = under->baseline + under->descent;
    }
    if (over) {
        over->x = (box.width - over->width) / 2;
        over->baseline = -(base.ascent + gap + over->descent);
        box.ascent = -(over->baseline - over->ascent);
    }
    box.children.push_back(std::move(base));
    if (under) {
        box.children.push_back(std::move(*under));
    }
    if (over) {
        box.children.push_back(std::move(*over));
    }
    return box;
}

EquationBox cell(const Context& ctx, const core::EquationNode& node, double em)
{
    EquationBox box = node.kind == Kind::TableCell ? row(ctx, node.children, em) : layoutNode(ctx, node, em);
    box.cursor = box.cursor || hasClass(node, kCursorClass);
    return box;
}

// Matrice : colonnes alignées (cellules centrées), lignes empilées, centrée sur l'axe.
EquationBox table(const Context& ctx, const core::EquationNode& node, double em)
{
    std::vector<std::vector<EquationBox>> cells;
    std::vector<const core::EquationNode*> rowNodes;
    for (const core::EquationNode& rowNode : node.children) {
        std::vector<EquationBox> line;
        if (rowNode.kind == Kind::TableRow) {
            for (const core::EquationNode& cellNode : rowNode.children) {
                line.push_back(cell(ctx, cellNode, em));
            }
        } else {
            line.push_back(cell(ctx, rowNode, em));
        }
        if (line.empty()) {
            line.push_back(emptyBox(em));
        }
        cells.push_back(std::move(line));
        rowNodes.push_back(&rowNode);
    }
    if (cells.empty()) {
        return emptyBox(em);
    }
    std::vector<double> columns;
    for (const auto& line : cells) {
        if (columns.size() < line.size()) {
            columns.resize(line.size(), 0.0);
        }
        for (std::size_t j = 0; j < line.size(); ++j) {
            columns[j] = std::max(columns[j], line[j].width);
        }
    }
    std::vector<double> left(columns.size(), 0.0);
    double width = 0;
    for (std::size_t j = 0; j < columns.size(); ++j) {
        left[j] = width;
        width += columns[j] + (j + 1 < columns.size() ? kColumnGap * em : 0.0);
    }
    double height = 0;
    std::vector<double> ascents;
    std::vector<double> tops;
    for (const auto& line : cells) {
        double ascent = 0;
        double descent = 0;
        for (const EquationBox& c : line) {
            ascent = std::max(ascent, c.ascent);
            descent = std::max(descent, c.descent);
        }
        if (!tops.empty()) {
            height += kRowGap * em;
        }
        tops.push_back(height);
        ascents.push_back(ascent);
        height += ascent + descent;
    }
    EquationBox box;
    box.width = width;
    const double top = -(height / 2 + kAxis * em);
    box.ascent = -top;
    box.descent = height / 2 - kAxis * em;
    for (std::size_t i = 0; i < cells.size(); ++i) {
        EquationBox line;
        line.width = width;
        line.baseline = top + tops[i] + ascents[i];
        line.cursor = hasClass(*rowNodes[i], kCursorClass);
        for (std::size_t j = 0; j < cells[i].size(); ++j) {
            EquationBox& c = cells[i][j];
            c.x = left[j] + (columns[j] - c.width) / 2;
            c.baseline = 0;
            line.ascent = std::max(line.ascent, c.ascent);
            line.descent = std::max(line.descent, c.descent);
            line.children.push_back(std::move(c));
        }
        box.children.push_back(std::move(line));
    }
    return box;
}

bool isSingleCharacter(const std::string& text)
{
    const QString string = QString::fromStdString(text);
    return string.size() == 1 || (string.size() == 2 && string.at(0).isHighSurrogate());
}

EquationBox layoutNode(const Context& ctx, const core::EquationNode& node, double em)
{
    const auto& c = node.children;
    EquationBox box;
    switch (node.kind) {
    case Kind::Identifier:
        box = token(ctx, node.text, em, isSingleCharacter(node.text));
        break;
    case Kind::Number:
    case Kind::Text:
        box = token(ctx, node.text, em, false);
        break;
    case Kind::Operator:
        box = operatorToken(ctx, node.text, em);
        break;
    case Kind::Fraction:
        box = c.size() == 2 ? fraction(ctx, node, em) : row(ctx, c, em);
        break;
    case Kind::Sup:
        box = c.size() == 2 ? scripts(ctx, c[0], nullptr, &c[1], em) : row(ctx, c, em);
        break;
    case Kind::Sub:
        box = c.size() == 2 ? scripts(ctx, c[0], &c[1], nullptr, em) : row(ctx, c, em);
        break;
    case Kind::SubSup:
        box = c.size() == 3 ? scripts(ctx, c[0], &c[1], &c[2], em) : row(ctx, c, em);
        break;
    case Kind::Sqrt:
        box = radical(ctx, row(ctx, c, em), nullptr, em);
        break;
    case Kind::Root:
        box = c.size() == 2 ? radical(ctx, layoutNode(ctx, c[0], em), &c[1], em) : row(ctx, c, em);
        break;
    case Kind::UnderOver:
        box = c.size() == 3 ? limits(ctx, c[0], &c[1], &c[2], em) : row(ctx, c, em);
        break;
    case Kind::Under:
        box = c.size() == 2 ? limits(ctx, c[0], &c[1], nullptr, em) : row(ctx, c, em);
        break;
    case Kind::Over:
        box = c.size() == 2 ? limits(ctx, c[0], nullptr, &c[1], em) : row(ctx, c, em);
        break;
    case Kind::Table:
        box = table(ctx, node, em);
        break;
    case Kind::Placeholder:
        box = emptyBox(em);
        box.placeholder = true;
        break;
    case Kind::Raw:
        break; // élément inconnu de cette version : gardé dans le texte, rien à dessiner
    case Kind::Math:
    case Kind::Row:
    case Kind::TableRow:
    case Kind::TableCell:
        box = row(ctx, c, em);
        break;
    }
    box.cursor = box.cursor || hasClass(node, kCursorClass);
    return box;
}

// Équation mise en page, aplatie en coordonnées de la racine : ce que le cache garde.
struct Drawing
{
    QPainterPath ink;
    std::vector<QRectF> placeholders;
    std::vector<QRectF> cursors;
    QRectF frame;
};

void flatten(const EquationBox& box, QPointF origin, Drawing& out)
{
    const QPointF at = origin + QPointF(box.x, box.baseline);
    if (!box.ink.isEmpty()) {
        out.ink.addPath(box.ink.translated(at));
    }
    const QRectF rect(at.x(), at.y() - box.ascent, box.width, box.ascent + box.descent);
    if (box.cursor) {
        out.cursors.push_back(rect);
    }
    if (box.placeholder) {
        const double inset = 0.08 * std::min(rect.width(), rect.height());
        out.placeholders.push_back(rect.adjusted(inset, inset, -inset, -inset));
    }
    for (const EquationBox& child : box.children) {
        flatten(child, at, out);
    }
}

double pointSizeOf(const core::Equation& equation)
{
    return std::isfinite(equation.pointSize) && equation.pointSize > 0 ? equation.pointSize : 24.0;
}

struct CacheEntry
{
    double pointSize = 0;
    std::optional<Drawing> drawing; // vide : MathML illisible
};

// Un cache par fil (peintre partagé : fil de rendu, miniatures, exports) : aucun objet partagé entre fils,
// aucun verrou. Le MathML n'est relu et remis en page que si son texte (clé : std::hash du texte) ou sa
// taille change, jamais à chaque image.
const Drawing* drawingOf(const core::Equation& equation)
{
    thread_local std::unordered_map<std::string, CacheEntry> cache;
    const double pointSize = pointSizeOf(equation);
    auto it = cache.find(equation.mathml);
    if (it != cache.end() && it->second.pointSize == pointSize) {
        return it->second.drawing ? &*it->second.drawing : nullptr;
    }
    if (it == cache.end()) {
        if (cache.size() >= kCacheEntries) {
            cache.clear();
        }
        it = cache.emplace(equation.mathml, CacheEntry{}).first;
    }
    CacheEntry& entry = it->second;
    entry.pointSize = pointSize;
    entry.drawing.reset();
    QString error;
    const std::optional<core::EquationNode> root = core::parseMathml(equation.mathml, error);
    if (root) {
        const EquationBox box = layoutEquation(*root, equationFont(pointSize));
        Drawing drawing;
        drawing.ink.setFillRule(Qt::WindingFill);
        flatten(box, QPointF(), drawing);
        drawing.frame = QRectF(0, -box.ascent, box.width, box.ascent + box.descent);
        entry.drawing = std::move(drawing);
    }
    return entry.drawing ? &*entry.drawing : nullptr;
}

// Repère neutre de paintUnknown (pagepainter.cpp) sur la boîte de l'équation : cadre gris pointillé d'un
// pixel quel que soit le zoom (plume cosmétique).
void paintNeutralFrame(QPainter& painter, const core::RectF& b)
{
    QPen pen(QColor(0x80, 0x80, 0x80), 0, Qt::DashLine);
    pen.setCosmetic(true);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(QRectF(b.x, b.y, b.width, b.height));
}

} // namespace

QFont equationFont(double pointSize)
{
    const double points = std::isfinite(pointSize) && pointSize > 0 ? pointSize : 24.0;
    QFont font;
    font.setFamilies(
        {QStringLiteral("Times New Roman"), QStringLiteral("Cambria Math"), QStringLiteral("DejaVu Serif")});
    font.setStyleHint(QFont::Serif, QFont::PreferOutline);
    font.setPixelSize(std::clamp(int(std::lround(points * 96.0 / 72.0)), 1, 4096));
    return font;
}

EquationBox layoutEquation(const core::EquationNode& node, const QFont& font)
{
    Context ctx;
    ctx.font = font;
    if (font.pixelSize() > 0) {
        ctx.baseEm = font.pixelSize();
    } else if (font.pointSizeF() > 0) {
        ctx.baseEm = font.pointSizeF() * 96.0 / 72.0;
    } else {
        ctx.baseEm = 32;
    }
    ctx.font.setPixelSize(kReferencePixels);
    return layoutNode(ctx, node, ctx.baseEm);
}

// Mise à l'échelle dans la boîte de l'équation (même rapport, centrée) ; boîte vide : à sa taille, en son
// coin. Mode Editor : surlignage du curseur sous les glyphes, cadre pointillé des cases vides au-dessus.
void paintEquation(QPainter& painter, const core::Equation& equation, EquationPaintMode mode)
{
    const Drawing* drawing = drawingOf(equation);
    if (!drawing) {
        paintNeutralFrame(painter, equation.bounds);
        return;
    }
    const QRectF& frame = drawing->frame;
    if (!(frame.width() > 0 && frame.height() > 0)) {
        return;
    }
    const core::RectF& b = equation.bounds;
    double scale = 1;
    QPointF at(b.x, b.y);
    if (std::isfinite(b.width) && std::isfinite(b.height) && b.width > 0 && b.height > 0) {
        scale = std::min(b.width / frame.width(), b.height / frame.height());
        at += QPointF((b.width - frame.width() * scale) / 2, (b.height - frame.height() * scale) / 2);
    }
    if (!std::isfinite(at.x()) || !std::isfinite(at.y()) || !std::isfinite(scale)) {
        return;
    }
    painter.save();
    QTransform t;
    t.translate(at.x(), at.y());
    t.scale(scale, scale);
    t.translate(-frame.left(), -frame.top());
    painter.setTransform(t, true);
    if (mode == EquationPaintMode::Editor) {
        for (const QRectF& r : drawing->cursors) {
            painter.fillRect(r, QColor(0x33, 0x99, 0xFF, 0x55));
        }
    }
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor::fromRgba(equation.color));
    painter.drawPath(drawing->ink);
    if (mode == EquationPaintMode::Editor && !drawing->placeholders.empty()) {
        QPen pen(QColor(0x80, 0x80, 0x80), 0, Qt::DotLine);
        pen.setCosmetic(true);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        for (const QRectF& r : drawing->placeholders) {
            painter.drawRect(r);
        }
    }
    painter.restore();
}

} // namespace actcraft::render
