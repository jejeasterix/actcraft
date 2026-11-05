#include <actcraft/core/document.h>
#include <actcraft/core/spellcheck.h>
#include <actcraft/core/textedits.h>
#include <actcraft/core/undo.h>

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include <algorithm>

using namespace actcraft::core;

class TstCoreSpellcheck : public QObject
{
    Q_OBJECT

    QTemporaryDir m_dir;
    QString m_aff;
    QString m_dic;
    std::unique_ptr<SpellEngine> m_engine;

    static void addText(Document& d, std::size_t page, ObjectId id, const char* html,
                        Layer layer = Layer::Middle)
    {
        Text text;
        text.bounds = {10, 10, 300, 30};
        text.html = html;
        PageObject object{text};
        object.common.id = id;
        object.common.layer = layer;
        d.page(page).objects.push_back(object);
        d.refreshNextId();
    }
    // Deux pages ; « bonjour mosieur » sur la seconde (objet 2).
    static Document twoPages()
    {
        Document d;
        d.appendPage();
        addText(d, 0, 1, "<p>le chat dort</p>");
        addText(d, 1, 2, "<p>bonjour mosieur</p>");
        return d;
    }
    static std::string htmlOf(const Document& d, std::size_t page, std::size_t index)
    {
        return std::get<Text>(body(d.page(page).objects[index])).html;
    }
    QString userPath() const { return m_dir.filePath(QStringLiteral("utilisateur.txt")); }

private slots:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
        // Dictionnaire posé par L6-T32 : copié dans le dossier temporaire, jamais modifié sur place.
        const QString aff = QFINDTESTDATA("../third_party/dictionaries/fr/fr.aff");
        const QString dic = QFINDTESTDATA("../third_party/dictionaries/fr/fr.dic");
        QVERIFY2(!aff.isEmpty() && !dic.isEmpty(), "dictionnaire français introuvable");
        qInfo().noquote() << "dictionnaire :" << aff;
        m_aff = m_dir.filePath(QStringLiteral("fr.aff"));
        m_dic = m_dir.filePath(QStringLiteral("fr.dic"));
        QVERIFY(QFile::copy(aff, m_aff));
        QVERIFY(QFile::copy(dic, m_dic));
    }
    void init()
    {
        m_engine = makeHunspellEngine(m_aff.toStdString(), m_dic.toStdString());
        QVERIFY(m_engine != nullptr);
    }

    void findsTheMisspelledWord()
    {
        const Document d = twoPages();
        SpellChecker checker(*m_engine, SpellRules{});
        const auto hit = checker.next(d);
        QVERIFY(hit.has_value());
        QCOMPARE(hit->word, std::string("mosieur"));
        QCOMPARE(hit->page, 1);
        QCOMPARE(hit->object, ObjectId{2});
        QCOMPARE(hit->start, std::size_t{8});
        QCOMPARE(hit->length, std::size_t{7});
        QVERIFY(!hit->repeated);
        // Sans avancer le curseur, le même mot revient.
        QCOMPARE(checker.next(d)->word, std::string("mosieur"));
    }
    void suggestsTheCorrection()
    {
        const auto suggestions = m_engine->suggest("mosieur");
        QVERIFY(std::find(suggestions.begin(), suggestions.end(), std::string("monsieur")) !=
                suggestions.end());
    }
    void walksPagesFromTheFirst()
    {
        Document d = twoPages();
        addText(d, 0, 4, "<p>devanttt</p>", Layer::Top);
        // Page::objects est rangé par couche : le fond d'abord.
        Text back;
        back.bounds = {10, 10, 300, 30};
        back.html = "<p>fondddd</p>";
        PageObject backObject{back};
        backObject.common.id = 3;
        backObject.common.layer = Layer::Background;
        d.page(0).objects.insert(d.page(0).objects.begin(), backObject);
        d.refreshNextId();
        SpellChecker checker(*m_engine, SpellRules{});
        auto hit = checker.next(d);
        QVERIFY(hit);
        QCOMPARE(hit->page, 0);
        QCOMPARE(hit->word, std::string("fondddd"));
        checker.ignore();
        hit = checker.next(d);
        QVERIFY(hit);
        QCOMPARE(hit->word, std::string("devanttt"));
        checker.ignore();
        hit = checker.next(d);
        QVERIFY(hit);
        QCOMPARE(hit->page, 1);
        QCOMPARE(hit->word, std::string("mosieur"));
        checker.ignore();
        QVERIFY(!checker.next(d));
        checker.restart();
        QCOMPARE(checker.next(d)->word, std::string("fondddd"));
    }
    void ignoreAlwaysSkipsTheWord()
    {
        Document d = twoPages();
        addText(d, 1, 3, "<p>mosieur et Mosieur</p>");
        addText(d, 1, 4, "<p>madamme</p>");
        SpellChecker checker(*m_engine, SpellRules{});
        QCOMPARE(checker.next(d)->word, std::string("mosieur"));
        checker.ignore(); // seulement celui-ci
        QCOMPARE(checker.next(d)->object, ObjectId{3});
        checker.ignoreAlways("mosieur");
        const auto hit = checker.next(d);
        QVERIFY(hit);
        QCOMPARE(hit->word, std::string("madamme"));
        checker.ignore();
        QVERIFY(!checker.next(d));
        checker.restart(); // la liste de la session est gardée
        QCOMPARE(checker.next(d)->word, std::string("madamme"));
    }
    void userDictionaryKept()
    {
        const std::string path = userPath().toStdString();
        QVERIFY(!m_engine->isCorrect("mosieur"));
        QVERIFY(addToUserDictionary(*m_engine, path, "mosieur"));
        QVERIFY(m_engine->isCorrect("mosieur"));
        QVERIFY(addToUserDictionary(*m_engine, path, "mosieur")); // déjà là : pas de doublon
        QVERIFY(addToUserDictionary(*m_engine, path, "zorglub"));
        QFile file(userPath());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("mosieur\nzorglub\n"));
        QVERIFY(!userPath().contains(QStringLiteral("third_party")));
        // Autre moteur, autre session : le fichier suffit.
        init();
        QVERIFY(!m_engine->isCorrect("zorglub"));
        QCOMPARE(loadUserDictionary(*m_engine, path), std::size_t{2});
        QVERIFY(m_engine->isCorrect("zorglub"));
        SpellChecker checker(*m_engine, SpellRules{});
        QVERIFY(!checker.next(twoPages()));
        QCOMPARE(loadUserDictionary(*m_engine, m_dir.filePath(QStringLiteral("absent.txt")).toStdString()),
                 std::size_t{0});
    }
    void repeatedWordsFlagged()
    {
        const SpellRules defaults;
        QVERIFY(!defaults.ignoreCapitalized && !defaults.ignoreUppercase && !defaults.ignoreDigits &&
                !defaults.ignoreMixedCase && !defaults.ignoreAddresses);
        QVERIFY(defaults.flagRepeated);
        Document d;
        addText(d, 0, 1, "<p>voici le le chat</p>");
        SpellChecker checker(*m_engine, defaults);
        const auto hit = checker.next(d);
        QVERIFY(hit);
        QCOMPARE(hit->word, std::string("le"));
        QVERIFY(hit->repeated);
        QCOMPARE(hit->start, std::size_t{9});
        SpellRules off;
        off.flagRepeated = false;
        SpellChecker quiet(*m_engine, off);
        QVERIFY(!quiet.next(d));
    }
    void rulesIgnoreWhatTheyName()
    {
        Document d;
        addText(d, 0, 1, "<p>Zorbo NASAA abc123 iPhonne joe@exemplee.fr https://exemplee.fr</p>");
        SpellRules all;
        all.ignoreCapitalized = all.ignoreUppercase = all.ignoreDigits = all.ignoreMixedCase =
            all.ignoreAddresses = true;
        SpellChecker none(*m_engine, all);
        QVERIFY(!none.next(d));
        // Par défaut, chacun est signalé.
        SpellChecker checker(*m_engine, SpellRules{});
        std::vector<std::string> words;
        for (auto hit = checker.next(d); hit; hit = checker.next(d)) {
            words.push_back(hit->word);
            checker.ignore();
        }
        for (const char* w : {"Zorbo", "NASAA", "abc123", "iPhonne"}) {
            QVERIFY2(std::find(words.begin(), words.end(), std::string(w)) != words.end(), w);
        }
        // Une règle à la fois.
        SpellRules onlyDigits;
        onlyDigits.ignoreDigits = true;
        SpellChecker digits(*m_engine, onlyDigits);
        for (auto hit = digits.next(d); hit; hit = digits.next(d)) {
            QVERIFY(hit->word != "abc123");
            digits.ignore();
        }
    }
    void replaceIsUndoneAtOnce()
    {
        Document d;
        d.appendPage();
        addText(d, 1, 2, "<p>bonjour <b>mosieur</b> &amp; madame</p>");
        const std::string before = htmlOf(d, 1, 0);
        SpellChecker checker(*m_engine, SpellRules{});
        const auto hit = checker.next(d);
        QVERIFY(hit);
        auto command = checker.replace(d, *hit, "monsieur");
        QVERIFY(command != nullptr);
        UndoStack stack;
        stack.push(d, std::move(command));
        QCOMPARE(stack.count(), std::size_t{1});
        QCOMPARE(htmlOf(d, 1, 0), std::string("<p>bonjour <b>monsieur</b> &amp; madame</p>"));
        QVERIFY(!checker.next(d));
        stack.undo(d);
        QCOMPARE(htmlOf(d, 1, 0), before);
        // Le mot n'est plus à sa place : rien.
        Document other;
        QVERIFY(checker.replace(other, *hit, "monsieur") == nullptr);
    }
    // Amendement 15 : le HTML de l'éditeur (QTextDocument::toHtml) porte des retours à la ligne entre ses
    // balises ; ils ne comptent pas dans les positions (« mosieur » commence à 8, comme dans QTextDocument).
    void editorHtmlStartsAtTheFirstLetter()
    {
        Document d;
        addText(d, 0, 1,
                "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
                "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
                "p, li { white-space: pre-wrap; }\n"
                "</style></head><body style=\" font-size:9pt;\">\n"
                "<p style=\" margin-top:0px;\"><span style=\" font-size:14pt;\">bonjour mosieur</span></p></body></html>");
        SpellChecker checker(*m_engine, SpellRules{});
        const auto hit = checker.next(d);
        QVERIFY(hit);
        QCOMPARE(hit->word, std::string("mosieur"));
        QCOMPARE(hit->start, std::size_t{8});
        QVERIFY(checker.replace(d, *hit, "monsieur") != nullptr);
    }
    void htmlTagsAreNotWords()
    {
        Document d;
        addText(
            d, 0, 1,
            "<html><head><style>p { color:red; fontt-size:12px }</style></head><body>"
            "<p style=\"color:#ff0000; fontfamily:zzz\"><b>bonjour</b> <i><span style=\"font-size:12px\">"
            "monsieur</span></i></p><p>bon<b>jour</b></p><p>chat&nbsp;&amp;&nbsp;chien</p></body></html>");
        SpellChecker checker(*m_engine, SpellRules{});
        const auto hit = checker.next(d);
        QVERIFY2(!hit, hit ? hit->word.c_str() : "");
    }
};

QTEST_MAIN(TstCoreSpellcheck)
#include "tst_core_spellcheck.moc"
