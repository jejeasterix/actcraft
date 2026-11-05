#include <actcraft/core/document.h>
#include <actcraft/core/spellcheck.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/spelling.h>

#include <QFile>
#include <QList>
#include <QPointF>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRectF>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>
#include <QTextDocument>
#include <QUrl>
#include <QVariant>
#include <QtQml/qqmlextensionplugin.h>

#include <memory>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

class TstUiSpelling : public QObject
{
    Q_OBJECT

    QTemporaryDir m_dir;

    static void addText(core::Document& d, std::size_t page, core::ObjectId id, const char* html)
    {
        core::Text text;
        text.bounds = {10, 10, 300, 30};
        text.html = html;
        core::PageObject object{text};
        object.common.id = id;
        d.page(page).objects.push_back(object);
        d.refreshNextId();
    }
    // Deux pages ; « bonjour mosieur » sur la seconde (objet 2).

    struct Session
    {
        ui::Commands commands;
        ui::DocumentController controller;
        ui::Spelling spelling;
    };
    // Dossier du dictionnaire et fichier de l'utilisateur : toujours sous le dossier temporaire du test.
    std::unique_ptr<Session> session(const QString& name)
    {
        auto s = std::make_unique<Session>();
        core::Document d;
        d.appendPage();
        addText(d, 0, 1, "<p>le chat dort</p>");
        addText(d, 1, 2, "<p>bonjour mosieur</p>");
        s->controller.replaceDocument(std::move(d), QUrl(), QString());
        s->commands.setProfileDir(m_dir.filePath(QStringLiteral("disposition")));
        s->spelling.setProperty("commands",
                                QVariant::fromValue(&s->commands)); // câblage de SpellingDialog.qml
        s->spelling.setDocument(&s->controller);
        s->spelling.setDictionaryFolder(m_dir.filePath(QStringLiteral("dico")));
        s->spelling.setUserDictionaryPath(m_dir.filePath(name));
        return s;
    }

    // La boîte dans une fenêtre, avec le raccourci global Échap de tool.select (Main.qml,
    // ApplicationShortcut) et une vue de page réelle. Le dictionnaire de la boîte vit sous le dossier
    // temporaire du test.
    struct Mounted
    {
        ui::Commands commands;
        ui::DocumentController controller;
        QQmlEngine engine;
        std::unique_ptr<QObject> root;
        QQuickWindow* window = nullptr;
        QObject* dialog = nullptr;
        QObject* spelling = nullptr;
        QObject* view = nullptr;
        bool opened() const { return dialog->property("opened").toBool(); }
    };
    std::unique_ptr<Mounted> mount()
    {
        auto m = std::make_unique<Mounted>();
        m->commands.setProfileDir(m_dir.filePath(QStringLiteral("disposition-boite")));
        core::Document d;
        d.appendPage();
        addText(d, 0, 1, "<p>le chat dort</p>");
        addText(d, 1, 2, "<p>bonjour mosieur</p>");
        m->controller.replaceDocument(std::move(d), QUrl(), QString());
        QJSEngine::setObjectOwnership(&m->commands, QJSEngine::CppOwnership);
        QJSEngine::setObjectOwnership(&m->controller, QJSEngine::CppOwnership);
        QQmlComponent component(&m->engine);
        component.setData("import QtQuick\nimport QtQuick.Window\nimport ActCraft.Ui\n"
                          "Window {\n"
                          "  id: w; width: 800; height: 600; visible: true\n"
                          "  required property Commands commands\n"
                          "  required property DocumentController documentController\n"
                          "  property int toolSelects: 0\n"
                          "  Shortcut { sequence: \"Esc\"; context: Qt.ApplicationShortcut\n"
                          "    onActivated: w.toolSelects++ }\n"
                          "  PageView { id: view; objectName: \"view\"; anchors.fill: parent\n"
                          "    documentController: w.documentController }\n"
                          "  SpellingDialog { objectName: \"dialog\"; commands: w.commands\n"
                          "    documentController: w.documentController; view: view }\n"
                          "}",
                          QUrl(QStringLiteral("qrc:/tst_ui_spelling/Window.qml")));
        m->root.reset(component.createWithInitialProperties(
            {{QStringLiteral("commands"), QVariant::fromValue(&m->commands)},
             {QStringLiteral("documentController"), QVariant::fromValue(&m->controller)}}));
        if (!m->root) {
            qWarning().noquote() << component.errorString();
            return nullptr;
        }
        m->window = qobject_cast<QQuickWindow*>(m->root.get());
        m->dialog = m->root->findChild<QObject*>(QStringLiteral("dialog"));
        m->view = m->root->findChild<QObject*>(QStringLiteral("view"));
        m->spelling = m->dialog ? m->dialog->findChild<QObject*>(QStringLiteral("appSpelling")) : nullptr;
        if (!m->window || !m->dialog || !m->view || !m->spelling) {
            return nullptr;
        }
        m->spelling->setProperty("dictionaryFolder", m_dir.filePath(QStringLiteral("dico-boite")));
        m->spelling->setProperty("userDictionaryPath", m_dir.filePath(QStringLiteral("boite.txt")));
        m->window->requestActivate();
        if (!QTest::qWaitForWindowActive(m->window)) {
            return nullptr;
        }
        return m;
    }
    static QQuickItem* itemNamed(QQuickItem* item, const QString& name)
    {
        if (!item) {
            return nullptr;
        }
        if (item->objectName() == name) {
            return item;
        }
        for (QQuickItem* child : item->childItems()) {
            if (QQuickItem* found = itemNamed(child, name)) {
                return found;
            }
        }
        return nullptr;
    }
    std::string htmlOfPage2(const ui::DocumentController& c) const
    {
        return std::get<core::Text>(core::body(c.document().page(1).objects[0])).html;
    }

    // Bloquant 5 : toute demande de ressource qui échappe au document isolé. Un QTextDocument sans
    // fournisseur propre consulte le fournisseur par défaut avant de lire le fichier
    // (QTextDocument::resource) ; le document isolé (ui/src/textediting.cpp, AnalysisDocument) a le sien et
    // n'y passe jamais.
    struct ResourceSpy
    {
        QList<QUrl> requests;
        ResourceSpy()
        {
            QTextDocument::setDefaultResourceProvider([this](const QUrl& url) {
                requests.append(url);
                return QVariant();
            });
        }
        ~ResourceSpy() { QTextDocument::setDefaultResourceProvider({}); }
        ResourceSpy(const ResourceSpy&) = delete;
        ResourceSpy& operator=(const ResourceSpy&) = delete;
        QByteArray seen() const
        {
            QStringList urls;
            for (const QUrl& url : requests) {
                urls.append(url.toString());
            }
            return urls.join(QLatin1Char(' ')).toUtf8();
        }
    };

private slots:
    // Majeur (z3) : zoom 200 %, page défilée (pan négatif) : le surlignage garde la taille et la place du
    // mot.
    void highlightFollowsZoomAndScroll()
    {
        const std::unique_ptr<Mounted> m = mount();
        QVERIFY(m);
        QVERIFY(m->commands.trigger(QStringLiteral("spell.check")));
        QTRY_VERIFY(m->opened());
        QCOMPARE(m->dialog->property("word").toString(), QStringLiteral("mosieur"));
        m->view->setProperty("zoom", 2.0);
        m->view->setProperty("pan", QPointF(-600, -400));
        const qreal zoom = m->view->property("zoom").toReal();
        const QPointF pan = m->view->property("pan").toPointF();
        QVERIFY2(zoom > 1.9 && zoom + pan.x() + pan.y() < 0, "attendu : zoom 200 %, page défilée");
        QQuickItem* marker = itemNamed(m->window->contentItem(), QStringLiteral("spellingHighlight"));
        QVERIFY(marker);
        QTRY_VERIFY(marker->isVisible());
        const QRectF box = m->spelling->property("highlight").toRectF();
        QPointF topLeft;
        QPointF bottomRight;
        QVERIFY(QMetaObject::invokeMethod(m->view, "pageToScene", Q_RETURN_ARG(QPointF, topLeft),
                                          Q_ARG(QPointF, box.topLeft())));
        QVERIFY(QMetaObject::invokeMethod(m->view, "pageToScene", Q_RETURN_ARG(QPointF, bottomRight),
                                          Q_ARG(QPointF, box.bottomRight())));
        QVERIFY(bottomRight.x() - topLeft.x() > 2 * box.width() - 1); // zoom 200 % pris en compte
        const QPointF origin = marker->mapToScene(QPointF(0, 0));
        QVERIFY2(qAbs(origin.x() - (topLeft.x() - 2)) < 0.5 && qAbs(origin.y() - (topLeft.y() - 2)) < 0.5,
                 qPrintable(QStringLiteral("surlignage en %1,%2 ; mot en %3,%4")
                                .arg(origin.x())
                                .arg(origin.y())
                                .arg(topLeft.x())
                                .arg(topLeft.y())));
        QVERIFY(qAbs(marker->width() - (bottomRight.x() - topLeft.x() + 4)) < 0.5);
        QVERIFY(qAbs(marker->height() - (bottomRight.y() - topLeft.y() + 4)) < 0.5);
    }
    // Majeur (z3) : Échap ferme la boîte sans déclencher tool.select (même raccourci, global).
    void escapeClosesTheDialogOnly()
    {
        const std::unique_ptr<Mounted> m = mount();
        QVERIFY(m);
        QVERIFY(m->commands.trigger(QStringLiteral("spell.check")));
        QTRY_VERIFY(m->opened());
        QTRY_VERIFY(m->window->activeFocusItem() != nullptr);
        QTest::keyClick(m->window, Qt::Key_Escape);
        QTRY_VERIFY(!m->opened());
        QCOMPARE(m->root->property("toolSelects").toInt(), 0);
        QVERIFY(!m->spelling->property("running").toBool());
    }
    // Majeur (conformité z2) : la boîte se ferme quand le verrou de classe est posé.
    void lockClosesTheDialog()
    {
        const std::unique_ptr<Mounted> m = mount();
        QVERIFY(m);
        QVERIFY(m->commands.trigger(QStringLiteral("spell.check")));
        QTRY_VERIFY(m->opened());
        m->commands.setLocked(true);
        QTRY_VERIFY(!m->opened());
        QVERIFY(!m->spelling->property("running").toBool());
    }
    // Le mot vient du fichier ouvert : affiché en texte brut, jamais interprété.
    void wordIsPlainText()
    {
        const std::unique_ptr<Mounted> m = mount();
        QVERIFY(m);
        QObject* label = m->dialog->findChild<QObject*>(QStringLiteral("spellingWord"));
        QVERIFY(label);
        QCOMPARE(label->property("textFormat").toInt(), 0); // Text.PlainText
    }
    void initTestCase() { QVERIFY(m_dir.isValid()); }

    void showsThePageOfTheWord()
    {
        auto s = session(QStringLiteral("a.txt"));
        QCOMPARE(s->controller.currentIndex(), 0);
        s->spelling.start();
        QVERIFY(s->spelling.running());
        QCOMPARE(s->spelling.word(), QStringLiteral("mosieur"));
        QCOMPARE(s->spelling.hitPage(), 1);
        QCOMPARE(s->controller.currentIndex(), 1);
        QVERIFY(s->spelling.highlight().width() > 0);
        QVERIFY(s->spelling.suggestions().contains(QStringLiteral("monsieur")));
        QVERIFY(!s->spelling.finished());
    }

    void endMessage()
    {
        auto s = session(QStringLiteral("b.txt"));
        s->spelling.start();
        s->spelling.replace(QStringLiteral("monsieur"));
        QVERIFY(!s->spelling.running());
        QVERIFY(s->spelling.finished());
        QCOMPARE(s->spelling.message(), QStringLiteral("Vérification terminée"));
        QVERIFY(htmlOfPage2(s->controller).find("monsieur") != std::string::npos);
        QVERIFY(s->controller.canUndo());
        s->controller.undo();
        QVERIFY(htmlOfPage2(s->controller).find("mosieur") != std::string::npos);
    }

    void pageChangeWaitsForTheEndOfContact()
    {
        auto s = session(QStringLiteral("c.txt"));
        s->spelling.setContactActive(true);
        s->spelling.start();
        QCOMPARE(s->spelling.word(), QStringLiteral("mosieur"));
        QCOMPARE(s->controller.currentIndex(), 0);
        s->spelling.setContactActive(false);
        QCOMPARE(s->controller.currentIndex(), 1);
    }

    void addToDictionaryIsKept()
    {
        auto s = session(QStringLiteral("d.txt"));
        s->spelling.start();
        s->spelling.addToDictionary();
        QVERIFY(s->spelling.finished());
        QFile file(m_dir.filePath(QStringLiteral("d.txt")));
        QVERIFY(file.open(QIODevice::ReadOnly));
        QVERIFY(file.readAll().contains("mosieur"));
        auto again = session(QStringLiteral("d.txt")); // autre moteur, même fichier
        again->spelling.start();
        QVERIFY(again->spelling.finished());
    }

    void ignoreAlwaysEndsTheCheck()
    {
        auto s = session(QStringLiteral("e.txt"));
        s->spelling.start();
        s->spelling.ignoreAlways();
        QVERIFY(s->spelling.finished());
        QVERIFY(htmlOfPage2(s->controller).find("mosieur") != std::string::npos);
    }

    // Bloquant (relecture du lot 6) : le cadre du mot (highlightOf) se mesure sans rien charger, même quand
    // le texte du fichier porte une image locale.
    void highlightLoadsNoResource()
    {
        ResourceSpy spy;
        auto s = std::make_unique<Session>();
        core::Document d;
        addText(d, 0, 1, "<p>bonjour mosieur <img src=\"file:///actcraft-absent/i.png\"></p>");
        s->controller.replaceDocument(std::move(d), QUrl(), QString());
        s->spelling.setDocument(&s->controller);
        s->spelling.setDictionaryFolder(m_dir.filePath(QStringLiteral("dico")));
        s->spelling.setUserDictionaryPath(m_dir.filePath(QStringLiteral("f.txt")));
        s->spelling.start();
        QCOMPARE(s->spelling.word(), QStringLiteral("mosieur"));
        QVERIFY(s->spelling.highlight().width() > 0);
        QVERIFY2(spy.requests.isEmpty(), spy.seen().constData());
    }
};

QTEST_MAIN(TstUiSpelling)
#include "tst_ui_spelling.moc"
