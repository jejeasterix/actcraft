#include <actcraft/core/equation.h>

#include <QStringConverter>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <algorithm>

namespace actcraft::core {

using namespace Qt::StringLiterals;

namespace {

using Kind = EquationNode::Kind;
using Token = QXmlStreamReader::TokenType;
using Attributes = decltype(EquationNode::attributes);

constexpr int kFreeArity = -1;

struct Known
{
    QLatin1StringView name;
    Kind kind;
    int arity; // nombre d'enfants exigé, ou kFreeArity
};

// Éléments lus en arbre. Tout autre élément (mspace, mstyle, menclose, semantics…) est gardé tel qu'écrit
// (Raw).
constexpr Known kKnown[] = {
    {"math"_L1, Kind::Math, kFreeArity},
    {"mrow"_L1, Kind::Row, kFreeArity},
    {"mi"_L1, Kind::Identifier, kFreeArity},
    {"mn"_L1, Kind::Number, kFreeArity},
    {"mo"_L1, Kind::Operator, kFreeArity},
    {"mtext"_L1, Kind::Text, kFreeArity},
    {"mfrac"_L1, Kind::Fraction, 2},
    {"msup"_L1, Kind::Sup, 2},
    {"msub"_L1, Kind::Sub, 2},
    {"msubsup"_L1, Kind::SubSup, 3},
    {"msqrt"_L1, Kind::Sqrt, kFreeArity},
    {"mroot"_L1, Kind::Root, 2},
    {"munderover"_L1, Kind::UnderOver, 3},
    {"munder"_L1, Kind::Under, 2},
    {"mover"_L1, Kind::Over, 2},
    {"mtable"_L1, Kind::Table, kFreeArity},
    {"mtr"_L1, Kind::TableRow, kFreeArity},
    {"mtd"_L1, Kind::TableCell, kFreeArity},
};

const Known* knownByName(QStringView name)
{
    for (const Known& known : kKnown) {
        if (name == known.name)
            return &known;
    }
    return nullptr;
}

QLatin1StringView elementName(Kind kind)
{
    for (const Known& known : kKnown) {
        if (known.kind == kind)
            return known.name;
    }
    return "mrow"_L1; // Placeholder (Raw ne s'écrit pas par son nom)
}

bool isToken(Kind kind)
{
    return kind == Kind::Identifier || kind == Kind::Number || kind == Kind::Operator || kind == Kind::Text;
}

QLatin1StringView placeholderClass()
{
    return QLatin1StringView(kPlaceholderClass.data(), qsizetype(kPlaceholderClass.size()));
}

// Retire la marque de case vide de la liste class : ce seul jeton, les autres classes restant séparées d'un
// blanc, et l'attribut disparaissant s'il n'en reste aucune. Faux, sans rien changer, si la marque n'y est
// pas.
bool takePlaceholderClass(Attributes& attributes)
{
    const auto attribute =
        std::find_if(attributes.begin(), attributes.end(), [](const auto& a) { return a.first == "class"; });
    if (attribute == attributes.end())
        return false;
    std::vector<std::string> tokens = classTokens(attribute->second);
    const auto mark = std::find(tokens.begin(), tokens.end(), kPlaceholderClass);
    if (mark == tokens.end())
        return false;
    tokens.erase(mark);
    if (tokens.empty()) {
        attributes.erase(attribute);
        return true;
    }
    std::string rest = tokens.front();
    for (std::size_t i = 1; i < tokens.size(); ++i) {
        rest += ' ';
        rest += tokens[i];
    }
    attribute->second = std::move(rest);
    return true;
}

// Texte UTF-8 de l'arbre décodé pour l'écriture ; un U+FEFF de tête y est un caractère comme un autre
// (QString::fromStdString le sauterait). nullopt si l'UTF-8 est invalide.
std::optional<QString> decodeUtf8(const std::string& text)
{
    QStringDecoder decoder(QStringConverter::Utf8,
                           QStringConverter::Flag::Stateless | QStringConverter::Flag::ConvertInitialBom);
    QString decoded = decoder.decode(QByteArrayView(text.data(), qsizetype(text.size())));
    if (decoder.hasError())
        return std::nullopt;
    return decoded;
}

// Lecteur récursif sur QXmlStreamReader, nourri de la QString décodée : characterOffset() y compte des unités
// UTF-16.
class Reader
{
public:
    explicit Reader(const QString& source) : source_(source), xml_(source)
    {
        // Sans traitement des espaces de noms, xmlns reste un attribut comme un autre, gardé et réécrit.
        xml_.setNamespaceProcessing(false);
    }

    const QString& error() const { return error_; }

    std::optional<EquationNode> read()
    {
        std::optional<EquationNode> root;
        while (!xml_.atEnd()) {
            const Token t = next();
            if (t == Token::StartElement) {
                if (root)
                    return fail(u"plus d'une racine"_s);
                if (xml_.qualifiedName() != "math"_L1)
                    return fail(u"la racine doit être <math>, lu <%1>"_s.arg(xml_.qualifiedName()));
                root = element(start_, 1);
                if (!root)
                    return std::nullopt;
            } else if (t == Token::DTD) {
                return fail(u"DTD refusée (aucune entité n'est déclarée ni lue)"_s);
            } else if (t == Token::Characters && !xml_.isWhitespace()) {
                return fail(u"texte hors de <math>"_s);
            } else if (t == Token::EntityReference) {
                return fail(u"entité non résolue : %1"_s.arg(xml_.name()));
            }
            // Déclaration XML, commentaires, instructions de traitement, blancs : rien à lire.
        }
        if (xml_.hasError())
            return failXml();
        if (!root)
            return fail(u"aucun élément <math>"_s);
        return root;
    }

private:
    // Lit le jeton suivant ; start_ garde la position d'avant, fin du jeton précédent, donc début de
    // celui-ci.
    Token next()
    {
        start_ = xml_.characterOffset();
        return xml_.readNext();
    }

    std::nullopt_t fail(const QString& message)
    {
        error_ = u"MathML : "_s + message;
        return std::nullopt;
    }

    std::nullopt_t failXml()
    {
        return fail(u"%1 (ligne %2, colonne %3)"_s.arg(xml_.errorString())
                        .arg(xml_.lineNumber())
                        .arg(xml_.columnNumber()));
    }

    std::nullopt_t failTooDeep()
    {
        return fail(u"plus de %1 niveaux d'imbrication"_s.arg(kMaxEquationDepth));
    }

    // Position dans source_ du « < » de l'élément dont xml_ vient de rendre le StartElement, ou -1. `before`
    // est la position relevée avant ce jeton : exacte après une balise, mais un caractère plus loin après un
    // blanc (jeton Characters), le lecteur ayant déjà lu le « < » qui le termine, et un caractère en retard
    // quand le texte commence par un U+FEFF que le lecteur saute sans le compter. Le nom qui suit le « < » le
    // confirme.
    qint64 tagStart(qint64 before) const
    {
        const QStringView name = xml_.qualifiedName();
        const qsizetype at = source_.indexOf(u'<', qMax(qint64(0), before - 1));
        if (at < 0)
            return -1;
        const QStringView tag = QStringView(source_).mid(at + 1);
        if (!tag.startsWith(name) || tag.size() <= name.size())
            return -1;
        const QChar follower = tag[name.size()];
        return follower.isSpace() || follower == u'/' || follower == u'>' ? at : -1;
    }

    // Position dans source_ juste après la fin de l'élément dont le « < » est en `start` et dont xml_ vient
    // de rendre le dernier EndElement, ou -1. Comme pour tagStart, la position du lecteur peut avoir un
    // caractère de retard : le « > » se cherche vers l'avant depuis cette position moins un, puis la fin est
    // confirmée par le nom : </nom> (blancs permis avant le « > »), ou /> pour un élément `empty`, sans
    // aucun jeton entre sa balise ouvrante et sa fin.
    qint64 rawEnd(qint64 start, bool empty) const
    {
        if (start < 0)
            return -1;
        const qsizetype at = source_.indexOf(u'>', qMax(start, xml_.characterOffset() - 1));
        if (at < 0)
            return -1;
        const QStringView slice = QStringView(source_).mid(start, at + 1 - start);
        if (!slice.startsWith(u'<'))
            return -1;
        qsizetype length = 1;
        while (length < slice.size() && !slice[length].isSpace() && slice[length] != u'/' &&
               slice[length] != u'>')
            ++length;
        const QStringView name = slice.mid(1, length - 1);
        if (name.isEmpty())
            return -1;
        const QStringView closing = slice.chopped(1).trimmed();
        if (closing.endsWith(name) && closing.chopped(name.size()).endsWith(u"</"))
            return at + 1;
        return empty && slice.endsWith(u"/>") ? at + 1 : -1;
    }

    // xml_ vient de rendre le StartElement de l'élément ; `start` est la position relevée avant ce jeton.
    std::optional<EquationNode> element(qint64 start, int depth)
    {
        if (depth > kMaxEquationDepth)
            return failTooDeep();
        const Known* known = knownByName(xml_.qualifiedName());
        if (!known)
            return raw(tagStart(start), depth, 1);
        if (known->kind == Kind::Math && depth > 1)
            return fail(u"<math> imbriqué"_s);
        EquationNode node;
        node.kind = known->kind;
        for (const QXmlStreamAttribute& attribute : xml_.attributes()) {
            node.attributes.emplace_back(attribute.qualifiedName().toUtf8().toStdString(),
                                         attribute.value().toUtf8().toStdString());
        }
        if (isToken(node.kind))
            return token(std::move(node), tagStart(start), depth);
        while (true) {
            const Token t = next();
            if (t == Token::StartElement) {
                std::optional<EquationNode> child = element(start_, depth + 1);
                if (!child)
                    return std::nullopt;
                node.children.push_back(std::move(*child));
            } else if (t == Token::EndElement) {
                return finish(std::move(node), *known);
            } else if (t == Token::Characters) {
                if (!xml_.isWhitespace())
                    return fail(u"texte dans <%1>, hors d'un élément jeton"_s.arg(known->name));
            } else if (t == Token::Invalid) {
                return failXml();
            } else if (t != Token::Comment && t != Token::ProcessingInstruction) {
                return fail(u"jeton XML inattendu dans <%1>"_s.arg(known->name));
            }
        }
    }

    // Élément jeton (mi, mn, mo, mtext) : son texte, références de caractères décodées.
    std::optional<EquationNode> token(EquationNode node, qint64 start, int depth)
    {
        QString text;
        while (true) {
            const Token t = next();
            if (t == Token::Characters) {
                text += xml_.text();
            } else if (t == Token::EndElement) {
                node.text = text.toStdString();
                return node;
            } else if (t == Token::StartElement) {
                // Un élément dans un jeton (mglyph…) n'est pas de cette version : le jeton entier est gardé
                // tel quel.
                return raw(start, depth, 2);
            } else if (t == Token::Invalid) {
                return failXml();
            } else if (t != Token::Comment && t != Token::ProcessingInstruction) {
                return fail(u"jeton XML inattendu dans un élément jeton"_s);
            }
        }
    }

    // Garde tel qu'écrit l'élément dont le « < » est en `start`, `open` éléments étant ouverts depuis lui
    // (lui compris). La tranche se prend dans source_, puis se reconvertit en UTF-8 : characterOffset()
    // compte des unités UTF-16, et une tranche des octets UTF-8 d'origine serait décalée par tout caractère
    // non ASCII qui la précède.
    std::optional<EquationNode> raw(qint64 start, int depth, int open)
    {
        bool empty = open == 1; // aucun jeton lu entre sa balise ouvrante et sa fin
        while (open > 0) {
            if (depth + open - 1 > kMaxEquationDepth)
                return failTooDeep();
            const Token t = next();
            if (t != Token::EndElement)
                empty = false;
            if (t == Token::StartElement)
                ++open;
            else if (t == Token::EndElement)
                --open;
            else if (t == Token::Invalid)
                return failXml();
            else if (t == Token::EntityReference)
                return fail(u"entité non résolue : %1"_s.arg(xml_.name()));
        }
        const qint64 end = rawEnd(start, empty);
        if (end < 0)
            return fail(u"position incohérente d'un élément inconnu"_s);
        EquationNode node;
        node.kind = Kind::Raw;
        node.text = QStringView(source_).mid(start, end - start).toUtf8().toStdString();
        return node;
    }

    // Arité et place des lignes et cellules de tableau ; la case vide marquée de l'éditeur devient
    // Placeholder.
    std::optional<EquationNode> finish(EquationNode node, const Known& known)
    {
        if (known.arity != kFreeArity && node.children.size() != std::size_t(known.arity)) {
            return fail(u"<%1> attend %2 enfants, lu %3"_s.arg(known.name)
                            .arg(known.arity)
                            .arg(qsizetype(node.children.size())));
        }
        for (const EquationNode& child : node.children) {
            if (child.kind == Kind::Raw)
                continue;
            const bool row = child.kind == Kind::TableRow;
            const bool cell = child.kind == Kind::TableCell;
            if (row != (node.kind == Kind::Table) || cell != (node.kind == Kind::TableRow))
                return fail(u"<%1> : mtable ne contient que des mtr, et mtr que des mtd"_s.arg(known.name));
        }
        if (node.kind == Kind::Row && node.children.empty() && takePlaceholderClass(node.attributes))
            node.kind = Kind::Placeholder;
        return node;
    }

    const QString& source_;
    QXmlStreamReader xml_;
    qint64 start_ = 0;
    QString error_;
};

// Écrivain canonique sur QXmlStreamWriter, sans indentation ; un Raw est recopié tel quel dans la sortie.
class Writer
{
public:
    // Chaîne vide si l'arbre ne s'écrit pas en XML 1.0 (UTF-8 invalide, caractère interdit).
    std::string write(const EquationNode& root)
    {
        if (!node(root) || xml_.hasError())
            return {};
        return out_.toStdString();
    }

private:
    bool node(const EquationNode& n)
    {
        if (n.kind == Kind::Raw) {
            const std::optional<QString> raw = decodeUtf8(n.text);
            if (!raw)
                return false;
            xml_.writeCharacters(QString()); // ferme la balise ouvrante en attente avant la recopie
            out_ += *raw;
            return true;
        }
        xml_.writeStartElement(elementName(n.kind));
        const bool placeholder = n.kind == Kind::Placeholder;
        if (placeholder) {
            // Un seul attribut class : la marque, puis les autres classes du nœud.
            QString classes(placeholderClass());
            for (const auto& [name, value] : n.attributes) {
                if (name != "class" || value.empty())
                    continue;
                const std::optional<QString> more = decodeUtf8(value);
                if (!more)
                    return false;
                classes += u' ';
                classes += *more;
            }
            xml_.writeAttribute("class"_L1, classes);
        }
        for (const auto& [name, value] : n.attributes) {
            if (placeholder && name == "class")
                continue;
            const std::optional<QString> qualifiedName = decodeUtf8(name);
            const std::optional<QString> decoded = decodeUtf8(value);
            if (!qualifiedName || !decoded)
                return false;
            xml_.writeAttribute(*qualifiedName, *decoded);
        }
        if (isToken(n.kind) && !n.text.empty()) {
            const std::optional<QString> text = decodeUtf8(n.text);
            if (!text)
                return false;
            characters(*text);
        }
        for (const EquationNode& child : n.children) {
            if (!node(child))
                return false;
        }
        xml_.writeEndElement();
        return true;
    }

    // Texte d'un jeton ; un retour chariot s'écrit &#13; : écrit brut, la relecture en ferait un saut de
    // ligne.
    void characters(const QString& text)
    {
        qsizetype from = 0;
        for (qsizetype at = text.indexOf(u'\r'); at >= 0; at = text.indexOf(u'\r', from)) {
            xml_.writeCharacters(QStringView(text).mid(from, at - from));
            xml_.writeEntityReference("#13"_L1);
            from = at + 1;
        }
        xml_.writeCharacters(QStringView(text).mid(from));
    }

    QString out_;
    QXmlStreamWriter xml_{&out_};
};

} // namespace

std::optional<EquationNode> parseMathml(const std::string& mathml, QString& error)
{
    error.clear();
    if (mathml.size() > kMaxMathmlBytes) {
        error = u"MathML : %1 octets, au plus %2"_s.arg(qulonglong(mathml.size()))
                    .arg(qulonglong(kMaxMathmlBytes));
        return std::nullopt;
    }
    // Sans état : une séquence incomplète en fin de texte est une erreur, pas un reste attendu.
    QStringDecoder decoder(QStringConverter::Utf8, QStringConverter::Flag::Stateless);
    const QString source = decoder.decode(QByteArrayView(mathml.data(), qsizetype(mathml.size())));
    if (decoder.hasError()) {
        error = u"MathML : UTF-8 invalide"_s;
        return std::nullopt;
    }
    Reader reader(source);
    std::optional<EquationNode> tree = reader.read();
    if (!tree)
        error = reader.error();
    return tree;
}

std::string toMathml(const EquationNode& root)
{
    return Writer().write(root);
}

std::vector<std::string> classTokens(const std::string& value)
{
    constexpr std::string_view blanks = " \t\n\r\f";
    std::vector<std::string> tokens;
    for (std::size_t at = value.find_first_not_of(blanks); at != std::string::npos;
         at = value.find_first_not_of(blanks, at)) {
        const std::size_t end = std::min(value.find_first_of(blanks, at), value.size());
        tokens.push_back(value.substr(at, end - at));
        at = end;
    }
    return tokens;
}

bool hasClassToken(const EquationNode& node, std::string_view token)
{
    for (const auto& [name, value] : node.attributes) {
        if (name != "class")
            continue;
        const std::vector<std::string> tokens = classTokens(value);
        if (std::find(tokens.begin(), tokens.end(), token) != tokens.end())
            return true;
    }
    return false;
}

void addClassToken(EquationNode& node, std::string_view token)
{
    for (auto& [name, value] : node.attributes) {
        if (name == "class") {
            if (!value.empty())
                value += ' ';
            value += token;
            return;
        }
    }
    node.attributes.emplace_back("class", std::string(token));
}

void removeClassToken(EquationNode& node, std::string_view token)
{
    auto& attributes = node.attributes;
    for (auto it = attributes.begin(); it != attributes.end();) {
        std::vector<std::string> tokens;
        if (it->first == "class")
            tokens = classTokens(it->second);
        const auto mark = std::find(tokens.begin(), tokens.end(), token);
        if (mark == tokens.end()) {
            ++it;
            continue;
        }
        tokens.erase(std::remove(tokens.begin(), tokens.end(), token), tokens.end());
        if (tokens.empty()) {
            it = attributes.erase(it);
            continue;
        }
        std::string rest = tokens.front();
        for (std::size_t i = 1; i < tokens.size(); ++i) {
            rest += ' ';
            rest += tokens[i];
        }
        it->second = std::move(rest);
        ++it;
    }
}

} // namespace actcraft::core
