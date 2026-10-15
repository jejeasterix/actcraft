#include <actcraft/actions/commandregistry.h>

#include <QSysInfo>
#include <QTest>
#include <QUrl>

#include <limits>

using namespace actcraft::actions;

namespace {

const QByteArray kTwoCommands = R"({
  "format": "actcraft-commands/1",
  "commands": [
    { "id": "page.next", "label": "Page suivante", "icon": "page-suivante", "shortcut": "PgDown" },
    { "id": "pen.color", "label": "Couleur du stylo", "argument": "color", "checkable": true,
      "platforms": ["windows", "android"] }
  ]
})";

Platforms bits(Platform a, Platform b)
{
    return static_cast<Platforms>(static_cast<Platforms>(a) | static_cast<Platforms>(b));
}

} // namespace

// Registre des commandes nommées (DA1, décision 20).
class TstActionsRegistry : public QObject
{
    Q_OBJECT

private slots:
    void loadsDeclarationsInOrder()
    {
        CommandRegistry r;
        QString error;
        QVERIFY2(r.loadDeclarations(kTwoCommands, error), qPrintable(error));
        QCOMPARE(r.ids(), (QStringList{QStringLiteral("page.next"), QStringLiteral("pen.color")}));
        const CommandInfo* next = r.find(QStringLiteral("page.next"));
        QVERIFY(next);
        QCOMPARE(next->label, QStringLiteral("Page suivante"));
        QCOMPARE(next->icon, QStringLiteral("page-suivante"));
        QCOMPARE(next->shortcut, QStringLiteral("PgDown"));
        QCOMPARE(next->platforms, kAllPlatforms);
        QVERIFY(next->argument == ArgumentKind::None);
        QVERIFY(!next->checkable);
        const CommandInfo* color = r.find(QStringLiteral("pen.color"));
        QVERIFY(color && color->checkable && color->argument == ArgumentKind::Color);
        QCOMPARE(color->platforms, bits(Platform::Windows, Platform::Android));
        QVERIFY(!r.find(QStringLiteral("absent")));
    }

    void rejectsWholeFileOnError_data()
    {
        QTest::addColumn<QByteArray>("json");
        QTest::addColumn<QString>("expected");
        QTest::newRow("json") << QByteArray("{") << QStringLiteral("JSON invalide");
        QTest::newRow("format") << QByteArray(R"({"format":"autre/1","commands":[]})")
                                << QStringLiteral("format");
        QTest::newRow("label") << QByteArray(R"({"format":"actcraft-commands/1","commands":[
            {"id":"a.b","label":"A"},{"id":"c.d"}]})")
                               << QStringLiteral("commande 2");
        QTest::newRow("doublon") << QByteArray(R"({"format":"actcraft-commands/1","commands":[
            {"id":"a.b","label":"A"},{"id":"a.b","label":"B"}]})")
                                 << QStringLiteral("déjà déclaré");
        QTest::newRow("plateforme") << QByteArray(R"({"format":"actcraft-commands/1","commands":[
            {"id":"a.b","label":"A","platforms":["amiga"]}]})")
                                    << QStringLiteral("plateforme inconnue");
        QTest::newRow("argument") << QByteArray(R"({"format":"actcraft-commands/1","commands":[
            {"id":"a.b","label":"A","argument":"image"}]})")
                                  << QStringLiteral("argument inconnu");
    }

    void rejectsWholeFileOnError()
    {
        QFETCH(QByteArray, json);
        QFETCH(QString, expected);
        CommandRegistry r;
        QString error;
        QVERIFY(!r.loadDeclarations(json, error));
        QVERIFY2(error.contains(expected), qPrintable(error));
        QVERIFY(r.ids().isEmpty());
    }

    void triggerCallsHandlerWithValidArgument()
    {
        CommandRegistry r;
        QString error;
        QVERIFY(r.loadDeclarations(kTwoCommands, error));
        QVariant received;
        int calls = 0;
        r.setHandler(QStringLiteral("pen.color"), [&](const QVariant& argument) {
            received = argument;
            ++calls;
        });
        QVERIFY(r.hasHandler(QStringLiteral("pen.color")));
        QVERIFY(r.trigger(QStringLiteral("pen.color"), QStringLiteral("#E53935"), Platform::Windows));
        QCOMPARE(calls, 1);
        QCOMPARE(received.toString(), QStringLiteral("#E53935"));
        QVERIFY(!r.trigger(QStringLiteral("pen.color"), QStringLiteral("rouge"), Platform::Windows));
        QCOMPARE(calls, 1);
    }

    void triggerRefusesWhatCannotRun()
    {
        CommandRegistry r;
        QString error;
        QVERIFY(r.loadDeclarations(kTwoCommands, error));
        int calls = 0;
        r.setHandler(QStringLiteral("pen.color"), [&](const QVariant&) { ++calls; });
        const QVariant red = QStringLiteral("#E53935");
        QVERIFY(!r.trigger(QStringLiteral("absent"), {}, Platform::Windows));
        QVERIFY(!r.trigger(QStringLiteral("page.next"), {}, Platform::Windows)); // sans gestionnaire
        QVERIFY(!r.trigger(QStringLiteral("pen.color"), red, Platform::MacOS));  // hors plateforme
        QVERIFY(!r.isAvailable(QStringLiteral("pen.color"), Platform::MacOS));
        QVERIFY(r.isAvailable(QStringLiteral("pen.color"), Platform::Android));
        r.setEnabled(QStringLiteral("pen.color"), false);
        QVERIFY(!r.trigger(QStringLiteral("pen.color"), red, Platform::Windows));
        r.setEnabled(QStringLiteral("pen.color"), true);
        QVERIFY(r.trigger(QStringLiteral("pen.color"), red, Platform::Windows));
        QCOMPARE(calls, 1);
    }

    void stateDefaultsAndChanges()
    {
        CommandRegistry r;
        QString error;
        QVERIFY(r.loadDeclarations(kTwoCommands, error));
        QVERIFY(r.isEnabled(QStringLiteral("page.next")));
        QVERIFY(!r.isChecked(QStringLiteral("pen.color")));
        r.setChecked(QStringLiteral("pen.color"), true);
        QVERIFY(r.isChecked(QStringLiteral("pen.color")));
        QVERIFY(!r.isEnabled(QStringLiteral("absent")));
        QVERIFY(!r.declare(CommandInfo{QStringLiteral("page.next"), QStringLiteral("Doublon"), QString(),
                                       QString(), false, kAllPlatforms, ArgumentKind::None}));
        QVERIFY(!r.declare(CommandInfo{}));
    }

    void argumentValidation()
    {
        QVERIFY(argumentIsValid(ArgumentKind::Color, QStringLiteral("#e53935")));
        QVERIFY(argumentIsValid(ArgumentKind::Color, QStringLiteral("#80E53935")));
        QVERIFY(!argumentIsValid(ArgumentKind::Color, QStringLiteral("e53935")));
        QVERIFY(!argumentIsValid(ArgumentKind::Color, QVariant(12)));
        QVERIFY(argumentIsValid(ArgumentKind::Number, QVariant(8)));
        QVERIFY(argumentIsValid(ArgumentKind::Number, QVariant(2.5)));
        QVERIFY(!argumentIsValid(ArgumentKind::Number, QVariant()));
        QVERIFY(!argumentIsValid(ArgumentKind::Number, QVariant(true)));
        QVERIFY(!argumentIsValid(ArgumentKind::Number, QVariant(std::numeric_limits<double>::quiet_NaN())));
        QVERIFY(argumentIsValid(ArgumentKind::Url,
                                QStringLiteral("content://com.android.providers/document/12")));
        QVERIFY(argumentIsValid(ArgumentKind::Url, QStringLiteral("C:/Documents/essai.actcraft")));
        QVERIFY(argumentIsValid(ArgumentKind::Url, QUrl::fromLocalFile(QStringLiteral("/tmp/a.actcraft"))));
        QVERIFY(!argumentIsValid(ArgumentKind::Url, QString()));
        QVERIFY(!argumentIsValid(ArgumentKind::Url, QUrl()));
        QVERIFY(argumentIsValid(ArgumentKind::None, QVariant(42)));
    }

    void platformsByName()
    {
        QCOMPARE(platformBit(QStringLiteral("windows")), static_cast<Platforms>(Platform::Windows));
        QCOMPARE(platformBit(QStringLiteral("android")), static_cast<Platforms>(Platform::Android));
        QCOMPARE(platformBit(QStringLiteral("macos")), static_cast<Platforms>(Platform::MacOS));
        QCOMPARE(platformBit(QStringLiteral("linux")), static_cast<Platforms>(Platform::Linux));
        QCOMPARE(platformBit(QStringLiteral("amiga")), Platforms{0});
        QCOMPARE(platformName(Platform::Android), QStringLiteral("android"));
        QVERIFY(!platformName(currentPlatform()).isEmpty());
        if (QSysInfo::productType() == QLatin1String("windows")) {
            QVERIFY(currentPlatform() == Platform::Windows);
        }
    }

    void defaultCommandsFileDeclaresLotOne()
    {
        const QByteArray json = defaultCommandsJson();
        QVERIFY2(!json.isEmpty(), "ressource :/actcraft/actions/commands.json absente");
        CommandRegistry r;
        QString error;
        QVERIFY2(r.loadDeclarations(json, error), qPrintable(error));
        const QStringList expected{QStringLiteral("file.new"),           QStringLiteral("file.new-screen"),
                                   QStringLiteral("file.new-1024x768"),  QStringLiteral("file.new-1152x864"),
                                   QStringLiteral("file.new-1280x1024"), QStringLiteral("file.new-custom"),
                                   QStringLiteral("file.open"),          QStringLiteral("file.save"),
                                   QStringLiteral("file.save-as"),       QStringLiteral("file.close"),
                                   QStringLiteral("file.quit"),          QStringLiteral("view.fullscreen"),
                                   QStringLiteral("view.fit"),           QStringLiteral("view.zoom"),
                                   QStringLiteral("page.previous"),      QStringLiteral("page.next"),
                                   QStringLiteral("tool.pen"),           QStringLiteral("pen.color"),
                                   QStringLiteral("pen.width"),          QStringLiteral("menu.main"),
                                   QStringLiteral("menu.tools"),         QStringLiteral("help.website"),
                                   QStringLiteral("help.about")};
        QCOMPARE(r.ids(), expected);
        QCOMPARE(r.find(QStringLiteral("view.fullscreen"))->shortcut, QStringLiteral("F5"));
        QVERIFY(r.find(QStringLiteral("view.fullscreen"))->checkable);
        QVERIFY(r.find(QStringLiteral("pen.width"))->argument == ArgumentKind::Number);
        QVERIFY(r.find(QStringLiteral("pen.color"))->argument == ArgumentKind::Color);
        QVERIFY(r.find(QStringLiteral("view.zoom"))->argument == ArgumentKind::Number);
        QVERIFY(r.find(QStringLiteral("file.open"))->argument == ArgumentKind::Url);
        QVERIFY(r.find(QStringLiteral("file.open"))->argumentOptional);
        QVERIFY(r.find(QStringLiteral("file.save-as"))->argumentOptional);
        QCOMPARE(r.find(QStringLiteral("file.new"))->shortcut, QStringLiteral("Ctrl+N"));
        QCOMPARE(r.find(QStringLiteral("file.open"))->shortcut, QStringLiteral("Ctrl+O"));
        QCOMPARE(r.find(QStringLiteral("file.save"))->shortcut, QStringLiteral("Ctrl+S"));
        QCOMPARE(r.find(QStringLiteral("file.save-as"))->shortcut, QStringLiteral("Ctrl+Shift+S"));
        QCOMPARE(r.find(QStringLiteral("file.close"))->shortcut, QStringLiteral("Ctrl+W"));
    }

    void optionalArgumentMayBeAbsent()
    {
        CommandRegistry r;
        QString error;
        QVERIFY2(r.loadDeclarations(R"({"format":"actcraft-commands/1","commands":[
            {"id":"file.open","label":"Ouvrir…","argument":"url","optional":true},
            {"id":"file.export","label":"Exporter","argument":"url"}]})",
                                    error),
                 qPrintable(error));
        QVERIFY(r.find(QStringLiteral("file.open"))->argumentOptional);
        QVERIFY(!r.find(QStringLiteral("file.export"))->argumentOptional);
        QVariantList seen;
        r.setHandler(QStringLiteral("file.open"), [&seen](const QVariant& a) { seen << a; });
        r.setHandler(QStringLiteral("file.export"), [&seen](const QVariant& a) { seen << a; });
        const Platform here = currentPlatform();
        QVERIFY(r.trigger(QStringLiteral("file.open"), QVariant(), here)); // absent : dialogue
        QVERIFY(r.trigger(QStringLiteral("file.open"), QStringLiteral("C:/a.actcraft"), here));
        QVERIFY(!r.trigger(QStringLiteral("file.open"), QStringLiteral(""), here)); // présent mais vide
        QVERIFY(!r.trigger(QStringLiteral("file.open"), 42, here));           // présent mais pas une URL
        QVERIFY(!r.trigger(QStringLiteral("file.export"), QVariant(), here)); // obligatoire
        QCOMPARE(seen.size(), 2);
        QVERIFY(!seen.at(0).isValid());
    }

    void optionalWithoutArgumentIsRefused()
    {
        CommandRegistry r;
        QString error;
        QVERIFY(!r.loadDeclarations(
            R"({"format":"actcraft-commands/1","commands":[{"id":"a.b","label":"A","optional":true}]})",
            error));
        QVERIFY2(error.contains(QStringLiteral("facultatif")), qPrintable(error));
    }
};

QTEST_GUILESS_MAIN(TstActionsRegistry)
#include "tst_actions_registry.moc"
