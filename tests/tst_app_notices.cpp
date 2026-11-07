#include <actcraft/ui/appinfo.h>

#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QtTest>

// Notices des composants tiers (L11-T25, décision 34, S1.1) : tout ce que livrent les paquets a sa notice
// dans THIRD_PARTY_NOTICES.md, chaque licence citée a son texte sous LICENSES/<identifiant SPDX>.txt, et « À
// propos » montre le fichier du dépôt. Les fichiers sont lus dans les sources (ACTCRAFT_SOURCE_DIR), pas dans
// la ressource.

namespace {

QString sourcePath(const QString& relative)
{
    return QString::fromUtf8(ACTCRAFT_SOURCE_DIR) + u'/' + relative;
}

QString readSource(const QString& relative)
{
    QFile file(sourcePath(relative));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QString::fromUtf8(file.readAll());
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

// Identifiants d'une expression SPDX : opérateurs OR, AND, WITH et parenthèses retirés.
QStringList spdxIds(QString expression)
{
    expression.replace(u'(', u' ').replace(u')', u' ');
    static const QRegularExpression blanks(QStringLiteral("\\s+"));
    QStringList ids;
    const QStringList tokens = expression.split(blanks, Qt::SkipEmptyParts);
    for (const QString& token : tokens) {
        if (token != u"OR" && token != u"AND" && token != u"WITH")
            ids << token;
    }
    return ids;
}

// Éléments de liste du Markdown (« - … »), lignes de suite (indentées) jointes par un blanc.
QStringList listItems(const QString& markdown)
{
    QStringList items;
    const QStringList lines = markdown.split(u'\n');
    for (const QString& line : lines) {
        if (line.startsWith(QStringLiteral("- ")))
            items << line;
        else if (!items.isEmpty() && line.startsWith(QStringLiteral("  ")) && !line.trimmed().isEmpty())
            items.last() += u' ' + line.trimmed();
        else if (!items.isEmpty() && !items.last().endsWith(u'\n'))
            items.last() += u'\n'; // fin de l'élément : plus aucune suite jointe
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
    // « - `<motif>` … Licence SPDX : `<expression entière>` », et chaque dossier de third_party/ est cité.
    void everyOtherLibraryHasANotice()
    {
        const QStringList lines = listLines(QStringLiteral("packaging/autres-autorises.txt"));
        QVERIFY(!lines.isEmpty());
        const QString notices = readSource(QStringLiteral("THIRD_PARTY_NOTICES.md"));
        const QStringList items = listItems(notices);
        static const QRegularExpression idShape(QStringLiteral("^[A-Za-z0-9.+-]+$"));
        QStringList missing;
        for (const QString& line : lines) {
            const OtherLibrary library = parseOtherLibrary(line);
            QVERIFY2(!library.expression.isEmpty(), qPrintable(QStringLiteral("sans licence : ") + line));
            const QStringList ids = spdxIds(library.expression);
            QVERIFY2(!ids.isEmpty(), qPrintable(line));
            for (const QString& id : ids)
                QVERIFY2(idShape.match(id).hasMatch(), qPrintable(QStringLiteral("identifiant : ") + id));
            if (!hasItemFor(items, library.pattern, library.expression))
                missing << line;
        }
        QVERIFY2(missing.isEmpty(), qPrintable(QStringLiteral("sans notice : ") + missing.join(u" | ")));

        const QStringList dirs =
            QDir(sourcePath(QStringLiteral("third_party"))).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        QVERIFY(dirs.size() >= 3);
        for (const QString& dir : dirs) {
            QVERIFY2(notices.contains(QStringLiteral("`third_party/") + dir + u'/'),
                     qPrintable(QStringLiteral("third_party/") + dir));
        }
    }

    // Chaque identifiant des expressions citées (notices et autres-autorises.txt) a son texte sous
    // LICENSES/<id>.txt ; chaque chemin LICENSES/… cité existe ; aucun texte de LICENSES/ n'est orphelin.
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
            for (const QString& id : split)
                ids.insert(id);
        }
        QStringList missing;
        for (const QString& id : std::as_const(ids)) {
            const QFileInfo text(sourcePath(QStringLiteral("LICENSES/") + id + QStringLiteral(".txt")));
            if (!text.isFile() || text.size() < 100)
                missing << id;
        }
        missing.sort();
        QVERIFY2(missing.isEmpty(), qPrintable(QStringLiteral("sans texte : ") + missing.join(u", ")));

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
