#include <actcraft/core/notesedits.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/notescontroller.h>

#include <QSignalSpy>
#include <QTest>

#include <memory>
#include <utility>

using namespace actcraft;

class TstUiNotes : public QObject
{
    Q_OBJECT

private:
    // Document de deux pages ; sans notes. Page 1 affichée.
    struct Fixture
    {
        ui::DocumentController document;
        ui::Commands commands;
        ui::NotesController notes;
        Fixture()
        {
            core::Document two;
            two.appendPage();
            document.replaceDocument(std::move(two), QUrl(), QStringLiteral("Notes")); // pile vide
            notes.setCommands(&commands);
            notes.setDocument(&document);
        }
    };

private slots:
    void followsTheCurrentPage()
    {
        Fixture f;
        f.document.execute(std::make_unique<core::SetPageNotesCommand>(0, "<p>A</p>"));
        f.document.showPage(1);
        f.document.execute(std::make_unique<core::SetPageNotesCommand>(1, "<p>B</p>"));
        QVERIFY(f.notes.html().contains(QLatin1String("B")));
        f.document.showPage(0);
        QVERIFY(f.notes.html().contains(QLatin1String("A")));
        f.document.showPage(1);
        QVERIFY(f.notes.html().contains(QLatin1String("B")));
    }

    void writingGoesThroughTheStackAndUndoRefreshes()
    {
        Fixture f;
        f.notes.setHtml(QStringLiteral("<p>x</p>"));
        QCOMPARE(f.document.document().page(0).notes, std::string("<p>x</p>"));
        QVERIFY(f.document.canUndo());
        f.document.undo();
        QCOMPARE(f.notes.html(), QString());
        QVERIFY(!f.notes.hasNotes());
    }

    void hasNotesFlag()
    {
        Fixture f;
        f.notes.setHtml(QStringLiteral("<p></p>"));
        QVERIFY(!f.notes.hasNotes());
        f.notes.setHtml(QStringLiteral("<p>x</p>"));
        QVERIFY(f.notes.hasNotes());
    }

    void switchingPageCreatesNoStep()
    {
        Fixture f;
        QVERIFY(!f.document.canUndo());
        const bool modified = f.document.isModified();
        QSignalSpy htmlChanged(&f.notes, &ui::NotesController::htmlChanged);
        f.document.showPage(1);
        f.document.showPage(0);
        f.document.showPage(1);
        QVERIFY(!f.document.canUndo());
        QCOMPARE(f.document.isModified(), modified);
        // Une écriture identique au texte courant, ou faite pendant le rechargement, ne change rien.
        f.notes.setHtml(f.notes.html());
        QVERIFY(!f.document.canUndo());
    }

    void refusedWhileLocked()
    {
        Fixture f;
        f.commands.setLocked(true);
        f.notes.setHtml(QStringLiteral("<p>x</p>"));
        QVERIFY(!f.document.canUndo());
        QCOMPARE(f.document.document().page(0).notes, std::string());
        QCOMPARE(f.notes.html(), QString());
    }

    void refusedWithoutCommands()
    {
        ui::DocumentController document;
        ui::NotesController notes;
        notes.setDocument(&document);
        notes.setHtml(QStringLiteral("<p>x</p>"));
        QVERIFY(!document.canUndo());
        QVERIFY(!notes.isEditable());
    }

    void editableFollowsTheLock()
    {
        Fixture f;
        QVERIFY(f.notes.isEditable());
        QSignalSpy editable(&f.notes, &ui::NotesController::editableChanged);
        f.commands.setLocked(true);
        QVERIFY(!f.notes.isEditable());
        f.commands.setLocked(false);
        QVERIFY(f.notes.isEditable());
        QCOMPARE(editable.count(), 2);
    }

    void contactDefersTheRefresh()
    {
        Fixture f;
        f.notes.setContactActive(true);
        f.document.execute(std::make_unique<core::SetPageNotesCommand>(0, "<p>A</p>"));
        QCOMPARE(f.notes.html(), QString());
        f.notes.setContactActive(false);
        QVERIFY(f.notes.html().contains(QLatin1String("A")));
    }

    void typingMergesIntoOneStep()
    {
        Fixture f;
        f.notes.setHtml(QStringLiteral("<p>a</p>"));
        f.notes.setHtml(QStringLiteral("<p>ab</p>"));
        f.notes.setHtml(QStringLiteral("<p>abc</p>"));
        f.document.undo();
        QVERIFY(!f.document.canUndo());
        QCOMPARE(f.notes.html(), QString());
    }
};

QTEST_MAIN(TstUiNotes)
#include "tst_ui_notes.moc"
