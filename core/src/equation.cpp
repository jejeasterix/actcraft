#include <actcraft/core/equation.h>

#include <QStringConverter>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace actcraft::core {

using namespace Qt::StringLiterals;

namespace {

using Kind = EquationNode::Kind;
using Token = QXmlStreamReader::TokenType;

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

    // Position dans source_ du « < » de l'élément dont xml_ vient de rendre le StartElement, ou -1. `before` est
    // la position relevée avant ce jeton : exacte après une balise, mais un caractère plus loin après un blanc
    // (jeton Characters), le lecteur ayant déjà lu le « < » qui le termine. Le nom qui suit le « < » le confirme.
    qint64 tagStart(qint64 before) const
    {
        const QStringView name = xml_.qualifiedName();
        const qsizetype at = source_.indexOf(u'<', qMax(qint64(0), before - 1));
        if (at < 0)
            return -1;
        const QStringView tag = QStringView(source_).mid(at + 1);
        if (!tag.startsWith(name) || tag.size() <= name.size())
            return -1;
        const QChar next = tag[name.size()];
        return next.isSpace() || next == u'/' || next == u'>' ? at : -1;
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
        while (open > 0) {
            if (depth + open - 1 > kMaxEquationDepth)
                return failTooDeep();
            const Token t = next();
            if (t == Token::StartElement)
                ++open;
            else if (t == Token::EndElement)
                --open;
            else if (t == Token::Invalid)
                return failXml();
            else if (t == Token::EntityReference)
                return fail(u"entité non résolue : %1"_s.arg(xml_.name()));
        }
        // Fin : le dernier « > » lu (un élément vide <x/> peut laisser le lecteur un caractère plus loin).
        const qint64 end = source_.lastIndexOf(u'>', qMin(xml_.characterOffset(), qint64(source_.size())) - 1) + 1;
        if (start < 0 || end <= start)
            return fail(u"position incohérente d'un élément inconnu"_s);
        const QStringView slice = QStringView(source_).mid(start, end - start);
        if (!slice.startsWith(u'<') || !slice.endsWith(u'>'))
            return fail(u"position incohérente d'un élément inconnu"_s);
        EquationNode node;
        node.kind = Kind::Raw;
        node.text = slice.toUtf8().toStdString();
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
        if (node.kind == Kind::Row && node.children.empty() && node.attributes.size() == 1 &&
            node.attributes.front().first == "class" && node.attributes.front().second == kPlaceholderClass) {
            node.kind = Kind::Placeholder;
            node.attributes.clear();
        }
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
    std::string write(const EquationNode& root)
    {
        node(root);
        return out_.toStdString();
    }

private:
    void node(const EquationNode& n)
    {
        if (n.kind == Kind::Raw) {
            xml_.writeCharacters(QString()); // ferme la balise ouvrante en attente avant la recopie
            out_ += QString::fromStdString(n.text);
            return;
        }
        xml_.writeStartElement(elementName(n.kind));
        if (n.kind == Kind::Placeholder)
            xml_.writeAttribute("class"_L1, placeholderClass());
        for (const auto& [name, value] : n.attributes)
            xml_.writeAttribute(QString::fromStdString(name), QString::fromStdString(value));
        if (isToken(n.kind) && !n.text.empty())
            xml_.writeCharacters(QString::fromStdString(n.text));
        for (const EquationNode& child : n.children)
            node(child);
        xml_.writeEndElement();
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

} // namespace actcraft::core
