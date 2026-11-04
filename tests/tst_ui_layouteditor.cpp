#include <actcraft/actions/commandregistry.h>
#include <actcraft/actions/layout.h>
#include <actcraft/actions/profilestore.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/layouteditor.h>
#include <actcraft/ui/preferences.h>
#include <actcraft/ui/profilecontroller.h>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

#include <memory>

using namespace actcraft;

namespace {

QByteArray fileHash(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256);
}

QJsonObject readJson(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}

bool writeJson(const QString& path, const QJsonObject& object)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    return file.open(QIODevice::WriteOnly | QIODevice::Truncate) &&
           file.write(QJsonDocument(object).toJson()) > 0;
}

QString defaultLayoutPath()
{
    return actions::dataResourceDir() + QStringLiteral("/layout.json");
}

// Contenu de la cible : identifiant, « - » (séparateur ou en-tête de groupe), « >id » (sous-menu).
QStringList tokens(const QVariantList& items)
{
    QStringList out;
    for (const QVariant& value : items) {
        const QVariantMap item = value.toMap();
        if (item.value(QStringLiteral("submenu")).toBool())
            out << QLatin1Char('>') + item.value(QStringLiteral("id")).toString();
        else if (item.value(QStringLiteral("separator")).toBool())
            out << QStringLiteral("-");
        else
            out << item.value(QStringLiteral("id")).toString();
    }
    return out;
}

QStringList ids(const QVariantList& entries)
{
    QStringList out;
    for (const QVariant& value : entries)
        out << value.toMap().value(QStringLiteral("id")).toString();
    return out;
}

QStringList paletteIds(const QVariantList& palette)
{
    QStringList out;
    for (const QVariant& group : palette)
        out << ids(group.toMap().value(QStringLiteral("items")).toList());
    return out;
}

QJsonObject menuObject(const QJsonObject& layout, const QString& id)
{
    for (const QJsonValue& menu : layout.value(QStringLiteral("menus")).toArray())
        if (menu.toObject().value(QStringLiteral("id")).toString() == id)
            return menu.toObject();
    return {};
}

// Banc : registre, profil courant dans un dossier temporaire, éditeur lié aux deux.
struct Bench
{
    QTemporaryDir root;
    ui::Commands commands;
    ui::Preferences preferences;
    std::unique_ptr<ui::ProfileController> profile;
    ui::LayoutEditor editor;

    explicit Bench(actions::Platform platform = actions::Platform::Windows)
    {
        commands.setPlatform(platform);
        profile = std::make_unique<ui::ProfileController>(actions::ProfileStore(root.path()), &preferences,
                                                          &commands);
        profile->start();
        editor.setCommands(&commands);
        editor.setProfile(profile.get());
    }

    QString profileLayout() const
    {
        return profile->store().directory(profile->currentId()) + QStringLiteral("/layout.json");
    }
};

} // namespace

class TstUiLayouteditor : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase() { QStandardPaths::setTestModeEnabled(true); }

    void targetsListed()
    {
        Bench bench;
        const QStringList targets = ids(bench.editor.targets());
        for (const QString& expected : {QStringLiteral("palette"), QStringLiteral("objectBar"),
                                        QStringLiteral("contextMenu"), QStringLiteral("objectMenu"),
                                        QStringLiteral("menu:file")})
            QVERIFY2(targets.contains(expected), qPrintable(expected));
        QCOMPARE(bench.editor.target(), QStringLiteral("palette"));
        bench.editor.setTarget(QStringLiteral("menu:file"));
        QCOMPARE(tokens(bench.editor.items()).value(0), QStringLiteral("file.new"));
        QVERIFY(tokens(bench.editor.items()).contains(QStringLiteral(">import")));
    }

    void addAppendsToTheTarget()
    {
        Bench bench;
        bench.editor.setTarget(QStringLiteral("objectBar"));
        const QStringList before = tokens(bench.editor.items());
        QVERIFY(!before.contains(QStringLiteral("file.save")));
        QVERIFY(bench.editor.add(QStringLiteral("file.save")));
        QCOMPARE(tokens(bench.editor.items()), QStringList(before) << QStringLiteral("file.save"));
        QVERIFY(!bench.editor.add(QStringLiteral("commande.inconnue")));
        QCOMPARE(tokens(bench.editor.items()).size(), before.size() + 1);
    }

    void moveUpAndDown()
    {
        Bench bench;
        bench.editor.setTarget(QStringLiteral("menu:file"));
        const QStringList before = tokens(bench.editor.items());
        QVERIFY(bench.editor.moveDown(0));
        QCOMPARE(tokens(bench.editor.items()).value(0), before.value(1));
        QCOMPARE(tokens(bench.editor.items()).value(1), before.value(0));
        QVERIFY(bench.editor.moveUp(1));
        QCOMPARE(tokens(bench.editor.items()), before);
        QVERIFY(!bench.editor.moveUp(0));
        QVERIFY(!bench.editor.moveDown(before.size() - 1));
        QVERIFY(bench.editor.removeAt(0));
        QCOMPARE(tokens(bench.editor.items()), before.mid(1));
        QVERIFY(!bench.editor.removeAt(before.size() + 4));
    }

    void separatorAllowed()
    {
        Bench bench;
        bench.editor.setTarget(QStringLiteral("menu:file"));
        QVERIFY(bench.editor.addSeparator());
        QCOMPARE(tokens(bench.editor.items()).constLast(), QStringLiteral("-"));
        bench.editor.setTarget(QStringLiteral("contextMenu"));
        QVERIFY(bench.editor.addSeparator());
        QCOMPARE(tokens(bench.editor.items()).constLast(), QStringLiteral("-"));
        // La barre d'objet n'a pas de séparateur dans le format.
        bench.editor.setTarget(QStringLiteral("objectBar"));
        const QStringList before = tokens(bench.editor.items());
        QVERIFY(!bench.editor.addSeparator());
        QCOMPARE(tokens(bench.editor.items()), before);
    }

    void writesTheProfileLayoutOnly()
    {
        Bench bench;
        const QByteArray resourceHash = fileHash(defaultLayoutPath());
        QVERIFY(!resourceHash.isEmpty());
        QVERIFY(!QFile::exists(bench.profileLayout()));
        bench.editor.setTarget(QStringLiteral("objectBar"));
        QVERIFY(bench.editor.add(QStringLiteral("file.save")));
        QVERIFY(bench.editor.apply());
        QVERIFY(QFile::exists(bench.profileLayout()));
        QCOMPARE(fileHash(defaultLayoutPath()), resourceHash);
        const QJsonArray objectBar =
            readJson(bench.profileLayout()).value(QStringLiteral("objectBar")).toArray();
        QCOMPARE(objectBar.last().toString(), QStringLiteral("file.save"));
        for (const QString& other : ui::ProfileController::builtinOrder())
            if (other != bench.profile->currentId())
                QVERIFY2(!QFile::exists(bench.profile->store().directory(other) + QStringLiteral("/layout.json")),
                         qPrintable(other));
    }

    void defaultLayoutUntouched()
    {
        Bench bench;
        const QByteArray resourceHash = fileHash(defaultLayoutPath());
        bench.editor.setTarget(QStringLiteral("palette"));
        QVERIFY(bench.editor.add(QStringLiteral("file.new-1024x768")));
        QVERIFY(bench.editor.apply());
        QCOMPARE(fileHash(defaultLayoutPath()), resourceHash);
        ui::Commands fresh;
        fresh.setPlatform(actions::Platform::Windows);
        fresh.setProfileDir(QString());
        QVERIFY(!paletteIds(fresh.palette()).contains(QStringLiteral("file.new-1024x768")));
    }

    void reloadShowsTheChange()
    {
        Bench bench;
        QVERIFY(!paletteIds(bench.commands.palette()).contains(QStringLiteral("file.new-1024x768")));
        bench.editor.setTarget(QStringLiteral("palette"));
        QVERIFY(bench.editor.add(QStringLiteral("file.new-1024x768")));
        QVERIFY(bench.editor.apply());
        QVERIFY(paletteIds(bench.commands.palette()).contains(QStringLiteral("file.new-1024x768")));
        QVERIFY(tokens(bench.editor.items()).contains(QStringLiteral("file.new-1024x768")));
    }

    void unavailableCommandsHidden()
    {
        {
            Bench desktop(actions::Platform::Windows);
            QVERIFY(ids(desktop.editor.catalog()).contains(QStringLiteral("launcher.edit")));
        }
        Bench android(actions::Platform::Android);
        const QVariantList catalog = android.editor.catalog();
        QVERIFY(!catalog.isEmpty());
        QVERIFY(!ids(catalog).contains(QStringLiteral("launcher.edit")));
        QCOMPARE(catalog.constFirst().toMap().value(QStringLiteral("category")).toString(),
                 ids(catalog).constFirst().section(QLatin1Char('.'), 0, 0));
        android.editor.setTarget(QStringLiteral("objectBar"));
        QVERIFY(!android.editor.add(QStringLiteral("launcher.edit")));
    }

    // Garde de l'invariant : « Terminé » part du JSON effectif et ne remplace que la liste de la cible. Une
    // réécriture à partir du seul contenu affiché perdrait les clés inconnues, les raccourcis et les autres
    // cibles telles qu'écrites.
    void keysOutsideTheTargetKept()
    {
        Bench bench;
        QJsonObject layout = readJson(defaultLayoutPath());
        QVERIFY(!layout.isEmpty());
        layout.insert(QStringLiteral("x-extension"), QJsonObject{{QStringLiteral("a"), 1}});
        layout.insert(QStringLiteral("shortcuts"), QJsonObject{{QStringLiteral("file.save"), QStringLiteral("Ctrl+Alt+S")}});
        QJsonArray menus = layout.value(QStringLiteral("menus")).toArray();
        QJsonObject file = menus.at(0).toObject();
        QCOMPARE(file.value(QStringLiteral("id")).toString(), QStringLiteral("file"));
        file.insert(QStringLiteral("x-note"), QStringLiteral("gardée"));
        menus.replace(0, file);
        layout.insert(QStringLiteral("menus"), menus);
        QVERIFY(writeJson(bench.profileLayout(), layout));
        bench.commands.reload();
        bench.editor.reload();

        bench.editor.setTarget(QStringLiteral("objectBar"));
        QVERIFY(bench.editor.add(QStringLiteral("file.save")));
        QVERIFY(bench.editor.apply());
        QJsonObject written = readJson(bench.profileLayout());
        QCOMPARE(written.value(QStringLiteral("x-extension")), layout.value(QStringLiteral("x-extension")));
        QCOMPARE(written.value(QStringLiteral("shortcuts")), layout.value(QStringLiteral("shortcuts")));
        for (const QString& key : layout.keys())
            if (key != QLatin1String("objectBar"))
                QVERIFY2(written.value(key) == layout.value(key), qPrintable(key));

        // Dans un menu : déplacer une entrée garde le sous-menu tel qu'écrit et les clés du menu.
        bench.editor.setTarget(QStringLiteral("menu:file"));
        QVERIFY(bench.editor.moveDown(0));
        QVERIFY(bench.editor.apply());
        written = readJson(bench.profileLayout());
        const QJsonObject writtenFile = menuObject(written, QStringLiteral("file"));
        QCOMPARE(writtenFile.value(QStringLiteral("x-note")).toString(), QStringLiteral("gardée"));
        QJsonArray expected = file.value(QStringLiteral("items")).toArray();
        const QJsonValue first = expected.at(0);
        expected.replace(0, expected.at(1));
        expected.replace(1, first);
        QCOMPARE(writtenFile.value(QStringLiteral("items")).toArray(), expected);
        QCOMPARE(written.value(QStringLiteral("x-extension")), layout.value(QStringLiteral("x-extension")));
    }
};

QTEST_MAIN(TstUiLayouteditor)
#include "tst_ui_layouteditor.moc"
