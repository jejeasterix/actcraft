#include <actcraft/actions/profilestore.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/preferences.h>
#include <actcraft/ui/profilecontroller.h>
#include <actcraft/ui/shortcutbar.h>
#include <actcraft/ui/toolhost.h>

#include <QBuffer>
#include <QByteArray>
#include <QImage>
#include <QTemporaryDir>
#include <QTest>

#include <memory>
#include <string>
#include <variant>

using namespace actcraft;

namespace {

// Documents, commandes et profil d'essai réels, tous propres à l'essai (aucun réglage du poste touché).
struct Fixture
{
    QTemporaryDir dir;
    ui::Commands commands;
    ui::Preferences prefs;
    ui::DocumentController document;
    std::unique_ptr<ui::ProfileController> profile;
    ui::ToolHost tools;
    ui::ShortcutBar bar;

    Fixture() : prefs(dir.filePath(QStringLiteral("poste.ini")), nullptr)
    {
        actions::ProfileStore store(dir.filePath(QStringLiteral("profiles")),
                                    QStringLiteral(":/actcraft/actions/profiles"),
                                    ui::ProfileController::builtinOrder());
        profile = std::make_unique<ui::ProfileController>(std::move(store), &prefs, &commands);
        profile->start();
        // Une commande n'est « déclenchée » que si elle a un gestionnaire.
        for (const QString& id : commands.registry().ids())
            commands.setNativeHandler(id, [](const QVariant&) {});
        document.newDocument(1024, 768);
        bar.setCommands(&commands);
        bar.setProfile(profile.get());
        bar.setProperty("document", QVariant::fromValue(&document));
        tools.setSettingsFile(dir.filePath(QStringLiteral("outils.ini")));
        tools.setDocumentController(&document);
        bar.setProperty("tools", QVariant::fromValue(&tools));
    }
    // Commandes outil ou fonction absentes de la palette par défaut, dans l'ordre du registre.
    QStringList outOfPalette()
    {
        QStringList palette;
        for (const QVariant& group : commands.palette())
            for (const QVariant& item : group.toMap().value(QStringLiteral("items")).toList())
                palette << item.toMap().value(QStringLiteral("id")).toString();
        QStringList out;
        for (const QString& id : commands.registry().ids())
            if ((id.startsWith(QLatin1String("tool.")) || id.startsWith(QLatin1String("present."))) &&
                !palette.contains(id) && commands.isAvailable(id))
                out << id;
        return out;
    }
    core::ObjectId addStroke()
    {
        core::Stroke stroke;
        stroke.points = {{300, 200, 0.5f}, {400, 260, 0.5f}, {500, 220, 0.5f}};
        core::PageObject object = stroke;
        object.common.layer = core::Layer::Middle;
        document.execute(std::make_unique<core::AddObjectCommand>(0, std::move(object)));
        return page().objects.back().common.id;
    }
    const core::Page& page() const { return document.document().page(0); }
    QVariantMap button(int row) const { return bar.buttons().at(row).toMap(); }
};

} // namespace

class TstUiShortcutbar : public QObject
{
    Q_OBJECT
private slots:
    void outOfPaletteToolIsRecorded()
    {
        Fixture f;
        // present.curtain est dans la palette par défaut : le spot carré, lui, n'y est pas.
        QVERIFY(f.outOfPalette().contains(QStringLiteral("present.spotlight-square")));
        QVERIFY(f.commands.trigger(QStringLiteral("present.spotlight-square")));
        QCOMPARE(f.bar.buttons().size(), 1);
        QCOMPARE(f.button(0).value(QStringLiteral("id")).toString(),
                 QStringLiteral("present.spotlight-square"));
        QCOMPARE(f.button(0).value(QStringLiteral("kind")).toString(), QStringLiteral("tool"));
    }
    void paletteToolIsNotRecorded()
    {
        Fixture f;
        // Un outil de la palette n'entre pas, qu'il vienne de la palette, du menu ou du raccourci : le filtre
        // sur commands.palette() suffit, la palette n'a pas à l'annoncer.
        QVERIFY(f.commands.trigger(QStringLiteral("tool.pen")));
        QVERIFY(f.bar.buttons().isEmpty());
        QVERIFY(f.commands.trigger(QStringLiteral("tool.pen")));
        QVERIFY(f.bar.buttons().isEmpty());
    }
    void historyIsBoundedAndMovesToFront()
    {
        Fixture f;
        const QStringList ids = f.outOfPalette();
        int played = 0; // les commandes à argument obligatoire ne se déclenchent pas sans lui : on les saute
        for (const QString& id : ids)
            if (played < 10 && f.commands.trigger(id))
                ++played;
        QCOMPARE(played, 10);
        QCOMPARE(f.bar.buttons().size(), 8);
        const QString fifth = f.button(4).value(QStringLiteral("id")).toString();
        QVERIFY(f.commands.trigger(fifth));
        QCOMPARE(f.bar.buttons().size(), 8);
        QCOMPARE(f.button(0).value(QStringLiteral("id")).toString(), fifth);
    }
    void droppedObjectBecomesAButton()
    {
        Fixture f;
        const core::ObjectId id = f.addStroke();
        const QString button = f.bar.addObjects({id});
        QVERIFY(!button.isEmpty());
        QCOMPARE(f.bar.buttons().size(), 1);
        QCOMPARE(f.button(0).value(QStringLiteral("kind")).toString(), QStringLiteral("object"));
        QCOMPARE(f.button(0).value(QStringLiteral("id")).toString(), button);
        f.profile->saveNow();
        QVERIFY(!f.profile->profile().shortcutObjects.isEmpty());
    }
    void clickInsertsACopyAtTheCenter()
    {
        Fixture f;
        const core::ObjectId id = f.addStroke();
        const QString button = f.bar.addObjects({id});
        const std::size_t before = f.page().objects.size();
        QVERIFY(f.bar.insert(button));
        QCOMPARE(f.page().objects.size(), before + 1);
        const core::PageObject& added = f.page().objects.back();
        QVERIFY(added.common.id != id);
        const core::RectF box = core::boundsOnPage(added);
        QVERIFY(qAbs(box.x + box.width / 2 - 512.0) <= 0.5);
        QVERIFY(qAbs(box.y + box.height / 2 - 384.0) <= 0.5);
    }
    void insertIsUndoneAtOnce()
    {
        Fixture f;
        const core::ObjectId id = f.addStroke();
        const QString button = f.bar.addObjects({id});
        const std::size_t before = f.page().objects.size();
        QVERIFY(f.bar.insert(button));
        QVERIFY(f.page().objects.size() > before);
        f.document.undo();
        QCOMPARE(f.page().objects.size(), before);
    }
    // Chemin de l'interface (menu d'objet) : la commande ajoute la sélection courante à la barre.
    void addSelectionCommandMakesAnObjectButton()
    {
        Fixture f;
        const core::ObjectId id = f.addStroke();
        f.tools.user().setSelection({id});
        QVERIFY(f.commands.registry().find(QStringLiteral("shortcutbar.add-selection")) != nullptr);
        QVERIFY(f.commands.trigger(QStringLiteral("shortcutbar.add-selection")));
        QCOMPARE(f.bar.buttons().size(), 1);
        QCOMPARE(f.button(0).value(QStringLiteral("kind")).toString(), QStringLiteral("object"));
        QVERIFY(!f.profile->profile().shortcutObjects.isEmpty());
        // Sans sélection : rien.
        f.tools.user().setSelection({});
        QVERIFY(f.bar.addSelection().isEmpty());
        QCOMPARE(f.bar.buttons().size(), 1);
    }
    void addSelectionRefusedWhileLocked()
    {
        Fixture f;
        const core::ObjectId id = f.addStroke();
        f.tools.user().setSelection({id});
        f.commands.setLocked(true);
        QVERIFY(f.bar.addSelection().isEmpty());
        f.commands.trigger(QStringLiteral("shortcutbar.add-selection"));
        QVERIFY(f.profile->profile().shortcutObjects.isEmpty());
        QVERIFY(f.bar.buttons().isEmpty());
    }
    void addSelectionRefusedWithoutCommands()
    {
        Fixture f;
        const core::ObjectId id = f.addStroke();
        f.tools.user().setSelection({id});
        ui::ShortcutBar bare;
        bare.setProfile(f.profile.get());
        bare.setProperty("document", QVariant::fromValue(&f.document));
        bare.setProperty("tools", QVariant::fromValue(&f.tools));
        QVERIFY(bare.addSelection().isEmpty());
        QVERIFY(f.profile->profile().shortcutObjects.isEmpty());
    }
    void refusedWhileLocked()
    {
        Fixture f;
        const core::ObjectId id = f.addStroke();
        const QString button = f.bar.addObjects({id});
        QVERIFY(!button.isEmpty());
        const std::size_t before = f.page().objects.size();
        f.commands.setLocked(true);
        QVERIFY(f.bar.addObjects({id}).isEmpty());
        QCOMPARE(f.profile->profile().shortcutObjects.size(), 1);
        QVERIFY(!f.bar.insert(button));
        QCOMPARE(f.page().objects.size(), before);
    }
    void refusedWithoutCommands()
    {
        Fixture f;
        const core::ObjectId id = f.addStroke();
        const QString button = f.bar.addObjects({id});
        ui::ShortcutBar bare;
        bare.setProfile(f.profile.get());
        bare.setProperty("document", QVariant::fromValue(&f.document));
        QVERIFY(bare.addObjects({id}).isEmpty());
        const std::size_t before = f.page().objects.size();
        QVERIFY(!bare.insert(button));
        QCOMPARE(f.page().objects.size(), before);
    }
    void imageButtonKeepsItsExtension()
    {
        Fixture f;
        QImage image(8, 8, QImage::Format_ARGB32);
        image.fill(Qt::red);
        QByteArray png;
        QBuffer buffer(&png);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG");
        auto& mutableDocument = const_cast<core::Document&>(f.document.document());
        const std::string key = mutableDocument.addResource(png, "png");
        core::ImageObject picture;
        picture.bounds = {100, 100, 80, 80};
        picture.resource = key;
        core::PageObject object = picture;
        object.common.layer = core::Layer::Middle;
        f.document.execute(std::make_unique<core::AddObjectCommand>(0, std::move(object)));
        const core::ObjectId id = f.page().objects.back().common.id;
        const QString button = f.bar.addObjects({id});
        QVERIFY(!button.isEmpty());
        f.document.newDocument(1024, 768); // plus de ressource
        QVERIFY(f.document.document().resource(key) == nullptr);
        QVERIFY(f.bar.insert(button));
        const core::Resource* back = f.document.document().resource(key);
        QVERIFY(back != nullptr);
        QCOMPARE(back->extension, std::string("png"));
    }
};

QTEST_MAIN(TstUiShortcutbar)
#include "tst_ui_shortcutbar.moc"
