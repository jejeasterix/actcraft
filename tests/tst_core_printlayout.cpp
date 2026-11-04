#include <actcraft/core/printlayout.h>

#include <QTest>

#include <string>

using namespace actcraft;

namespace {

// Document synthétique de S17 : 4 pages, la troisième allongée (1024 × 1536), notes sur les pages 2 et 4.
core::Document documentS17()
{
    std::vector<core::Page> pages(4);
    pages[2].height = 1536;
    pages[1].notes = "ligne a\nligne b";
    pages[3].notes = "ligne c";
    return core::Document(std::move(pages));
}

constexpr core::SizeF kA4{210, 297};

} // namespace

class TestPrintLayout : public QObject
{
    Q_OBJECT
private slots:
    void twoPerSheetRange2to4()
    {
        const auto d = documentS17();
        core::PrintSettings s;
        s.rows = 2;
        s.range = core::PrintRange::FromTo;
        s.from = 2;
        s.to = 4;
        const auto sheets = core::layoutSheets(d, s, 0, kA4);
        QCOMPARE(sheets.size(), std::size_t(2)); // 3 pages, 2 par feuille
        QCOMPARE(sheets[0].cells.size(), std::size_t(2));
        QCOMPARE(sheets[1].cells.size(), std::size_t(1));
        QCOMPARE(sheets[0].cells[0].page, std::size_t(1));
        QCOMPARE(sheets[1].cells[0].page, std::size_t(3));
    }
    void notesAddSheets()
    {
        const auto d = documentS17();
        core::PrintSettings s;
        s.rows = 2;
        s.range = core::PrintRange::FromTo;
        s.from = 2;
        s.to = 4;
        s.notes = core::PrintNotes::WithPages;
        QCOMPARE(core::sheetCount(d, s, 0, kA4), 4); // 2 de pages + 2 de notes (pages 2 et 4)
        const auto sheets = core::layoutSheets(d, s, 0, kA4);
        QCOMPARE(sheets.size(), std::size_t(4));
        QVERIFY(!sheets[0].notes && !sheets[1].notes && sheets[2].notes && sheets[3].notes);
        QCOMPARE(sheets[2].noteOf, (std::vector<std::size_t>{1}));
        QCOMPARE(sheets[3].noteOf, (std::vector<std::size_t>{3}));
    }
    void notesOnlyAndContinuousNotes()
    {
        const auto d = documentS17();
        core::PrintSettings s;
        s.notes = core::PrintNotes::NotesOnly;
        QCOMPARE(core::sheetCount(d, s, 0, kA4), 2);
        for (const auto& sh : core::layoutSheets(d, s, 0, kA4))
            QVERIFY(sh.notes && sh.cells.empty());
        s.noteBreak = false; // 3 lignes au total : une seule feuille de notes
        QCOMPARE(core::sheetCount(d, s, 0, kA4), 1);
        const auto one = core::layoutSheets(d, s, 0, kA4);
        QCOMPARE(one[0].noteOf, (std::vector<std::size_t>{1, 3}));
        // Plus de kNoteLinesPerSheet lignes : ⌈lignes / 60⌉ feuilles.
        std::vector<core::Page> pages(1);
        for (int i = 0; i < 2 * core::kNoteLinesPerSheet - 1; ++i)
            pages[0].notes += "x\n";
        pages[0].notes += "x"; // 2 × 60 lignes pile
        QCOMPARE(core::sheetCount(core::Document(pages), s, 0, kA4), 2);
        pages[0].notes += "\ny"; // 121 lignes
        QCOMPARE(core::sheetCount(core::Document(pages), s, 0, kA4), 3);
    }
    void continuousNotesSheetsCarryTheirFirstLine()
    {
        std::vector<core::Page> pages(1);
        for (int i = 0; i < 129; ++i)
            pages[0].notes += "x\n";
        pages[0].notes += "x"; // 130 lignes : trois feuilles
        core::PrintSettings s;
        s.notes = core::PrintNotes::NotesOnly;
        s.noteBreak = false;
        const auto sheets = core::layoutSheets(core::Document(pages), s, 0, kA4);
        QCOMPARE(sheets.size(), std::size_t(3));
        QCOMPARE(sheets[0].firstNoteLine, 0);
        QCOMPARE(sheets[1].firstNoteLine, core::kNoteLinesPerSheet);
        QCOMPARE(sheets[2].firstNoteLine, 2 * core::kNoteLinesPerSheet);
        // Un saut de feuille par page : toujours 0.
        s.noteBreak = true;
        for (const auto& sh : core::layoutSheets(core::Document(pages), s, 0, kA4))
            QCOMPARE(sh.firstNoteLine, 0);
    }
    void longPageFitsWhole()
    {
        const auto d = documentS17();
        core::PrintSettings s;
        s.range = core::PrintRange::FromTo;
        s.from = 3;
        s.to = 3;
        const auto cell = core::layoutSheets(d, s, 0, kA4)[0].cells[0];
        QCOMPARE(cell.target.height / cell.target.width, 1536.0 / 1024.0);
        QVERIFY(cell.target.y + cell.target.height <= 297 - 5 + 1e-9);
        QVERIFY(cell.target.x + cell.target.width <= 210 - 5 + 1e-9);
        // La page allongée est réduite : sa cellule est plus étroite que celle d'une page normale.
        s.from = s.to = 1;
        QVERIFY(core::layoutSheets(d, s, 0, kA4)[0].cells[0].target.width > cell.target.width);
    }
    void copiesMultiply()
    {
        core::PrintSettings s;
        s.copies = 3;
        QCOMPARE(core::sheetCount(documentS17(), s, 0, kA4), 12);
        s.copies = 0; // au moins une copie
        QCOMPARE(core::sheetCount(documentS17(), s, 0, kA4), 4);
    }
    void marginRespected()
    {
        core::PrintSettings s;
        s.marginMm = 10;
        s.rows = 2;
        s.columns = 2;
        for (const auto& sh : core::layoutSheets(documentS17(), s, 0, kA4))
            for (const auto& c : sh.cells) {
                QVERIFY(c.target.x >= 10 - 1e-9 && c.target.y >= 10 - 1e-9);
                QVERIFY(c.target.x + c.target.width <= 210 - 10 + 1e-9);
                QVERIFY(c.target.y + c.target.height <= 297 - 10 + 1e-9);
            }
    }
    void currentOnly()
    {
        core::PrintSettings s;
        s.range = core::PrintRange::Current;
        const auto sheets = core::layoutSheets(documentS17(), s, 2, kA4);
        QCOMPARE(sheets.size(), std::size_t(1));
        QCOMPARE(sheets[0].cells[0].page, std::size_t(2));
    }
    void rangeIsClamped()
    {
        core::PrintSettings s;
        s.range = core::PrintRange::FromTo;
        s.from = 3;
        s.to = 99; // borné à la dernière page
        QCOMPARE(core::sheetCount(documentS17(), s, 0, kA4), 2);
        s.from = 4;
        s.to = 2; // bornes inversées : plage 2 à 4
        QCOMPARE(core::sheetCount(documentS17(), s, 0, kA4), 3);
        s.range = core::PrintRange::Current;
        QCOMPARE(core::layoutSheets(documentS17(), s, 40, kA4)[0].cells[0].page, std::size_t(3));
    }
};

QTEST_GUILESS_MAIN(TestPrintLayout)
#include "tst_core_printlayout.moc"
