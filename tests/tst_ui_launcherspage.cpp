// L6-T30 : onglet « Raccourcis vers des programmes », boîte de confirmation et boutons de la barre.
// Rien ici ne dépend de Windows : les raccourcis étrangers sont écrits dans le profil avec une signature
// d'un autre poste, donc toujours à confirmer.
#include <actcraft/ui/launchers.h>
#include <actcraft/ui/profilecontroller.h>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonObject>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <QUuid>
#include <QtQml/qqmlextensionplugin.h>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

struct Shell
{
    QTemporaryDir profiles; // détruit après le moteur
    QQmlApplicationEngine engine;
    QQuickWindow* window = nullptr;
    ui::ProfileController* profile = nullptr;
};

bool start(Shell& s)
{
    s.engine.setInitialProperties({{QStringLiteral("cacheEnabled"), true},
                                   {QStringLiteral("generatedPage"), false},
                                   {QStringLiteral("profileRoot"), s.profiles.path()}});
    s.engine.loadFromModule("ActCraft.Ui", "Main");
    if (s.engine.rootObjects().isEmpty())
        return false;
    s.window = qobject_cast<QQuickWindow*>(s.engine.rootObjects().constFirst());
    if (!s.window || !QTest::qWaitForWindowExposed(s.window))
        return false;
    s.profile = s.window->findChild<ui::ProfileController*>(QStringLiteral("appProfile"));
    return s.profile != nullptr;
}

// Les délégués des Repeater et Loader ne sont pas des enfants QObject de la fenêtre : on suit l'arbre visuel.
QQuickItem* findItem(QQuickItem* root, const QString& name)
{
    if (!root)
        return nullptr;
    if (root->objectName() == name)
        return root;
    for (QQuickItem* child : root->childItems())
        if (QQuickItem* found = findItem(child, name))
            return found;
    return nullptr;
}

QObject* named(Shell& s, const char* name)
{
    return s.window->findChild<QObject*>(QString::fromLatin1(name));
}

// Les deux instances de LaunchersPage : celle de Main (porte les gestionnaires et la boîte) et celle de
// l'éditeur.
QObject* page(Shell& s, bool embedded)
{
    const QList<QObject*> pages = s.window->findChildren<QObject*>(QStringLiteral("launchersPage"));
    for (QObject* candidate : pages)
        if (candidate->property("embedded").toBool() == embedded)
            return candidate;
    return nullptr;
}

ui::Launchers* launchersOf(QObject* thePage)
{
    return thePage ? qobject_cast<ui::Launchers*>(thePage->property("launchers").value<QObject*>()) : nullptr;
}

} // namespace

class TstUiLauncherspage : public QObject
{
    Q_OBJECT
    QTemporaryDir m_settingsDir;
    QTemporaryDir m_work;
    QString m_probe;

    QString witness(const QString& name) const { return m_work.filePath(name); }
    bool exists(const QString& name) const { return QFile::exists(witness(name)); }

    // Raccourci d'un autre poste : signature d'une autre clé, donc jamais de confiance.
    void putForeign(Shell& s, const QString& id, const QString& path, const QStringList& arguments)
    {
        QJsonArray entries = s.profile->profile().launchers.value(QStringLiteral("entries")).toArray();
        const QByteArray signature =
            ui::Launchers::signatureOf(path, arguments, QStringLiteral("Étranger"),
                                       QCryptographicHash::hash("autre", QCryptographicHash::Sha256));
        entries.append(QJsonObject{{QStringLiteral("id"), id},
                                   {QStringLiteral("path"), path},
                                   {QStringLiteral("arguments"), QJsonArray::fromStringList(arguments)},
                                   {QStringLiteral("toolTip"), QStringLiteral("Étranger")},
                                   {QStringLiteral("signature"), QString::fromLatin1(signature.toBase64())}});
        s.profile->editProfile().launchers.insert(QStringLiteral("entries"), entries);
        s.profile->touch();
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_settingsDir.isValid());
        QVERIFY(m_work.isValid());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
        m_probe = QStandardPaths::findExecutable(QStringLiteral("launcher_probe"),
                                                 {QCoreApplication::applicationDirPath()});
        QVERIFY2(!m_probe.isEmpty(), qPrintable(QCoreApplication::applicationDirPath()));
    }

    // Trois LaunchersPage / Launchers existent ; une seule instance pose les gestionnaires et porte la boîte.
    void oneConfirmationBoxAndOnePrimaryPage()
    {
        Shell s;
        QVERIFY(start(s));
        QVERIFY(page(s, false));
        QVERIFY(page(s, true));
        QCOMPARE(s.window->findChildren<QObject*>(QStringLiteral("launcherConfirmation")).size(), 1);
        QCOMPARE(s.window->findChildren<QObject*>(QStringLiteral("launcherFailure")).size(), 1);
        int primary = 0;
        for (QObject* p : s.window->findChildren<QObject*>(QStringLiteral("launchersPage")))
            primary += p->property("embedded").toBool() ? 0 : 1;
        QCOMPARE(primary, 1);
    }

    // confirm(id, …) répond à la dernière demande de l'id : la boîte montre la dernière et ne confirme
    // qu'elle.
    void boxShowsTheLastRequest()
    {
        Shell s;
        QVERIFY(start(s));
        ui::Launchers* launchers = launchersOf(page(s, false));
        QVERIFY(launchers);
        putForeign(s, QStringLiteral("un"), m_probe, {witness(QStringLiteral("t-un.txt"))});
        putForeign(s, QStringLiteral("deux"), m_probe, {witness(QStringLiteral("t-deux.txt"))});
        QObject* box = named(s, "launcherConfirmation");
        QVERIFY(box);
        QCOMPARE(launchers->run(QStringLiteral("un")), ui::Launchers::kConfirmationNeeded);
        QTRY_COMPARE(box->property("shownId").toString(), QStringLiteral("un"));
        QCOMPARE(launchers->run(QStringLiteral("deux")), ui::Launchers::kConfirmationNeeded);
        QTRY_COMPARE(box->property("shownId").toString(), QStringLiteral("deux"));
        QCOMPARE(box->property("shownPath").toString(), m_probe);
        QVERIFY(box->property("opened").toBool());
        // Accepter agit sur la demande affichée, jamais sur la précédente.
        QVERIFY(QMetaObject::invokeMethod(box, "accept"));
        QTRY_VERIFY(exists(QStringLiteral("t-deux.txt")));
        QTest::qWait(300);
        QVERIFY(!exists(QStringLiteral("t-un.txt")));
        QTRY_VERIFY(!box->property("opened").toBool());
    }

    void refusalLaunchesNothing()
    {
        Shell s;
        QVERIFY(start(s));
        ui::Launchers* launchers = launchersOf(page(s, false));
        putForeign(s, QStringLiteral("refus"), m_probe, {witness(QStringLiteral("t-refus.txt"))});
        QObject* box = named(s, "launcherConfirmation");
        QCOMPARE(launchers->run(QStringLiteral("refus")), ui::Launchers::kConfirmationNeeded);
        QTRY_VERIFY(box->property("opened").toBool());
        QVERIFY(QMetaObject::invokeMethod(box, "refuse"));
        QTRY_VERIFY(!box->property("opened").toBool());
        QTest::qWait(300);
        QVERIFY(!exists(QStringLiteral("t-refus.txt")));
        // La demande est close : une réponse tardive ne lance rien.
        QVERIFY(!launchers->confirm(QStringLiteral("refus"), true));
    }

    // Amendement 10 de L6-T29 : un document ne prend aucun paramètre.
    void documentsHaveNoParameters()
    {
        Shell s;
        QVERIFY(start(s));
        ui::Launchers* launchers = launchersOf(page(s, false));
        const QString document = witness(QStringLiteral("note.txt"));
        QFile file(document);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("essai");
        file.close();
        QObject* box = named(s, "launcherConfirmation");
        QObject* failure = named(s, "launcherFailure");
        // Sans paramètre : la boîte n'en montre aucun.
        putForeign(s, QStringLiteral("doc"), document, {});
        QCOMPARE(launchers->run(QStringLiteral("doc")), ui::Launchers::kConfirmationNeeded);
        QTRY_VERIFY(box->property("opened").toBool());
        QVERIFY(box->property("shownArguments").toStringList().isEmpty());
        QVERIFY(!box->property("argumentsVisible").toBool());
        QVERIFY(QMetaObject::invokeMethod(box, "refuse"));
        // Avec un paramètre : refusé visiblement, aucune boîte de confirmation.
        putForeign(s, QStringLiteral("doc-params"), document, {QStringLiteral("-x")});
        QCOMPARE(launchers->run(QStringLiteral("doc-params")), ui::Launchers::kFailed);
        QTRY_VERIFY(failure->property("opened").toBool());
        QVERIFY(!failure->property("message").toString().isEmpty());
        QVERIFY(!box->property("opened").toBool());
    }

    // Onglet d'édition : l'ajout passe par les champs ; un document n'offre pas de paramètres.
    void editorTabAddsAndRemoves()
    {
        Shell s;
        QVERIFY(start(s));
        QObject* tab = page(s, true);
        QVERIFY(tab);
        QObject* pathField = tab->findChild<QObject*>(QStringLiteral("launcherPathField"));
        QObject* argsField = tab->findChild<QObject*>(QStringLiteral("launcherArgsField"));
        QObject* tipField = tab->findChild<QObject*>(QStringLiteral("launcherTipField"));
        QObject* addButton = tab->findChild<QObject*>(QStringLiteral("launcherAddButton"));
        QVERIFY(pathField && argsField && tipField && addButton);
        auto* bar = named(s, "shortcutBar");
        QVERIFY(bar);
        QCOMPARE(bar->property("launcherCount").toInt(), 0);

        // Un document : le champ des paramètres est fermé.
        pathField->setProperty("text", witness(QStringLiteral("note.txt")));
        QVERIFY(!argsField->property("enabled").toBool());
        // Un programme : les paramètres sont offerts.
        pathField->setProperty("text", m_probe);
        QVERIFY(argsField->property("enabled").toBool());
        argsField->setProperty("text", witness(QStringLiteral("t-onglet.txt")));
        tipField->setProperty("text", QStringLiteral("Programme d'essai"));
        QVERIFY(addButton->property("enabled").toBool());
        QVERIFY(QMetaObject::invokeMethod(addButton, "clicked"));
        QTRY_COMPARE(bar->property("launcherCount").toInt(), 1);
        QObject* button = findItem(s.window->contentItem(), QStringLiteral("launcherButton0"));
        QVERIFY(button);
        QCOMPARE(button->property("toolTip").toString(), QStringLiteral("Programme d'essai"));
        QVERIFY(pathField->property("text").toString().isEmpty()); // champs vidés après l'ajout

        QObject* remove = findItem(qobject_cast<QQuickItem*>(tab), QStringLiteral("launcherRemove0"));
        QVERIFY(remove);
        QVERIFY(QMetaObject::invokeMethod(remove, "clicked"));
        QTRY_COMPARE(bar->property("launcherCount").toInt(), 0);
    }

    // Le sélecteur de fichier rend une URL : la page la passe à Launchers::localPath (jamais de retrait de
    // « file:// » à la main : un « %20 » resterait, et « C:/ » perdrait sa lettre).
    void fileDialogGoesThroughLocalPath()
    {
        Shell s;
        QVERIFY(start(s));
        QObject* tab = page(s, true);
        QVERIFY(tab);
        QObject* dialog = tab->findChild<QObject*>(QStringLiteral("launcherFileDialog"));
        QObject* pathField = tab->findChild<QObject*>(QStringLiteral("launcherPathField"));
        QVERIFY(dialog && pathField);
        const QString path = m_work.filePath(QStringLiteral("mon programme.app"));
        QFile program(path);
        QVERIFY(program.open(QIODevice::WriteOnly)); // le sélecteur refuse un fichier absent
        program.close();
        QVERIFY(dialog->setProperty("selectedFile", QUrl::fromLocalFile(path)));
        QVERIFY(QMetaObject::invokeMethod(dialog, "accepted"));
        QCOMPARE(pathField->property("text").toString(), path);
    }

    // Menu des dossiers : « Renommer… » vise le dossier, même si un élément était choisi avant.
    void folderMenuRenamesTheFolder()
    {
        Shell s;
        QVERIFY(start(s));
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString folder = dir.filePath(QStringLiteral("B"));
        QVERIFY(QDir().mkpath(folder));
        const QString photo = dir.filePath(QStringLiteral("photo.png"));
        QVERIFY(QImage(8, 8, QImage::Format_RGB32).save(photo, "PNG"));
        QObject* browser = named(s, "resourceBrowser");
        QObject* menu = named(s, "folderMenu");
        QObject* dialog = named(s, "nameDialog");
        QVERIFY(browser && menu && dialog);
        browser->setProperty("selectedPath", photo);
        browser->setProperty("selectedFolder", folder);
        QQuickItem* item = nullptr;
        QVERIFY(QMetaObject::invokeMethod(menu, "itemAt", Q_RETURN_ARG(QQuickItem*, item), Q_ARG(int, 1)));
        QVERIFY(item);
        QCOMPARE(item->property("text").toString(), QStringLiteral("Renommer…"));
        QVERIFY(QMetaObject::invokeMethod(item, "triggered"));
        QCOMPARE(dialog->property("mode").toString(), QStringLiteral("rename"));
        QCOMPARE(dialog->property("target").toString(), folder);
    }

    // Le tampon se coupe quand l'élément choisi est renommé (il n'existe plus sous ce chemin).
    void stampStopsWhenTheChosenItemVanishes()
    {
        Shell s;
        QVERIFY(start(s));
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString photo = dir.filePath(QStringLiteral("photo.png"));
        QVERIFY(QImage(8, 8, QImage::Format_RGB32).save(photo, "PNG"));
        QObject* browser = named(s, "resourceBrowser");
        QObject* library = named(s, "appResources");
        QVERIFY(browser && library);
        QVERIFY(QMetaObject::invokeMethod(library, "setOtherFolder", Q_ARG(QUrl, QUrl::fromLocalFile(dir.path()))));
        QTRY_COMPARE(library->property("itemCount").toInt(), 1);
        browser->setProperty("selectedPath", photo);
        QVERIFY(QMetaObject::invokeMethod(browser, "setStamping", Q_ARG(QVariant, true)));
        QVERIFY(browser->property("stamping").toBool());
        bool renamed = false;
        QVERIFY(QMetaObject::invokeMethod(library, "rename", Q_RETURN_ARG(bool, renamed),
                                          Q_ARG(QString, photo), Q_ARG(QString, QStringLiteral("autre"))));
        QVERIFY(renamed);
        QTRY_VERIFY(!browser->property("stamping").toBool());
    }

    // E3 : bouton d'édition en bas de la barre, caché si la palette est réduite ou la barre masquée.
    void editButtonFollowsThePalette()
    {
        Shell s;
        QVERIFY(start(s));
        QObject* bar = named(s, "shortcutBar");
        QObject* edit = findItem(qobject_cast<QQuickItem*>(bar), QStringLiteral("launcherEditButton"));
        QVERIFY(edit);
        // Visible sous Windows seulement (la commande n'existe pas ailleurs) ; jamais si la palette est
        // réduite.
        s.profile->setProperty("paletteCollapsed", true);
        QVERIFY(!edit->property("visible").toBool());
    }
};

QTEST_MAIN(TstUiLauncherspage)
#include "tst_ui_launcherspage.moc"
