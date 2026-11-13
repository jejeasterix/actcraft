#include "actcraft/ui/webframeitem.h"

#include "actcraft/ui/documentcontroller.h"

#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/core/objectedits.h>

#include <QBuffer>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QScreen>
#include <QSet>
#include <QStringDecoder>
#include <QTemporaryDir>
#include <QThreadPool>
#include <QTransform>

#include <algorithm>
#include <memory>
#include <utility>

namespace actcraft::ui {

namespace {

using Verdict = WebContentPolicy::Verdict;

constexpr int kMaxRedirects = 10;
// Une page plus lourde n'est pas montrée : 4 Mo couvrent les pages ordinaires (quelques centaines de ko) et
// les plus longues, réécrites en moins d'une seconde en construction de débogage, hors du fil de l'interface.
constexpr qint64 kMaxBody = 4 * 1024 * 1024;
constexpr int kTransferTimeoutMs = 20000; // une page muette n'occupe pas le cadre
const QString kLinkPrefix = QStringLiteral("actcraft-link-");
constexpr char16_t kHoldOpen = u'\x02'; // repère d'une construction CSS
                                        // réécrite, le temps de la vérification
constexpr char16_t kHoldClose = u'\x03';
// Limite de loadHtml (url::kMaxURLChars = 2 097 152, sur l'adresse data: encodée) ; seuil sous la limite,
// pour le préfixe.
constexpr qsizetype kLoadHtmlLimit = 2000000;
// Un QChar UTF-16 donne au plus 3 octets UTF-8, chacun au plus 3 caractères encodés : en dessous, inutile de
// compter.
constexpr qsizetype kSurelyShortHtml = kLoadHtmlLimit / 9;

// OA92, erreur 153 : le lecteur intégré de YouTube veut un en-tête Referer https qui nomme l'application
// (« Embedded player identification », exigences minimales de YouTube : la base de loadHtml le donne, valeur
// https:// et identifiant de l'application en minuscules, stable d'une version à l'autre). Une page sans base
// (about:blank, data:) ou de base file: n'envoie aucun Referer. Identifiant : MACOSX_BUNDLE_GUI_IDENTIFIER.
// Réservée aux pages du lecteur : une page de base https peint plus tard qu'une page data: (nouveau processus
// du moteur, mesuré au harnais : non peinte à 300 ms, peinte à 1 500 ms), et ses adresses relatives ne visent
// plus le dossier du document (n° 11).
const QUrl kPlayerIdentity(QStringLiteral("https://org.actcraft.actcraft/"));

bool isPlayerIdentity(const QUrl& url)
{
    return url.scheme() == QLatin1String("https") && url.host() == kPlayerIdentity.host();
}

} // namespace

// Adresse que nomment le titre et « Ouvrir dans le navigateur ». L'identité n'est pas une page : la vue qui
// la rapporte montre le HTML saisi ou la page du lecteur.
QUrl WebFrameItems::shownUrl(const Row& row) const
{
    if (row.blocked)
        return row.pendingUrl;
    const QUrl view = isPlayerIdentity(row.viewUrl) ? QUrl() : row.viewUrl;
    return !m_guardsEnabled && view.isValid() ? view : row.baseUrl;
}

// Sans <title>, le moteur donne l'adresse pour titre : celle de l'identité ne nomme rien.
bool WebFrameItems::namesThePlayerIdentity(const Row& row)
{
    return isPlayerIdentity(row.viewUrl) &&
           (row.viewTitle.startsWith(kPlayerIdentity.host()) ||
            row.viewTitle.startsWith(kPlayerIdentity.toString(QUrl::RemovePath)));
}

namespace {

bool isYouTubeHost(const QString& host)
{
    const QString h = host.toLower();
    for (const QLatin1String domain : {QLatin1String("youtube.com"), QLatin1String("youtube-nocookie.com")})
        if (h == domain || h.endsWith(u'.' + domain))
            return true;
    return false;
}

bool isYouTubePlayer(const QUrl& url)
{
    const QString scheme = url.scheme().toLower();
    return (scheme == QLatin1String("http") || scheme == QLatin1String("https")) &&
           isYouTubeHost(url.host()) && url.path().startsWith(QLatin1String("/embed/"));
}

// Un HTML saisi qui intègre ce lecteur, rien n'est réécrit : code « Intégrer » (iframe /embed/, /embed?…),
// API IFrame (iframe_api), adresse échappée en JSON (\/). L'hôte est précédé de « / » ou d'un sous-domaine
// (« . ») : notyoutube.com ne compte pas. Une simple mention de l'adresse compte (faux positif accepté : la
// page perd seulement ses adresses relatives au dossier).
bool embedsYouTubePlayer(const QString& html)
{
    static const QRegularExpression player(
        QStringLiteral(R"((?:^|[/.])youtube(?:-nocookie)?\.com\\?/(?:embed\b|iframe_api))"),
        QRegularExpression::CaseInsensitiveOption);
    return html.contains(player);
}

// Adresse d'un lecteur saisie seule : la vue ne pose pas d'en-tête, une page d'une iframe porte l'identité.
// L'iframe est en https : une iframe http dans une page https serait bloquée (contenu mixte).
QString playerPage(QUrl player)
{
    player.setScheme(QStringLiteral("https"));
    return QStringLiteral("<!DOCTYPE html><html><head><style>html,body{margin:0;height:100%;overflow:hidden}"
                          "iframe{display:block;border:0;width:100%;height:100%}</style></head><body>"
                          "<iframe src=\"%1\" allow=\"autoplay; encrypted-media; picture-in-picture; "
                          "fullscreen\" allowfullscreen></iframe></body></html>")
        .arg(player.toString(QUrl::FullyEncoded).toHtmlEscaped());
}

// Fil secondaire seulement. Fichier écrit avec sa marque d'ordre : sans déclaration de charset, UTF-8 reste
// lu comme tel.
// `failed` : le contenu est lourd mais le fichier n'a pas pu être écrit en entier (disque plein, dossier
// perdu) ; le fichier partiel est retiré et rien n'est rendu.
QUrl writeHeavyHtml(const QString& html, const QString& dir, std::uint64_t ticket, bool& failed)
{
    failed = false;
    if (html.size() <= kSurelyShortHtml)
        return {};
    const QByteArray utf8 = html.toUtf8();
    if (utf8.toPercentEncoding().size() <= kLoadHtmlLimit)
        return {};
    failed = true;
    if (dir.isEmpty())
        return {};
    const QString path = QDir(dir).filePath(QStringLiteral("cadre-%1.html").arg(ticket));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return {};
    static constexpr char kBom[] = "\xEF\xBB\xBF";
    if (file.write(kBom, 3) != 3 || file.write(utf8) != utf8.size() || !file.flush()) {
        file.close();
        QFile::remove(path);
        return {};
    }
    file.close();
    failed = false;
    return QUrl::fromLocalFile(path);
}

QRectF pageBox(const core::PageObject& object, const core::WebFrame& frame)
{
    const core::Transform& t = object.common.transform;
    const QTransform q(t.m11, t.m12, t.m21, t.m22, t.dx, t.dy);
    return q.mapRect(QRectF(frame.bounds.x, frame.bounds.y, frame.bounds.width, frame.bounds.height));
}

QString escaped(const QString& text)
{
    return text.toHtmlEscaped();
}

bool isHtmlSpace(QChar c)
{
    return c == u' ' || c == u'\t' || c == u'\n' || c == u'\f' || c == u'\r';
}

bool isAsciiLetter(QChar c)
{
    return (c >= u'a' && c <= u'z') || (c >= u'A' && c <= u'Z');
}

bool isDigit(QChar c)
{
    return c >= u'0' && c <= u'9';
}

int hexValue(QChar c)
{
    if (isDigit(c))
        return c.unicode() - u'0';
    if (c >= u'a' && c <= u'f')
        return c.unicode() - u'a' + 10;
    if (c >= u'A' && c <= u'F')
        return c.unicode() - u'A' + 10;
    return -1;
}

void appendCode(QString& out, char32_t code)
{
    if (code == 0 || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF))
        code = 0xFFFD;
    out += QString::fromUcs4(&code, 1);
}

// Valeur d'attribut telle que le parseur HTML la rend : entités numériques, et
// entités nommées qui donnent un caractère ASCII (les seules qui pourraient
// former une adresse, un nom de fonction CSS ou un échappement) ; une entité
// nommée inconnue reste telle quelle.
QString decodeEntities(QStringView raw)
{
    static const QHash<QString, QString> named = {{QStringLiteral("amp"), QStringLiteral("&")},
                                                  {QStringLiteral("AMP"), QStringLiteral("&")},
                                                  {QStringLiteral("lt"), QStringLiteral("<")},
                                                  {QStringLiteral("LT"), QStringLiteral("<")},
                                                  {QStringLiteral("gt"), QStringLiteral(">")},
                                                  {QStringLiteral("GT"), QStringLiteral(">")},
                                                  {QStringLiteral("quot"), QStringLiteral("\"")},
                                                  {QStringLiteral("QUOT"), QStringLiteral("\"")},
                                                  {QStringLiteral("apos"), QStringLiteral("'")},
                                                  {QStringLiteral("nbsp"), QStringLiteral(" ")},
                                                  {QStringLiteral("colon"), QStringLiteral(":")},
                                                  {QStringLiteral("Tab"), QStringLiteral("\t")},
                                                  {QStringLiteral("NewLine"), QStringLiteral("\n")},
                                                  {QStringLiteral("sol"), QStringLiteral("/")},
                                                  {QStringLiteral("bsol"), QStringLiteral("\\")},
                                                  {QStringLiteral("lpar"), QStringLiteral("(")},
                                                  {QStringLiteral("rpar"), QStringLiteral(")")},
                                                  {QStringLiteral("period"), QStringLiteral(".")},
                                                  {QStringLiteral("comma"), QStringLiteral(",")},
                                                  {QStringLiteral("semi"), QStringLiteral(";")},
                                                  {QStringLiteral("equals"), QStringLiteral("=")},
                                                  {QStringLiteral("num"), QStringLiteral("#")},
                                                  {QStringLiteral("excl"), QStringLiteral("!")},
                                                  {QStringLiteral("quest"), QStringLiteral("?")},
                                                  {QStringLiteral("commat"), QStringLiteral("@")},
                                                  {QStringLiteral("percnt"), QStringLiteral("%")},
                                                  {QStringLiteral("dollar"), QStringLiteral("$")},
                                                  {QStringLiteral("ast"), QStringLiteral("*")},
                                                  {QStringLiteral("midast"), QStringLiteral("*")},
                                                  {QStringLiteral("plus"), QStringLiteral("+")},
                                                  {QStringLiteral("lsqb"), QStringLiteral("[")},
                                                  {QStringLiteral("lbrack"), QStringLiteral("[")},
                                                  {QStringLiteral("rsqb"), QStringLiteral("]")},
                                                  {QStringLiteral("rbrack"), QStringLiteral("]")},
                                                  {QStringLiteral("lcub"), QStringLiteral("{")},
                                                  {QStringLiteral("lbrace"), QStringLiteral("{")},
                                                  {QStringLiteral("rcub"), QStringLiteral("}")},
                                                  {QStringLiteral("rbrace"), QStringLiteral("}")},
                                                  {QStringLiteral("lowbar"), QStringLiteral("_")},
                                                  {QStringLiteral("UnderBar"), QStringLiteral("_")},
                                                  {QStringLiteral("grave"), QStringLiteral("`")},
                                                  {QStringLiteral("DiacriticalGrave"), QStringLiteral("`")},
                                                  {QStringLiteral("verbar"), QStringLiteral("|")},
                                                  {QStringLiteral("vert"), QStringLiteral("|")},
                                                  {QStringLiteral("VerticalLine"), QStringLiteral("|")},
                                                  {QStringLiteral("Hat"), QStringLiteral("^")},
                                                  {QStringLiteral("fjlig"), QStringLiteral("fj")}};
    // Sans « ; », seules les anciennes entités sont lues, et pas devant « = »
    // (règle des attributs).
    static const QSet<QString> legacy = {
        QStringLiteral("amp"),  QStringLiteral("AMP"),  QStringLiteral("lt"),
        QStringLiteral("LT"),   QStringLiteral("gt"),   QStringLiteral("GT"),
        QStringLiteral("quot"), QStringLiteral("QUOT"), QStringLiteral("nbsp")};
    QString out;
    out.reserve(raw.size());
    const qsizetype n = raw.size();
    qsizetype i = 0;
    while (i < n) {
        if (raw[i] != u'&') {
            out += raw[i++];
            continue;
        }
        if (i + 1 < n && raw[i + 1] == u'#') {
            qsizetype j = i + 2;
            const bool hex = j < n && (raw[j] == u'x' || raw[j] == u'X');
            if (hex)
                ++j;
            const qsizetype first = j;
            char32_t code = 0;
            while (j < n) {
                const int digit = hex ? hexValue(raw[j]) : (isDigit(raw[j]) ? raw[j].unicode() - u'0' : -1);
                if (digit < 0)
                    break;
                code = std::min<char32_t>(code * (hex ? 16U : 10U) + static_cast<char32_t>(digit), 0x110000U);
                ++j;
            }
            if (j == first) {
                out += raw[i++];
                continue;
            }
            if (j < n && raw[j] == u';')
                ++j;
            appendCode(out, code);
            i = j;
            continue;
        }
        qsizetype j = i + 1;
        while (j < n && raw[j].unicode() < 128 && (isAsciiLetter(raw[j]) || isDigit(raw[j])))
            ++j;
        const QString name = raw.mid(i + 1, j - i - 1).toString();
        const bool semicolon = j < n && raw[j] == u';';
        const auto found = named.constFind(name);
        if (found != named.cend() && (semicolon || (legacy.contains(name) && !(j < n && raw[j] == u'=')))) {
            out += *found;
            i = semicolon ? j + 1 : j;
            continue;
        }
        out += raw[i++];
    }
    return out;
}

// Échappements CSS rendus (`u\72l` → `url`), comme le tokeniseur CSS les lit ;
// un caractère de contrôle obtenu ainsi devient U+FFFD (il ne peut pas imiter
// les repères de la réécriture).
QString cssUnescape(QStringView text)
{
    QString out;
    out.reserve(text.size());
    const qsizetype n = text.size();
    qsizetype i = 0;
    while (i < n) {
        if (text[i] != u'\\' || i + 1 >= n) {
            out += text[i++];
            continue;
        }
        qsizetype j = i + 1;
        if (text[j] == u'\n') { // suite de chaîne sur la ligne suivante
            i = j + 1;
            continue;
        }
        char32_t code = 0;
        int digits = 0;
        while (j < n && digits < 6 && hexValue(text[j]) >= 0) {
            code = code * 16U + static_cast<char32_t>(hexValue(text[j]));
            ++j;
            ++digits;
        }
        if (digits == 0) {
            out += text[j];
            i = j + 1;
            continue;
        }
        if (j < n && isHtmlSpace(text[j]))
            ++j;
        appendCode(out, code < 0x20 ? 0xFFFD : code);
        i = j;
    }
    return out;
}

// Commentaires CSS, `<!--` et `-->` hors des chaînes : une espace (ils ne
// séparent que des jetons).
QString stripCssComments(QStringView text)
{
    QString out;
    out.reserve(text.size());
    const qsizetype n = text.size();
    qsizetype i = 0;
    while (i < n) {
        const QChar c = text[i];
        if (c == u'"' || c == u'\'') {
            qsizetype j = i + 1;
            while (j < n && text[j] != c && text[j] != u'\n')
                j += (text[j] == u'\\' && j + 1 < n) ? 2 : 1;
            if (j < n && text[j] == c)
                ++j;
            j = std::min(j, n);
            out += text.mid(i, j - i);
            i = j;
        } else if (c == u'/' && i + 1 < n && text[i + 1] == u'*') {
            const qsizetype end = text.indexOf(u"*/", i + 2);
            i = end < 0 ? n : end + 2;
            out += u' ';
        } else if (text.mid(i, 4) == u"<!--") {
            i += 4;
            out += u' ';
        } else if (text.mid(i, 3) == u"-->") {
            i += 3;
            out += u' ';
        } else {
            out += c;
            ++i;
        }
    }
    return out;
}

// Chaînes posées directement dans image-set() : des adresses pour CSS, écrites
// ici en url() pour être décidées comme les autres. Chaque appel est lu une
// fois, jusqu'à sa parenthèse fermante.
QString imageSetStrings(const QString& text)
{
    thread_local const QRegularExpression re(QStringLiteral(R"(image-set\()"),
                                             QRegularExpression::CaseInsensitiveOption);
    QString out;
    out.reserve(text.size());
    qsizetype last = 0;
    for (QRegularExpressionMatch m = re.match(text); m.hasMatch(); m = re.match(text, last)) {
        qsizetype i = m.capturedEnd();
        out += QStringView(text).mid(last, i - last);
        int depth = 1;
        while (i < text.size() && depth > 0) {
            const QChar c = text[i];
            if (c == u'"' || c == u'\'') {
                qsizetype j = i + 1;
                while (j < text.size() && text[j] != c && text[j] != u'\n')
                    j += (text[j] == u'\\' && j + 1 < text.size()) ? 2 : 1;
                if (j < text.size() && text[j] == c)
                    ++j;
                j = std::min(j, text.size());
                if (depth == 1)
                    out += QStringLiteral("url(") + QStringView(text).mid(i, j - i) + u')';
                else
                    out += QStringView(text).mid(i, j - i);
                i = j;
                continue;
            }
            if (c == u'(')
                ++depth;
            else if (c == u')')
                --depth;
            out += c;
            ++i;
        }
        last = i;
    }
    out += QStringView(text).mid(last);
    return out;
}

// Source d'une règle de sécurité du contenu : adresse exacte, sans « ; » ni « ,
// » ni espace (encodés).
QString cspSource(const QUrl& url)
{
    QString s = QString::fromLatin1(url.adjusted(QUrl::RemoveFragment | QUrl::RemoveQuery).toEncoded());
    s.replace(QLatin1Char(';'), QStringLiteral("%3B"));
    s.replace(QLatin1Char(','), QStringLiteral("%2C"));
    s.replace(QLatin1Char('\''), QStringLiteral("%27"));
    return s;
}

// Attribut d'une balise, lu comme le tokeniseur HTML le lit : nom en
// minuscules, valeur brute (entités non rendues).
struct Attribute
{
    QString name;
    QStringView raw;
    bool hasValue = false;
};

// Nom de balise à `i` : lettres, chiffres, `:` et `-`, suivi d'un blanc, de
// `/`, de `>` ou de la fin (sinon le moteur lirait un autre nom). Longueur, 0
// si non reconnu.
qsizetype tagNameLength(QStringView text, qsizetype i)
{
    const qsizetype n = text.size();
    if (i >= n || !isAsciiLetter(text[i]))
        return 0;
    qsizetype j = i + 1;
    while (j < n && (isAsciiLetter(text[j]) || isDigit(text[j]) || text[j] == u':' || text[j] == u'-'))
        ++j;
    if (j < n && !isHtmlSpace(text[j]) && text[j] != u'/' && text[j] != u'>')
        return 0;
    return j - i;
}

// Attributs de `i` jusqu'au `>` qui ferme la balise, selon l'automate du
// tokeniseur HTML : blancs et `/` séparent, une valeur est entre guillemets ou
// nue, un attribut peut suivre un guillemet fermant sans blanc. Position après
// le `>`, ou -1 si la page finit avant (le moteur ignorerait la balise).
qsizetype readAttributes(QStringView text, qsizetype i, QVector<Attribute>& attrs, bool& selfClosing)
{
    const qsizetype n = text.size();
    selfClosing = false;
    for (;;) {
        while (i < n && (isHtmlSpace(text[i]) || text[i] == u'/')) {
            if (text[i] == u'/' && i + 1 < n && text[i + 1] == u'>') {
                selfClosing = true;
                return i + 2;
            }
            ++i;
        }
        if (i >= n)
            return -1;
        if (text[i] == u'>')
            return i + 1;
        const qsizetype start = i++; // le premier caractère, même `=`, ouvre le nom
        while (i < n && !isHtmlSpace(text[i]) && text[i] != u'/' && text[i] != u'>' && text[i] != u'=')
            ++i;
        Attribute a;
        a.name = text.mid(start, i - start).toString().toLower();
        while (i < n && isHtmlSpace(text[i]))
            ++i;
        if (i < n && text[i] == u'=') {
            ++i;
            while (i < n && isHtmlSpace(text[i]))
                ++i;
            if (i >= n)
                return -1;
            a.hasValue = true;
            const QChar quote = text[i];
            if (quote == u'"' || quote == u'\'') {
                const qsizetype end = text.indexOf(quote, i + 1);
                if (end < 0)
                    return -1;
                a.raw = text.mid(i + 1, end - i - 1);
                i = end + 1;
            } else if (quote != u'>') {
                const qsizetype valueStart = i;
                while (i < n && !isHtmlSpace(text[i]) && text[i] != u'>')
                    ++i;
                a.raw = text.mid(valueStart, i - valueStart);
            }
        }
        attrs.append(a);
    }
}

// Nom d'attribut réécrit tel quel : ASCII ordinaire seulement.
bool plainAttributeName(const QString& name)
{
    if (name.isEmpty() || !(isAsciiLetter(name.front()) || name.front() == u'_' || name.front() == u':'))
        return false;
    return std::all_of(name.cbegin(), name.cend(), [](QChar c) {
        return isAsciiLetter(c) || isDigit(c) || c == u'-' || c == u'_' || c == u':' || c == u'.';
    });
}

class Rewriter
{
public:
    Rewriter(const QUrl& base, const std::function<Verdict(const QUrl&)>& decide)
        : m_base(base), m_decide(decide)
    {}

    // Adresse passée par la politique, rendue absolue ; vide si refusée ou
    // bloquée. `value` : déjà rendue (entités ou échappements CSS).
    QString allowed(QString value, QStringList& into)
    {
        value.remove(u'\t');
        value.remove(u'\n');
        value.remove(u'\r');
        value = value.trimmed();
        if (value.isEmpty())
            return {};
        const QUrl url = m_base.isValid() ? m_base.resolved(QUrl(value)) : QUrl(value);
        if (!url.isValid() || url.isRelative())
            return {};
        const Verdict verdict = m_decide(url);
        if (verdict == Verdict::Blocked)
            m_blocked = true;
        if (verdict != Verdict::Load)
            return {};
        const QString scheme = url.scheme();
        if (scheme == QLatin1String("data")) {
            m_data = true;
        } else if (scheme == QLatin1String("http") || scheme == QLatin1String("https")) {
            into.append(cspSource(url));
        } else {
            return {}; // file, about : rien qu'une vue sans accès aux fichiers puisse
                       // lire
        }
        return QString::fromLatin1(url.toEncoded());
    }

    // Feuille de style ou valeur CSS : chaque url() et @import passe par la
    // politique ; si quelque chose peut encore désigner une adresse (échappement,
    // url( mal formée, chaîne d'image-set, `<`), la feuille est vidée. Rien n'en
    // sort que la politique n'ait vu.
    QString css(QStringView text, QStringList& into)
    {
        static const QString dq = QStringLiteral(R"re("((?:[^"\\\n]++|\\.)*+)")re");
        static const QString sq = QStringLiteral(R"re('((?:[^'\\\n]++|\\.)*+)')re");
        static const QString bare = QStringLiteral(R"re(((?:[^"'()\\\s]++|\\.)*+))re");
        static const QString url = QStringLiteral(R"re(url\(\s*+(?:%1|%2|%3)\s*+\))re").arg(dq, sq, bare);
        const auto options =
            QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption;
        // Expressions propres à chaque fil : la réécriture tourne hors du fil de l'interface, plusieurs à la
        // fois.
        thread_local const QRegularExpression importRe(
            QStringLiteral(R"re(@import\s*+(?:%1|%2|%3))re").arg(dq, sq, url), options);
        thread_local const QRegularExpression urlRe(url, options);
        thread_local const QRegularExpression fragmentRe(QStringLiteral(R"(^#[A-Za-z0-9_:.-]*$)"));
        thread_local const QRegularExpression dangerRe(QStringLiteral(
            R"re(url\(|@import|(?<![a-z0-9_-])(?:src|image)\(|expression\(|image-set\([^)"'(]*+["']|<)re"));
        QString work = stripCssComments(text);
        work.remove(QChar(kHoldOpen));
        work.remove(QChar(kHoldClose));
        work = imageSetStrings(work);
        QStringList made;
        const auto hold = [&made](const QString& construct) {
            made.append(construct);
            return QChar(kHoldOpen) + QString::number(made.size() - 1) + QChar(kHoldClose);
        };
        const auto value = [](const QRegularExpressionMatch& m) {
            for (int g = 1; g <= m.lastCapturedIndex(); ++g) {
                if (m.capturedStart(g) >= 0)
                    return cssUnescape(m.capturedView(g));
            }
            return QString();
        };
        work = replaceAll(work, importRe, [&](const QRegularExpressionMatch& m) {
            const QString kept = allowed(value(m), m_styles);
            return hold(kept.isEmpty() ? QStringLiteral("@import \"about:blank\"")
                                       : QStringLiteral("@import \"%1\"").arg(kept));
        });
        work = replaceAll(work, urlRe, [&](const QRegularExpressionMatch& m) {
            const QString target = value(m).trimmed();
            if (fragmentRe.match(target).hasMatch())
                return hold(
                    QStringLiteral("url(\"%1\")").arg(target)); // élément de ce document : rien demandé
            const QString kept = allowed(target, into);
            return hold(kept.isEmpty() ? QStringLiteral("url(about:blank)")
                                       : QStringLiteral("url(\"%1\")").arg(kept));
        });
        if (dangerRe.match(cssUnescape(work).toLower()).hasMatch())
            return {};
        QString out;
        out.reserve(work.size());
        for (qsizetype i = 0; i < work.size(); ++i) {
            if (work[i] != QChar(kHoldOpen)) {
                out += work[i];
                continue;
            }
            const qsizetype end = work.indexOf(QChar(kHoldClose), i);
            out += made.value(work.mid(i + 1, end - i - 1).toInt());
            i = end;
        }
        return out;
    }

    static QString replaceAll(const QString& text, const QRegularExpression& re,
                              const std::function<QString(const QRegularExpressionMatch&)>& with)
    {
        QString out;
        qsizetype last = 0;
        auto it = re.globalMatch(text);
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            out += QStringView(text).mid(last, m.capturedStart() - last);
            out += with(m);
            last = m.capturedEnd();
        }
        out += QStringView(text).mid(last);
        return out;
    }

    // Balise ouvrante réécrite : nom, puis seulement les attributs gardés, chacun
    // une fois (le premier, comme le moteur). Adresses décidées et rendues
    // absolues, CSS réécrit, valeurs ordinaires gardées telles quelles.
    QString tag(const QString& name, const QVector<Attribute>& attrs, bool selfClosing)
    {
        static const QSet<QString> droppedElements = {QStringLiteral("base"),
                                                      QStringLiteral("set"),
                                                      QStringLiteral("animate"),
                                                      QStringLiteral("animatemotion"),
                                                      QStringLiteral("animatetransform"),
                                                      QStringLiteral("animatecolor"),
                                                      QStringLiteral("script")};
        static const QSet<QString> droppedAttributes = {
            QStringLiteral("ping"),       QStringLiteral("download"),   QStringLiteral("target"),
            QStringLiteral("formaction"), QStringLiteral("action"),     QStringLiteral("manifest"),
            QStringLiteral("codebase"),   QStringLiteral("archive"),    QStringLiteral("longdesc"),
            QStringLiteral("cite"),       QStringLiteral("lowsrc"),     QStringLiteral("dynsrc"),
            QStringLiteral("srcdoc"),     QStringLiteral("classid"),    QStringLiteral("attributionsrc"),
            QStringLiteral("xml:base"),   QStringLiteral("http-equiv"), QStringLiteral("profile"),
            QStringLiteral("icon")};
        static const QSet<QString> cssAttributes = {
            QStringLiteral("style"),      QStringLiteral("fill"),         QStringLiteral("stroke"),
            QStringLiteral("filter"),     QStringLiteral("mask"),         QStringLiteral("clip-path"),
            QStringLiteral("marker"),     QStringLiteral("marker-start"), QStringLiteral("marker-mid"),
            QStringLiteral("marker-end"), QStringLiteral("cursor")};
        static const QSet<QString> hints = {QStringLiteral("preconnect"),    QStringLiteral("prefetch"),
                                            QStringLiteral("dns-prefetch"),  QStringLiteral("preload"),
                                            QStringLiteral("modulepreload"), QStringLiteral("prerender"),
                                            QStringLiteral("next")};
        static const QSet<QString> nested = {QStringLiteral("iframe"), QStringLiteral("frame"),
                                             QStringLiteral("object"), QStringLiteral("embed"),
                                             QStringLiteral("script")};
        static const QSet<QString> media = {QStringLiteral("video"), QStringLiteral("audio"),
                                            QStringLiteral("source"), QStringLiteral("track")};
        static const QSet<QString> svgResource = {
            // éléments SVG dont href est une ressource, non un lien
            QStringLiteral("image"),          QStringLiteral("use"),
            QStringLiteral("feimage"),        QStringLiteral("pattern"),
            QStringLiteral("lineargradient"), QStringLiteral("radialgradient"),
            QStringLiteral("filter"),         QStringLiteral("textpath"),
            QStringLiteral("mpath"),          QStringLiteral("cursor")};
        const QString lower = name.toLower();
        if (droppedElements.contains(lower))
            return {}; // la base est celle du modèle ; une animation SVG pourrait poser une adresse ; un
                       // script n'est jamais lu (JavaScript coupé), ni par src ni par href (SVG)
        const auto find = [&attrs](const QString& key) -> const Attribute* {
            for (const Attribute& a : attrs) {
                if (a.name == key)
                    return &a;
            }
            return nullptr;
        };
        const auto decoded = [&find](const QString& key) {
            const Attribute* a = find(key);
            return a ? decodeEntities(a->raw).trimmed().toLower() : QString();
        };
        if (lower == QLatin1String("meta"))
            return {}; // toute balise meta : navigation (refresh, que la CSP ne couvre pas), en-tête ou règle
                       // que la politique ne verrait pas ; seconde barrière, indépendante des attributs
        const QString rel = decoded(QStringLiteral("rel"));
        if (lower == QLatin1String("link")) {
            const QStringList tokens = rel.simplified().split(u' ', Qt::SkipEmptyParts);
            const bool hinted = std::any_of(tokens.cbegin(), tokens.cend(),
                                            [](const QString& t) { return hints.contains(t); });
            if (hinted ||
                (!rel.contains(QLatin1String("stylesheet")) && !rel.contains(QLatin1String("icon"))))
                return {}; // connexion, préchargement : une requête que rien n'affiche
        }
        QString out = QLatin1Char('<') + name;
        QSet<QString> seen;
        for (const Attribute& a : attrs) {
            const QString& key = a.name;
            if (!plainAttributeName(key) || seen.contains(key))
                continue;
            seen.insert(key);
            if (key.startsWith(QLatin1String("on")) || droppedAttributes.contains(key))
                continue;
            QString value;
            bool rewritten = true;
            if (key == QLatin1String("href") || key == QLatin1String("xlink:href")) {
                const QString target = decodeEntities(a.raw);
                if (lower == QLatin1String("a") || lower == QLatin1String("area"))
                    value = link(target);
                else if (lower == QLatin1String("link"))
                    value = allowed(target, rel.contains(QLatin1String("stylesheet")) ? m_styles : m_images);
                else if (target.trimmed().startsWith(u'#'))
                    value = target.trimmed(); // élément de ce document (SVG) : rien demandé
                else if (svgResource.contains(lower))
                    value = allowed(target, m_images); // ressource d'un élément SVG
                else
                    value = link(target); // lien pour le moteur (MathML, SVG a…) : navigation décidée au clic
                if (value.isEmpty())
                    continue;
            } else if (key == QLatin1String("src") || key == QLatin1String("data") ||
                       key == QLatin1String("poster") || key == QLatin1String("background")) {
                if (nested.contains(lower)) {
                    value = QStringLiteral("about:blank"); // contexte imbriqué : ni réécrit ni lu
                } else {
                    value = allowed(decodeEntities(a.raw), media.contains(lower) ? m_media : m_images);
                    if (value.isEmpty())
                        value = QStringLiteral("about:blank");
                }
            } else if (key == QLatin1String("srcset") || key == QLatin1String("imagesrcset")) {
                value = srcset(decodeEntities(a.raw));
                if (value.isEmpty())
                    continue;
            } else if (cssAttributes.contains(key)) {
                value = css(decodeEntities(a.raw), m_images);
            } else {
                rewritten = false;
            }
            out += QLatin1Char(' ') + key;
            if (rewritten) {
                out += QStringLiteral("=\"") + value.toHtmlEscaped() + QLatin1Char('"');
            } else if (a.hasValue) {
                // Valeur ordinaire : entités laissées au moteur (ni doublées ni rendues), mais ni guillemet
                // ni `<` ni `>` tels quels : là où le moteur ne lit pas de balise (RCDATA, RAWTEXT), une
                // valeur qui porterait `</textarea><meta …>` fermerait l'élément et poserait une balise que
                // rien n'a décidée. Les entités ajoutées commencent par `&` : elles ne prolongent aucune
                // entité de la valeur.
                QString raw = a.raw.toString();
                raw.replace(u'"', QStringLiteral("&quot;"));
                raw.replace(u'\'', QStringLiteral("&#39;"));
                raw.replace(u'<', QStringLiteral("&lt;"));
                raw.replace(u'>', QStringLiteral("&gt;"));
                out += QStringLiteral("=\"") + raw + QLatin1Char('"');
            }
        }
        out += selfClosing ? QStringLiteral(" />") : QStringLiteral(">");
        return out;
    }

    // Candidats d'un srcset : chaque adresse décidée, descripteurs de taille
    // seuls gardés.
    QString srcset(const QString& value)
    {
        thread_local const QRegularExpression descriptorRe(QStringLiteral(R"(^[0-9.eE+-]+[wxhWXH]$)"));
        QStringList kept;
        for (const QString& candidate : value.split(u',')) {
            const QStringList parts = candidate.simplified().split(u' ', Qt::SkipEmptyParts);
            if (parts.isEmpty())
                continue;
            const QString url = allowed(parts.front(), m_images);
            if (url.isEmpty())
                continue;
            QStringList entry{url};
            for (qsizetype k = 1; k < parts.size(); ++k) {
                if (descriptorRe.match(parts[k]).hasMatch())
                    entry.append(parts[k]);
            }
            kept.append(entry.join(u' '));
        }
        return kept.join(QStringLiteral(", "));
    }

    // Lien : un renvoi interne vers la cible, décidée au clic ; un renvoi vers la
    // même page reste.
    QString link(QString value)
    {
        value.remove(u'\t');
        value.remove(u'\n');
        value.remove(u'\r');
        value = value.trimmed();
        if (value.startsWith(QLatin1Char('#')) && !value.startsWith(QLatin1Char('#') + kLinkPrefix))
            return value;
        const QUrl url = m_base.isValid() ? m_base.resolved(QUrl(value)) : QUrl(value);
        if (!url.isValid() || url.isRelative())
            return {};
        const QString scheme = url.scheme();
        if (scheme != QLatin1String("http") && scheme != QLatin1String("https") &&
            scheme != QLatin1String("file"))
            return {};
        m_links.append(url);
        return QLatin1Char('#') + kLinkPrefix + QString::number(m_links.size() - 1);
    }

    QString policy() const
    {
        const auto list = [](QStringList sources, const QStringList& extra = {}) {
            sources += extra;
            sources.removeDuplicates();
            return sources.isEmpty() ? QStringLiteral("'none'") : sources.join(QLatin1Char(' '));
        };
        QStringList images = m_images;
        if (m_data)
            images.append(QStringLiteral("data:"));
        return QStringLiteral("default-src 'none'; img-src %1; media-src %2; "
                              "style-src 'unsafe-inline' %3; "
                              "font-src %4; script-src 'none'; connect-src 'none'; "
                              "frame-src 'none'; "
                              "child-src 'none'; object-src 'none'; worker-src "
                              "'none'; manifest-src 'none'; "
                              "form-action 'none'; base-uri 'none'")
            .arg(list(images), list(m_media), list(m_styles).replace(QStringLiteral("'none'"), QString()),
                 list(m_images, m_styles));
    }

    QStringList all() const
    {
        QStringList everything = m_images + m_media + m_styles;
        everything.removeDuplicates();
        return everything;
    }

    QVector<QUrl> m_links;
    QStringList m_images;
    QStringList m_media;
    QStringList m_styles;
    bool m_data = false;
    bool m_blocked = false; // une adresse au moins bloquée (document reçu, page non débloquée)

private:
    QUrl m_base;
    const std::function<Verdict(const QUrl&)>& m_decide;
};

// Image fixe du cadre : ressource (clé SHA-256) puis objet, une étape.
class SnapshotCommand final : public core::UndoCommand
{
public:
    SnapshotCommand(std::size_t page, core::PageObject object, QByteArray png)
        : m_page(page), m_object(std::move(object)), m_png(std::move(png))
    {}
    void redo(core::Document& document, core::Changes& changes) override
    {
        const std::string key = document.addResource(m_png, "png");
        if (!m_inner) {
            if (auto* image = std::get_if<core::ImageObject>(&core::body(m_object)))
                image->resource = key;
            m_inner = std::make_unique<core::AddObjectCommand>(m_page, m_object);
        }
        m_inner->redo(document, changes);
    }
    void undo(core::Document& document, core::Changes& changes) override
    {
        if (m_inner)
            m_inner->undo(document, changes);
    }

private:
    std::size_t m_page;
    core::PageObject m_object;
    QByteArray m_png;
    std::unique_ptr<core::AddObjectCommand> m_inner;
};

QString messagePage(const QString& text)
{
    return QStringLiteral("<!DOCTYPE html><html><body "
                          "style=\"font-family:sans-serif;color:#555\"><p>%1</p>"
                          "</body></html>")
        .arg(escaped(text));
}

// Texte de la page : tout `<` devient `&lt;` (du texte pour le moteur).
void appendText(QString& out, QStringView part)
{
    qsizetype from = 0;
    for (qsizetype lt = part.indexOf(u'<'); lt >= 0; lt = part.indexOf(u'<', from)) {
        out += part.mid(from, lt - from);
        out += QStringLiteral("&lt;");
        from = lt + 1;
    }
    out += part.mid(from);
}

// Éléments dont le moteur lit le contenu en texte, sans balise (RCDATA, RAWTEXT, PLAINTEXT), et ce que la
// réécriture en fait. noscript n'en est pas : que le moteur le lise en HTML (JavaScript coupé) ou en texte,
// rien n'en sort, la sortie ne portant de `<` qu'en tête de ses propres balises.
enum class RawText
{
    None,
    Css,    // style : feuille réécrite
    Text,   // textarea, title (RCDATA) : texte, entités rendues par le moteur
    Pre,    // xmp, plaintext (RAWTEXT montré) : un pre de texte échappé
    Hidden, // iframe, noembed, noframes : contenu jamais montré, élément vide
    Script  // jamais lu ni montré : rien
};

RawText rawText(const QString& lower)
{
    if (lower == QLatin1String("style"))
        return RawText::Css;
    if (lower == QLatin1String("textarea") || lower == QLatin1String("title"))
        return RawText::Text;
    if (lower == QLatin1String("xmp") || lower == QLatin1String("plaintext"))
        return RawText::Pre;
    if (lower == QLatin1String("iframe") || lower == QLatin1String("noembed") ||
        lower == QLatin1String("noframes"))
        return RawText::Hidden;
    if (lower == QLatin1String("script"))
        return RawText::Script;
    return RawText::None;
}

// Fin du texte d'un élément `lower` : `</nom` suivi d'un blanc, de `/` ou de `>`, comme le moteur la lit ;
// `text.size()` si absente.
qsizetype rawTextEnd(QStringView text, qsizetype from, const QString& lower)
{
    const QString closer = QStringLiteral("</") + lower;
    for (qsizetype close = from; (close = text.indexOf(closer, close, Qt::CaseInsensitive)) >= 0;) {
        const qsizetype after = close + closer.size();
        if (after < text.size() && (isHtmlSpace(text[after]) || text[after] == u'/' || text[after] == u'>'))
            return close;
        close = after;
    }
    return text.size();
}

// Feuille d'un <style> qui pourrait porter une entité (`&#…`, `&nom`) : en contenu étranger (SVG, MathML), le
// moteur les rend avant CSS (`u&#114;l(` y devient `url(`) ; une telle feuille est vidée, quel que soit le
// contexte (en HTML, une entité n'y est qu'un texte que CSS rejette). Le `&` de CSS imbriqué reste.
bool mayHoldEntity(QStringView sheet)
{
    for (qsizetype k = sheet.indexOf(u'&'); k >= 0 && k + 1 < sheet.size(); k = sheet.indexOf(u'&', k + 1)) {
        if (sheet[k + 1] == u'#' || isAsciiLetter(sheet[k + 1]))
            return true;
    }
    return false;
}

// Corps d'une réponse en texte : jeu de caractères de l'en-tête, sinon celui que la page déclare ; un type
// autre que HTML est montré en texte.
QString decodeBody(const QByteArray& bytes, const QString& type)
{
    QStringDecoder decoder = QStringDecoder::decoderForHtml(bytes);
    thread_local const QRegularExpression charsetRe(QStringLiteral(R"(charset\s*=\s*"?([^";\s]+))"));
    const QRegularExpressionMatch charset = charsetRe.match(type);
    if (charset.hasMatch()) {
        if (const auto encoding = QStringConverter::encodingForName(charset.captured(1)))
            decoder = QStringDecoder(*encoding);
    }
    QString html = decoder.decode(bytes);
    if (!type.isEmpty() && !type.contains(QLatin1String("html")))
        html = QStringLiteral("<pre>%1</pre>").arg(html.toHtmlEscaped());
    return html;
}

} // namespace

WebFrameItems::Prepared WebFrameItems::prepareHtml(const QString& html, const QUrl& base,
                                                   const std::function<Verdict(const QUrl&)>& decide)
{
    // Une lecture, linéaire, à la manière du tokeniseur HTML du moteur : chaque
    // balise de la sortie est réécrite ici, tout `<` qui n'ouvre pas une balise
    // reconnue devient du texte (`&lt;`), les commentaires partent, le contenu
    // d'un <style> est réécrit comme CSS. Rien de la page ne passe tel quel entre
    // `<` et
    // `>` : une forme que la réécriture ne connaît pas ne demande rien.
    Rewriter rewriter(base, decide);
    const QStringView text(html);
    const qsizetype n = text.size();
    QString body;
    body.reserve(n + n / 8);
    qsizetype i = 0;
    while (i < n) {
        const qsizetype lt = text.indexOf(u'<', i);
        if (lt < 0) {
            body += text.mid(i);
            break;
        }
        body += text.mid(i, lt - i);
        i = lt;
        const QStringView rest = text.mid(i);
        if (rest.startsWith(u"<!--")) {
            const qsizetype end = text.indexOf(u"-->", i + 2);
            i = end < 0 ? n : end + 3; // jamais fermé : le reste est commentaire,
                                       // comme pour le moteur
            continue;
        }
        if (rest.startsWith(u"<!doctype", Qt::CaseInsensitive)) {
            const qsizetype end = text.indexOf(u'>', i);
            if (end < 0) {
                appendText(body, rest);
                break;
            }
            body += u'<';
            appendText(body, text.mid(i + 1, end - i));
            i = end + 1;
            continue;
        }
        const bool closing = i + 1 < n && text[i + 1] == u'/';
        const qsizetype nameStart = i + (closing ? 2 : 1);
        const qsizetype length = tagNameLength(text, nameStart);
        if (length == 0) {
            body += QStringLiteral("&lt;");
            ++i;
            continue;
        }
        const QString name = text.mid(nameStart, length).toString();
        QVector<Attribute> attrs;
        bool selfClosing = false;
        const qsizetype end = readAttributes(text, nameStart + length, attrs, selfClosing);
        if (end < 0) { // balise jamais fermée : le moteur l'ignorerait ; ici, du texte
            appendText(body, rest);
            break;
        }
        i = end;
        if (closing) {
            body += QStringLiteral("</") + name + QLatin1Char('>');
            continue;
        }
        const QString lower = name.toLower();
        const RawText raw = rawText(lower);
        // Une feuille de style s'ouvre même par `<style/>` (le moteur l'ouvre en HTML) ; un autre élément à
        // texte fermé par `/>` (SVG) n'a pas de contenu : la suite est lue comme balises, ce qui ne laisse
        // rien passer même si le moteur y lisait du texte.
        if (raw == RawText::None || (raw != RawText::Css && selfClosing)) {
            body += rewriter.tag(name, attrs, selfClosing);
            continue;
        }
        // Contenu jusqu'à `</nom` suivi d'un blanc, de `/` ou de `>`, comme le moteur le lit (plaintext :
        // jusqu'à la fin), réécrit selon l'élément et fermé ici dans tous les cas : rien de la suite ne reste
        // dans l'élément, rien du contenu n'y devient balise.
        const qsizetype close = lower == QLatin1String("plaintext") ? n : rawTextEnd(text, i, lower);
        const QStringView inner = text.mid(i, close - i);
        switch (raw) {
        case RawText::Css:
            body += rewriter.tag(name, attrs, false);
            if (!mayHoldEntity(inner))
                body += rewriter.css(inner, rewriter.m_images);
            body += QStringLiteral("</style>");
            break;
        case RawText::Text:
            body += rewriter.tag(name, attrs, false);
            appendText(body, inner);
            body += QStringLiteral("</") + lower + QLatin1Char('>');
            break;
        case RawText::Pre:
            body += rewriter.tag(QStringLiteral("pre"), attrs, false);
            body += inner.toString().toHtmlEscaped();
            body += QStringLiteral("</pre>");
            break;
        case RawText::Hidden:
            body += rewriter.tag(name, attrs, false);
            body += QStringLiteral("</") + lower + QLatin1Char('>');
            break;
        case RawText::Script:
        case RawText::None:
            break;
        }
        i = close;
        if (close < n) {
            QVector<Attribute> ignored;
            bool unused = false;
            const qsizetype after = readAttributes(text, close + 2 + lower.size(), ignored, unused);
            if (after < 0) {
                appendText(body, text.mid(close));
                break;
            }
            i = after;
        }
    }
    Prepared prepared;
    const QString head = QStringLiteral("<meta http-equiv=\"Content-Security-Policy\" content=\"%1\">"
                                        "<meta name=\"referrer\" content=\"no-referrer\">")
                             .arg(rewriter.policy());
    // Après le doctype (sinon le moteur passerait en mode de compatibilité),
    // avant tout le reste : le parseur range ces balises dans l'en-tête, avant la
    // première ressource.
    thread_local const QRegularExpression doctypeRe(QStringLiteral(R"(^\s*<!doctype[^>]*>)"),
                                                    QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch doctype = doctypeRe.match(body);
    if (doctype.hasMatch())
        prepared.html = body.left(doctype.capturedEnd()) + head + body.mid(doctype.capturedEnd());
    else
        prepared.html = head + body;
    prepared.links = rewriter.m_links;
    prepared.allowed = rewriter.all();
    prepared.blocked = rewriter.m_blocked;
    return prepared;
}

ProfileController* WebFrameItems::profile() const noexcept
{
    return m_profile;
}

void WebFrameItems::setProfile(ProfileController* profile)
{
    if (m_profile == profile)
        return;
    disconnect(m_profileConnection);
    m_profile = profile;
    if (profile)
        m_profileConnection =
            connect(profile, &ProfileController::profileChanged, this, &WebFrameItems::readGuards);
    readGuards();
    emit profileChanged();
}

// Le seul signal disponible, `profileChanged`, part aussi pour une couleur : setGuardsEnabled ne fait rien si
// la valeur ne change pas (cadrage n° 10).
void WebFrameItems::readGuards()
{
    if (m_profile)
        setGuardsEnabled(
            m_profile->setting(QStringLiteral("tools"), QStringLiteral("web-frame-guards"), false).toBool());
}

void WebFrameItems::setGuardsEnabled(bool enabled)
{
    if (m_guardsEnabled == enabled)
        return;
    m_guardsEnabled = enabled;
    for (int i = 0; i < m_rows.size(); ++i) {
        Row& row = m_rows[i];
        row.history.clear(); // E12 : la session du cadre est oubliée
        row.historyIndex = -1;
        row.content.clear(); // une vue neuve ne reçoit jamais le contenu de l'autre régime
        row.directUrl = QUrl();
        releaseContentFile(row);
        prepareRow(row);
        rowChanged(i);
    }
    emit guardsEnabledChanged();
    emit countChanged(); // blockedCount
}

QString WebFrameItems::engineKind()
{
#if !ACTCRAFT_WEBFRAME
    return QStringLiteral("none");
#elif defined(Q_OS_ANDROID)
    return QStringLiteral("system");
#else
#ifdef Q_OS_WIN
    // Reste PC 4 : le moteur n'est pas prouvé hors écran sous Windows (cadrage n° 30).
    if (QGuiApplication::platformName() == QLatin1String("offscreen"))
        return QStringLiteral("none");
#endif
    return QStringLiteral("engine");
#endif
}

WebFrameItems::WebFrameItems(QObject* parent) : QAbstractListModel(parent) {}

WebFrameItems::~WebFrameItems()
{
    if (!m_key.isEmpty())
        m_policy.forget(m_key);
}

DocumentController* WebFrameItems::document() const noexcept
{
    return m_document;
}

void WebFrameItems::setDocument(DocumentController* document)
{
    if (m_document == document)
        return;
    for (QMetaObject::Connection& c : m_connections)
        disconnect(c);
    m_document = document;
    if (document) {
        m_connections[0] =
            connect(document, &DocumentController::documentChanged, this, &WebFrameItems::rebuild);
        m_connections[1] =
            connect(document, &DocumentController::pagesChanged, this, &WebFrameItems::rebuild);
        m_connections[2] =
            connect(document, &DocumentController::liftingChanged, this, &WebFrameItems::rebuild);
        m_connections[3] = connect(document, &DocumentController::fileUrlChanged, this, [this] {
            rebuild();
            fileChanged();
        });
        // Ouverture réussie : le document est reçu, même sans fichier (un
        // .flipchart, décision 18).
        m_connections[4] =
            connect(document, &DocumentController::loadFinished, this, [this](bool ok, const QString&) {
                if (!ok || !m_document)
                    return;
                m_policy.setReceived(m_key, true);
                for (int i = 0; i < m_rows.size(); ++i) {
                    prepareRow(m_rows[i]);
                    rowChanged(i);
                }
                emit countChanged();
            });
    }
    m_generation = 0;
    m_page = -1;
    rebuild();
    emit documentChanged();
}

// Enregistrer ou « Enregistrer sous » donne un autre dossier au document sans changer sa génération : la
// base d'un HTML saisi le suit (n° 11), et les lignes de ce HTML sont repréparées (gardes coupées).
void WebFrameItems::fileChanged()
{
    if (!m_document)
        return;
    const QUrl file = m_document->fileUrl();
    const QUrl base = file.isLocalFile()
                          ? QUrl::fromLocalFile(QFileInfo(file.toLocalFile()).absolutePath() + u'/')
                          : QUrl();
    if (base == m_resourceBase)
        return;
    m_resourceBase = base;
    if (m_guardsEnabled)
        return;
    for (int i = 0; i < m_rows.size(); ++i) {
        if (!m_rows[i].address.trimmed().isEmpty())
            continue;
        prepareRow(m_rows[i]);
        rowChanged(i);
    }
}

void WebFrameItems::documentReplaced()
{
    if (!m_key.isEmpty())
        m_policy.forget(m_key);
    m_generation = m_document->generation();
    m_key = QStringLiteral("document-%1").arg(++m_keys);
    const QUrl file = m_document->fileUrl();
    m_policy.setReceived(m_key, file.isValid() && !file.isEmpty());
    if (file.isLocalFile())
        m_policy.setResourceFolder(m_key, QFileInfo(file.toLocalFile()).absolutePath());
    m_resourceBase = file.isLocalFile()
                         ? QUrl::fromLocalFile(QFileInfo(file.toLocalFile()).absolutePath() + u'/')
                         : QUrl();
    m_page = -1; // tout est relu
    emit documentKeyChanged();
}

namespace {

std::uint64_t g_htmlConversions = 0; // fil de l'interface seulement (rebuild)

QString htmlOf(const core::WebFrame& frame)
{
    ++g_htmlConversions;
    return QString::fromStdString(frame.html);
}

// Même texte, sans convertir `utf8` en QString d'un bloc : décodage par morceaux de 4 Kio, chacun comparé à
// la suite du texte (le décodeur garde un caractère coupé entre deux morceaux). Pour un contenu inchangé,
// rebuild ne paie donc ni la conversion ni une seconde copie.
bool sameText(const QString& text, const std::string& utf8)
{
    constexpr qsizetype kChunk = 4096;
    const qsizetype size = static_cast<qsizetype>(utf8.size());
    QStringDecoder decoder(QStringConverter::Utf8);
    qsizetype at = 0;
    for (qsizetype from = 0; from < size; from += kChunk) {
        const QString part = decoder(QByteArrayView(utf8.data() + from, std::min(kChunk, size - from)));
        if (QStringView(text).mid(at, part.size()) != QStringView(part))
            return false;
        at += part.size();
    }
    return at == text.size();
}

} // namespace

std::uint64_t WebFrameItems::htmlConversionCount() noexcept
{
    return g_htmlConversions;
}

void WebFrameItems::rebuild()
{
    if (!m_document) {
        if (!m_rows.isEmpty()) {
            for (Row& row : m_rows)
                releaseContentFile(row);
            beginResetModel();
            m_rows.clear();
            endResetModel();
            emit countChanged();
        }
        return;
    }
    if (m_document->isLifting())
        return; // objets soulevés hors de la page : les vues restent, relues au
                // dépôt
    if (m_key.isEmpty() || m_document->generation() != m_generation)
        documentReplaced();
    const int page = m_document->currentIndex();
    const bool samePage = page == m_page;
    m_page = page;
    // Cadres visibles de la page. Leur texte reste dans le document (std::string) : il est comparé à ce que
    // le modèle garde AVANT toute conversion en QString, et seuls un cadre neuf ou un contenu changé sont
    // convertis (htmlOf, compté par htmlConversionCount).
    struct Source
    {
        std::uint64_t id = 0;
        QRectF bounds;
        const core::WebFrame* frame = nullptr;
    };
    QVector<Source> sources;
    std::uint64_t pageKey = 0;
    const core::Document& doc = m_document->document();
    if (page >= 0 && static_cast<std::size_t>(page) < doc.pageCount()) {
        for (const core::PageObject& object : doc.page(static_cast<std::size_t>(page)).objects) {
            const auto* frame = std::get_if<core::WebFrame>(&core::body(object));
            if (!frame || !object.common.visible)
                continue;
            sources.append({object.common.id, pageBox(object, *frame), frame});
            pageKey = pageKey == 0 ? object.common.id : std::min<std::uint64_t>(pageKey, object.common.id);
        }
    }
    m_pageKey = pageKey; // avant toute décision (prepareRow)
    bool sameIds = samePage && sources.size() == m_rows.size();
    for (qsizetype i = 0; sameIds && i < sources.size(); ++i)
        sameIds = sources[i].id == m_rows[i].id;
    if (sameIds) {
        for (int i = 0; i < m_rows.size(); ++i) {
            Row& row = m_rows[i];
            const Source& source = sources[i];
            const bool contentChanged =
                !sameText(row.html, source.frame->html) || !sameText(row.address, source.frame->address);
            if (!contentChanged && row.bounds == source.bounds)
                continue;
            row.bounds = source.bounds;
            if (contentChanged) {
                row.html = htmlOf(*source.frame);
                row.address = QString::fromStdString(source.frame->address);
                row.history.clear();
                row.historyIndex = -1;
                prepareRow(row);
            }
            rowChanged(i);
        }
        return;
    }
    // Cadres gardés d'une relecture à l'autre de la même page : leur contenu
    // reste.
    QVector<Row> fresh;
    fresh.reserve(sources.size());
    for (const Source& source : sources) {
        const int old = samePage ? rowOf(source.id) : -1;
        if (old >= 0 && sameText(m_rows[old].html, source.frame->html) &&
            sameText(m_rows[old].address, source.frame->address)) {
            Row kept = m_rows[old];
            kept.bounds = source.bounds;
            fresh.append(std::move(kept));
            continue;
        }
        Row row;
        row.id = source.id;
        row.bounds = source.bounds;
        row.html = htmlOf(*source.frame);
        row.address = QString::fromStdString(source.frame->address);
        row.serial = 0;
        prepareRow(row);
        fresh.append(std::move(row));
    }
    // Réponses en attente d'une ligne recréée ou disparue : laissées finir sans effet (le numéro de
    // navigation, unique pour toute la vie du modèle, les écarterait déjà ; on les retire de la table).
    for (auto it = m_replies.begin(); it != m_replies.end();) {
        bool kept = false;
        for (const Row& row : fresh)
            kept = kept || (row.id == it->id && row.request == it->request);
        if (kept) {
            ++it;
            continue;
        }
        QNetworkReply* reply = it.key();
        it = m_replies.erase(it);
        reply->disconnect(this); // elle finit seule (le serveur la voit), sans plus rien changer au modèle
        connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
    }
    // Lignes parties (autre page, contenu changé) : leur fichier de HTML lourd part avec elles. Une ligne
    // neuve n'en a pas encore (il arrive par la boucle d'événements) : seule une ligne gardée peut porter le
    // même.
    for (Row& old : m_rows) {
        const bool kept = std::any_of(fresh.cbegin(), fresh.cend(),
                                      [&old](const Row& row) { return row.contentFile == old.contentFile; });
        if (!kept)
            releaseContentFile(old);
    }
    beginResetModel();
    m_rows = std::move(fresh);
    endResetModel();
    if (m_pendingRow >= m_rows.size()) {
        m_pendingRow = -1;
        emit unblockPendingChanged();
    }
    emit countChanged();
}

Verdict WebFrameItems::decide(const QUrl& url) const
{
    return m_policy.decide(url, m_key, m_pageKey);
}

void WebFrameItems::prepareRow(Row& row)
{
    row.request = ++m_requests; // une réponse réseau en vol d'une navigation précédente ne vaut plus
    row.blocked = false;
    row.refused = false;
    row.loading = false;
    row.pendingUrl = QUrl();
    row.viewUrl = QUrl(); // l'état rapporté par la vue ne vaut plus pour un contenu repris
    row.viewTitle.clear();
    row.viewCanBack = row.viewCanForward = false;
    if (!m_guardsEnabled) {
        showDirect(row);
        return;
    }
    if (!row.address.trimmed().isEmpty()) {
        const QUrl url = QUrl::fromUserInput(row.address.trimmed());
        if (row.historyIndex >= 0 && row.historyIndex < row.history.size())
            navigate(row, row.history[row.historyIndex], false);
        else
            navigate(row, url, true);
        return;
    }
    showContent(row, row.html, QUrl());
}

QString WebFrameItems::heavyDir()
{
    if (!m_heavyOverride.isEmpty())
        return m_heavyOverride;
    if (!m_heavy)
        m_heavy = std::make_unique<QTemporaryDir>();
    return m_heavy->isValid() ? m_heavy->path() : QString();
}

void WebFrameItems::dropShown(Row& row)
{
    releaseContentFile(row);
    row.directUrl = QUrl();
    row.links.clear();
}

bool WebFrameItems::sameAsPreviousCapture(const QImage& image, bool first)
{
    QByteArray print;
    if (!image.isNull()) {
        // Format, taille et pixels (lignes sans leur remplissage) : empreinte rapide non cryptographique (une
        // capture pèse 8 Mo à 1920 x 1080, jusqu'à 20 fois par image fixe, sur le fil de l'interface).
        size_t seed = qHashMulti(0, static_cast<int>(image.format()), image.width(), image.height());
        const qsizetype rowBytes =
            (static_cast<qsizetype>(image.width()) * image.pixelFormat().bitsPerPixel() + 7) / 8;
        for (int y = 0; y < image.height(); ++y)
            seed = qHashBits(image.constScanLine(y), static_cast<size_t>(rowBytes), seed);
        print = QByteArray::number(static_cast<qulonglong>(seed)) + ':' + QByteArray::number(image.width()) +
                'x' + QByteArray::number(image.height());
    }
    const bool same = !first && !print.isEmpty() && print == m_lastCapture;
    m_lastCapture = print;
    return same;
}

void WebFrameItems::releaseContentFile(Row& row)
{
    if (!row.contentFile.isEmpty())
        QFile::remove(row.contentFile.toLocalFile());
    row.contentFile = QUrl();
}

// Gardes coupées (n° 11) : rien n'est lu ni réécrit. Un HTML court part tout de suite ; un long passe d'abord
// par un fil secondaire, qui compte et écrit le fichier si la limite de loadHtml est dépassée.
void WebFrameItems::showDirect(Row& row)
{
    row.links.clear();
    row.directUrl = QUrl();
    const QString address = row.address.trimmed();
    if (!address.isEmpty()) {
        row.preparing = 0;
        row.content.clear();
        releaseContentFile(row);
        row.directUrl = QUrl::fromUserInput(address);
        row.baseUrl = row.directUrl;
        row.viewBase = QUrl();
        if (isYouTubePlayer(row.directUrl)) {
            row.content = playerPage(row.directUrl);
            row.viewBase = kPlayerIdentity;
            row.directUrl = QUrl();
        }
        row.loading = false;
        ++row.serial;
        return;
    }
    if (row.html.size() <= kSurelyShortHtml) {
        row.preparing = 0;
        releaseContentFile(row);
        row.content = row.html;
        row.baseUrl = m_resourceBase;
        row.viewBase = embedsYouTubePlayer(row.html) ? kPlayerIdentity : m_resourceBase;
        row.loading = false;
        ++row.serial;
        return;
    }
    const std::uint64_t ticket = ++m_tickets;
    row.preparing = ticket;
    row.loading = true;
    const QPointer<WebFrameItems> self(this);
    QThreadPool::globalInstance()->start([self, id = row.id, ticket, html = row.html, dir = heavyDir()] {
        bool failed = false;
        const QUrl file = writeHeavyHtml(html, dir, ticket, failed);
        const bool player = embedsYouTubePlayer(html); // HTML moyen : sans fichier, il part à loadHtml
        if (QCoreApplication* home = QCoreApplication::instance()) {
            QMetaObject::invokeMethod(
                home,
                [self, id, ticket, file, failed, player] {
                    if (self)
                        self->finishDirect(id, ticket, file, failed, player);
                    else if (!file.isEmpty())
                        QFile::remove(file.toLocalFile());
                },
                Qt::QueuedConnection);
        }
    });
}

void WebFrameItems::finishDirect(std::uint64_t id, std::uint64_t ticket, const QUrl& file, bool failed,
                                 bool player)
{
    const int index = rowOf(id);
    if (index < 0 || m_rows[index].preparing != ticket) {
        if (!file.isEmpty())
            QFile::remove(file.toLocalFile()); // Review Focus 2 : résultat périmé
        return;
    }
    Row& row = m_rows[index];
    row.preparing = 0;
    releaseContentFile(row);
    row.contentFile = file;
    row.content = failed ? messagePage(tr("Page trop lourde : le fichier n'a pas pu être écrit.")) : row.html;
    row.baseUrl = m_resourceBase;
    // Au-delà de la limite de loadHtml, la vue lit le fichier (file:, sans Referer : limite connue).
    row.viewBase = player && !failed ? kPlayerIdentity : m_resourceBase;
    row.loading = false;
    ++row.serial;
    rowChanged(index);
    emit countChanged();
}

void WebFrameItems::showContent(Row& row, const QString& html, const QUrl& base)
{
    prepareContent(row, [html] { return html; }, base);
}

void WebFrameItems::prepareContent(Row& row, std::function<QString()> source, const QUrl& base)
{
    // Lecture et réécriture hors du fil de l'interface (BRIEF § 4.1). La politique n'est pas partagée entre
    // fils : le fil secondaire lit une copie figée, prise ici (valeurs à partage implicite), qui répond pour
    // ce document et cette page à cet instant. Le résultat revient au fil du modèle par la boucle
    // d'événements ; il est abandonné si le cadre est parti ou a changé de contenu entre-temps (jeton), comme
    // une réponse réseau périmée (fetched).
    const std::uint64_t ticket = ++m_tickets;
    row.preparing = ticket;
    row.loading = true;
    const QString dir = heavyDir();
    const QPointer<WebFrameItems> self(this);
    QThreadPool::globalInstance()->start([self, id = row.id, ticket, base, policy = m_policy, key = m_key,
                                          page = m_pageKey, source = std::move(source), dir] {
        const Prepared prepared =
            prepareHtml(source(), base, [&](const QUrl& url) { return policy.decide(url, key, page); });
        bool failed = false;
        const QUrl heavy = writeHeavyHtml(prepared.html, dir, ticket, failed); // même limite (n° 38)
        if (QCoreApplication* home = QCoreApplication::instance()) {
            QMetaObject::invokeMethod(
                home,
                [self, id, ticket, base, prepared, heavy, failed] {
                    if (self)
                        self->finishContent(id, ticket, base, prepared, heavy, failed);
                    else if (!heavy.isEmpty())
                        QFile::remove(heavy.toLocalFile());
                },
                Qt::QueuedConnection);
        }
    });
}

void WebFrameItems::finishContent(std::uint64_t id, std::uint64_t ticket, const QUrl& base,
                                  const Prepared& prepared, const QUrl& heavy, bool failed)
{
    const int index = rowOf(id);
    if (index < 0 || m_rows[index].preparing != ticket) {
        if (!heavy.isEmpty())
            QFile::remove(heavy.toLocalFile());
        return; // cadre parti, ou passé à un autre contenu
    }
    Row& row = m_rows[index];
    row.preparing = 0;
    releaseContentFile(row);
    row.contentFile = heavy;
    row.directUrl = QUrl();
    row.content =
        failed ? messagePage(tr("Page trop lourde : le fichier n'a pas pu être écrit.")) : prepared.html;
    row.links = failed ? QVector<QUrl>() : prepared.links;
    row.baseUrl = base;
    row.blocked = prepared.blocked; // sous-ressource d'un document reçu, page non débloquée : repère
    row.loading = false;
    ++row.serial;
    rowChanged(index);
    emit countChanged(); // blockedCount
}

void WebFrameItems::navigate(Row& row, const QUrl& url, bool pushHistory)
{
    if (pushHistory) {
        row.history.resize(row.historyIndex + 1);
        row.history.append(url);
        row.historyIndex = static_cast<int>(row.history.size()) - 1;
    }
    row.request = ++m_requests; // une réponse en vol de la navigation précédente ne vaut plus (fichier local
                                // et about: compris : `serial` n'avance qu'à l'arrivée de leur contenu)
    row.preparing = 0;          // une réécriture en cours ne vaut plus
    row.blocked = false;
    row.refused = false;
    row.pendingUrl = QUrl();
    switch (decide(url)) {
    case Verdict::Blocked:
        row.blocked = true;
        row.pendingUrl = url;
        row.content.clear();
        releaseContentFile(row);
        row.directUrl = QUrl();
        row.links.clear();
        row.loading = false;
        ++row.serial;
        return;
    case Verdict::Refuse:
        row.refused = true;
        row.content.clear();
        releaseContentFile(row);
        row.directUrl = QUrl();
        row.links.clear();
        row.loading = false;
        ++row.serial;
        return;
    case Verdict::Load:
        break;
    }
    if (url.isLocalFile()) {
        const QString path = url.toLocalFile();
        const QString unreadable = messagePage(tr("Fichier illisible."));
        const QFileInfo info(path);
        if (!info.isFile() || !info.isReadable() || info.size() > kMaxBody) {
            showContent(row, unreadable, QUrl());
            return;
        }
        prepareContent(
            row,
            [path, unreadable] { // lu par le fil secondaire
                QFile file(path);
                if (file.size() > kMaxBody || !file.open(QIODevice::ReadOnly))
                    return unreadable;
                return QString(QStringDecoder::decoderForHtml(file.peek(4096)).decode(file.readAll()));
            },
            url);
        return;
    }
    if (url.scheme() == QLatin1String("about")) {
        showContent(row, QString(), QUrl());
        return;
    }
    fetch(row, url, 0);
}

void WebFrameItems::fetch(Row& row, const QUrl& url, int redirects)
{
    if (!m_network)
        m_network = new QNetworkAccessManager(this);
    QNetworkRequest request(url);
    // Chaque redirection revient ici, décidée avant d'être suivie.
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setAttribute(QNetworkRequest::CookieSaveControlAttribute, QNetworkRequest::Manual);
    request.setAttribute(QNetworkRequest::CookieLoadControlAttribute, QNetworkRequest::Manual);
    request.setTransferTimeout(kTransferTimeoutMs);
    QNetworkReply* reply = m_network->get(request);
    row.preparing = 0;
    row.loading = true;
    ++row.serial;
    m_replies.insert(reply, Pending{row.id, row.request, redirects});
    connect(reply, &QNetworkReply::downloadProgress, reply, [reply](qint64 received, qint64) {
        if (received > kMaxBody)
            reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply] { fetched(reply); });
}

void WebFrameItems::fetched(QNetworkReply* reply)
{
    reply->deleteLater();
    const Pending pending = m_replies.take(reply);
    const int index = rowOf(pending.id);
    if (index < 0 || m_rows[index].request != pending.request)
        return; // cadre parti, ou passé à une autre navigation
    Row& row = m_rows[index];
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QUrl target = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    if (status >= 300 && status < 400 && target.isValid()) {
        const QUrl next = reply->url().resolved(target);
        if (pending.redirects >= kMaxRedirects) {
            row.refused = true;
            showContent(row, messagePage(tr("Trop de redirections.")), QUrl());
        } else {
            switch (decide(next)) {
            case Verdict::Load:
                if (next.scheme() == QLatin1String("http") || next.scheme() == QLatin1String("https")) {
                    fetch(row, next, pending.redirects + 1);
                    rowChanged(index);
                    return;
                }
                row.refused = true;
                row.content.clear();
                dropShown(row);
                break;
            case Verdict::Blocked:
                row.blocked = true;
                row.pendingUrl = next;
                row.content.clear();
                dropShown(row);
                break;
            case Verdict::Refuse:
                row.refused = true;
                row.content.clear();
                dropShown(row);
                break;
            }
            row.loading = false;
            ++row.serial;
        }
        rowChanged(index);
        emit countChanged();
        return;
    }
    if (reply->error() != QNetworkReply::NoError) {
        showContent(row, messagePage(tr("Page indisponible : %1").arg(reply->errorString())), QUrl());
        rowChanged(index);
        return;
    }
    const QByteArray bytes = reply->readAll();
    const QString type = reply->header(QNetworkRequest::ContentTypeHeader).toString().toLower();
    const QUrl base = reply->url();
    if (type.startsWith(QLatin1String("image/"))) {
        showContent(row,
                    QStringLiteral("<img src=\"%1\" style=\"max-width:100%\">")
                        .arg(QString::fromLatin1(base.toEncoded()).toHtmlEscaped()),
                    base);
    } else {
        prepareContent(
            row, [bytes, type] { return decodeBody(bytes, type); }, base); // décodé par le fil secondaire
    }
    rowChanged(index);
}

int WebFrameItems::rowOf(std::uint64_t id) const
{
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows[i].id == id)
            return i;
    }
    return -1;
}

void WebFrameItems::rowChanged(int row)
{
    const QModelIndex i = index(row, 0);
    emit dataChanged(i, i);
}

int WebFrameItems::blockedCount() const noexcept
{
    return static_cast<int>(
        std::count_if(m_rows.cbegin(), m_rows.cend(), [](const Row& r) { return r.blocked; }));
}

int WebFrameItems::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : count();
}

QVariant WebFrameItems::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};
    const Row& row = m_rows[index.row()];
    switch (role) {
    case ObjectIdRole:
        return QVariant::fromValue<quint64>(row.id);
    case BoundsRole:
        return row.bounds;
    case HtmlRole:
        return row.html;
    case AddressRole:
        return row.address;
    case BlockedRole:
        return row.blocked;
    case RefusedRole:
        return row.refused;
    case LoadingRole:
        return row.loading;
    case ContentRole:
        return row.content;
    case BaseUrlRole:
        return row.baseUrl;
    case TitleRole: {
        if (!m_guardsEnabled && !row.viewTitle.isEmpty() && !namesThePlayerIdentity(row))
            return row.viewTitle;
        const QUrl shown = shownUrl(row);
        return shown.host().isEmpty() ? tr("Cadre web") : shown.host();
    }
    case CanGoBackRole:
        return m_guardsEnabled ? row.historyIndex > 0 : row.viewCanBack;
    case CanGoForwardRole:
        return m_guardsEnabled ? row.historyIndex >= 0 && row.historyIndex + 1 < row.history.size()
                               : row.viewCanForward;
    case SerialRole:
        return row.serial;
    case DirectUrlRole:
        return row.directUrl;
    case ContentFileRole:
        return row.contentFile;
    case ExternalUrlRole:
        return externalUrlOf(row);
    case ViewBaseRole:
        return m_guardsEnabled ? QUrl() : row.viewBase;
    default:
        return {};
    }
}

QHash<int, QByteArray> WebFrameItems::roleNames() const
{
    return {{ObjectIdRole, "objectId"},
            {BoundsRole, "bounds"},
            {HtmlRole, "html"},
            {AddressRole, "address"},
            {BlockedRole, "blocked"},
            {RefusedRole, "refused"},
            {LoadingRole, "loading"},
            {ContentRole, "content"},
            {BaseUrlRole, "baseUrl"},
            {TitleRole, "title"},
            {CanGoBackRole, "canGoBack"},
            {CanGoForwardRole, "canGoForward"},
            {SerialRole, "serial"},
            {DirectUrlRole, "directUrl"},
            {ContentFileRole, "contentFile"},
            {ExternalUrlRole, "externalUrl"},
            {ViewBaseRole, "viewBase"}};
}

namespace {
bool isWebAddress(const QUrl& url)
{
    const QString scheme = url.scheme().toLower();
    return (scheme == QLatin1String("http") || scheme == QLatin1String("https")) && url.isValid() &&
           !url.host().isEmpty();
}
} // namespace

QUrl WebFrameItems::externalUrlOf(const Row& row) const
{
    const QUrl shown = shownUrl(row);
    if (isWebAddress(shown))
        return shown.adjusted(QUrl::RemoveUserInfo);
    const QString typed = row.address.trimmed();
    if (typed.isEmpty())
        return {};
    const QUrl url = QUrl::fromUserInput(typed);
    return isWebAddress(url) ? url.adjusted(QUrl::RemoveUserInfo) : QUrl();
}

QUrl WebFrameItems::externalUrl(int row) const
{
    return row >= 0 && row < m_rows.size() ? externalUrlOf(m_rows[row]) : QUrl();
}

bool WebFrameItems::openExternally(int row)
{
    const QUrl url = externalUrl(row);
    if (url.isEmpty())
        return false;
    const bool opened = m_opener ? m_opener(url) : QDesktopServices::openUrl(url);
    if (!opened)
        emit externalOpenFailed(tr("Le navigateur du système n’a pas pu ouvrir %1.").arg(url.toString()));
    return opened;
}

bool WebFrameItems::insertFrame(const QString& html, const QString& address)
{
    if (!m_document || m_document->isBusy())
        return false;
    const QString code = html.trimmed().isEmpty() ? QString() : html;
    QString typed = address.trimmed();
    if (code.isEmpty() && typed.isEmpty())
        return false;
    if (!typed.isEmpty()) {
        const QUrl url = QUrl::fromUserInput(typed);
        if (!url.isValid())
            return false;
        m_policy.typedByUser(m_key, url);
        typed = url.toString();
    }
    const int page = m_document->currentIndex();
    const QSizeF size = m_document->pageSize();
    const double width = std::min(640.0, size.width() * 0.8);
    const double height = std::min(400.0, size.height() * 0.8);
    core::WebFrame frame;
    frame.bounds = {(size.width() - width) / 2, (size.height() - height) / 2, width, height};
    frame.html = code.toStdString();
    frame.address = typed.toStdString();
    core::PageObject object = frame;
    object.common.layer = core::Layer::Middle;
    m_document->execute(std::make_unique<core::AddObjectCommand>(static_cast<std::size_t>(page), object));
    return true;
}

bool WebFrameItems::resizeFrame(int row, const QRectF& bounds)
{
    if (!m_document || row < 0 || row >= m_rows.size() || bounds.width() < 1 || bounds.height() < 1)
        return false;
    const QRectF old = m_rows[row].bounds;
    if (old == bounds || old.width() <= 0 || old.height() <= 0)
        return false;
    const core::Transform delta = core::compose(
        core::compose(core::translation(-old.x(), -old.y()),
                      core::scaling(bounds.width() / old.width(), bounds.height() / old.height())),
        core::translation(bounds.x(), bounds.y()));
    m_document->execute(std::make_unique<core::TransformObjectsCommand>(
        static_cast<std::size_t>(m_page), std::vector<core::ObjectId>{m_rows[row].id}, delta));
    return true;
}

void WebFrameItems::requestUnblock(int row)
{
    if (row < 0 || row >= m_rows.size() || !m_rows[row].blocked)
        return;
    m_pendingRow = row;
    emit unblockPendingChanged();
}

void WebFrameItems::answer(bool confirmed)
{
    if (m_pendingRow < 0)
        return;
    m_pendingRow = -1;
    emit unblockPendingChanged();
    if (!confirmed)
        return;
    m_policy.unblock(m_key, m_pageKey); // cette page de ce document seulement
    for (int i = 0; i < m_rows.size(); ++i) {
        Row& row = m_rows[i];
        if (!row.blocked && row.preparing == 0)
            continue; // réécriture en cours : reprise avec la politique débloquée
        if (row.pendingUrl.isValid()) {
            const QUrl url = row.pendingUrl;
            navigate(row, url, false);
        } else {
            prepareRow(row); // code HTML ou page locale : relu, ses sous-ressources
                             // décidées à nouveau
        }
        rowChanged(i);
    }
    emit countChanged();
}

bool WebFrameItems::followLink(int row, const QUrl& viewUrl)
{
    if (row < 0 || row >= m_rows.size() || !viewUrl.isValid())
        return false;
    if (!m_guardsEnabled)
        return false; // gardes coupées : la vue navigue seule
    Row& r = m_rows[row];
    const QString fragment = viewUrl.fragment();
    if (fragment.startsWith(kLinkPrefix)) {
        bool ok = false;
        const int n = fragment.mid(kLinkPrefix.size()).toInt(&ok);
        if (!ok || n < 0 || n >= r.links.size())
            return true;
        navigate(r, r.links[n], true);
        rowChanged(row);
        emit countChanged();
        return true;
    }
    // Rendus à la vue : about:blank et data: (contenu donné par loadHtml, avec ses renvois de fragment) ; le
    // fichier du HTML lourd. Toute autre adresse, même celle de la page affichée, serait chargée sans la
    // règle de sécurité du contenu : rendue au modèle, la vue s'arrête.
    const QUrl page = viewUrl.adjusted(QUrl::RemoveFragment);
    if (page.isEmpty() || page == QUrl(QStringLiteral("about:blank")) ||
        page.scheme() == QLatin1String("data"))
        return false;
    if (!r.contentFile.isEmpty() && page.isLocalFile() &&
        QFileInfo(page.toLocalFile()).canonicalFilePath() ==
            QFileInfo(r.contentFile.toLocalFile()).canonicalFilePath())
        return false;
    navigate(r, page, true); // navigation du moteur : rendue à la politique
    rowChanged(row);
    emit countChanged();
    return true;
}

void WebFrameItems::goBack(int row)
{
    if (row < 0 || row >= m_rows.size())
        return;
    if (!m_guardsEnabled) {
        emit viewCommand(row, QStringLiteral("back")); // l'historique est celui du moteur
        return;
    }
    if (m_rows[row].historyIndex <= 0)
        return;
    Row& r = m_rows[row];
    --r.historyIndex;
    navigate(r, r.history[r.historyIndex], false);
    rowChanged(row);
    emit countChanged();
}

void WebFrameItems::goForward(int row)
{
    if (row < 0 || row >= m_rows.size())
        return;
    if (!m_guardsEnabled) {
        emit viewCommand(row, QStringLiteral("forward"));
        return;
    }
    Row& r = m_rows[row];
    if (r.historyIndex < 0 || r.historyIndex + 1 >= r.history.size())
        return;
    ++r.historyIndex;
    navigate(r, r.history[r.historyIndex], false);
    rowChanged(row);
    emit countChanged();
}

// Gardes coupées : l'état de la vue (adresse, titre, historique) revient au modèle sans rien recharger ;
// gardes allumées, l'historique et le titre sont ceux du modèle. `serial` n'avance pas.
void WebFrameItems::reportView(int row, const QUrl& url, bool canBack, bool canForward, const QString& title,
                               bool loading)
{
    if (m_guardsEnabled || row < 0 || row >= m_rows.size())
        return;
    Row& r = m_rows[row];
    if (r.viewUrl == url && r.viewCanBack == canBack && r.viewCanForward == canForward &&
        r.viewTitle == title && r.loading == loading)
        return;
    r.viewUrl = url;
    r.viewCanBack = canBack;
    r.viewCanForward = canForward;
    r.viewTitle = title;
    r.loading = loading;
    rowChanged(row);
}

void WebFrameItems::reload(int row)
{
    if (row < 0 || row >= m_rows.size())
        return;
    if (!m_guardsEnabled) {
        emit viewCommand(row, QStringLiteral("reload"));
        return;
    }
    prepareRow(m_rows[row]);
    rowChanged(row);
    emit countChanged();
}

bool WebFrameItems::snapshot(int row)
{
    if (!m_document || !m_grabber || row < 0 || row >= m_rows.size())
        return false;
    return insertSnapshot(row, m_grabber(m_rows[row].bounds));
}

bool WebFrameItems::insertSnapshot(int row, const QImage& image)
{
    if (!m_document || row < 0 || row >= m_rows.size() || image.isNull())
        return false;
    const QRectF zone = m_rows[row].bounds;
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    if (!image.save(&buffer, "PNG"))
        return false;
    core::ImageObject picture;
    picture.bounds = {zone.x(), zone.y(), zone.width(), zone.height()};
    picture.sourceWidth = image.width();
    picture.sourceHeight = image.height();
    core::PageObject object = picture;
    object.common.layer = core::Layer::Middle;
    object.common.name = tr("Image du cadre web").toStdString();
    m_document->execute(std::make_unique<SnapshotCommand>(static_cast<std::size_t>(m_page), std::move(object),
                                                          std::move(png)));
    return true;
}

void WebFrameItems::useWindowCapture(QQuickItem* pageView)
{
    const QPointer<QQuickItem> view(pageView);
    m_grabber = [view](const QRectF& bounds) -> QImage {
        if (!view || !view->window())
            return {};
        QPointF a;
        QPointF b;
        if (!QMetaObject::invokeMethod(view, "pageToScene", Q_RETURN_ARG(QPointF, a),
                                       Q_ARG(QPointF, bounds.topLeft())) ||
            !QMetaObject::invokeMethod(view, "pageToScene", Q_RETURN_ARG(QPointF, b),
                                       Q_ARG(QPointF, bounds.bottomRight())))
            return {};
        QQuickWindow* window = view->window();
        QScreen* screen = window->screen();
        if (!screen)
            return {};
        const QRect zone = QRectF(a, b).normalized().toAlignedRect();
        // La fenêtre telle qu'à l'écran : la vue native y est, à la différence de
        // grabToImage.
        return screen->grabWindow(window->winId(), zone.x(), zone.y(), zone.width(), zone.height()).toImage();
    };
}

} // namespace actcraft::ui
