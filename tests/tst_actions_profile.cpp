#include <actcraft/actions/profilestore.h>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QtTest>

#include <memory>

using namespace actcraft::actions;

namespace {
bool writeFile(const QString& path, const QByteArray& bytes)
{
    QFile f(path);
    return f.open(QIODevice::WriteOnly) && f.write(bytes) == bytes.size();
}
QByteArray readFile(const QString& path)
{
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}
QJsonObject objectOf(const QByteArray& json)
{
    return QJsonDocument::fromJson(json).object();
}
// Liste récursive d'un dossier, chemins relatifs triés (amendement 3, n2).
QStringList treeOf(const QString& root)
{
    QStringList entries;
    QDirIterator it(root, QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden,
                    QDirIterator::Subdirectories);
    while (it.hasNext())
        entries << QDir(root).relativeFilePath(it.next());
    entries.sort();
    return entries;
}
Profile sample()
{
    Profile p;
    p.id = QStringLiteral("essai");
    p.label = QStringLiteral("Essai");
    p.iconPixels = 32;
    p.palette.dock = Dock::Left;
    p.palette.collapsed = true;
    p.palette.pinned = false;
    p.palette.swatches = 24;
    p.browsers.visible = true;
    p.browsers.current = QStringLiteral("objets"); // identifiants de navigators (amendement 4, A8)
    p.extraColors = {QStringLiteral("#12AB34")};
    p.recentTools = {QStringLiteral("present.curtain")};
    p.pinned = {QStringLiteral("math.ruler")};
    p.fitPage = true;
    p.settings.insert(QStringLiteral("objects"), QJsonObject{{QStringLiteral("auto-group"), false}});
    return p;
}
const QStringList kOrder{QStringLiteral("b1"), QStringLiteral("b2")};
} // namespace

class TstActionsProfile : public QObject
{
    Q_OBJECT
    std::unique_ptr<QTemporaryDir> m_user;
    QTemporaryDir m_builtin;
    ProfileStore store() const { return ProfileStore(m_user->path(), m_builtin.path(), kOrder); }
private slots:
    void init()
    {
        m_user = std::make_unique<QTemporaryDir>();
        QVERIFY(m_user->isValid());
        QVERIFY(writeFile(m_builtin.filePath(QStringLiteral("b1.json")),
                          R"({"format":"actcraft-profile/1","id":"b1","label":"Un"})"));
        QVERIFY(writeFile(m_builtin.filePath(QStringLiteral("b2.json")),
                          R"({"format":"actcraft-profile/1","id":"b2","label":"Deux"})"));
    }
    void roundTripKeepsEveryField()
    {
        Profile back;
        QString error;
        QVERIFY2(profileFromJson(profileToJson(sample()), back, error), qPrintable(error));
        QCOMPARE(profileToJson(back), profileToJson(sample()));
        QCOMPARE(back.palette.dock, Dock::Left);
        QCOMPARE(back.iconPixels, 32);
        QCOMPARE(back.extraColors, QStringList{QStringLiteral("#12AB34")});
        QCOMPARE(back.browsers.current, QStringLiteral("objets"));
        QVERIFY(back.unknownKeys.isEmpty());
    }
    // Discrimine le patron interdit : une écriture de la seule structure perdrait « future ».
    void unknownKeysKept()
    {
        Profile p;
        QString error;
        QVERIFY(profileFromJson(R"({"format":"actcraft-profile/1","id":"x","label":"X","future":{"a":1}})", p,
                                error));
        QCOMPARE(p.unknownKeys.keys(), QStringList{QStringLiteral("future")});
        const QJsonObject again = objectOf(profileToJson(p));
        QCOMPARE(again.value(QStringLiteral("future")).toObject().value(QStringLiteral("a")).toInt(), 1);
        // Une clé inconnue n'écrase jamais une clé connue.
        p.unknownKeys.insert(QStringLiteral("id"), QStringLiteral("autre"));
        QCOMPARE(objectOf(profileToJson(p)).value(QStringLiteral("id")).toString(), QStringLiteral("x"));
    }
    // Le magasin ne reconstruit jamais un profil depuis ses seuls champs : enregistrer, exporter et importer
    // gardent les clés d'une version plus récente.
    void unknownKeysSurviveTheStore()
    {
        auto s = store();
        QString error;
        QVERIFY(writeFile(m_user->filePath(QStringLiteral("u.json")),
                          R"({"format":"actcraft-profile/1","id":"u","label":"U","future":[1,2]})"));
        Profile p = *s.load(QStringLiteral("u"), error);
        p.iconPixels = 20;
        QVERIFY2(s.save(p, error), qPrintable(error));
        QCOMPARE(
            objectOf(readFile(m_user->filePath(QStringLiteral("u.json")))).value(QStringLiteral("future")),
            QJsonValue(QJsonArray{1, 2}));
        const QString file = m_user->filePath(QStringLiteral("u-export.json"));
        QVERIFY2(s.exportTo(QStringLiteral("u"), file, error), qPrintable(error));
        QTemporaryDir other;
        ProfileStore fresh(other.path(), m_builtin.path(), kOrder);
        const auto id = fresh.importFrom(file, error);
        QVERIFY2(id, qPrintable(error));
        QCOMPARE(
            objectOf(readFile(other.filePath(*id + QStringLiteral(".json")))).value(QStringLiteral("future")),
            QJsonValue(QJsonArray{1, 2}));
    }
    void majorVersionRefused()
    {
        Profile p;
        QString error;
        QVERIFY(!profileFromJson(R"({"format":"actcraft-profile/2","id":"x","label":"X"})", p, error));
        QVERIFY(!error.isEmpty());
        error.clear();
        QVERIFY(!profileFromJson(R"({"format":"actcraft-profile/10","id":"x","label":"X"})", p, error));
        QVERIFY(!error.isEmpty());
        error.clear();
        QVERIFY(!profileFromJson(R"({"id":"x","label":"X"})", p, error));
        QVERIFY(!error.isEmpty());
        error.clear();
        QVERIFY(!profileFromJson("{pas du json", p, error));
        QVERIFY(!error.isEmpty());
        QVERIFY2(profileFromJson(R"({"format":"actcraft-profile/1.4","id":"x","label":"X"})", p, error),
                 qPrintable(error));
    }
    void iconPixelsClamped()
    {
        QCOMPARE(clampIconPixels(8), 16);
        QCOMPARE(clampIconPixels(40), 32);
        QCOMPARE(clampIconPixels(20), 20);
        Profile p;
        QString error;
        QVERIFY(profileFromJson(R"({"format":"actcraft-profile/1","id":"x","label":"X","icon-pixels":99})", p,
                                error));
        QCOMPARE(p.iconPixels, 32);
    }
    void outOfRangeValuesFallBack()
    {
        Profile p;
        QString error;
        QVERIFY(profileFromJson(R"({"format":"actcraft-profile/1","id":"x","label":"X",)"
                                R"("palette":{"dock":"diagonale","swatches":12},)"
                                R"("recent-tools":["a","b","c","d","e","f","g","h","i","j"]})",
                                p, error));
        QCOMPARE(p.palette.dock, Dock::Right);
        QCOMPARE(p.palette.swatches, 16);
        QCOMPARE(p.recentTools.size(), 8);
        QCOMPARE(p.recentTools.first(), QStringLiteral("a"));
        QVERIFY(p.unknownKeys.isEmpty());
    }
    // Amendement 4 (A8) : identifiants de BrowserPanel.navigators ; une valeur inconnue devient « pages » et
    // n'est pas gardée.
    void browserCurrentUsesNavigatorIds()
    {
        Profile p;
        QString error;
        QVERIFY(profileFromJson(R"({"format":"actcraft-profile/1","id":"x","label":"X",)"
                                R"("browsers":{"current":"proprietes"}})",
                                p, error));
        QCOMPARE(p.browsers.current, QStringLiteral("proprietes"));
        QVERIFY(profileFromJson(R"({"format":"actcraft-profile/1","id":"x","label":"X",)"
                                R"("browsers":{"current":"tabs"}})",
                                p, error));
        QCOMPARE(p.browsers.current, QStringLiteral("pages"));
        QVERIFY(!profileToJson(p).contains("tabs"));
        QCOMPARE(Profile().browsers.current, QStringLiteral("pages"));
    }
    void builtinFirstThenUser()
    {
        auto s = store();
        QString error;
        QVERIFY(s.saveAs(sample(), QStringLiteral("a-moi"), QStringLiteral("À moi"), error));
        QCOMPARE(s.ids(), (QStringList{QStringLiteral("b1"), QStringLiteral("b2"), QStringLiteral("a-moi")}));
        QVERIFY(!s.saveAs(sample(), QStringLiteral("a-moi"), QStringLiteral("Encore"), error));
        QVERIFY(!s.saveAs(sample(), QStringLiteral("b1"), QStringLiteral("Encore"), error));
        QCOMPARE(s.load(QStringLiteral("a-moi"), error)->label, QStringLiteral("À moi"));
    }
    void userCopyShadowsBuiltin()
    {
        auto s = store();
        QString error;
        Profile p = *s.load(QStringLiteral("b1"), error);
        p.iconPixels = 20;
        QVERIFY(s.save(p, error));
        QVERIFY(s.hasUserCopy(QStringLiteral("b1")));
        QCOMPARE(s.load(QStringLiteral("b1"), error)->iconPixels, 20);
        QCOMPARE(s.ids().count(QStringLiteral("b1")), 1);
    }
    void resetRemovesTheUserCopy()
    {
        auto s = store();
        QString error;
        Profile p = *s.load(QStringLiteral("b2"), error);
        p.iconPixels = 18;
        QVERIFY(s.save(p, error));
        QVERIFY(QDir().mkpath(s.directory(QStringLiteral("b2"))));
        QVERIFY(s.reset(QStringLiteral("b2"), error));
        QCOMPARE(s.load(QStringLiteral("b2"), error)->iconPixels, 24);
        QVERIFY(!s.hasUserCopy(QStringLiteral("b2")));
        QVERIFY(!QFileInfo::exists(s.directory(QStringLiteral("b2"))));
    }
    void builtinCannotBeRemoved()
    {
        auto s = store();
        QString error;
        QVERIFY(!s.remove(QStringLiteral("b1"), error));
        QVERIFY(!error.isEmpty());
        QVERIFY(s.ids().contains(QStringLiteral("b1")));
        QVERIFY(s.saveAs(sample(), QStringLiteral("f"), QStringLiteral("F"), error));
        QVERIFY(QDir().mkpath(s.directory(QStringLiteral("f"))));
        QVERIFY2(s.remove(QStringLiteral("f"), error), qPrintable(error));
        QVERIFY(!s.ids().contains(QStringLiteral("f")));
        QVERIFY(!QFileInfo::exists(s.directory(QStringLiteral("f"))));
    }
    void saveIsAtomic()
    {
        auto s = store();
        QString error;
        QVERIFY(s.saveAs(sample(), QStringLiteral("c"), QStringLiteral("C"), error));
        const QStringList left = QDir(m_user->path()).entryList(QDir::Files | QDir::Hidden);
        QCOMPARE(left, QStringList{QStringLiteral("c.json")}); // aucun fichier temporaire laissé
    }
    void exportImportRoundTrip()
    {
        auto s = store();
        QString error;
        QVERIFY(s.saveAs(sample(), QStringLiteral("d"), QStringLiteral("D"), error));
        QVERIFY(QDir().mkpath(s.directory(QStringLiteral("d"))));
        QVERIFY(writeFile(s.directory(QStringLiteral("d")) + QStringLiteral("/layout.json"), "{\"x\":1}"));
        const QString file = m_user->filePath(QStringLiteral("export.json"));
        QVERIFY2(s.exportTo(QStringLiteral("d"), file, error), qPrintable(error));
        QCOMPARE(objectOf(readFile(file)).value(QStringLiteral("format")).toString(),
                 QStringLiteral("actcraft-profile-export/1"));
        QTemporaryDir other;
        ProfileStore fresh(other.path(), m_builtin.path(), kOrder);
        const auto id = fresh.importFrom(file, error);
        QVERIFY2(id, qPrintable(error));
        QCOMPARE(*id, QStringLiteral("d"));
        QCOMPARE(fresh.load(*id, error)->iconPixels, 32);
        QCOMPARE(objectOf(readFile(fresh.directory(*id) + QStringLiteral("/layout.json")))
                     .value(QStringLiteral("x")),
                 QJsonValue(1));
    }
    void importNeverOverwrites()
    {
        auto s = store();
        QString error;
        QVERIFY(s.saveAs(sample(), QStringLiteral("e"), QStringLiteral("E"), error));
        const QString file = m_user->filePath(QStringLiteral("e-export.json"));
        QVERIFY(s.exportTo(QStringLiteral("e"), file, error));
        const auto first = s.importFrom(file, error);
        QVERIFY2(first, qPrintable(error));
        QCOMPARE(*first, QStringLiteral("e-2"));
        const auto second = s.importFrom(file, error);
        QVERIFY2(second, qPrintable(error));
        QCOMPARE(*second, QStringLiteral("e-3"));
        QCOMPARE(s.load(QStringLiteral("e-3"), error)->id, QStringLiteral("e-3"));
        // Un profil fourni n'est jamais masqué par un import.
        QVERIFY(s.exportTo(QStringLiteral("b1"), file, error));
        const auto third = s.importFrom(file, error);
        QVERIFY2(third, qPrintable(error));
        QCOMPARE(*third, QStringLiteral("b1-2"));
        QVERIFY(!s.hasUserCopy(QStringLiteral("b1")));
    }
    // Amendements 1, 2 et 3 : un identifiant importé en forme de chemin est refusé, rien n'est écrit.
    void importRejectsPathId()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        ProfileStore s(root.path() + QStringLiteral("/a/b"), m_builtin.path(), kOrder);
        const QString file = root.filePath(QStringLiteral("mauvais.json"));
        const QByteArray files[] = {
            R"({"format":"actcraft-profile-export/1","profile":{"format":"actcraft-profile/1","id":"..\\..\\x","label":"X"},"layout":null})",
            R"({"format":"actcraft-profile-export/1","profile":{"format":"actcraft-profile/1","id":"../../x","label":"X"},"layout":{"x":1}})",
        };
        for (const QByteArray& bytes : files) {
            QVERIFY(writeFile(file, bytes));
            const QStringList before = treeOf(root.path());
            QString error;
            QVERIFY(!s.importFrom(file, error));
            QVERIFY(!error.isEmpty());
            QCOMPARE(treeOf(root.path()), before);
        }
    }
    // Amendement 1, point 8 : chaque entrée du magasin refuse un identifiant hors ^[a-z0-9-]+$.
    void invalidIdRefusedEverywhere()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        QVERIFY(QDir().mkpath(root.filePath(QStringLiteral("a/b"))));
        QVERIFY(writeFile(root.filePath(QStringLiteral("a/x.json")),
                          R"({"format":"actcraft-profile/1","id":"x","label":"X"})"));
        ProfileStore s(root.filePath(QStringLiteral("a/b")), m_builtin.path(), kOrder);
        const QStringList before = treeOf(root.path());
        // « x\n » : le `$` de QRegularExpression l'accepterait.
        for (const QString& bad :
             {QStringLiteral("../x"), QStringLiteral("B1"), QStringLiteral("x\n"), QString()}) {
            QString error;
            Profile p = sample();
            p.id = bad;
            QVERIFY(!s.load(bad, error));
            QVERIFY(!s.save(p, error));
            QVERIFY(!s.saveAs(sample(), bad, QStringLiteral("X"), error));
            QVERIFY(!s.remove(bad, error));
            QVERIFY(!s.reset(bad, error));
            QVERIFY(!s.exportTo(bad, root.filePath(QStringLiteral("out.json")), error));
            QVERIFY(!error.isEmpty());
            QVERIFY(s.directory(bad).isEmpty());
            QVERIFY(!s.hasUserCopy(bad));
            QCOMPARE(treeOf(root.path()), before);
        }
    }
};

QTEST_MAIN(TstActionsProfile)
#include "tst_actions_profile.moc"
