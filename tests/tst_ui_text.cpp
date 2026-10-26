#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/core/pagecache.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pagescontroller.h>
#include <actcraft/ui/textediting.h>

#include <QColor>
#include <QFile>
#include <QImage>
#include <QList>
#include <QMetaType>
#include <QObject>
#include <QPainter>
#include <QPointF>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickTextDocument>
#include <QSignalSpy>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QUrl>
#include <QVariant>

#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

using namespace actcraft;

namespace actcraft::ui {
// Fonctions libres de ui/src/textediting.cpp (amendement 1, bloquant 5 ; L4-C13) : pas d'en-tête public, le
// contrat de L4-T1 reste tel quel. `observer` voit chaque appel de loadResource du document isolé (celui de
// safeTextHtml et de la mesure) et sa réponse.
QString safeTextHtml(const QString& html, const std::function<void(const QUrl&, const QVariant&)>& observer);
std::unique_ptr<QTextDocument>
isolatedTextDocument(std::function<void(const QUrl&, const QVariant&)> observer);
} // namespace actcraft::ui

namespace {

// Réponse de la garde du peintre (render/src/pagepainter.cpp:77-88) : vide mais valide. Une réponse nulle
// laisse Qt se rabattre sur la lecture directe du fichier.
bool emptyButValid(const QVariant& answer)
{
    if (answer.metaType() == QMetaType::fromType<QImage>()) {
        const QImage image = answer.value<QImage>();
        return image.size() == QSize(1, 1) && image.pixelColor(0, 0).alpha() == 0;
    }
    if (answer.metaType() == QMetaType::fromType<QString>()) {
        return answer.toString().isEmpty();
    }
    if (answer.metaType() == QMetaType::fromType<QByteArray>()) {
        return answer.toByteArray().isEmpty();
    }
    return false;
}

int redPixels(const QImage& image)
{
    int reds = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor c = image.pixelColor(x, y);
            reds += c.red() > 200 && c.green() < 80 && c.blue() < 80 ? 1 : 0;
        }
    }
    return reds;
}

// Cache espion (D-S7) : ce que core::applyToCache lui envoie.
class CountingCache final : public core::PageCache
{
public:
    void insert(core::ObjectId) override { ++inserts; }
    void remove(core::ObjectId, const core::RectF&) override { ++removes; }
    void invalidate(core::ObjectId, const core::CacheArea&) override { ++invalidates; }
    void reset() { inserts = removes = invalidates = 0; }
    int total() const { return inserts + removes + invalidates; }
    int inserts = 0;
    int removes = 0;
    int invalidates = 0;
};

QString plainOfHtml(const std::string& html)
{
    QTextDocument document; // HTML écrit par ce test ou assaini par TextEditing : aucune ressource
    document.setHtml(QString::fromStdString(html));
    return document.toPlainText();
}

// Contrôleur, TextEdit QML (son QQuickTextDocument) et TextEditing branchés ; espion de cache par
// addChangeObserver puis core::applyToCache.
struct Harness
{
    QQmlEngine engine;
    std::unique_ptr<QObject> edit;
    ui::DocumentController controller;
    ui::TextEditing editing;
    CountingCache cacheSpy;

    Harness()
    {
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nTextEdit { width: 400; height: 200 }", QUrl());
        edit.reset(component.create());
        Q_ASSERT(edit);
        controller.addChangeObserver(
            [this](const core::Changes& applied, ui::DocumentController::ChangeOrigin) {
                core::applyToCache(applied, static_cast<std::size_t>(controller.currentIndex()), cacheSpy);
            });
        editing.setDocumentController(&controller);
        editing.setTextDocument(quickDocument());
    }

    QQuickTextDocument* quickDocument() const
    {
        return edit->property("textDocument").value<QQuickTextDocument*>();
    }
    QTextDocument* document() const { return quickDocument()->textDocument(); }

    void addText(const QString& plain, const core::RectF& bounds, bool locked = false)
    {
        addHtml(QStringLiteral("<p>%1</p>").arg(plain), bounds, locked);
    }
    void addHtml(const QString& html, const core::RectF& bounds, bool locked = false)
    {
        core::Text text;
        text.bounds = bounds;
        text.html = html.toStdString();
        core::PageObject object = std::move(text);
        object.common.locked = locked;
        controller.execute(std::make_unique<core::AddObjectCommand>(
            static_cast<std::size_t>(controller.currentIndex()), std::move(object)));
    }
    void replaceAllText(const QString& value)
    {
        QTextCursor cursor(document());
        cursor.select(QTextCursor::Document);
        cursor.insertText(value);
    }
    void typeChar(QChar c)
    {
        QTextCursor cursor(document());
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(QString(c));
    }
    QString plainOf(std::size_t page, std::size_t index) const
    {
        const core::Page& p = controller.document().page(page);
        if (index >= p.objects.size()) {
            return QStringLiteral("<absent>");
        }
        const auto* text = std::get_if<core::Text>(&core::body(p.objects[index]));
        return text ? plainOfHtml(text->html).trimmed() : QStringLiteral("<pas un texte>");
    }
    QString htmlPlainOfFirst() const
    {
        return plainOf(static_cast<std::size_t>(controller.currentIndex()), 0);
    }
    qreal firstBlockTopMargin() const { return document()->begin().blockFormat().topMargin(); }
};

bool saveAndWait(ui::DocumentController& controller, const QString& path)
{
    QSignalSpy saved(&controller, &ui::DocumentController::saveFinished);
    controller.saveToUrl(QUrl::fromLocalFile(path));
    return saved.wait(10000) && saved.first().first().toBool();
}

bool openAndWait(ui::DocumentController& controller, const QString& path)
{
    QSignalSpy loaded(&controller, &ui::DocumentController::loadFinished);
    controller.openUrl(QUrl::fromLocalFile(path));
    return loaded.wait(10000) && loaded.first().first().toBool();
}

} // namespace

// Édition du texte (F-250 à F-253, F-100, D7, D8, P17 à P22) : soulèvement, une étape à la validation, texte
// vide, barre de mise en forme, validation avant tout dépôt forcé (editAboutToApply ET pageAboutToChange,
// V6).
class TstUiText : public QObject
{
    Q_OBJECT

private slots:
    // Bloquant 5 (amendements 1 à 3) : le HTML d'un fichier ouvert n'entre jamais tel quel dans le TextEdit.
    // L4-C13 (C9) : le document isolé de l'édition (safeTextHtml, mesure) suit la règle du peintre, qui est
    // interne à render. Il passe donc le crible du test du peintre (tst_render_painter,
    // htmlLoadsNoExternalResource), mêmes variantes : rien n'est lu ni dessiné, même à la densité 2, et le
    // gestionnaire d'images ne demande aucune ressource. Amendé : chaque demande reçoit une réponse vide mais
    // valide, comme au peintre ; l'ancienne garde répondait nul (allNull), et Qt lisait alors le fichier.
    void editingLoadsNoExternalResource()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QImage red(60, 60, QImage::Format_ARGB32);
        red.fill(Qt::red);
        const QString png = dir.filePath(QStringLiteral("rouge.png"));
        QVERIFY(red.save(png, "PNG"));
        QVERIFY(red.save(dir.filePath(QStringLiteral("rouge@2x.png")), "PNG")); // variante de densité 2
        const QString css = dir.filePath(QStringLiteral("style.css"));
        {
            QFile file(css);
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("p { color: #ff0000; background-color: #ff0000; }");
        }
        const QString pngUrl = QUrl::fromLocalFile(png).toString();
        const QString cssUrl = QUrl::fromLocalFile(css).toString();
        // Les trois premières variantes portent une image : c'est le gestionnaire d'images qui répond.
        const QStringList variants = {
            QStringLiteral("<img src=\"") + pngUrl + QStringLiteral("\" width=\"60\" height=\"60\">"),
            QStringLiteral("<img src=\"") + png + QStringLiteral("\" width=\"60\" height=\"60\">"),
            QStringLiteral("<img src=\"") + pngUrl + QStringLiteral("\">"),
            QStringLiteral("<table background=\"") + pngUrl +
                QStringLiteral("\" width=\"200\" height=\"60\"><tr><td>XXXX</td></tr></table>"),
            QStringLiteral("<p style=\"background-image:url(") + pngUrl + QStringLiteral(")\">XXXX XXXX</p>"),
            QStringLiteral("<style>@import url(") + cssUrl + QStringLiteral(");</style><p>XXXX</p>"),
        };
        for (qsizetype v = 0; v < variants.size(); ++v) {
            const QString& variant = variants.at(v);
            QList<QUrl> asked;
            bool answersValid = true;
            const std::unique_ptr<QTextDocument> document =
                ui::isolatedTextDocument([&](const QUrl& url, const QVariant& answer) {
                    asked << url;
                    answersValid = answersValid && emptyButValid(answer);
                });
            document->setHtml(variant);
            document->setTextWidth(300);
            if (v == 2) { // sans dimensions : sa place vient de ses seuls attributs, l'image n'est pas lue
                QVERIFY2(document->size().height() < 60,
                         qPrintable(QString::number(document->size().height())));
            }
            for (const qreal ratio : {1.0, 2.0}) {
                QImage canvas(qRound(320 * ratio), qRound(200 * ratio), QImage::Format_ARGB32_Premultiplied);
                canvas.setDevicePixelRatio(ratio);
                canvas.fill(Qt::white);
                {
                    QPainter painter(&canvas);
                    document->drawContents(&painter);
                }
                const int reds = redPixels(canvas);
                QVERIFY2(reds == 0, qPrintable(variant + QStringLiteral(" : ") + QString::number(reds)));
            }
            QVERIFY2(answersValid, qPrintable(variant));
            if (v < 3) {
                QVERIFY2(asked.isEmpty(),
                         qPrintable(variant + QStringLiteral(" : ") +
                                    (asked.isEmpty() ? QString() : asked.first().toString())));
            }
        }

        const QString html = QStringLiteral(
            "<style>@import url(file://hote/s.css);</style>"
            "<p>visible<img src=\"file://hote/partage/x.png\"><img src=\"http://exemple.invalid/y.png\">"
            "<a href=\"http://exemple.invalid/lien\">lien</a></p>"
            "<table background=\"file://hote/z.png\"><tr><td "
            "background=\"file://hote/w.png\">cellule</td></tr></table>"
            "<p style=\"background-image:url(file://hote/v.png)\">fond</p>");
        const QList<QUrl> inputs{QUrl(QStringLiteral("file://hote/partage/x.png")),
                                 QUrl(QStringLiteral("http://exemple.invalid/y.png")),
                                 QUrl(QStringLiteral("file://hote/z.png")),
                                 QUrl(QStringLiteral("file://hote/w.png")),
                                 QUrl(QStringLiteral("file://hote/v.png")),
                                 QUrl(QStringLiteral("file://hote/s.css"))};
        QList<QUrl> requested;
        bool allValid = true;
        const QString safe = ui::safeTextHtml(html, [&](const QUrl& url, const QVariant& answer) {
            requested << url;
            allValid = allValid && emptyButValid(answer);
        });
        // Une image de fond et une feuille de style sont bien demandées : allValid ne passe pas à vide.
        QVERIFY(requested.contains(QUrl(QStringLiteral("file://hote/z.png"))));
        QVERIFY(requested.contains(QUrl(QStringLiteral("file://hote/s.css"))));
        QVERIFY(allValid);
        for (const QUrl& url : requested) {
            QVERIFY2(inputs.contains(url), qPrintable(url.toString()));
        }
        QVERIFY(!safe.contains(QStringLiteral("file:")));
        QVERIFY(!safe.contains(QStringLiteral("exemple.invalid")));
        QVERIFY(!safe.contains(QStringLiteral("hote/")));

        Harness h;
        h.addHtml(html, {200, 200, 300, 120});
        QVERIFY(h.editing.beginAt({210, 210}, false));
        const QString loaded = h.document()->toHtml();
        QVERIFY(!loaded.contains(QStringLiteral("file:")));
        QVERIFY(!loaded.contains(QStringLiteral("exemple.invalid")));
        QVERIFY(!loaded.contains(QStringLiteral("hote/")));
        const QString plain = h.editing.plainText();
        QVERIFY(plain.contains(QStringLiteral("visible")));
        QVERIFY(plain.contains(QStringLiteral("lien")));
        // Q4a2-m7 : mots témoins distincts (« visible » contenait déjà le « b » d'avant).
        QVERIFY(plain.contains(QStringLiteral("cellule")));
        QVERIFY(plain.contains(QStringLiteral("fond")));
    }

    // Enregistrer pendant l'édition (editAboutToApply) : le fichier relu contient le texte frappé.
    void saveDuringEditCommitsFirst()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("texte.actcraft"));
        Harness h;
        QVERIFY(h.editing.beginAt({200, 200}, false));
        h.replaceAllText(QStringLiteral("Bonjour"));
        QVERIFY(saveAndWait(h.controller, path));
        QVERIFY(!h.editing.isActive());
        ui::DocumentController reread;
        QVERIFY(openAndWait(reread, path));
        QCOMPARE(reread.shownObjectCount(), 1);
        const auto* text = std::get_if<core::Text>(&core::body(reread.document().page(0).objects[0]));
        QVERIFY(text);
        QCOMPARE(plainOfHtml(text->html).trimmed(), QStringLiteral("Bonjour"));
    }

    // Annuler pendant l'édition (editAboutToApply) : la frappe est validée (une étape), puis c'est elle qui
    // est annulée ; Rétablir la rend.
    void undoDuringEditCommitsFirst()
    {
        Harness h;
        h.addText(QStringLiteral("un"), {200, 200, 120, 30});
        QVERIFY(h.editing.beginAt({210, 210}, false));
        h.replaceAllText(QStringLiteral("deux"));
        h.controller.undo();
        QVERIFY(!h.editing.isActive());
        QVERIFY(!h.controller.isLifting());
        QCOMPARE(h.controller.shownObjectCount(), 1);
        QCOMPARE(h.htmlPlainOfFirst(), QStringLiteral("un"));
        QVERIFY(h.controller.canRedo());
        h.controller.redo();
        QCOMPARE(h.htmlPlainOfFirst(), QStringLiteral("deux"));
    }

    // V6 : changer de page émet pageAboutToChange, PAS editAboutToApply. La page quittée garde le texte
    // frappé, texte existant soulevé comme texte neuf ; rien ne reste soulevé ni en édition.
    void pageChangeDuringEditCommitsFirst()
    {
        Harness h;
        h.controller.nextPage();
        h.controller.showPage(0);
        h.addText(QStringLiteral("un"), {200, 200, 120, 30});
        QSignalSpy edits(&h.controller, &ui::DocumentController::editAboutToApply);
        QVERIFY(h.editing.beginAt({210, 210}, false));
        h.replaceAllText(QStringLiteral("deux"));
        h.controller.showPage(1);
        QCOMPARE(edits.count(), 0); // le chemin qu'on garde : aucun editAboutToApply
        QCOMPARE(h.controller.currentIndex(), 1);
        QVERIFY(!h.editing.isActive());
        QVERIFY(!h.controller.isLifting());
        QCOMPARE(h.controller.document().page(0).objects.size(), std::size_t{1});
        QCOMPARE(h.plainOf(0, 0), QStringLiteral("deux"));

        QVERIFY(h.editing.beginAt({300, 300}, false)); // texte neuf sur la page 1
        h.replaceAllText(QStringLiteral("Bonjour"));
        h.controller.pageBack();
        QCOMPARE(h.controller.currentIndex(), 0);
        QVERIFY(!h.editing.isActive());
        QCOMPARE(h.controller.document().page(1).objects.size(), std::size_t{1});
        QCOMPARE(h.plainOf(1, 0), QStringLiteral("Bonjour"));
        QCOMPARE(edits.count(), 0);
    }

    void newTextStartsWithPlaceholderSelected()
    {
        Harness h;
        QVERIFY(h.editing.beginAt({200, 200}, false));
        QVERIFY(h.editing.isActive());
        QCOMPARE(h.editing.plainText(), QStringLiteral("Votre texte"));
        QCOMPARE(h.editing.selectionStart(), 0);
        QCOMPARE(h.editing.selectionEnd(), h.editing.plainText().size());
        QCOMPARE(h.controller.shownObjectCount(), 0);
        QCOMPARE(h.editing.fontSize(), 14.0); // D7
        QCOMPARE(h.editing.frame().topLeft(), QPointF(200, 200));
    }

    void typingThenCommitAddsOneObject()
    {
        Harness h;
        h.editing.beginAt({200, 200}, false);
        h.replaceAllText(QStringLiteral("Bonjour"));
        h.editing.commit();
        QCOMPARE(h.controller.shownObjectCount(), 1);
        QVERIFY(!h.editing.isActive());
        QCOMPARE(h.htmlPlainOfFirst(), QStringLiteral("Bonjour"));
        const core::PageObject& object = h.controller.document().page(0).objects[0];
        QCOMPARE(object.common.layer, core::Layer::Middle); // F-253
        const auto* text = std::get_if<core::Text>(&core::body(object));
        QVERIFY(text);
        QCOMPARE(text->bounds.x, 200.0);
        QCOMPARE(text->bounds.y, 200.0);
        QVERIFY(text->bounds.width > 0);
        QVERIFY(text->bounds.height > 0);
    }

    // D7 : texte neuf laissé vide oublié sans étape ; texte existant vidé supprimé en une étape.
    void emptyTextIsNotKept()
    {
        Harness h;
        h.editing.beginAt({200, 200}, false);
        h.replaceAllText(QString());
        h.editing.commit();
        QCOMPARE(h.controller.shownObjectCount(), 0);
        QVERIFY(!h.controller.canUndo());
        h.addText(QStringLiteral("un"), {200, 200, 120, 30});
        QVERIFY(h.editing.beginAt({210, 210}, false));
        h.replaceAllText(QStringLiteral("  "));
        h.editing.commit();
        QCOMPARE(h.controller.shownObjectCount(), 0);
        QVERIFY(!h.controller.isLifting());
        h.controller.undo();
        QCOMPARE(h.controller.shownObjectCount(), 1);
        QCOMPARE(h.htmlPlainOfFirst(), QStringLiteral("un"));
    }

    // Le patron évident (une commande par frappe) est le mauvais : un retrait au soulèvement, rien pendant la
    // frappe, une insertion à la validation.
    void editingExistingLiftsIt()
    {
        Harness h;
        h.addText(QStringLiteral("un"), {200, 200, 120, 30});
        h.cacheSpy.reset();
        QVERIFY(h.editing.beginAt({210, 210}, false));
        QVERIFY(h.controller.isLifting());
        QCOMPARE(h.cacheSpy.removes, 1);
        for (int i = 0; i < 20; ++i) {
            h.typeChar(QLatin1Char('x'));
        }
        // Rien pendant la frappe. Q4a2-m8 : la comparaison de canUndo() avant et après est retirée, elle
        // valait toujours vrai (l'AddObjectCommand de addText précède) ; l'espion de cache porte le test.
        QCOMPARE(h.cacheSpy.total(), 1);
        h.editing.commit();
        QCOMPARE(h.cacheSpy.inserts, 1);
        QVERIFY(!h.controller.isLifting());
    }

    void commitMakesASingleStep()
    {
        Harness h;
        h.addText(QStringLiteral("un"), {200, 200, 120, 30});
        h.editing.beginAt({210, 210}, false);
        h.replaceAllText(QStringLiteral("deux"));
        h.editing.commit();
        QCOMPARE(h.htmlPlainOfFirst(), QStringLiteral("deux"));
        h.controller.undo();
        QCOMPARE(h.htmlPlainOfFirst(), QStringLiteral("un"));
        h.controller.undo();
        QCOMPARE(h.controller.shownObjectCount(), 0); // l'étape suivante est l'ajout du texte
    }

    void boldAppliesToSelection()
    {
        Harness h;
        h.editing.beginAt({200, 200}, false); // tout sélectionné
        QVERIFY(!h.editing.bold());
        h.editing.apply(QStringLiteral("text.bold"), {});
        QVERIFY(h.editing.bold());
        h.editing.apply(QStringLiteral("text.bold"), {});
        QVERIFY(!h.editing.bold());
    }

    void sizeUpStepsByTwo()
    {
        Harness h;
        h.editing.beginAt({200, 200}, false);
        const qreal before = h.editing.fontSize();
        h.editing.apply(QStringLiteral("text.size-up"), {});
        QCOMPARE(h.editing.fontSize(), before + 2);
    }

    void sizeDownStopsAtEight() // amendement 4 (D8)
    {
        Harness h;
        h.editing.beginAt({200, 200}, false); // 14 points (D7)
        for (int i = 0; i < 5; ++i) {
            h.editing.apply(QStringLiteral("text.size-down"), {});
        }
        QCOMPARE(h.editing.fontSize(), 8.0);
    }

    void spacingAddsTwoPointsAbove() // amendement 4 (D8)
    {
        Harness h;
        h.editing.beginAt({200, 200}, false);
        const qreal before = h.firstBlockTopMargin();
        h.editing.apply(QStringLiteral("text.spacing-up"), {});
        QCOMPARE(h.firstBlockTopMargin(), before + 2);
        for (int i = 0; i < 3; ++i) {
            h.editing.apply(QStringLiteral("text.spacing-down"), {});
        }
        QCOMPARE(h.firstBlockTopMargin(), 0.0);
    }

    void lockedTextIsNotEdited() // amendement 4 (P17)
    {
        Harness h;
        h.addText(QStringLiteral("un"), {200, 200, 120, 30}, /*locked=*/true);
        QVERIFY(h.editing.beginAt({210, 210}, false));
        QCOMPARE(h.editing.plainText(), QStringLiteral("Votre texte")); // texte neuf, par-dessus
        QVERIFY(!h.controller.isLifting());
        QCOMPARE(h.controller.shownObjectCount(), 1);
    }

    // Texte du lot 7 : son API d'insertion n'a pas fusionné (D-S8) ; un core::Text ordinaire posé par
    // execute(AddObjectCommand), placé par sa transformation comme un objet inséré.
    void textOfLot7IsEditable()
    {
        Harness h;
        core::Text text;
        text.bounds = core::RectF{0, 0, 80, 40};
        text.html = "<p>12</p>";
        core::PageObject object = std::move(text);
        object.common.transform.dx = 300;
        object.common.transform.dy = 300;
        h.controller.execute(std::make_unique<core::AddObjectCommand>(0, std::move(object)));
        QVERIFY(h.editing.beginAt({310, 310}, false));
        QCOMPARE(h.editing.plainText(), QStringLiteral("12"));
        QVERIFY(h.controller.isLifting());
        h.replaceAllText(QStringLiteral("13"));
        h.editing.commit();
        QCOMPARE(h.htmlPlainOfFirst(), QStringLiteral("13"));
        QCOMPARE(h.controller.document().page(0).objects[0].common.transform.dx, 300.0);
    }

    // Annuler l'édition (pas Échap, qui valide : D7) : le texte soulevé revient tel qu'il était, sans étape.
    void cancelRestoresLifted()
    {
        Harness h;
        h.addText(QStringLiteral("un"), {200, 200, 120, 30});
        h.editing.beginAt({210, 210}, false);
        h.replaceAllText(QStringLiteral("deux"));
        h.editing.cancel();
        QVERIFY(!h.editing.isActive());
        QCOMPARE(h.controller.shownObjectCount(), 1);
        QVERIFY(!h.controller.isLifting());
        QCOMPARE(h.htmlPlainOfFirst(), QStringLiteral("un"));
        h.controller.undo();
        QCOMPARE(h.controller.shownObjectCount(), 0); // seule étape : l'ajout
    }

    // L4-C2 : une commande de pages lancée pendant l'édition valide d'abord le texte (comme Annuler).
    // Dupliquer la page affichée : la copie porte le texte frappé ; sans validation, le texte soulevé
    // manquait à la copie.
    void duplicateDuringEditKeepsTheText()
    {
        Harness h;
        ui::PagesController pages;
        pages.setDocumentController(&h.controller);
        h.addText(QStringLiteral("un"), {200, 200, 120, 30});
        QVERIFY(h.editing.beginAt({210, 210}, false));
        h.replaceAllText(QStringLiteral("deux"));
        QVERIFY(h.controller.isLifting());
        pages.duplicateSelection(); // sélection = {0}, la page affichée
        QVERIFY(!h.editing.isActive());
        QVERIFY(!h.controller.isLifting());
        QCOMPARE(h.controller.pageCount(), 2);
        QCOMPARE(h.plainOf(0, 0), QStringLiteral("deux"));
        QCOMPARE(h.plainOf(1, 0), QStringLiteral("deux"));
    }

    // L4-C2 : couper la page affichée pendant l'édition : le presse-papiers reçoit la page avec le texte
    // frappé ; collée, elle le porte. cutSelection lit les pages lui-même, sans passer par copySelection :
    // la validation doit précéder cette lecture, celle de deleteSelection vient trop tard.
    void cutDuringEditKeepsTheText()
    {
        Harness h;
        h.controller.nextPage();
        h.controller.showPage(0);
        ui::PagesController pages;
        pages.setDocumentController(&h.controller);
        h.addText(QStringLiteral("un"), {200, 200, 120, 30});
        QVERIFY(h.editing.beginAt({210, 210}, false));
        h.replaceAllText(QStringLiteral("deux"));
        pages.cutSelection(); // sélection = {0}
        QVERIFY(!h.editing.isActive());
        QVERIFY(!h.controller.isLifting());
        QCOMPARE(h.controller.pageCount(), 1);
        QVERIFY(pages.canPaste());
        pages.paste(); // avant la page active : la page collée est la page 0
        QCOMPARE(h.controller.pageCount(), 2);
        QCOMPARE(h.plainOf(0, 0), QStringLiteral("deux"));
    }

    // L4-C2 : supprimer la page affichée pendant l'édition valide le texte (une étape), puis retire la page
    // (une étape) : deux Annuler rendent la page, puis son texte d'avant.
    void deleteDuringEditCommitsFirst()
    {
        Harness h;
        h.controller.nextPage();
        h.controller.showPage(0);
        ui::PagesController pages;
        pages.setDocumentController(&h.controller);
        h.addText(QStringLiteral("un"), {200, 200, 120, 30});
        QVERIFY(h.editing.beginAt({210, 210}, false));
        h.replaceAllText(QStringLiteral("deux"));
        pages.deleteSelection(); // sélection = {0}
        QVERIFY(!h.editing.isActive());
        QVERIFY(!h.controller.isLifting());
        QCOMPARE(h.controller.pageCount(), 1);
        h.controller.undo(); // la page revient, avec le texte validé
        QCOMPARE(h.controller.pageCount(), 2);
        QCOMPARE(h.plainOf(0, 0), QStringLiteral("deux"));
        h.controller.undo(); // l'édition est sa propre étape
        QCOMPARE(h.plainOf(0, 0), QStringLiteral("un"));
    }
};

QTEST_MAIN(TstUiText)
#include "tst_ui_text.moc"
