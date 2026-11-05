#include <actcraft/core/spellcheck.h>
#include <actcraft/core/textedits.h>

#include <QByteArray>
#include <QFile>
#include <QString>
#include <QStringList>

#include <algorithm>
#include <utility>

namespace actcraft::core {

namespace {

// Texte brut d'un texte riche : balises retirées, entités décodées, saut de ligne à chaque bloc. Pour chaque
// caractère brut, la tranche [begin, end) du HTML d'où il vient (remplacer un mot récrit cette tranche).
struct Plain
{
    QString text;
    std::vector<int> begin;
    std::vector<int> end;

    void push(QChar c, int b, int e)
    {
        text += c;
        begin.push_back(b);
        end.push_back(e);
    }
    void breakLine(int at)
    {
        if (!text.isEmpty() && text.back() != QLatin1Char('\n')) {
            push(QLatin1Char('\n'), at, at);
        }
    }
};

bool isBlockTag(const QString& name)
{
    static const QStringList blocks{
        QStringLiteral("p"),         QStringLiteral("div"),  QStringLiteral("br"),  QStringLiteral("li"),
        QStringLiteral("ul"),        QStringLiteral("ol"),   QStringLiteral("h1"),  QStringLiteral("h2"),
        QStringLiteral("h3"),        QStringLiteral("h4"),   QStringLiteral("h5"),  QStringLiteral("h6"),
        QStringLiteral("tr"),        QStringLiteral("td"),   QStringLiteral("th"),  QStringLiteral("table"),
        QStringLiteral("body"),      QStringLiteral("html"), QStringLiteral("pre"), QStringLiteral("hr"),
        QStringLiteral("blockquote")};
    return blocks.contains(name);
}

// Code Unicode d'une entité (sans « & » ni « ; ») ; 0 si elle est inconnue.
char32_t entityCode(const QString& name)
{
    if (name == QLatin1String("amp")) {
        return U'&';
    }
    if (name == QLatin1String("lt")) {
        return U'<';
    }
    if (name == QLatin1String("gt")) {
        return U'>';
    }
    if (name == QLatin1String("quot")) {
        return U'"';
    }
    if (name == QLatin1String("apos")) {
        return U'\'';
    }
    if (name == QLatin1String("nbsp")) {
        return U' ';
    }
    if (name.size() > 1 && name[0] == QLatin1Char('#')) {
        const bool hex = name[1] == QLatin1Char('x') || name[1] == QLatin1Char('X');
        bool ok = false;
        const uint code = name.mid(hex ? 2 : 1).toUInt(&ok, hex ? 16 : 10);
        return ok && code > 0 && code <= 0x10FFFF ? static_cast<char32_t>(code) : U'\0';
    }
    return U'\0';
}

Plain extractPlain(const QString& html)
{
    Plain plain;
    const int n = static_cast<int>(html.size());
    int i = 0;
    while (i < n) {
        const QChar c = html[i];
        if (c == QLatin1Char('<')) {
            if (html.mid(i, 4) == QLatin1String("<!--")) {
                const int endComment = static_cast<int>(html.indexOf(QLatin1String("-->"), i + 4));
                i = endComment < 0 ? n : endComment + 3;
                continue;
            }
            const int close = static_cast<int>(html.indexOf(QLatin1Char('>'), i));
            const int tagEnd = close < 0 ? n : close + 1;
            const QString inside = html.mid(i + 1, (close < 0 ? n : close) - i - 1);
            const bool closing = inside.startsWith(QLatin1Char('/'));
            int k = closing ? 1 : 0;
            int e = k;
            while (e < inside.size() && !inside[e].isSpace() && inside[e] != QLatin1Char('/')) {
                ++e;
            }
            const QString name = inside.mid(k, e - k).toLower();
            if (!closing && (name == QLatin1String("head") || name == QLatin1String("style") ||
                             name == QLatin1String("script") || name == QLatin1String("title"))) {
                const int endTag =
                    static_cast<int>(html.indexOf(QLatin1String("</") + name, tagEnd, Qt::CaseInsensitive));
                const int endClose =
                    endTag < 0 ? -1 : static_cast<int>(html.indexOf(QLatin1Char('>'), endTag));
                plain.breakLine(i);
                i = endClose < 0 ? n : endClose + 1;
                continue;
            }
            if (isBlockTag(name)) {
                plain.breakLine(i);
            }
            i = tagEnd;
            continue;
        }
        if (c == QLatin1Char('&')) {
            const int semi = static_cast<int>(html.indexOf(QLatin1Char(';'), i));
            if (semi > i && semi - i <= 10) {
                const char32_t code = entityCode(html.mid(i + 1, semi - i - 1));
                if (code != U'\0') {
                    const QString s = QString::fromUcs4(&code, 1);
                    for (const QChar unit : s) {
                        plain.push(unit, i, semi + 1);
                    }
                    i = semi + 1;
                    continue;
                }
            }
        }
        // Retour à la ligne du source HTML (entre les balises de QTextDocument::toHtml) : de l'espace, pas un
        // saut de ligne du texte ; il ne décale pas les positions en début de bloc.
        if ((c == QLatin1Char('\n') || c == QLatin1Char('\r')) &&
            (plain.text.isEmpty() || plain.text.back() == QLatin1Char('\n'))) {
            ++i;
            continue;
        }
        plain.push(c, i, i + 1);
        ++i;
    }
    return plain;
}

bool isWordChar(QChar c)
{
    return c.isLetterOrNumber() || c.isMark();
}
// Apostrophes (droite, typographique, modificatrice) et traits d'union : gardés dans le mot entre deux
// lettres.
bool isConnector(QChar c)
{
    return c == QLatin1Char('\'') || c == QChar(0x2019) || c == QChar(0x02BC) || c == QLatin1Char('-') ||
           c == QChar(0x2010);
}

struct Word
{
    int start;
    int length;
};

std::vector<Word> splitWords(const QString& text)
{
    std::vector<Word> words;
    const int n = static_cast<int>(text.size());
    int i = 0;
    while (i < n) {
        if (!isWordChar(text[i])) {
            ++i;
            continue;
        }
        int j = i;
        while (j < n) {
            if (isWordChar(text[j])) {
                ++j;
            } else if (isConnector(text[j]) && j + 1 < n && isWordChar(text[j + 1])) {
                ++j;
            } else {
                break;
            }
        }
        words.push_back({i, j - i});
        i = j;
    }
    return words;
}

// Forme demandée au moteur : apostrophes et traits d'union typographiques ramenés aux signes simples.
std::string engineForm(const QString& word)
{
    QString s = word;
    s.replace(QChar(0x2019), QLatin1Char('\'')).replace(QChar(0x02BC), QLatin1Char('\''));
    s.replace(QChar(0x2010), QLatin1Char('-'));
    return s.toStdString();
}

bool looksLikeAddress(const QString& text, int wordStart)
{
    int from = wordStart;
    while (from > 0 && !text[from - 1].isSpace()) {
        --from;
    }
    int to = wordStart;
    while (to < text.size() && !text[to].isSpace()) {
        ++to;
    }
    const QString chunk = text.mid(from, to - from).toLower();
    return chunk.contains(QLatin1Char('@')) || chunk.contains(QLatin1String("://")) ||
           chunk.startsWith(QLatin1String("www."));
}

bool ignoredByRules(const SpellRules& rules, const QString& word, const QString& text, int start)
{
    int letters = 0;
    int upper = 0;
    int lower = 0;
    bool firstLetterUpper = false;
    bool upperAfterFirst = false;
    bool digit = false;
    for (const QChar c : word) {
        if (c.isDigit()) {
            digit = true;
        }
        if (!c.isLetter()) {
            continue;
        }
        if (c.isUpper()) {
            ++upper;
            if (letters == 0) {
                firstLetterUpper = true;
            } else {
                upperAfterFirst = true;
            }
        } else if (c.isLower()) {
            ++lower;
        }
        ++letters;
    }
    if (rules.ignoreDigits && digit) {
        return true;
    }
    if (rules.ignoreUppercase && letters >= 2 && lower == 0 && upper == letters) {
        return true;
    }
    if (rules.ignoreCapitalized && firstLetterUpper && !upperAfterFirst && lower > 0) {
        return true;
    }
    if (rules.ignoreMixedCase && upperAfterFirst && lower > 0) {
        return true;
    }
    return rules.ignoreAddresses && looksLikeAddress(text, start);
}

QString escapeHtml(const QString& s)
{
    QString out;
    for (const QChar c : s) {
        if (c == QLatin1Char('&')) {
            out += QLatin1String("&amp;");
        } else if (c == QLatin1Char('<')) {
            out += QLatin1String("&lt;");
        } else if (c == QLatin1Char('>')) {
            out += QLatin1String("&gt;");
        } else {
            out += c;
        }
    }
    return out;
}

QStringList readLines(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    QStringList lines;
    for (const QByteArray& raw : file.readAll().split('\n')) {
        const QString line = QString::fromUtf8(raw).trimmed();
        if (!line.isEmpty()) {
            lines.push_back(line);
        }
    }
    return lines;
}

} // namespace

std::size_t loadUserDictionary(SpellEngine& engine, const std::string& path)
{
    const QStringList lines = readLines(QString::fromStdString(path));
    for (const QString& line : lines) {
        engine.addWord(line.toStdString());
    }
    return static_cast<std::size_t>(lines.size());
}

bool addToUserDictionary(SpellEngine& engine, const std::string& path, const std::string& word)
{
    const QString clean = QString::fromStdString(word).trimmed();
    if (clean.isEmpty()) {
        return false;
    }
    engine.addWord(clean.toStdString());
    const QString file = QString::fromStdString(path);
    if (readLines(file).contains(clean)) {
        return true;
    }
    QFile out(file);
    if (!out.open(QIODevice::ReadWrite | QIODevice::Append)) {
        return false;
    }
    QByteArray data;
    if (out.size() > 0) {
        QFile probe(file);
        if (probe.open(QIODevice::ReadOnly) && probe.seek(probe.size() - 1) && probe.read(1) != "\n") {
            data += '\n';
        }
    }
    data += clean.toUtf8() + '\n';
    return out.write(data) == data.size();
}

SpellChecker::SpellChecker(SpellEngine& engine, SpellRules rules) : m_engine(engine), m_rules(rules) {}

std::optional<SpellHit> SpellChecker::next(const Document& document)
{
    for (std::size_t page = m_page; page < document.pageCount(); ++page) {
        const std::vector<PageObject>& objects = document.page(page).objects;
        for (std::size_t index = page == m_page ? m_object : 0; index < objects.size(); ++index) {
            const Text* text = std::get_if<Text>(&body(objects[index]));
            if (text == nullptr) {
                continue;
            }
            const bool resumed = page == m_page && index == m_object;
            const Plain plain = extractPlain(QString::fromStdString(text->html));
            QString previous;     // dernier mot, en minuscules
            int previousEnd = -1; // fin du dernier mot dans le texte brut
            for (const Word& w : splitWords(plain.text)) {
                const QString word = plain.text.mid(w.start, w.length);
                const QString lowered = word.toLower();
                bool hasLetter = false;
                for (const QChar c : word) {
                    hasLetter = hasLetter || c.isLetter();
                }
                bool repeated = false;
                if (hasLetter && lowered == previous && previousEnd >= 0) {
                    repeated = true;
                    for (int k = previousEnd; k < w.start; ++k) {
                        repeated = repeated && plain.text[k].isSpace() && plain.text[k] != QLatin1Char('\n');
                    }
                }
                previous = hasLetter ? lowered : QString();
                previousEnd = w.start + w.length;
                if (!hasLetter || (resumed && static_cast<std::size_t>(w.start) < m_offset)) {
                    continue;
                }
                if (m_always.count(lowered.toStdString()) != 0 ||
                    ignoredByRules(m_rules, word, plain.text, w.start)) {
                    continue;
                }
                const bool misspelled = !m_engine.isCorrect(engineForm(word));
                if (!misspelled && !(repeated && m_rules.flagRepeated)) {
                    continue;
                }
                SpellHit hit;
                hit.page = static_cast<int>(page);
                hit.object = objects[index].common.id;
                hit.start = static_cast<std::size_t>(w.start);
                hit.length = static_cast<std::size_t>(w.length);
                hit.word = word.toStdString();
                hit.repeated = !misspelled;
                m_page = page;
                m_object = index;
                m_offset = hit.start;
                m_current = hit;
                return hit;
            }
        }
    }
    m_page = document.pageCount();
    m_object = 0;
    m_offset = 0;
    m_current.reset();
    return std::nullopt;
}

void SpellChecker::ignore()
{
    if (m_current) {
        m_offset = m_current->start + m_current->length;
        m_current.reset();
    }
}

void SpellChecker::ignoreAlways(const std::string& word)
{
    m_always.insert(QString::fromStdString(word).toLower().toStdString());
    ignore();
}

void SpellChecker::restart()
{
    m_page = 0;
    m_object = 0;
    m_offset = 0;
    m_current.reset();
}

std::unique_ptr<UndoCommand> SpellChecker::replace(const Document& document, const SpellHit& hit,
                                                   const std::string& replacement) const
{
    if (hit.page < 0 || static_cast<std::size_t>(hit.page) >= document.pageCount()) {
        return nullptr;
    }
    for (const PageObject& object : document.page(static_cast<std::size_t>(hit.page)).objects) {
        if (object.common.id != hit.object) {
            continue;
        }
        const Text* text = std::get_if<Text>(&body(object));
        if (text == nullptr) {
            return nullptr;
        }
        const QString html = QString::fromStdString(text->html);
        const Plain plain = extractPlain(html);
        if (hit.length == 0 || hit.start + hit.length > static_cast<std::size_t>(plain.text.size()) ||
            plain.text.mid(static_cast<qsizetype>(hit.start), static_cast<qsizetype>(hit.length))
                    .toStdString() != hit.word) {
            return nullptr;
        }
        const int from = plain.begin[hit.start];
        const int to = plain.end[hit.start + hit.length - 1];
        // Balises de l'intérieur du mot (« <b>mo</b>sieur ») : gardées après le remplaçant ; sans elles, une
        // balise fermante disparaît et la mise en forme déborde sur la suite du paragraphe.
        QString inner;
        for (std::size_t i = hit.start; i + 1 < hit.start + hit.length; ++i) {
            const int gap = plain.end[i];
            if (plain.begin[i + 1] > gap) {
                inner += html.mid(gap, plain.begin[i + 1] - gap);
            }
        }
        Text edited = *text;
        edited.html =
            (html.left(from) + escapeHtml(QString::fromStdString(replacement)) + inner + html.mid(to))
                .toStdString();
        edited.runs.clear(); // lignes lues de l'ancien texte : périmées
        edited.firstBaseline.reset();
        return std::make_unique<SetTextCommand>(static_cast<std::size_t>(hit.page), hit.object,
                                                std::move(edited));
    }
    return nullptr;
}

} // namespace actcraft::core
