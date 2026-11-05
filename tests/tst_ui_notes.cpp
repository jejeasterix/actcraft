#include <actcraft/core/notesedits.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/notescontroller.h>

#include <QByteArray>
#include <QList>
#include <QSignalSpy>
#include <QStringList>
#include <QTest>
#include <QTextDocument>
#include <QUrl>
#include <QVariant>

#include <memory>
#include <string>
#include <utility>

using namespace actcraft;

class TstUiNotes : public QObject
{
    Q_OBJECT

private:
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

    // Image data: SVG qui référence un fichier (la TextArea la décoderait, Qt Svg lirait le fichier) ; image
    // data: PNG d'un point.
    static std::string svgData()
    {
        return "data:image/"
               "svg+xml;base64,"
               "PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSIxMCIgaGVpZ2h0PSIxMCI+"
               "PGltYWdlIGhyZWY9ImZpbGU6Ly8vYWN0Y3JhZnQtYWJzZW50L3gucG5nIiB3aWR0aD0iMTAiIGhlaWdodD0iMTAiLz48L"
               "3N2Zz4=";
    }
    static std::string pngData()
    {
        return "data:image/"
               "png;base64,"
               "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNkYPhfDwAChwGA60e6kgAAAABJRU5ErkJgg"
               "g==";
    }

    // Notes d'un fichier reçu : feuille importée, images locale, distante, data: SVG et data: PNG, lien.
    static std::string hostileNotes()
    {
        return "<style>@import url(file:///actcraft-absent/s.css);</style>"
               "<p><b>Gras</b> texte <img src=\"file:///actcraft-absent/i.png\">"
               "<img src=\"http://actcraft.invalid/i.png\"> <a "
               "href=\"http://actcraft.invalid/\">lien</a><img src=\"" +
               svgData() + "\"><img src=\"" + pngData() + "\"></p>";
    }

    // Premier fragment interdit dans le HTML donné à l'éditeur QML, ou vide.
    static QString firstForbidden(const QString& shown)
    {
        for (const char* forbidden :
             {"actcraft-absent", "actcraft.invalid", "@import", "<img", "data:image"}) {
            if (shown.contains(QLatin1String(forbidden))) {
                return QLatin1String(forbidden);
            }
        }
        return {};
    }

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

    // Bloquant (relecture du lot 6) : relire la page (refresh) et écrire (setHtml) ne chargent rien.
    void openedNotesLoadNoResource()
    {
        ResourceSpy spy;
        Fixture f;
        f.document.execute(std::make_unique<core::SetPageNotesCommand>(0, hostileNotes()));
        QVERIFY2(spy.requests.isEmpty(), spy.seen().constData());
        QVERIFY(f.notes.hasNotes());
        f.notes.setHtml(QString::fromStdString(hostileNotes()) + QStringLiteral("<p>x</p>"));
        QVERIFY2(spy.requests.isEmpty(), spy.seen().constData());
        QVERIFY(f.document.canUndo());
    }

    // L'éditeur QML (TextArea riche) chargerait lui-même les ressources : il ne reçoit que du HTML assaini,
    // texte et mise en forme gardés ; le document garde les notes reçues tant que personne n'écrit.
    void displayedNotesAreSanitized()
    {
        Fixture f;
        f.document.execute(std::make_unique<core::SetPageNotesCommand>(0, hostileNotes()));
        const QString shown = f.notes.html();
        QVERIFY(shown.contains(QLatin1String("Gras")));
        QVERIFY(shown.contains(QLatin1String("texte")));
        QVERIFY(shown.contains(QLatin1String("lien")));
        QVERIFY2(shown.contains(QLatin1String("font-weight:700")), qPrintable(shown));
        QVERIFY2(!shown.contains(QLatin1String("actcraft-absent")), qPrintable(shown));
        QVERIFY2(!shown.contains(QLatin1String("actcraft.invalid")), qPrintable(shown));
        QVERIFY2(!shown.contains(QLatin1String("@import")), qPrintable(shown));
        QVERIFY2(!shown.contains(QLatin1String("<img")), qPrintable(shown));
        QCOMPARE(f.document.document().page(0).notes, hostileNotes());
        QVERIFY(f.notes.hasNotes());
        // Le texte montré, rendu tel quel par l'éditeur, n'est pas une frappe.
        f.notes.setHtml(shown);
        QCOMPARE(f.document.document().page(0).notes, hostileNotes());
    }

    // Essai 2 (contre-relecture) : la TextArea décode les images data: et choisit le décodeur sur le contenu,
    // pas sur le type déclaré ; un SVG y lirait l'image qu'il référence. Choix : l'éditeur ne reçoit aucune
    // image, data: raster comprises ; la page garde les siennes tant que personne n'écrit.
    void displayedNotesCarryNoImage()
    {
        Fixture f;
        const std::string notes =
            "<p>avant <img src=\"" + svgData() + "\"> milieu <img src=\"" + pngData() + "\"> fin</p>";
        f.document.execute(std::make_unique<core::SetPageNotesCommand>(0, notes));
        const QString shown = f.notes.html();
        QVERIFY(shown.contains(QLatin1String("avant")));
        QVERIFY(shown.contains(QLatin1String("milieu")));
        QVERIFY(shown.contains(QLatin1String("fin")));
        QVERIFY2(firstForbidden(shown).isEmpty(),
                 qPrintable(firstForbidden(shown) + QLatin1Char(' ') + shown));
        QCOMPARE(f.document.document().page(0).notes, notes);
    }

    // Retour par annulation : notes hostiles à l'ouverture, frappe, annulation ; le texte montré est
    // réassaini.
    void undoShowsTheNotesSanitized()
    {
        ui::DocumentController document;
        ui::Commands commands;
        ui::NotesController notes;
        core::Document opened;
        opened.page(0).notes = hostileNotes();
        document.replaceDocument(std::move(opened), QUrl(), QStringLiteral("Notes")); // pile vide
        notes.setCommands(&commands);
        notes.setDocument(&document);
        QVERIFY2(firstForbidden(notes.html()).isEmpty(), qPrintable(notes.html()));
        notes.setHtml(QStringLiteral("<p>x</p>"));
        QCOMPARE(document.document().page(0).notes, std::string("<p>x</p>"));
        document.undo();
        QCOMPARE(document.document().page(0).notes, hostileNotes());
        const QString shown = notes.html();
        QVERIFY(shown.contains(QLatin1String("Gras")));
        QVERIFY2(firstForbidden(shown).isEmpty(),
                 qPrintable(firstForbidden(shown) + QLatin1Char(' ') + shown));
    }
};

QTEST_MAIN(TstUiNotes)
#include "tst_ui_notes.moc"
