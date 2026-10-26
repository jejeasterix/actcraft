#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pagescontroller.h>

#include <QByteArray>
#include <QList>
#include <QSignalSpy>
#include <QString>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

constexpr int kShift = static_cast<int>(Qt::ShiftModifier);
constexpr int kCtrl = static_cast<int>(Qt::ControlModifier);

// Document de `count` pages nommées « 1 », « 2 »… et son contrôleur de pages.
struct Fixture
{
    explicit Fixture(int count)
    {
        std::vector<core::Page> pages;
        for (int i = 1; i <= count; ++i) {
            core::Page page;
            page.name = std::to_string(i);
            pages.push_back(std::move(page));
        }
        document.replaceDocument(core::Document(std::move(pages)), QUrl(), QStringLiteral("essai"));
        pagesController.setDocumentController(&document);
    }

    // Noms des pages dans l'ordre : « 3412 ».
    QString order() const
    {
        QString text;
        for (std::size_t i = 0; i < document.document().pageCount(); ++i) {
            const std::string& name = document.document().page(i).name;
            text += name.empty() ? QStringLiteral("-") : QString::fromStdString(name); // « - » : page vierge
        }
        return text;
    }

    ui::DocumentController document;
    ui::PagesController pagesController;
};

} // namespace

class TstUiPages : public QObject
{
    Q_OBJECT

private slots:
    void clickSelectsAndShows()
    {
        Fixture f(4);
        f.pagesController.click(2, 0);
        QCOMPARE(f.pagesController.selection(), (QList<int>{2}));
        QCOMPARE(f.document.currentIndex(), 2);
    }

    void shiftSelectsRange()
    {
        Fixture f(4);
        f.pagesController.click(1, 0);
        f.pagesController.click(3, kShift);
        QCOMPARE(f.pagesController.selection(), (QList<int>{1, 2, 3}));
        QCOMPARE(f.document.currentIndex(), 3);
        // L'ancre reste la première page cliquée : la plage se resserre ou s'inverse.
        f.pagesController.click(0, kShift);
        QCOMPARE(f.pagesController.selection(), (QList<int>{0, 1}));
    }

    void ctrlToggles()
    {
        Fixture f(4);
        f.pagesController.click(0, 0);
        f.pagesController.click(2, kCtrl);
        QCOMPARE(f.pagesController.selection(), (QList<int>{0, 2}));
        f.pagesController.click(0, kCtrl);
        QCOMPARE(f.pagesController.selection(), (QList<int>{2}));
        f.pagesController.click(2, kCtrl); // la dernière page sélectionnée ne se retire pas
        QCOMPARE(f.pagesController.selection(), (QList<int>{2}));
    }

    void selectionFollowsShownPage()
    {
        Fixture f(4);
        f.pagesController.click(0, 0);
        f.pagesController.click(1, kShift);
        f.document.showPage(3); // autre voie (clavier, barre de pages)
        QCOMPARE(f.pagesController.selection(), (QList<int>{3}));
        f.document.undo(); // rien à défaire : la sélection reste valide
        QVERIFY(!f.pagesController.selection().isEmpty());
        f.pagesController.deleteSelection(); // page 4 retirée : la sélection suit la page affichée
        QCOMPARE(f.document.pageCount(), 3);
        QCOMPARE(f.pagesController.selection(), (QList<int>{f.document.currentIndex()}));
        f.document.undo(); // la page revient : la sélection reste dans les bornes
        // Q16 (L4-C11) : la boucle seule passerait sur une sélection vide ; elle garde la page affichée.
        QVERIFY(f.pagesController.selection().contains(f.document.currentIndex()));
        for (const int index : f.pagesController.selection()) {
            QVERIFY(index >= 0 && index < f.document.pageCount());
        }
    }

    // M-J (L4-C11) : cliquer la miniature de la page affichée (seule, Maj, Ctrl qui en retire une autre, goTo)
    // ne la remontre pas. showPage annoncerait un changement de page et referait la page fixe (show → attach),
    // ce que trahissent pageAboutToChange et pagesChanged.
    void clickOnShownPageDoesNotShowItAgain()
    {
        Fixture f(4);
        f.pagesController.click(0, 0);
        f.pagesController.click(2, kCtrl); // sélection {0, 2}, page 2 affichée
        QSignalSpy aboutToChange(&f.document, &ui::DocumentController::pageAboutToChange);
        QSignalSpy pagesChanged(&f.document, &ui::DocumentController::pagesChanged);
        f.pagesController.click(0, kCtrl); // retire la page 0 : la page 2 reste affichée
        QCOMPARE(f.pagesController.selection(), (QList<int>{2}));
        f.pagesController.click(2, 0);
        f.pagesController.click(2, kShift);
        f.pagesController.goTo(2);
        QCOMPARE(f.document.currentIndex(), 2);
        QCOMPARE(f.pagesController.selection(), (QList<int>{2}));
        QCOMPARE(aboutToChange.count(), 0);
        QCOMPARE(pagesChanged.count(), 0);
        f.pagesController.click(1, 0); // un vrai changement de page s'annonce toujours
        QCOMPARE(f.document.currentIndex(), 1);
        QCOMPARE(aboutToChange.count(), 1);
    }

    // M-K (L4-C11) : les pages copiées ou coupées emportent les octets de leurs ressources (fond, image,
    // fichier d'une action) ; un collage dans un autre document, ouvert depuis, les y ajoute. Le
    // presse-papiers survit au changement de document ; une ressource que la page ne cite pas ne suit pas.
    void pasteIntoAnotherDocumentKeepsResources()
    {
        for (const bool cut : {false, true}) {
            Fixture f(1);
            core::Document source(std::vector<core::Page>(2));
            const QByteArray imageBytes("octets-image");
            const QByteArray backgroundBytes("octets-fond");
            const QByteArray fileBytes("octets-fichier");
            const std::string imageKey = source.addResource(imageBytes, "png");
            const std::string backgroundKey = source.addResource(backgroundBytes, "jpg");
            const std::string fileKey = source.addResource(fileBytes, "pdf");
            static_cast<void>(source.addResource(QByteArray("octets-ailleurs"), "png"));
            core::Page& page = source.page(0);
            page.name = "A";
            page.background.image = backgroundKey;
            core::ImageObject image;
            image.resource = imageKey;
            core::PageObject object = image;
            object.common.id = source.allocateId();
            core::Action open;
            open.resource = fileKey;
            object.common.actions.push_back(open);
            page.objects.push_back(std::move(object));
            f.document.replaceDocument(std::move(source), QUrl(), QStringLiteral("source"));
            f.pagesController.click(0, 0);
            if (cut) {
                f.pagesController.cutSelection();
            } else {
                f.pagesController.copySelection();
            }

            f.document.replaceDocument(core::Document(std::vector<core::Page>(2)), QUrl(),
                                       QStringLiteral("cible"));
            QVERIFY2(f.pagesController.canPaste(), cut ? "cut" : "copy");
            QVERIFY(f.document.document().resources().empty());
            f.pagesController.paste();
            const core::Document& target = f.document.document();
            QCOMPARE(f.document.pageCount(), 3);
            QCOMPARE(target.page(0).name, std::string("A"));
            const core::Resource* pastedImage = target.resource(imageKey);
            QVERIFY2(pastedImage != nullptr, cut ? "cut" : "copy");
            QCOMPARE(pastedImage->data, imageBytes);
            QCOMPARE(pastedImage->extension, std::string("png"));
            QVERIFY(target.resource(backgroundKey) && target.resource(backgroundKey)->data == backgroundBytes);
            QVERIFY(target.resource(fileKey) && target.resource(fileKey)->data == fileBytes);
            QCOMPARE(target.resources().size(), std::size_t{3});
            f.document.undo();
            QCOMPARE(f.document.pageCount(), 2);
            f.document.redo();
            QVERIFY(f.document.document().resource(imageKey) != nullptr);
        }
    }

    void moveKeepsSelection()
    {
        Fixture f(4);
        f.pagesController.click(2, 0);
        f.pagesController.click(3, kShift);
        f.pagesController.moveSelection(0);
        QCOMPARE(f.order(), QStringLiteral("3412"));
        QCOMPARE(f.pagesController.selection(), (QList<int>{0, 1}));
        QCOMPARE(f.document.currentIndex(), 0); // la première page déplacée devient la page courante
    }

    void pasteInsertsBeforeActive()
    {
        Fixture f(4);
        f.pagesController.click(0, 0);
        f.pagesController.copySelection();
        QVERIFY(f.pagesController.canPaste());
        f.pagesController.goTo(2);
        f.pagesController.paste();
        QCOMPARE(f.document.pageCount(), 5);
        QCOMPARE(f.order(), QStringLiteral("12134"));
        QCOMPARE(f.pagesController.selection(), (QList<int>{2}));
        QCOMPARE(f.document.currentIndex(), 2);
    }

    void cutThenPasteMoves()
    {
        Fixture f(4);
        f.pagesController.click(0, 0);
        f.pagesController.cutSelection();
        QCOMPARE(f.order(), QStringLiteral("234"));
        f.pagesController.goTo(2);
        f.pagesController.paste();
        QCOMPARE(f.order(), QStringLiteral("2314"));
    }

    void duplicateAfterSelection()
    {
        Fixture f(4);
        f.pagesController.click(1, 0);
        f.pagesController.duplicateSelection();
        QCOMPARE(f.document.pageCount(), 5);
        QCOMPARE(f.order(), QStringLiteral("12234"));
        QCOMPARE(f.pagesController.selection(), (QList<int>{2}));
    }

    void deleteRefusesLastPage()
    {
        Fixture f(1);
        f.pagesController.deleteSelection();
        QCOMPARE(f.document.pageCount(), 1);
        QVERIFY(!f.document.canUndo());
        Fixture g(3);
        g.pagesController.click(0, 0);
        g.pagesController.click(2, kShift);
        g.pagesController.deleteSelection(); // toutes les pages : refusé aussi
        QCOMPARE(g.document.pageCount(), 3);
        g.pagesController.cutSelection();
        QCOMPARE(g.document.pageCount(), 3);
        QVERIFY(!g.pagesController.canPaste());
    }

    void oneUndoPerCommand()
    {
        const auto check = [](const char* what, auto action) {
            Fixture f(4);
            const QString before = f.order();
            action(f);
            QVERIFY2(f.order() != before, what);
            f.document.undo();
            QCOMPARE(f.order(), before);
            QVERIFY2(!f.document.canUndo(), what); // une seule étape
            f.document.redo();
            QVERIFY2(f.order() != before, what);
        };
        check("move", [](Fixture& f) {
            f.pagesController.click(2, 0);
            f.pagesController.moveSelection(0);
        });
        check("insert before", [](Fixture& f) { f.pagesController.insertBlank(false); });
        check("insert after", [](Fixture& f) { f.pagesController.insertBlank(true); });
        check("cut", [](Fixture& f) {
            f.pagesController.click(1, 0);
            f.pagesController.click(2, kShift);
            f.pagesController.cutSelection();
        });
        check("paste", [](Fixture& f) {
            f.pagesController.click(0, 0);
            f.pagesController.copySelection();
            f.pagesController.goTo(3);
            f.pagesController.paste();
        });
        check("duplicate", [](Fixture& f) {
            f.pagesController.click(0, 0);
            f.pagesController.click(1, kShift);
            f.pagesController.duplicateSelection();
        });
        check("delete", [](Fixture& f) {
            f.pagesController.click(3, 0);
            f.pagesController.deleteSelection();
        });
    }
};

QTEST_MAIN(TstUiPages)
#include "tst_ui_pages.moc"
