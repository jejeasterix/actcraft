#include <actcraft/actions/commandregistry.h>
#include <actcraft/actions/layout.h>
#include <actcraft/actions/profilestore.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/layouteditor.h>
#include <actcraft/ui/preferences.h>
#include <actcraft/ui/profilecontroller.h>

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtQml/qqmlextensionplugin.h>
#include <QtTest>

#include <memory>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

QByteArray fileHash(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256);
}

// Empreinte de chaque fichier sous `root` : la vraie garde de « rien d'écrit ailleurs que le dossier du
// profil » (le fichier des ressources est en qrc, non inscriptible : son empreinte ne peut pas changer).
QHash<QString, QByteArray> snapshot(const QString& root)
{
    QHash<QString, QByteArray> out;
    QDirIterator it(root, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        out.insert(path, fileHash(path));
    }
    return out;
}

QStringList changedFiles(const QHash<QString, QByteArray>& before, const QHash<QString, QByteArray>& after)
{
    QStringList out;
    for (auto it = after.cbegin(); it != after.cend(); ++it)
        if (before.value(it.key()) != it.value())
            out << QDir::cleanPath(it.key());
    for (auto it = before.cbegin(); it != before.cend(); ++it)
        if (!after.contains(it.key()))
            out << QDir::cleanPath(it.key());
    return out;
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

// Groupes de la palette que ToolPalette ne dessine pas en boutons : il y charge le nuancier et l'épaisseur.
bool undrawnGroup(const QString& group)
{
    return group == QLatin1String("couleurs") || group == QLatin1String("epaisseur");
}

// Commandes de Commands::palette() rangées sous un groupe non dessiné : absentes de l'écran.
QStringList hiddenCommands(const QVariantList& palette)
{
    QStringList out;
    for (const QVariant& group : palette)
        if (undrawnGroup(group.toMap().value(QStringLiteral("group")).toString()))
            out << ids(group.toMap().value(QStringLiteral("items")).toList());
    return out;
}

// Même garde sur la palette aplatie que montre l'éditeur.
QStringList hiddenItems(const QVariantList& items)
{
    QStringList out;
    bool undrawn = false;
    for (const QVariant& value : items) {
        const QVariantMap item = value.toMap();
        if (item.contains(QStringLiteral("group")))
            undrawn = undrawnGroup(item.value(QStringLiteral("group")).toString());
        else if (undrawn)
            out << item.value(QStringLiteral("id")).toString();
    }
    return out;
}

int indexOf(const QVariantList& items, const QString& id)
{
    return static_cast<int>(ids(items).indexOf(id));
}

int groupIndex(const QVariantList& items, const QString& group)
{
    for (int i = 0; i < items.size(); ++i)
        if (items.at(i).toMap().value(QStringLiteral("group")).toString() == group)
            return i;
    return -1;
}

bool currentModified(const ui::ProfileController& profile)
{
    for (const QVariant& entry : profile.profiles())
        if (entry.toMap().value(QStringLiteral("id")).toString() == profile.currentId())
            return entry.toMap().value(QStringLiteral("modified")).toBool();
    return false;
}

// Banc : registre, profil courant dans un dossier temporaire, éditeur lié aux deux.
struct Bench
{
    QTemporaryDir root;
    ui::Commands commands;
    ui::Preferences preferences{root.filePath(QStringLiteral("preferences.ini")), nullptr};
    std::unique_ptr<ui::ProfileController> profile;
    ui::LayoutEditor editor;

    explicit Bench(actions::Platform platform = actions::Platform::Windows)
    {
        commands.setPlatform(platform);
        profile = std::make_unique<ui::ProfileController>(
            actions::ProfileStore(root.filePath(QStringLiteral("profiles")),
                                  QStringLiteral(":/actcraft/actions/profiles"),
                                  ui::ProfileController::builtinOrder()),
            &preferences, &commands);
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
        for (const QString& expected :
             {QStringLiteral("palette"), QStringLiteral("objectBar"), QStringLiteral("contextMenu"),
              QStringLiteral("objectMenu"), QStringLiteral("menu:file")})
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
        const int last = static_cast<int>(before.size()) - 1;
        QVERIFY(!bench.editor.moveDown(last));
        QVERIFY(bench.editor.removeAt(0));
        QCOMPARE(tokens(bench.editor.items()), before.mid(1));
        QVERIFY(!bench.editor.removeAt(last + 4));
        QVERIFY(!bench.editor.removeAt(-1));
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
        const auto before = snapshot(bench.root.path());
        bench.editor.setTarget(QStringLiteral("objectBar"));
        QVERIFY(bench.editor.add(QStringLiteral("file.save")));
        QVERIFY(bench.editor.apply());
        QVERIFY(QFile::exists(bench.profileLayout()));
        QCOMPARE(fileHash(defaultLayoutPath()), resourceHash);
        QCOMPARE(changedFiles(before, snapshot(bench.root.path())),
                 QStringList{QDir::cleanPath(bench.profileLayout())});
        const QJsonArray objectBar =
            readJson(bench.profileLayout()).value(QStringLiteral("objectBar")).toArray();
        QCOMPARE(objectBar.last().toString(), QStringLiteral("file.save"));
    }

    void defaultLayoutUntouched()
    {
        Bench bench;
        const QByteArray resourceHash = fileHash(defaultLayoutPath());
        const auto before = snapshot(bench.root.path());
        bench.editor.setTarget(QStringLiteral("palette"));
        QVERIFY(bench.editor.add(QStringLiteral("file.new-1024x768")));
        QVERIFY(bench.editor.apply());
        QCOMPARE(fileHash(defaultLayoutPath()), resourceHash);
        const QString profileDir =
            QDir::cleanPath(bench.profile->store().directory(bench.profile->currentId()));
        for (const QString& path : changedFiles(before, snapshot(bench.root.path())))
            QVERIFY2(path.startsWith(profileDir + QLatin1Char('/')), qPrintable(path));
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
        layout.insert(QStringLiteral("shortcuts"),
                      QJsonObject{{QStringLiteral("file.save"), QStringLiteral("Ctrl+Alt+S")}});
        QJsonArray menus = layout.value(QStringLiteral("menus")).toArray();
        QJsonObject file = menus.at(0).toObject();
        QCOMPARE(file.value(QStringLiteral("id")).toString(), QStringLiteral("file"));
        file.insert(QStringLiteral("x-note"), QStringLiteral("gardée"));
        menus.replace(0, file);
        layout.insert(QStringLiteral("menus"), menus);
        QVERIFY(writeJson(bench.profileLayout(), layout));
        bench.commands.setProfileDir(QFileInfo(bench.profileLayout()).absolutePath());
        bench.editor.reload();
        bench.editor.setTarget(QStringLiteral("menu:file"));
        QVERIFY(tokens(bench.editor.items()).contains(QStringLiteral(">import")));

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

    // Majeur 1 : une commande n'atterrit jamais sous « couleurs » ou « epaisseur », que ToolPalette ne
    // dessine pas en boutons. Le patron écarté (ces en-têtes traités comme les autres) range tool.select sous
    // « epaisseur » au premier « Monter » : l'outil disparaît de la palette, sans message.
    void commandsStayInDrawnGroups()
    {
        Bench bench;
        bench.editor.setTarget(QStringLiteral("palette"));
        QVariantList items = bench.editor.items();
        const int select = indexOf(items, QStringLiteral("tool.select"));
        // Témoin : tool.select ouvre son groupe, et l'en-tête d'avant est un bloc non dessiné.
        QVERIFY(select >= 2);
        QVERIFY(items.at(select - 1).toMap().contains(QStringLiteral("group")));
        QVERIFY(undrawnGroup(items.at(select - 2).toMap().value(QStringLiteral("group")).toString()));
        QVERIFY(hiddenCommands(bench.commands.palette()).isEmpty());

        bench.editor.moveUp(select);
        QVERIFY(hiddenItems(bench.editor.items()).isEmpty());
        QVERIFY(bench.editor.apply());
        QVERIFY(paletteIds(bench.commands.palette()).contains(QStringLiteral("tool.select")));
        QVERIFY(hiddenCommands(bench.commands.palette()).isEmpty());

        // Retirer l'en-tête du groupe de tool.select, pousser chaque bloc non dessiné en bas, puis ajouter.
        items = bench.editor.items();
        int header = indexOf(items, QStringLiteral("tool.select"));
        while (header >= 0 && !items.at(header).toMap().contains(QStringLiteral("group")))
            --header;
        QVERIFY(header >= 0);
        bench.editor.removeAt(header);
        QVERIFY(hiddenItems(bench.editor.items()).isEmpty());
        for (const QString& group : {QStringLiteral("couleurs"), QStringLiteral("epaisseur")}) {
            for (qsizetype step = 0; step < bench.editor.items().size(); ++step) {
                const int at = groupIndex(bench.editor.items(), group);
                if (at < 0 || !bench.editor.moveDown(at))
                    break;
            }
            QVERIFY2(hiddenItems(bench.editor.items()).isEmpty(), qPrintable(group));
        }
        QVERIFY(bench.editor.add(QStringLiteral("file.new-1024x768")));
        QVERIFY(hiddenItems(bench.editor.items()).isEmpty());
        QVERIFY(bench.editor.apply());
        const QVariantList palette = bench.commands.palette();
        QVERIFY(hiddenCommands(palette).isEmpty());
        for (const QString& id : {QStringLiteral("tool.select"), QStringLiteral("file.new-1024x768")})
            QVERIFY2(paletteIds(palette).contains(id), qPrintable(id));
    }

    // Majeur 2 : « Revenir à l'origine » relit la disposition, même à identifiant de profil égal. Le patron
    // écarté (relire au seul changement d'identifiant) montre la liste périmée et la réécrit au prochain
    // « Terminé ». Modifier une autre liste ne suffit pas à le voir (apply() relit le disque) : la garde est
    // le contenu montré et la même liste remodifiée.
    void resetReloadsTheLayout()
    {
        Bench bench;
        const QJsonArray origin = readJson(defaultLayoutPath()).value(QStringLiteral("objectBar")).toArray();
        bench.editor.setTarget(QStringLiteral("objectBar"));
        const QStringList shown = tokens(bench.editor.items());
        QVERIFY(bench.editor.add(QStringLiteral("file.save")));
        QVERIFY(bench.editor.apply());
        QVERIFY(QFile::exists(bench.profileLayout()));
        const QString id = bench.profile->currentId();
        bench.commands.trigger(QStringLiteral("profile.reset"));
        QCOMPARE(bench.profile->currentId(), id);
        QVERIFY(!QFile::exists(bench.profileLayout()));
        QCOMPARE(tokens(bench.editor.items()), shown);
        QVERIFY(!bench.editor.modified());

        bench.editor.setTarget(QStringLiteral("contextMenu"));
        QVERIFY(bench.editor.addSeparator());
        QVERIFY(bench.editor.apply());
        QCOMPARE(readJson(bench.profileLayout()).value(QStringLiteral("objectBar")).toArray(), origin);

        bench.commands.trigger(QStringLiteral("profile.reset"));
        bench.editor.setTarget(QStringLiteral("objectBar"));
        QVERIFY(bench.editor.moveDown(0));
        QVERIFY(bench.editor.apply());
        QJsonArray expected = origin;
        expected.replace(0, origin.at(1));
        expected.replace(1, origin.at(0));
        QCOMPARE(readJson(bench.profileLayout()).value(QStringLiteral("objectBar")).toArray(), expected);
        QCOMPARE(readJson(bench.profileLayout()).value(QStringLiteral("contextMenu")),
                 readJson(defaultLayoutPath()).value(QStringLiteral("contextMenu")));
    }

    // Amendement 9 : après « Terminé », la liste des profils est signalée et le profil fourni est « modifié »
    // (« Revenir à l'origine » actif), sans copie <id>.json.
    void applyMarksTheProfileModified()
    {
        Bench bench;
        const QString id = bench.profile->currentId();
        QVERIFY(bench.profile->store().isBuiltin(id));
        QVERIFY(!currentModified(*bench.profile));
        QSignalSpy list(bench.profile.get(), &ui::ProfileController::listChanged);
        bench.editor.setTarget(QStringLiteral("objectBar"));
        QVERIFY(bench.editor.add(QStringLiteral("file.save")));
        QVERIFY(bench.editor.apply());
        QVERIFY(list.count() > 0);
        QVERIFY(!bench.profile->store().hasUserCopy(id));
        QVERIFY(currentModified(*bench.profile));
    }

    // Majeur 3 : un seul « Terminé » dans la fenêtre ; l'onglet a « Appliquer », et le quitter (popup fermée,
    // autre onglet : la visibilité effective tombe) applique ce qui ne l'est pas. Le patron écarté (deux
    // « Terminé », celui de l'éditeur sans écrire) perd la modification en silence. Mineur vérifié : un
    // nouveau modèle remet currentIndex à 0 ; la sélection suit l'entrée ajoutée.
    void leavingTheTabApplies()
    {
        Bench bench;
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nimport ActCraft.Ui\n"
                          "Item { width: 640; height: 480; CommandsPage { anchors.fill: parent } }\n",
                          QUrl());
        std::unique_ptr<QObject> holder(component.create());
        QVERIFY2(holder, qPrintable(component.errorString()));
        QQuickWindow window;
        auto* frame = qobject_cast<QQuickItem*>(holder.get());
        QVERIFY(frame);
        frame->setParentItem(window.contentItem());
        auto* page = holder->findChild<QQuickItem*>(QStringLiteral("commandsPage"));
        QVERIFY(page);
        page->setProperty("commands", QVariant::fromValue(&bench.commands));
        page->setProperty("profile", QVariant::fromValue(bench.profile.get()));
        auto* editor = page->findChild<ui::LayoutEditor*>(QStringLiteral("layoutEditor"));
        QVERIFY(editor);
        QCOMPARE(editor->profile(), bench.profile.get());
        auto* applyButton = page->findChild<QQuickItem*>(QStringLiteral("applyButton"));
        QVERIFY(applyButton);
        QCOMPARE(applyButton->property("text").toString(), QStringLiteral("Appliquer"));

        auto* list = page->findChild<QQuickItem*>(QStringLiteral("targetList"));
        QVERIFY(list);
        editor->setTarget(QStringLiteral("objectBar"));
        editor->setCurrent(1);
        QCOMPARE(list->property("currentIndex").toInt(), 1);
        QVERIFY(editor->add(QStringLiteral("file.save")));
        QCOMPARE(editor->current(), 2);
        QCOMPARE(list->property("currentIndex").toInt(), 2);

        QVERIFY(!QFile::exists(bench.profileLayout()));
        frame->setVisible(false);
        QVERIFY(QFile::exists(bench.profileLayout()));
        QVERIFY(!editor->modified());
        QCOMPARE(
            readJson(bench.profileLayout()).value(QStringLiteral("objectBar")).toArray().at(2).toString(),
            QStringLiteral("file.save"));
    }

    // Mineur : add() et addSeparator() insèrent juste après l'entrée choisie, qui devient l'entrée ajoutée.
    void insertsAfterTheSelection()
    {
        Bench bench;
        bench.editor.setTarget(QStringLiteral("menu:file"));
        const QStringList before = tokens(bench.editor.items());
        QVERIFY(!before.contains(QStringLiteral("tool.select")));
        bench.editor.setCurrent(1);
        QVERIFY(bench.editor.add(QStringLiteral("tool.select")));
        QStringList expected = before;
        expected.insert(2, QStringLiteral("tool.select"));
        QCOMPARE(tokens(bench.editor.items()), expected);
        QCOMPARE(bench.editor.current(), 2);
        QVERIFY(bench.editor.addSeparator());
        expected.insert(3, QStringLiteral("-"));
        QCOMPARE(tokens(bench.editor.items()), expected);
        QCOMPARE(bench.editor.current(), 3);

        // Palette : un groupe neuf s'ouvre après l'entrée choisie et emporte la suite de son groupe.
        bench.editor.setTarget(QStringLiteral("palette"));
        const int select = indexOf(bench.editor.items(), QStringLiteral("tool.select"));
        bench.editor.setCurrent(select);
        QVERIFY(bench.editor.addSeparator());
        QCOMPARE(bench.editor.current(), select + 1);
        QCOMPARE(bench.editor.items().at(select + 1).toMap().value(QStringLiteral("group")).toString(),
                 QStringLiteral("groupe-1"));
        QCOMPARE(bench.editor.items().at(select + 1).toMap().value(QStringLiteral("label")).toString(),
                 QStringLiteral("Groupe 1"));
    }

    // Mineur : doublon refusé, rien de changé.
    void duplicateRefused()
    {
        Bench bench;
        bench.editor.setTarget(QStringLiteral("objectBar"));
        const QStringList before = tokens(bench.editor.items());
        QVERIFY(!before.isEmpty());
        QVERIFY(!bench.editor.add(before.constFirst()));
        QCOMPARE(tokens(bench.editor.items()), before);
        QVERIFY(!bench.editor.modified());
    }

    // Mineur : aller-retour de la palette. Blocs fixes sans clé items, clés de chaque groupe gardées ; seule
    // la liste du groupe modifié change.
    void paletteRoundTrip()
    {
        Bench bench;
        const QJsonArray origin = readJson(defaultLayoutPath()).value(QStringLiteral("palette")).toArray();
        QVERIFY(origin.at(0).toObject().value(QStringLiteral("items")).toArray().size() >= 2);
        bool fixedWithoutItems = false;
        for (const QJsonValue& group : origin)
            fixedWithoutItems = fixedWithoutItems ||
                                (undrawnGroup(group.toObject().value(QStringLiteral("group")).toString()) &&
                                 !group.toObject().contains(QStringLiteral("items")));
        QVERIFY(fixedWithoutItems);
        bench.editor.setTarget(QStringLiteral("palette"));
        QVERIFY(bench.editor.moveDown(1));
        QVERIFY(bench.editor.apply());
        QJsonArray expected = origin;
        QJsonObject first = origin.at(0).toObject();
        QJsonArray items = first.value(QStringLiteral("items")).toArray();
        const QJsonValue a = items.at(0);
        items.replace(0, items.at(1));
        items.replace(1, a);
        first.insert(QStringLiteral("items"), items);
        expected.replace(0, first);
        QCOMPARE(readJson(bench.profileLayout()).value(QStringLiteral("palette")).toArray(), expected);
    }

    // Mineur : sur Android, une entrée absente de la plateforme (launcher.edit, menu Outils) est montrée
    // grisée et gardée telle qu'écrite.
    void absentEntryKeptOnWrite()
    {
        Bench bench(actions::Platform::Android);
        const QJsonArray origin = menuObject(readJson(defaultLayoutPath()), QStringLiteral("tools"))
                                      .value(QStringLiteral("items"))
                                      .toArray();
        QVERIFY(QJsonDocument(origin).toJson().contains("launcher.edit"));
        QVERIFY(!ids(bench.editor.catalog()).contains(QStringLiteral("launcher.edit")));
        bench.editor.setTarget(QStringLiteral("menu:tools"));
        QVERIFY(bench.editor.addSeparator());
        QVERIFY(bench.editor.apply());
        QJsonArray expected = origin;
        expected.append(QStringLiteral("-"));
        QCOMPARE(menuObject(readJson(bench.profileLayout()), QStringLiteral("tools"))
                     .value(QStringLiteral("items"))
                     .toArray(),
                 expected);
    }

    // Mineur : changer de profil n'abandonne pas les modifications : elles vont au profil quitté, comme ses
    // réglages (ProfileController::activate).
    void editsGoToTheDepartedProfile()
    {
        Bench bench;
        const QString departed = bench.profile->currentId();
        const QString departedLayout = bench.profileLayout();
        QString other;
        for (const QString& id : ui::ProfileController::builtinOrder())
            if (other.isEmpty() && id != departed)
                other = id;
        bench.editor.setTarget(QStringLiteral("objectBar"));
        QVERIFY(bench.editor.add(QStringLiteral("file.save")));
        QVERIFY(bench.profile->activate(other));
        QVERIFY(!bench.editor.modified());
        QCOMPARE(readJson(departedLayout).value(QStringLiteral("objectBar")).toArray().last().toString(),
                 QStringLiteral("file.save"));
        QVERIFY(!QFile::exists(bench.profileLayout()));
        QVERIFY(!tokens(bench.editor.items()).contains(QStringLiteral("file.save")));
    }

    // Mineurs : apply() sans modification n'écrit rien ; layout.json du profil refusé par parseLayout :
    // l'éditeur montre la disposition des ressources (comme Commands) et apply() la remplace.
    void applyWithoutEditsAndInvalidProfileLayout()
    {
        Bench bench;
        QVERIFY(bench.editor.apply());
        QVERIFY(!QFile::exists(bench.profileLayout()));

        const QJsonObject invalid{{QStringLiteral("palette"), 3}};
        actions::Layout parsed;
        QString error;
        QVERIFY(!actions::parseLayout(QJsonDocument(invalid).toJson(), parsed, error));
        QVERIFY(writeJson(bench.profileLayout(), invalid));
        bench.editor.reload();
        bench.editor.setTarget(QStringLiteral("objectBar"));
        const QJsonArray origin = readJson(defaultLayoutPath()).value(QStringLiteral("objectBar")).toArray();
        QCOMPARE(bench.editor.items().size(), origin.size());
        QVERIFY(bench.editor.add(QStringLiteral("file.save")));
        QVERIFY(bench.editor.apply());
        QFile written(bench.profileLayout());
        QVERIFY(written.open(QIODevice::ReadOnly));
        QVERIFY2(actions::parseLayout(written.readAll(), parsed, error), qPrintable(error));
        QCOMPARE(readJson(bench.profileLayout()).value(QStringLiteral("objectBar")).toArray().size(),
                 origin.size() + 1);
    }

    // Mineur : un échec d'apply() est signalé (error, ProfileController::failed) et garde les modifications.
    void applyFailureReported()
    {
        Bench bench;
        const QString dir = bench.profile->store().directory(bench.profile->currentId());
        QVERIFY(QDir().mkpath(QFileInfo(dir).absolutePath()));
        QFile blocker(dir); // un fichier à la place du dossier du profil : écriture impossible
        QVERIFY(blocker.open(QIODevice::WriteOnly));
        blocker.close();
        QSignalSpy failed(bench.profile.get(), &ui::ProfileController::failed);
        bench.editor.setTarget(QStringLiteral("objectBar"));
        QVERIFY(bench.editor.add(QStringLiteral("file.save")));
        QVERIFY(!bench.editor.apply());
        QVERIFY(!bench.editor.error().isEmpty());
        QCOMPARE(failed.count(), 1);
        QVERIFY(bench.editor.modified());
    }
};

QTEST_MAIN(TstUiLayouteditor)
#include "tst_ui_layouteditor.moc"
