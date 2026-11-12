#include <actcraft/ui/appinfo.h>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <QtTest>

// Notices des composants tiers (L11-T25, décision 34, S1.1) : tout ce que livrent les paquets a sa notice
// dans THIRD_PARTY_NOTICES.md, chaque licence citée a son texte sous LICENSES/<identifiant SPDX>.txt, livré
// aussi dans la ressource de l'application (APK compris), et « À propos » montre le fichier du dépôt. Les
// fichiers sont lus dans les sources (ACTCRAFT_SOURCE_DIR) et comparés à la ressource.

namespace {

// Empreintes SHA-256 des textes de LICENSES/, relevés le 2026-11-07 : textes de la liste SPDX
// (https://github.com/spdx/license-list-data, dossier text/), LicenseRef-* de Qt (qt/qtbase v6.12.0, dossier
// LICENSES/), LicenseRef-Microsoft-Redistributable écrit par ActCraft (renvoi, sans conditions reproduites).
// Un texte modifié ou paraphrasé change son empreinte : en ajouter un = le relever à la source, puis écrire
// son empreinte ici.
const QHash<QString, QByteArray>& pinnedTexts()
{
    static const QHash<QString, QByteArray> pinned = {
        {QStringLiteral("0BSD"), "e3f18c71e10d673590eb9856c1d79dd3b4b0d65404efb5e8584dbede7edd608b"},
        {QStringLiteral("AFL-2.1"), "fbedf33db9a433cf87c6f0974f7e488599ee0c9446ed543235c99785ce7d22e6"},
        {QStringLiteral("Apache-2.0"), "074e6e32c86a4c0ef8b3ed25b721ca23aca83df277cd88106ef7177c354615ff"},
        {QStringLiteral("BSD-2-Clause"), "f32fb3b417a194167cfad068223fc975ba96c5960513a10f66a3c28720aec1df"},
        {QStringLiteral("BSD-3-Clause"), "5a93d5831e1297ab10fe643e1a631e83be392896da14ee2951285a79012df69d"},
        {QStringLiteral("BSD-Source-Code"),
         "1f1b677d26ab73e159a55c3a914cf12d888e18a9d0d469fedab4aa6e5a48b1c0"},
        {QStringLiteral("BSL-1.0"), "84c6ef3ea9e3254a54d0acf5d3e0c61ae011b8fef7dd6940591cf060e6380a8f"},
        {QStringLiteral("Bitstream-Vera"),
         "5b3b4d3cef4029bda987a71f4bfb6087e40c5e91a2cb853fe7f6ccb78d9d8e38"},
        {QStringLiteral("CC0-1.0"), "a2010f343487d3f7618affe54f789f5487602331c0a8d03f49e9a7c547cf0499"},
        {QStringLiteral("FTL"), "ced6622122ce451cb1ea0c3c3f507a640e2a44c075c04900ddd9fae8acb5369f"},
        {QStringLiteral("GPL-2.0-only"), "aaf135472f81c5b4a0dca9367e5bb5e9750032b5bebe5442b36e4c0a47430df3"},
        {QStringLiteral("GPL-2.0-or-later"),
         "aaf135472f81c5b4a0dca9367e5bb5e9750032b5bebe5442b36e4c0a47430df3"},
        {QStringLiteral("GPL-3.0-only"), "fb981668c18a279e285fc4d83fba1e836cc84dd4daa73c9697d3cfd2d8aca6e0"},
        {QStringLiteral("IJG"), "7658542977bfdced9e1059a6c934ce4281de76d103b831007b825917dc62511a"},
        {QStringLiteral("ISC"), "521c6f0ed8e64736684f89843d16a4df6b4b7449acbcfc6ca6630a3037ca8c53"},
        {QStringLiteral("Imlib2"), "19d21c4402df7020d631511274d991393267fd56b59aa014c0be90becd7631e4"},
        {QStringLiteral("LGPL-2.1-or-later"),
         "5749785c8bdefafcb5d798270ed0a967036fe2ca63dcedade1627565dfef81d2"},
        {QStringLiteral("LGPL-3.0-only"), "996af0513df21f7496288951c41428a03c174e9e4a9d63665c57d670f845ccb1"},
        {QStringLiteral("LLVM-exception"),
         "e34c58338bd89d43e709e226610d8f32b3e3c47f4ad9a99a8dc1d4ac7842488e"},
        {QStringLiteral("Libpng"), "7667a8c88c7a63690244988d626bcddd27ed895526e2c3ab1a9adb463a5fa287"},
        {QStringLiteral("LicenseRef-BSD-3-Clause-with-PCRE2-Binary-Like-Packages-Exception"),
         "0526625fcc746a42d3f19a0ab357d3dc8479fdbe3bfd168825498f50215c7a63"},
        {QStringLiteral("LicenseRef-ICC-License"),
         "9e34ff47b3a44814e183342e7e198d127a6740e29d20e8381fba7688dafc79dd"},
        {QStringLiteral("LicenseRef-Lcs-Telegraphics"),
         "c10225ebbd5191bfdd761b0d7752c785f2453ab665d6c454473fb12d254668aa"},
        {QStringLiteral("LicenseRef-Microsoft-Redistributable"),
         "c3ef96d5af9cd623ff7c601e41f80492a4051a3026dcba367fe01ae292bfa3a2"},
        {QStringLiteral("LicenseRef-SHA1-Public-Domain"),
         "65d86b8ec6b1cd8b8f6e64fd8710274ba00d6a3c520e00dfdcd1c3d2071f3305"},
        {QStringLiteral("MIT-0"), "59746d6285ffa44bfc7ecada352aa5d6a20dc8eab418a60ce091cc739012c135"},
        {QStringLiteral("MIT-open-group"),
         "d5942d51b7e079b0eab4d6f9e06f1b8625f918b8802117a9c3127c004c5df9b6"},
        {QStringLiteral("MIT"), "b05785f9f18e6716bab63424b11454513b9943a222595b70411009202fc592b5"},
        {QStringLiteral("MPL-2.0"), "66a3107d5ad6a058aab753eaac2047ccb2ed0e39465dd0fe5844da3e300d5172"},
        {QStringLiteral("Unicode-3.0"), "f7db81051789b729fea528a63ec4c938fdcb93d9d61d97dc8cc2e9df6d47f2a1"},
        {QStringLiteral("Unlicense"), "0bdebfeda07d45dada625ae1317c6f833186e798b171d0db640bcf32e92a8240"},
        {QStringLiteral("Zlib"), "bfb1112d49db5b1daecdfef24bd7e2f3ea0bafb33aa67aa0ab51e2bf8407c03d"},
        {QStringLiteral("libpng-2.0"), "f207d0f7375b977f76ce2437962c1c12a646f15cda3df48d5472ac95eb68e8ff"},
        {QStringLiteral("libtiff"), "a6ecaa20c8c1b7a8215ed05e5f58764f821596e36d31acc05282c6154cf0dc44"},
    };
    return pinned;
}

QString sourcePath(const QString& relative)
{
    return QString::fromUtf8(ACTCRAFT_SOURCE_DIR) + u'/' + relative;
}

QByteArray readBytes(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return file.readAll();
}

QString readSource(const QString& relative)
{
    return QString::fromUtf8(readBytes(sourcePath(relative)));
}

// Lignes utiles d'une liste de packaging/ : ni vides, ni commentaires (« # »).
QStringList listLines(const QString& relative)
{
    QStringList lines;
    const QStringList raw = readSource(relative).split(u'\n');
    for (const QString& line : raw) {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty() && !trimmed.startsWith(u'#'))
            lines << trimmed;
    }
    return lines;
}

struct OtherLibrary
{
    QString pattern;
    QString expression;
};

// Format de l'amendement 3 : le PREMIER blanc sépare le motif de l'expression SPDX, qui peut contenir des
// blancs (« libfreetype.*.dylib FTL OR GPL-2.0-or-later »). Couper à chaque blanc tronquerait l'expression.
OtherLibrary parseOtherLibrary(const QString& line)
{
    static const QRegularExpression blank(QStringLiteral("\\s"));
    const qsizetype at = line.indexOf(blank);
    if (at < 0)
        return {line, {}};
    return {line.left(at), line.mid(at).trimmed()};
}

// Identifiants d'une expression SPDX bien formée : identifiants reliés par OR ou AND, « <id> WITH <exception>
// », parenthèses équilibrées. Liste vide si l'expression est mal formée (« MIT GPL-2.0-only », « MIT OR », «
// WITH X »).
QStringList spdxIds(QString expression)
{
    expression.replace(u'(', QStringLiteral(" ( ")).replace(u')', QStringLiteral(" ) "));
    static const QRegularExpression blanks(QStringLiteral("\\s+"));
    static const QRegularExpression idShape(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9.+-]*$"));
    enum class Last
    {
        Start,
        Open,
        Close,
        Id,
        Operator,
        With
    };
    Last last = Last::Start;
    int depth = 0;
    QStringList ids;
    const QStringList tokens = expression.split(blanks, Qt::SkipEmptyParts);
    for (const QString& token : tokens) {
        const bool expectId = last == Last::Start || last == Last::Open || last == Last::Operator;
        if (token == u"(") {
            if (!expectId)
                return {};
            ++depth;
            last = Last::Open;
        } else if (token == u")") {
            if (depth == 0 || (last != Last::Id && last != Last::Close))
                return {};
            --depth;
            last = Last::Close;
        } else if (token == u"OR" || token == u"AND") {
            if (last != Last::Id && last != Last::Close)
                return {};
            last = Last::Operator;
        } else if (token == u"WITH") {
            if (last != Last::Id)
                return {};
            last = Last::With;
        } else {
            if ((!expectId && last != Last::With) || !idShape.match(token).hasMatch())
                return {};
            ids << token;
            last = Last::Id;
        }
    }
    if (depth != 0 || (last != Last::Id && last != Last::Close))
        return {};
    return ids;
}

// Éléments de liste du Markdown (« - … »), lignes de suite (indentées) jointes par un blanc.
QStringList listItems(const QString& markdown)
{
    QStringList items;
    bool open = false; // l'élément courant accepte encore des lignes de suite
    const QStringList lines = markdown.split(u'\n');
    for (const QString& line : lines) {
        if (line.startsWith(QStringLiteral("- "))) {
            items << line;
            open = true;
        } else if (open && line.startsWith(QStringLiteral("  ")) && !line.trimmed().isEmpty()) {
            items.last() += u' ' + line.trimmed();
        } else {
            open = false;
        }
    }
    for (QString& item : items)
        item = item.trimmed();
    return items;
}

// Expressions citées par les notices, sous la forme « Licence SPDX : `<expression>` ».
QStringList citedExpressions(const QString& markdown)
{
    QString flat = markdown;
    static const QRegularExpression wrap(QStringLiteral("\\n[ \\t]*"));
    flat.replace(wrap, QStringLiteral(" "));
    static const QRegularExpression cited(QStringLiteral("Licence SPDX : `([^`]+)`"));
    QStringList expressions;
    auto it = cited.globalMatch(flat);
    while (it.hasNext())
        expressions << it.next().captured(1);
    return expressions;
}

bool hasItemFor(const QStringList& items, const QString& name, const QString& expression = {})
{
    const QString head = QStringLiteral("- `") + name + u'`';
    for (const QString& item : items) {
        if (!item.startsWith(head))
            continue;
        if (expression.isEmpty() || item.contains(QStringLiteral("Licence SPDX : `") + expression + u'`'))
            return true;
    }
    return false;
}

} // namespace

class TstAppNotices : public QObject
{
    Q_OBJECT
private slots:
    // Un élément de liste « - `Qt6<Module>` … » par module de packaging/qt-autorises.txt, et aucun autre.
    void everyAllowedQtModuleHasANotice()
    {
        const QStringList modules = listLines(QStringLiteral("packaging/qt-autorises.txt"));
        QVERIFY(modules.size() > 10);
        const QStringList items = listItems(readSource(QStringLiteral("THIRD_PARTY_NOTICES.md")));
        QStringList missing;
        for (const QString& module : modules) {
            if (!hasItemFor(items, module))
                missing << module;
        }
        QVERIFY2(missing.isEmpty(), qPrintable(QStringLiteral("sans notice : ") + missing.join(u", ")));

        static const QRegularExpression moduleItem(QStringLiteral("^- `(Qt6[A-Za-z0-9]+)`"));
        QStringList unknown;
        for (const QString& item : items) {
            const auto match = moduleItem.match(item);
            if (match.hasMatch() && !modules.contains(match.captured(1)))
                unknown << match.captured(1);
        }
        QVERIFY2(unknown.isEmpty(), qPrintable(QStringLiteral("hors liste : ") + unknown.join(u", ")));
    }

    // Chaque ligne de packaging/autres-autorises.txt (« <motif> <expression SPDX> ») a son élément
    // « - `<motif>` … Licence SPDX : `<expression entière>` » ; chaque élément qui cite une licence nomme ses
    // titulaires (« Copyright … », ou la réserve « Copyright : à relever au PC ») ; chaque dossier de
    // third_party/ est cité.
    void everyOtherLibraryHasANotice()
    {
        const QStringList lines = listLines(QStringLiteral("packaging/autres-autorises.txt"));
        QVERIFY(!lines.isEmpty());
        const QString notices = readSource(QStringLiteral("THIRD_PARTY_NOTICES.md"));
        const QStringList items = listItems(notices);
        QStringList missing;
        for (const QString& line : lines) {
            const OtherLibrary library = parseOtherLibrary(line);
            QVERIFY2(!library.expression.isEmpty(), qPrintable(QStringLiteral("sans licence : ") + line));
            QVERIFY2(!spdxIds(library.expression).isEmpty(),
                     qPrintable(QStringLiteral("expression invalide : ") + line));
            if (!hasItemFor(items, library.pattern, library.expression))
                missing << line;
        }
        QVERIFY2(missing.isEmpty(), qPrintable(QStringLiteral("sans notice : ") + missing.join(u" | ")));

        QStringList anonymous;
        for (const QString& item : items) {
            if (item.contains(QStringLiteral("Licence SPDX : `")) &&
                !item.contains(QStringLiteral("Copyright")))
                anonymous << item.left(60);
        }
        QVERIFY2(anonymous.isEmpty(),
                 qPrintable(QStringLiteral("sans copyright : ") + anonymous.join(u" | ")));

        const QStringList dirs =
            QDir(sourcePath(QStringLiteral("third_party"))).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        QVERIFY(dirs.size() >= 3);
        for (const QString& dir : dirs) {
            QVERIFY2(notices.contains(QStringLiteral("`third_party/") + dir + u'/'),
                     qPrintable(QStringLiteral("third_party/") + dir));
        }
    }

    // Chaque identifiant des expressions citées (notices et autres-autorises.txt, expressions bien formées) a
    // son texte sous LICENSES/<id>.txt, d'empreinte relevée, et le même texte dans la ressource
    // :/actcraft/LICENSES/ (livrée dans l'APK) ; chaque chemin LICENSES/… cité existe ; aucun texte de
    // LICENSES/ n'est orphelin.
    void everyCitedLicenseHasItsText()
    {
        const QString notices = readSource(QStringLiteral("THIRD_PARTY_NOTICES.md"));
        QStringList expressions = citedExpressions(notices);
        QVERIFY(expressions.size() > 10);
        const QStringList lines = listLines(QStringLiteral("packaging/autres-autorises.txt"));
        for (const QString& line : lines)
            expressions << parseOtherLibrary(line).expression;

        QSet<QString> ids;
        for (const QString& expression : std::as_const(expressions)) {
            const QStringList split = spdxIds(expression);
            QVERIFY2(!split.isEmpty(), qPrintable(QStringLiteral("expression invalide : ") + expression));
            for (const QString& id : split)
                ids.insert(id);
        }

        const actcraft::ui::AppInfo info; // enregistre la ressource ui_notices
        QStringList missing;
        QStringList changed;
        QStringList notInResource;
        for (const QString& id : std::as_const(ids)) {
            const QByteArray text =
                readBytes(sourcePath(QStringLiteral("LICENSES/") + id + QStringLiteral(".txt")));
            if (text.size() < 100) {
                missing << id;
                continue;
            }
            const QByteArray sha = QCryptographicHash::hash(text, QCryptographicHash::Sha256).toHex();
            if (pinnedTexts().value(id) != sha)
                changed << id;
            if (readBytes(QStringLiteral(":/actcraft/LICENSES/") + id + QStringLiteral(".txt")) != text)
                notInResource << id;
        }
        missing.sort();
        changed.sort();
        notInResource.sort();
        QVERIFY2(missing.isEmpty(), qPrintable(QStringLiteral("sans texte : ") + missing.join(u", ")));
        QVERIFY2(changed.isEmpty(),
                 qPrintable(QStringLiteral("empreinte inattendue : ") + changed.join(u", ")));
        QVERIFY2(notInResource.isEmpty(),
                 qPrintable(QStringLiteral("absent de la ressource : ") + notInResource.join(u", ")));

        static const QRegularExpression path(QStringLiteral("LICENSES/([A-Za-z0-9.+-]+)\\.txt"));
        auto it = path.globalMatch(notices);
        while (it.hasNext()) {
            const QString id = it.next().captured(1);
            QVERIFY2(ids.contains(id), qPrintable(QStringLiteral("chemin cité sans expression : ") + id));
        }

        const QStringList texts =
            QDir(sourcePath(QStringLiteral("LICENSES"))).entryList({QStringLiteral("*.txt")}, QDir::Files);
        for (const QString& text : texts) {
            const QString id = text.chopped(4);
            QVERIFY2(ids.contains(id), qPrintable(QStringLiteral("texte orphelin : ") + text));
        }
    }

    // « À propos » (AppInfo::notices, ressource :/actcraft/THIRD_PARTY_NOTICES.md) montre le fichier du
    // dépôt.
    void aboutShowsTheSameNotices()
    {
        const QString file = readSource(QStringLiteral("THIRD_PARTY_NOTICES.md"));
        QVERIFY(file.size() > 1000);
        actcraft::ui::AppInfo info;
        QCOMPARE(info.notices(), file);
    }
};

QTEST_GUILESS_MAIN(TstAppNotices)
#include "tst_app_notices.moc"
