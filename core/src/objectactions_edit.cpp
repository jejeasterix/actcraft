#include "objectactions_common.h"

#include <actcraft/core/arrange.h>
#include <actcraft/core/clipboardedits.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/objectactions.h>
#include <actcraft/core/objectedits.h>
#include <actcraft/core/objectflags.h>
#include <actcraft/core/replace.h>
#include <actcraft/core/textedits.h>

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QStringList>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

// Actions d'objet d'édition (L8-T8). Aucune commande neuve : chaque action délègue, en une étape, à la
// commande que l'écran emploie déjà pour la même fonction — superposition et couches comme le menu Organiser
// (ui/src/objectediting.cpp, arrange), masquer et verrouiller comme le menu Verrouiller / Masquer (D9 :
// propagés aux membres d'un groupe), translucidité bornée comme le navigateur de propriétés, effacements par
// Effacer ▸, suppression comme l'outil Sélection, duplication par makeDuplicateCommand, texte par
// SetTextCommand et AddObjectCommand comme l'éditeur de texte (ui/src/textediting.cpp). Copier, couper et
// coller passent par le presse-papiers, hors de core (A18) : le moteur ne les connaît pas, l'exécuteur
// (L8-T9) déclenche edit.*.
namespace actcraft::core {

namespace {

using detail::number;

constexpr int kTranslucencyStep = 32;
// Emprise d'un texte neuf quand la cible n'est pas un texte : core ne mesure pas le texte (aucun
// QTextDocument).
constexpr double kCharWidth = 14;
constexpr double kLineHeight = 30;

struct Target
{
    const Page& page;
    const PageObject& object;
    const ActionContext& context;
    QJsonObject parameters;
};

using Builder = std::function<std::unique_ptr<UndoCommand>(const Target&)>;

// Une action, une étape : SetTranslucencyCommand fusionne avec la précédente de sa famille (réglette glissée)
// ; deux clics sur « plus translucide » font ici deux étapes.
class OneStepCommand final : public UndoCommand
{
public:
    explicit OneStepCommand(std::unique_ptr<UndoCommand> inner) : m_inner(std::move(inner)) {}
    void redo(Document& document, Changes& changes) override { m_inner->redo(document, changes); }
    void undo(Document& document, Changes& changes) override { m_inner->undo(document, changes); }

private:
    std::unique_ptr<UndoCommand> m_inner;
};

std::optional<QString> textParameter(const QJsonObject& parameters, const char* name)
{
    const QJsonValue v = parameters.value(QString::fromLatin1(name));
    if (!v.isString()) {
        return std::nullopt;
    }
    return v.toString();
}

ObjectId idOf(const Target& t) noexcept
{
    return t.object.common.id;
}

const Text* asText(const PageObject& object) noexcept
{
    return std::get_if<Text>(&body(object));
}

// ---- Texte : lecture du HTML d'un objet, écriture de texte brut échappé (D8-5) ----

bool isBlank(char c) noexcept
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f';
}

std::string lowered(const std::string& s)
{
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c; });
    return out;
}

// Entités nommées du HTML 4 : Latin-1 (U+00A0 à U+00FF, dans l'ordre des codes), puis les autres courantes.
// Même lecteur que core/src/spellcheck.cpp (espace de noms anonyme, avec ses positions) : duplication
// consignée pour la simplification de fin de lot.
constexpr const char* kLatin1Entities[] = {
    "nbsp",   "iexcl",  "cent",   "pound",  "curren", "yen",    "brvbar", "sect",   "uml",    "copy",
    "ordf",   "laquo",  "not",    "shy",    "reg",    "macr",   "deg",    "plusmn", "sup2",   "sup3",
    "acute",  "micro",  "para",   "middot", "cedil",  "sup1",   "ordm",   "raquo",  "frac14", "frac12",
    "frac34", "iquest", "Agrave", "Aacute", "Acirc",  "Atilde", "Auml",   "Aring",  "AElig",  "Ccedil",
    "Egrave", "Eacute", "Ecirc",  "Euml",   "Igrave", "Iacute", "Icirc",  "Iuml",   "ETH",    "Ntilde",
    "Ograve", "Oacute", "Ocirc",  "Otilde", "Ouml",   "times",  "Oslash", "Ugrave", "Uacute", "Ucirc",
    "Uuml",   "Yacute", "THORN",  "szlig",  "agrave", "aacute", "acirc",  "atilde", "auml",   "aring",
    "aelig",  "ccedil", "egrave", "eacute", "ecirc",  "euml",   "igrave", "iacute", "icirc",  "iuml",
    "eth",    "ntilde", "ograve", "oacute", "ocirc",  "otilde", "ouml",   "divide", "oslash", "ugrave",
    "uacute", "ucirc",  "uuml",   "yacute", "thorn",  "yuml"};
constexpr std::pair<const char*, char32_t> kOtherEntities[] = {
    {"lt", U'<'},          {"gt", U'>'},          {"amp", U'&'},         {"quot", U'"'},
    {"apos", U'\''},       {"OElig", U'\u0152'},  {"oelig", U'\u0153'},  {"Scaron", U'\u0160'},
    {"scaron", U'\u0161'}, {"Yuml", U'\u0178'},   {"circ", U'\u02c6'},   {"tilde", U'\u02dc'},
    {"ensp", U'\u2002'},   {"emsp", U'\u2003'},   {"thinsp", U'\u2009'}, {"ndash", U'\u2013'},
    {"mdash", U'\u2014'},  {"lsquo", U'\u2018'},  {"rsquo", U'\u2019'},  {"sbquo", U'\u201a'},
    {"ldquo", U'\u201c'},  {"rdquo", U'\u201d'},  {"bdquo", U'\u201e'},  {"dagger", U'\u2020'},
    {"Dagger", U'\u2021'}, {"bull", U'\u2022'},   {"hellip", U'\u2026'}, {"permil", U'\u2030'},
    {"lsaquo", U'\u2039'}, {"rsaquo", U'\u203a'}, {"euro", U'\u20ac'},   {"trade", U'\u2122'},
    {"minus", U'\u2212'},
};

// Code d'une entité (sans « & » ni « ; ») ; 0 si elle est inconnue.
char32_t entityCode(const QString& name)
{
    for (std::size_t k = 0; k < std::size(kLatin1Entities); ++k) {
        if (name == QLatin1String(kLatin1Entities[k])) {
            return static_cast<char32_t>(0xA0 + k);
        }
    }
    for (const auto& [entity, code] : kOtherEntities) {
        if (name == QLatin1String(entity)) {
            return code;
        }
    }
    if (name.size() > 1 && name[0] == u'#') {
        const bool hex = name[1] == u'x' || name[1] == u'X';
        bool ok = false;
        const uint code = name.mid(hex ? 2 : 1).toUInt(&ok, hex ? 16 : 10);
        return ok && code > 0 && code <= 0x10FFFF ? static_cast<char32_t>(code) : U'\0';
    }
    return U'\0';
}

QString decodedEntities(const QString& s)
{
    QString out;
    out.reserve(s.size());
    for (qsizetype i = 0; i < s.size(); ++i) {
        const qsizetype semi = s[i] == u'&' ? s.indexOf(u';', i) : -1;
        const char32_t code = semi > i && semi - i <= 10 ? entityCode(s.mid(i + 1, semi - i - 1)) : U'\0';
        if (code == U'\0') {
            out += s[i];
            continue;
        }
        out += QString::fromUcs4(&code, 1);
        i = semi;
    }
    return out;
}

// Parcours du HTML d'un texte : en-tête, styles et scripts sautés ; [first, last) couvre le texte visible (du
// premier au dernier caractère hors blancs, hors balises), npos s'il n'y en a pas ; plain : le texte lu,
// entités décodées, une fin de ligne par <br> et par fin de paragraphe.
struct Scan
{
    std::size_t first = std::string::npos;
    std::size_t last = std::string::npos;
    QString plain;
};

Scan scan(const std::string& html)
{
    Scan s;
    const std::string lower = lowered(html);
    std::string text;
    std::size_t i = 0;
    while (i < html.size()) {
        if (html[i] != '<') {
            const std::size_t next = std::min(html.find('<', i), html.size());
            for (std::size_t k = i; k < next; ++k) {
                if (!isBlank(html[k])) {
                    s.first = std::min(s.first, k);
                    s.last = k + 1;
                }
            }
            text.append(html, i, next - i);
            i = next;
            continue;
        }
        const std::size_t close = html.find('>', i);
        if (close == std::string::npos) {
            break; // balise non fermée : le reste n'est pas du texte
        }
        const bool closing = i + 1 < close && html[i + 1] == '/';
        const std::size_t from = i + (closing ? 2 : 1);
        std::size_t to = from;
        while (to < close && !isBlank(lower[to]) && lower[to] != '/' && lower[to] != '>') {
            ++to;
        }
        const std::string name = lower.substr(from, to - from);
        if (!closing && (name == "head" || name == "style" || name == "script" || name == "title")) {
            const std::size_t end = lower.find("</" + name, close);
            const std::size_t after = end == std::string::npos ? std::string::npos : html.find('>', end);
            i = after == std::string::npos ? html.size() : after + 1;
            continue;
        }
        if (name == "br" || (closing && (name == "p" || name == "div" || name == "li"))) {
            text += '\n';
        }
        i = close + 1;
    }
    s.plain = decodedEntities(QString::fromStdString(text));
    return s;
}

std::string escaped(const QString& plain)
{
    QString html = plain.toHtmlEscaped();
    html.replace(u'\n', QLatin1String("<br />"));
    return html.toStdString();
}

// Le texte visible de `html` remplacé par `plain`, échappé : la mise en forme qui l'ouvre et le ferme est
// gardée.
std::string replacedText(const std::string& html, const QString& plain)
{
    const Scan s = scan(html);
    if (s.first == std::string::npos) {
        return "<p>" + escaped(plain) + "</p>";
    }
    return html.substr(0, s.first) + escaped(plain) + html.substr(s.last);
}

// `plain`, échappé, ajouté après le dernier caractère visible (même mise en forme).
std::string appendedText(const std::string& html, const QString& plain)
{
    const Scan s = scan(html);
    if (s.first == std::string::npos) {
        return replacedText(html, plain);
    }
    return html.substr(0, s.last) + escaped(plain) + html.substr(s.last);
}

QStringList words(const QString& plain)
{
    QStringList out;
    QString word;
    for (const QChar c : plain) {
        if (c.isSpace()) {
            if (!word.isEmpty()) {
                out << word;
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.isEmpty()) {
        out << word;
    }
    return out;
}

// Lignes non vides (blancs des bords retirés) : nombre, et longueur de la plus longue.
std::pair<int, qsizetype> lineShape(const QString& plain)
{
    int lines = 0;
    qsizetype longest = 0;
    for (const QString& line : plain.split(u'\n')) {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            ++lines;
            longest = std::max(longest, trimmed.size());
        }
    }
    return {lines, longest};
}

// Mise en forme d'un texte neuf : ce qui ouvre et ferme le texte visible de la cible si elle est un texte, et
// les tailles d'un caractère et d'une ligne pour estimer son emprise (core ne mesure pas : aucun
// QTextDocument ; l'exécuteur, L8-T9, ou l'éditeur la remesurent).
struct TextFormat
{
    std::string open = "<p>";
    std::string close = "</p>";
    double perChar = kCharWidth;
    double perLine = kLineHeight;
};

// `s` : le parcours du HTML de `source`, fait une fois par l'appelant.
TextFormat formatOf(const Text& source, const Scan& s)
{
    TextFormat f;
    if (s.first != std::string::npos) {
        f.open = source.html.substr(0, s.first);
        f.close = source.html.substr(s.last);
    }
    const auto [lines, longest] = lineShape(s.plain);
    if (source.bounds.width > 0 && source.bounds.height > 0 && lines > 0 && longest > 0) {
        f.perChar = source.bounds.width / static_cast<double>(longest);
        f.perLine = source.bounds.height / lines;
    }
    return f;
}

TextFormat formatOf(const PageObject& object)
{
    const Text* text = asText(object);
    return text != nullptr ? formatOf(*text, scan(text->html)) : TextFormat{};
}

QString numberText(double value)
{
    if (std::abs(value) < 1e15 && value == std::trunc(value)) {
        return QString::number(static_cast<qlonglong>(value));
    }
    return QString::number(value, 'g', 15);
}

// Le texte remplacé ; `runs`, qui fait foi pour le dessin (model.h), est vidé : le HTML écrit est dessiné.
std::unique_ptr<UndoCommand> setText(const Target& t, const Text& from, std::string html)
{
    Text text = from;
    text.html = std::move(html);
    text.runs.clear();
    return std::make_unique<SetTextCommand>(t.context.page, idOf(t), std::move(text));
}

// Textes neufs en colonne sous la cible, sur sa couche ; la colonne remonte si elle sortirait de la page, et
// chaque texte est borné à la page.
std::vector<PageObject> newTexts(const Target& t, const TextFormat& f, const QStringList& plains)
{
    std::vector<PageObject> out;
    double column = 0;
    for (const QString& plain : plains) {
        Text text;
        text.html = f.open + escaped(plain) + f.close;
        const auto [lines, longest] = lineShape(plain);
        text.bounds = {0, 0, std::max(1.0, f.perChar * static_cast<double>(longest)),
                       std::max(1.0, f.perLine * std::max(1, lines))};
        column += kDuplicateOffset + text.bounds.height;
        PageObject object{text};
        object.common.layer = t.object.common.layer;
        out.push_back(std::move(object));
    }
    const RectF box = boundsOnPage(t.page, t.object);
    double y = std::min(box.y + box.height, t.context.pageHeight - column);
    for (PageObject& object : out) {
        const RectF b = std::get<Text>(body(object)).bounds;
        y += kDuplicateOffset;
        const double x = std::clamp(box.x, 0.0, std::max(0.0, t.context.pageWidth - b.width));
        object.common.transform =
            translation(x, std::clamp(y, 0.0, std::max(0.0, t.context.pageHeight - b.height)));
        y += b.height;
    }
    return out;
}

// ---- Superposition, couches, drapeaux, translucidité ----

std::unique_ptr<UndoCommand> arrange(const Target& t, ArrangeMove move)
{
    return std::make_unique<ReorderObjectsCommand>(t.context.page, arrangedOrder(t.page, {idOf(t)}, move));
}

std::unique_ptr<UndoCommand> layer(const Target& t, Layer to)
{
    return std::make_unique<SetLayerCommand>(t.context.page, expandGroups(t.page, {idOf(t)}), to);
}

std::unique_ptr<UndoCommand> toggle(const Target& t, ObjectFlag flag)
{
    std::vector<ObjectId> ids = expandGroups(t.page, {idOf(t)});
    const bool value = !flagOfAll(t.page, ids, flag);
    return std::make_unique<SetObjectFlagCommand>(t.context.page, std::move(ids), flag, value);
}

std::unique_ptr<UndoCommand> translucency(const Target& t, double value)
{
    const double v = std::clamp(std::round(value), 0.0, 255.0);
    return std::make_unique<OneStepCommand>(std::make_unique<SetTranslucencyCommand>(
        t.context.page, std::vector<ObjectId>{idOf(t)}, static_cast<std::uint8_t>(v)));
}

std::unique_ptr<UndoCommand> translucencyStep(const Target& t, int step)
{
    const int now = t.object.common.translucency;
    if ((step < 0 && now == 0) || (step > 0 && now == 255)) {
        return nullptr; // déjà à la borne : rien à faire
    }
    return translucency(t, now + step);
}

std::unique_ptr<UndoCommand> clear(const Target& t, ClearKind kind)
{
    return std::make_unique<ClearPageCommand>(t.context.page, kind);
}

std::unordered_map<std::string, Builder> makeTable()
{
    std::unordered_map<std::string, Builder> table;
    table["object.forward"] = [](const Target& t) { return arrange(t, ArrangeMove::Forward); };
    table["object.backward"] = [](const Target& t) { return arrange(t, ArrangeMove::Backward); };
    table["object.front"] = [](const Target& t) { return arrange(t, ArrangeMove::Front); };
    table["object.back"] = [](const Target& t) { return arrange(t, ArrangeMove::Back); };
    table["object.layer-top"] = [](const Target& t) { return layer(t, Layer::Top); };
    table["object.layer-middle"] = [](const Target& t) { return layer(t, Layer::Middle); };
    table["object.layer-bottom"] = [](const Target& t) { return layer(t, Layer::Bottom); };
    table["object.toggle-hidden"] = [](const Target& t) { return toggle(t, ObjectFlag::Hidden); };
    table["object.toggle-locked"] = [](const Target& t) { return toggle(t, ObjectFlag::Locked); };
    table["object.translucency"] = [](const Target& t) -> std::unique_ptr<UndoCommand> {
        const std::optional<double> value = number(t.parameters, "value");
        if (!value) {
            return nullptr;
        }
        return translucency(t, *value);
    };
    table["object.more-translucent"] = [](const Target& t) { return translucencyStep(t, kTranslucencyStep); };
    table["object.less-translucent"] = [](const Target& t) {
        return translucencyStep(t, -kTranslucencyStep);
    };
    table["object.add-text"] = [](const Target& t) -> std::unique_ptr<UndoCommand> {
        const Text* text = asText(t.object);
        const std::optional<QString> added = textParameter(t.parameters, "text");
        if (text == nullptr || !added) {
            return nullptr;
        }
        return setText(t, *text, appendedText(text->html, *added));
    };
    table["object.set-text"] = [](const Target& t) -> std::unique_ptr<UndoCommand> {
        const Text* text = asText(t.object);
        const std::optional<QString> replacement = textParameter(t.parameters, "text");
        if (text == nullptr || !replacement) {
            return nullptr;
        }
        return setText(t, *text, replacedText(text->html, *replacement));
    };
    table["object.text-value"] = [](const Target& t) -> std::unique_ptr<UndoCommand> {
        // Le texte entier et la valeur sont des nombres (« 5 » puis −2 donne « 3 ») ; sinon rien.
        const Text* text = asText(t.object);
        const std::optional<double> value = number(t.parameters, "value");
        if (text == nullptr || !value) {
            return nullptr;
        }
        // Virgule décimale (« 2,5 ») lue, et gardée dans le résultat.
        QString written = scan(text->html).plain.trimmed();
        const bool comma = written.contains(u',') && !written.contains(u'.');
        if (comma) {
            written.replace(u',', u'.');
        }
        bool ok = false;
        const double now = written.toDouble(&ok);
        if (!ok || !std::isfinite(now) || !std::isfinite(now + *value)) {
            return nullptr;
        }
        QString result = numberText(now + *value);
        if (comma) {
            result.replace(u'.', u',');
        }
        return setText(t, *text, replacedText(text->html, result));
    };
    table["object.new-text"] = [](const Target& t) -> std::unique_ptr<UndoCommand> {
        const std::optional<QString> plain = textParameter(t.parameters, "text");
        if (!plain || plain->trimmed().isEmpty()) {
            return nullptr;
        }
        return std::make_unique<AddObjectCommand>(
            t.context.page, std::move(newTexts(t, formatOf(t.object), {*plain}).front()));
    };
    table["object.extract-text"] = [](const Target& t) -> std::unique_ptr<UndoCommand> {
        // Un objet texte par mot, ajoutés en une étape ; le texte de départ reste.
        const Text* text = asText(t.object);
        if (text == nullptr) {
            return nullptr;
        }
        const Scan s = scan(text->html); // un seul passage du HTML
        const QStringList list = words(s.plain);
        if (list.isEmpty()) {
            return nullptr;
        }
        return std::make_unique<ReplaceObjectsCommand>(t.context.page, std::vector<ObjectId>{},
                                                       newTexts(t, formatOf(*text, s), list));
    };
    table["object.clear-annotations"] = [](const Target& t) { return clear(t, ClearKind::Annotations); };
    table["object.clear-background"] = [](const Target& t) { return clear(t, ClearKind::Background); };
    table["object.clear-grid"] = [](const Target& t) { return clear(t, ClearKind::Grid); };
    table["object.clear-objects"] = [](const Target& t) { return clear(t, ClearKind::Objects); };
    table["object.delete"] = [](const Target& t) -> std::unique_ptr<UndoCommand> {
        return std::make_unique<RemoveObjectsCommand>(t.context.page, expandGroups(t.page, {idOf(t)}));
    };
    table["object.duplicate"] = [](const Target& t) -> std::unique_ptr<UndoCommand> {
        return makeDuplicateCommand(t.page, t.context.page, {idOf(t)});
    };
    return table;
}

const std::unordered_map<std::string, Builder>& editTable()
{
    static const std::unordered_map<std::string, Builder> table = makeTable();
    return table;
}

} // namespace

bool isEditAction(const std::string& id) noexcept
{
    const auto& table = editTable();
    return table.find(id) != table.end();
}

// Appelée par objectActionCommand (objectactions_geometry.cpp). La cible doit être sur la page ; une emprise
// n'est pas exigée (un texte vide se modifie, un objet masqué se réaffiche).
std::unique_ptr<UndoCommand> detail::editActionCommand(const Document& document, const std::string& id,
                                                       const std::string& parameters,
                                                       const ActionContext& context)
{
    const auto& table = editTable();
    const auto builder = table.find(id);
    if (builder == table.end() || context.page >= document.pageCount()) {
        return nullptr;
    }
    const Page& page = document.page(context.page);
    const std::size_t index = detail::targetIndex(page, context);
    if (index >= page.objects.size()) {
        return nullptr;
    }
    const std::optional<QJsonObject> values = detail::parseParameters(parameters);
    if (!values) {
        return nullptr;
    }
    return builder->second(Target{page, page.objects[index], context, *values});
}

} // namespace actcraft::core
