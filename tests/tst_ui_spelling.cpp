#include <actcraft/core/document.h>
#include <actcraft/core/spellcheck.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/spelling.h>

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

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
        s->spelling.setDocument(&s->controller);
        s->spelling.setDictionaryFolder(m_dir.filePath(QStringLiteral("dico")));
        s->spelling.setUserDictionaryPath(m_dir.filePath(name));
        return s;
    }
    std::string htmlOfPage2(const ui::DocumentController& c) const
    {
        return std::get<core::Text>(core::body(c.document().page(1).objects[0])).html;
    }

private slots:
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
};

QTEST_MAIN(TstUiSpelling)
#include "tst_ui_spelling.moc"
