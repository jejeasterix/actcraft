#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/core/pagecache.h>
#include <actcraft/render/pagepainter.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pagescontroller.h>
#include <actcraft/ui/textediting.h>

#include <QClipboard>
#include <QColor>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QFont>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QImage>
#include <QInputMethodEvent>
#include <QList>
#include <QMetaType>
#include <QMimeData>
#include <QObject>
#include <QPainter>
#include <QPointF>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickTextDocument>
#include <QQuickWindow>
#include <QRegularExpression>
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
#include <QTextLayout>
#include <QTextOption>
#include <QUrl>
#include <QVariant>
#include <QtQml/qqmlextensionplugin.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

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

// Lignes mises en page d'un document (lineCount() de QTextDocument est le nombre de blocs).
int laidOutLines(QTextDocument& document)
{
    document.size(); // la mise en page se fait ici
    int lines = 0;
    for (QTextBlock block = document.begin(); block.isValid(); block = block.next()) {
        lines += block.layout()->lineCount();
    }
    return lines;
}

// Ligne mise en page (blocs confondus) où tombe une position du document.
int lineOf(QTextDocument& document, int position)
{
    document.size(); // la mise en page se fait ici
    int line = 0;
    for (QTextBlock block = document.begin(); block.isValid(); block = block.next()) {
        if (block.contains(position) || !block.next().isValid()) {
            const QTextLine at = block.layout()->lineForTextPosition(position - block.position());
            return at.isValid() ? line + at.lineNumber() : -1;
        }
        line += block.layout()->lineCount();
    }
    return -1;
}

// La colonne montrée d'un texte de haut en bas, comme dans TextEditor.qml (L4-C17).
std::unique_ptr<QObject> columnTextEdit(QQmlEngine& engine)
{
    QQmlComponent component(&engine);
    component.setData("import QtQuick\nTextEdit { readOnly: true; wrapMode: TextEdit.WrapAnywhere; "
                      "horizontalAlignment: TextEdit.AlignHCenter }",
                      QUrl());
    return std::unique_ptr<QObject>(component.create());
}

QTextDocument* documentOf(const QObject& textEdit)
{
    return textEdit.property("textDocument").value<QQuickTextDocument*>()->textDocument();
}

// Ligne k mise en page du premier bloc, en coordonnées du document.
QRectF lineRect(QTextDocument& document, int k)
{
    document.size(); // la mise en page se fait ici
    const QTextBlock block = document.firstBlock();
    const QTextLine line = block.layout()->lineAt(k);
    const QPointF at = block.layout()->position();
    return QRectF(at.x() + line.x(), at.y() + line.y(), line.width(), line.height());
}

// Le vrai TextEditor.qml (module ActCraft.Ui) dans une fenêtre hors écran, branché à un TextEditing (L4-C17).
struct EditorHarness
{
    QQmlEngine engine;
    ui::DocumentController controller;
    ui::TextEditing editing;
    QQuickWindow window;
    std::unique_ptr<QQuickItem> editor; // détruit le premier : avant la fenêtre et TextEditing

    EditorHarness()
    {
        editing.setDocumentController(&controller);
        QQmlComponent component(&engine);
        component.loadFromModule("ActCraft.Ui", "TextEditor");
        if (component.isError()) {
            qWarning() << component.errors();
            return;
        }
        editor.reset(qobject_cast<QQuickItem*>(component.createWithInitialProperties(
            {{QStringLiteral("editing"), QVariant::fromValue(static_cast<QObject*>(&editing))}})));
        if (editor) {
            editor->setParentItem(window.contentItem());
            editor->setSize(QSizeF(400, 400));
        }
        window.resize(400, 400);
    }

    QQuickItem* item(const char* name) const
    {
        return editor ? editor->findChild<QQuickItem*>(QLatin1String(name)) : nullptr;
    }

    // Fenêtre active, texte de haut en bas commencé, « A B » tapé au clavier.
    bool typeVertical()
    {
        window.show();
        window.requestActivate();
        if (!QTest::qWaitForWindowActive(&window) || !editing.beginAt({100, 100}, true)) {
            return false;
        }
        QQuickItem* input = item("textEdit");
        if (!input || !QTest::qWaitFor([input] { return input->hasActiveFocus(); })) {
            return false;
        }
        for (const char key : {'A', ' ', 'B'}) {
            QTest::keyClick(&window, key); // keyClicks est réservé aux QWidget
        }
        return editing.plainText() == QStringLiteral("A B");
    }
};

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
    void pasteIsPlainTextInTheEditor()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData(
            "import QtQuick\nTextEdit { textFormat: TextEdit.PlainText; width: 400; height: 200 }", QUrl());
        const std::unique_ptr<QObject> edit(component.create());
        QVERIFY(edit);
        auto* quick = edit->property("textDocument").value<QQuickTextDocument*>();
        QVERIFY(quick);
        QTextDocument* document = quick->textDocument();
        auto* mime = new QMimeData;
        mime->setHtml(QStringLiteral("<p>collé</p><img src=\"file://hote/partage/x.png\"><a "
                                     "href=\"http://exemple.invalid/\">lien</a>"));
        mime->setText(QStringLiteral("collé"));
        QGuiApplication::clipboard()->setMimeData(mime);
        QVERIFY(QMetaObject::invokeMethod(edit.get(), "paste"));
        QCOMPARE(document->toPlainText(), QStringLiteral("collé"));
        const QString html = document->toHtml();
        QVERIFY(!html.contains(QLatin1String("<img")));
        QVERIFY(!html.contains(QLatin1String("hote/")));
        QVERIFY(!html.contains(QLatin1String("exemple.invalid")));
    }

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

    // fixedWidth (L4-T23) : l'éditeur QML ne replie le texte qu'à une largeur imposée. Un texte neuf n'en a
    // pas : le cadre suit idealWidth, sans passage à la ligne (sinon idealWidth s'effondre sur un document
    // replié, le cadre aussi, et le texte devient une colonne d'une lettre).
    void fixedWidthOnlyWhenImposed()
    {
        Harness h;
        QVERIFY(h.editing.beginAt({100, 100}, false));
        QVERIFY(!h.editing.hasFixedWidth());
        for (int i = 0; i < 40; ++i) {
            h.typeChar(QLatin1Char('m'));
        }
        QVERIFY(!h.editing.hasFixedWidth());
        QCOMPARE(h.document()->blockCount(), 1);
        QVERIFY(h.editing.frame().width() >= h.document()->idealWidth());
        QVERIFY(h.editing.frame().width() > 100); // quarante lettres : bien plus large qu'une colonne
        // Poignée d'extension : la largeur est imposée, le cadre la prend.
        h.editing.setFrameWidth(60);
        QVERIFY(h.editing.hasFixedWidth());
        QCOMPARE(h.editing.frame().width(), 60.0);
        h.editing.commit();
        QVERIFY(!h.editing.hasFixedWidth());
        // Texte existant : sa largeur est celle de son emprise.
        h.addText(QStringLiteral("un"), {300, 300, 120, 30});
        QVERIFY(h.editing.beginAt({310, 310}, false));
        QVERIFY(h.editing.hasFixedWidth());
        QCOMPARE(h.editing.frame().width(), 120.0);
        h.editing.commit();
        QVERIFY(!h.editing.hasFixedWidth());
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

    // --- Texte de haut en bas II (L4-T25, H4-3, D9)

    // Avance du « W » à la police par défaut d'un texte neuf (famille de l'application, 14 points).
    static qreal widestLetterAdvance()
    {
        QFont font(QGuiApplication::font().family());
        font.setPointSizeF(14);
        return QFontMetricsF(font).horizontalAdvance(QLatin1Char('W'));
    }

    // Le cadre d'un texte de haut en bas est une colonne d'un caractère (largeur de L4-T24), pas la largeur
    // du texte écrit à plat.
    void verticalFrameIsOneColumn()
    {
        Harness h;
        QVERIFY(h.editing.beginAt({200, 200}, true));
        QVERIFY(h.editing.isVertical());
        const qreal wide = widestLetterAdvance();
        QVERIFY2(h.editing.frame().width() > 0, "cadre sans largeur");
        QVERIFY2(h.editing.frame().width() < 1.5 * wide,
                 qPrintable(
                     QStringLiteral("largeur %1, avance de W %2").arg(h.editing.frame().width()).arg(wide)));
        // La même frappe à plat reste large : la colonne vient bien du mode.
        h.editing.commit();
        QVERIFY(h.editing.beginAt({400, 400}, false));
        QVERIFY(h.editing.frame().width() > 3 * wide);
    }

    // La largeur d'un texte de haut en bas ne se tire pas à la poignée (elle est masquée) et reste celle de
    // la colonne même pour un texte rouvert (largeur imposée de l'objet).
    void verticalFrameKeepsItsWidth()
    {
        Harness h;
        QVERIFY(h.editing.beginAt({200, 200}, true));
        const qreal column = h.editing.frame().width();
        h.editing.setFrameWidth(300);
        QCOMPARE(h.editing.frame().width(), column);
        QVERIFY(!h.editing.hasFixedWidth()); // la poignée masquée n'impose rien
        h.replaceAllText(QStringLiteral("ABC"));
        h.editing.commit();
        QCOMPARE(h.controller.shownObjectCount(), 1);
        QVERIFY(
            h.editing.beginAt({201, 201}, false)); // rouvert : le drapeau de l'objet commande, pas l'appel
        QVERIFY(h.editing.isVertical());
        QVERIFY(h.editing.frame().width() < 1.5 * widestLetterAdvance());
    }

    // Validé, l'objet porte `vertical` et l'emprise d'une colonne : sa largeur est celle de la colonne, sa
    // hauteur celle des lettres empilées (une par ligne), non celle du mot à plat.
    void verticalCommitKeepsTheFlagAndTheColumn()
    {
        Harness h;
        QVERIFY(h.editing.beginAt({200, 200}, true));
        h.replaceAllText(QStringLiteral("ABC"));
        h.editing.commit();
        const core::Page& page = h.controller.document().page(0);
        QCOMPARE(page.objects.size(), std::size_t{1});
        const auto* text = std::get_if<core::Text>(&core::body(page.objects[0]));
        QVERIFY(text);
        QVERIFY(text->vertical);
        QVERIFY2(text->bounds.width < 1.5 * widestLetterAdvance(),
                 qPrintable(QString::number(text->bounds.width)));
        QFont font(QGuiApplication::font().family());
        font.setPointSizeF(14);
        QVERIFY2(text->bounds.height > 2.5 * QFontMetricsF(font).height(),
                 qPrintable(QString::number(text->bounds.height)));
    }

    // La barre du texte de haut en bas : police, taille, gras, italique, souligné (F-301 b), cinq boutons
    // nommés.
    void verticalBarOffersFiveCommands()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nimport ActCraft.Ui\nItem {\n"
                          "  width: 900; height: 200\n"
                          "  property QtObject bar: b\n"
                          "  Commands { id: cmd }\n"
                          "  VerticalTextBar { id: b; objectName: \"verticalTextBar\"; commands: cmd }\n"
                          "}\n",
                          QUrl());
        std::unique_ptr<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        QObject* bar = root->property("bar").value<QObject*>();
        QVERIFY(bar);
        for (const char* name : {"vbar-font", "vbar-size", "vbar-bold", "vbar-italic", "vbar-underline"}) {
            QVERIFY2(bar->findChild<QObject*>(QString::fromLatin1(name)) != nullptr, name);
        }
        // Chaque bouton déclenche la commande qui porte son nom.
        const QList<std::pair<const char*, const char*>> commandOf{
            {"vbar-bold", "text.bold"}, {"vbar-italic", "text.italic"}, {"vbar-underline", "text.underline"}};
        for (const auto& [name, id] : commandOf) {
            QObject* button = bar->findChild<QObject*>(QString::fromLatin1(name));
            QCOMPARE(button->property("commandId").toString(), QString::fromLatin1(id));
        }
        // Visible seulement pour un texte de haut en bas en cours d'édition.
        QCOMPARE(bar->property("visible").toBool(), false);
    }

    // Une lettre par ligne pendant la frappe, comme le résultat validé (« Wii » : W large, deux i étroits ;
    // « iWi », « lWl »). Un espace reste hors de ce contrat pour le TextEdit d'entrée : Qt le laisse pendre
    // au bout de la ligne de la lettre précédente ; la colonne montrée le met sur sa ligne (L4-C17,
    // verticalEditShowsSpacesLikeTheResult). Le TextEdit du test prend la largeur de repli donnée par
    // TextEditing, comme l'éditeur QML.
    void verticalEditShowsOneLetterPerLine()
    {
        for (const QString& word : {QStringLiteral("Wii"), QStringLiteral("iWi"), QStringLiteral("lWl")}) {
            Harness h;
            QVERIFY(h.editing.beginAt({200, 200}, true));
            h.replaceAllText(word);
            QVERIFY(h.edit->setProperty("wrapMode", 3)); // TextEdit.WrapAnywhere
            QVERIFY(h.edit->setProperty("width", h.editing.wrapWidth()));
            QVERIFY2(h.editing.wrapWidth() < h.editing.frame().width(), qPrintable(word));
            // Le document du TextEdit garde l'option des espaces de fin après le réglage du repli ; un
            // document de même option et de même HTML, à la largeur de repli, a trois lignes de la hauteur du
            // cadre.
            QTextDocument shown;
            shown.setDefaultFont(h.document()->defaultFont());
            QTextOption option = h.document()->defaultTextOption();
            option.setWrapMode(QTextOption::WrapAnywhere); // le repli du TextEdit QML (wrapMode)
            shown.setDefaultTextOption(option);
            shown.setDocumentMargin(h.document()->documentMargin());
            shown.setHtml(h.document()->toHtml());
            shown.setTextWidth(h.editing.wrapWidth());
            const qreal shownHeight = shown.size().height(); // la mise en page se fait ici, avant de compter
            int lines =
                0; // lineCount() de QTextDocument est le nombre de blocs : on compte les lignes mises en page
            for (QTextBlock block = shown.begin(); block.isValid(); block = block.next()) {
                lines += block.layout()->lineCount();
            }
            QVERIFY2(lines == 3, qPrintable(QStringLiteral("%1 : %2 lignes").arg(word).arg(lines)));
            const qreal frameHeight = h.editing.frame().height();
            QVERIFY2(
                qAbs(shownHeight - frameHeight) < 1.0,
                qPrintable(
                    QStringLiteral("%1 : repli %2, cadre %3").arg(word).arg(shownHeight).arg(frameHeight)));
            h.editing.commit();
            const auto* text =
                std::get_if<core::Text>(&core::body(h.controller.document().page(0).objects[0]));
            QVERIFY(text);
            QCOMPARE(text->bounds.height, std::ceil(frameHeight));
        }
    }

    // L4-C17 : pendant la frappe, la colonne montrée est le résultat validé, espaces compris (Qt laisse un
    // espace pendre au bout de la ligne de la lettre précédente : le TextEdit d'entrée ne peut pas le
    // montrer). Le document édité n'est pas touché par l'affichage : texte brut, curseur, annulation, texte
    // enregistré.
    void verticalEditShowsSpacesLikeTheResult()
    {
        for (const QString& word : {QStringLiteral("A B"), QStringLiteral("A  B"), QStringLiteral("AB"),
                                    QStringLiteral(" AB"), QStringLiteral("AB ")}) {
            Harness h;
            std::unique_ptr<QObject> column = columnTextEdit(h.engine);
            QVERIFY(column);
            h.editing.setColumnDocument(column->property("textDocument").value<QQuickTextDocument*>());
            QVERIFY(h.editing.beginAt({200, 200}, true));
            h.replaceAllText(word);
            // Qt fusionne une insertion contiguë à la dernière insertion d'un bloc d'édition fermé
            // (QTextDocumentPrivate::appendUndoItem) : sans cette remise à zéro, annuler « C » annulerait le
            // mot.
            h.document()->clearUndoRedoStacks();
            QVERIFY(h.edit->setProperty("cursorPosition", 1));
            h.typeChar(QLatin1Char('C')); // après le curseur du TextEdit, qui ne bouge pas
            const QString typed = word + QLatin1Char('C');
            // Le résultat validé : la colonne du peintre, une ligne par graphème, espaces compris.
            QTextDocument result;
            result.setDocumentMargin(0);
            result.setHtml(h.document()->toHtml());
            render::layOutVerticalColumn(result);
            QCOMPARE(laidOutLines(result), typed.size());
            // Montré pendant la frappe : autant de lignes, la hauteur du cadre.
            QVERIFY(column->setProperty("width", h.editing.frame().width())); // comme TextEditor.qml
            QTextDocument* shown = documentOf(*column);
            const int shownLines = laidOutLines(*shown);
            QVERIFY2(shownLines == typed.size(),
                     qPrintable(QStringLiteral("[%1] : %2 lignes montrées, %3 validées")
                                    .arg(typed)
                                    .arg(shownLines)
                                    .arg(typed.size())));
            QVERIFY2(qAbs(shown->size().height() - h.editing.frame().height()) < 1.0,
                     qPrintable(QStringLiteral("[%1] : montré %2, cadre %3")
                                    .arg(typed)
                                    .arg(shown->size().height())
                                    .arg(h.editing.frame().height())));
            // Le curseur montré : au bout de la ligne du graphème précédent, ou au début de celle du suivant.
            for (int at = 0; at <= typed.size(); ++at) {
                if (at > 0) {
                    QCOMPARE(lineOf(*shown, h.editing.columnPosition(at, false)), at - 1);
                }
                if (at < typed.size()) {
                    QCOMPARE(lineOf(*shown, h.editing.columnPosition(at, true)), at);
                }
            }
            // Le document édité : sans séparateur ajouté, curseur en place, la frappe s'annule seule. La mise
            // en page du peintre, appliquée à lui, y mettrait des séparateurs et lui ôterait son annulation
            // (render/src/pagepainter.cpp, layOutColumn : setUndoRedoEnabled(false)).
            QCOMPARE(h.document()->toPlainText(), typed);
            QCOMPARE(h.editing.plainText(), typed);
            QVERIFY(!h.document()->toRawText().contains(QChar(QChar::LineSeparator)));
            QCOMPARE(h.document()->availableUndoSteps(), 1); // la frappe, rien de l'affichage
            QCOMPARE(h.edit->property("cursorPosition").toInt(), 1);
            h.document()->undo();
            QCOMPARE(h.document()->toPlainText(), word);
            h.document()->redo();
            QCOMPARE(h.document()->toPlainText(), typed);
            h.editing.commit();
            const auto* text =
                std::get_if<core::Text>(&core::body(h.controller.document().page(0).objects[0]));
            QVERIFY(text);
            QTextDocument saved;
            saved.setHtml(QString::fromStdString(text->html));
            QCOMPARE(saved.toPlainText(), typed);
        }
    }

    // L4-C17 : le curseur et la sélection du texte édité se placent dans la colonne montrée, où « A B » est
    // « A⏎ ⏎B » (deux séparateurs de l'affichage) : au bout de la ligne de la lettre précédente, ou au début
    // de la ligne suivante. Un séparateur tapé (Maj+Entrée) est au texte : rien n'est ajouté à côté. À plat :
    // rien.
    void verticalColumnPositionsFollowTheLetters()
    {
        Harness h;
        std::unique_ptr<QObject> column = columnTextEdit(h.engine);
        QVERIFY(column);
        h.editing.setColumnDocument(column->property("textDocument").value<QQuickTextDocument*>());
        QVERIFY(h.editing.beginAt({200, 200}, true));
        h.replaceAllText(QStringLiteral("A B"));
        QCOMPARE(documentOf(*column)->characterCount(), 6); // A, séparateur, espace, séparateur, B, fin
        for (const auto& [edit, shown] : QList<std::pair<int, int>>{{0, 0}, {1, 1}, {2, 3}, {3, 5}}) {
            QCOMPARE(h.editing.columnPosition(edit, false), shown);
        }
        QCOMPARE(h.editing.columnPosition(0, true), 0);
        QCOMPARE(h.editing.columnPosition(1, true), 2);
        QCOMPARE(h.editing.columnPosition(2, true), 4);
        for (const auto& [shown, edit] :
             QList<std::pair<int, int>>{{0, 0}, {1, 1}, {2, 1}, {3, 2}, {4, 2}, {5, 3}}) {
            QCOMPARE(h.editing.editPosition(shown), edit);
        }
        h.replaceAllText(QStringLiteral("A") + QChar(QChar::LineSeparator) + QStringLiteral("B"));
        QCOMPARE(h.editing.columnPosition(2, false), 2);
        QCOMPARE(h.editing.editPosition(3), 3);

        Harness flat;
        std::unique_ptr<QObject> unused = columnTextEdit(flat.engine);
        QVERIFY(unused);
        flat.editing.setColumnDocument(unused->property("textDocument").value<QQuickTextDocument*>());
        QVERIFY(flat.editing.beginAt({200, 200}, false));
        flat.replaceAllText(QStringLiteral("A B"));
        QVERIFY(documentOf(*unused)->isEmpty());
        QCOMPARE(flat.editing.columnPosition(2, false), 2);
    }

    // L4-C17 : l'édition finie, validée ou abandonnée, la colonne montrée est vidée et ses positions sont
    // celles du texte : l'édition suivante, à plat, ne garde rien du texte de haut en bas.
    void verticalColumnIsEmptiedWhenEditingEnds()
    {
        for (const bool validate : {true, false}) {
            Harness h;
            std::unique_ptr<QObject> column = columnTextEdit(h.engine);
            QVERIFY(column);
            h.editing.setColumnDocument(column->property("textDocument").value<QQuickTextDocument*>());
            QVERIFY(h.editing.beginAt({200, 200}, true));
            h.replaceAllText(QStringLiteral("A B"));
            QVERIFY(!documentOf(*column)->isEmpty());
            QCOMPARE(h.editing.columnPosition(2, false), 3);
            if (validate) {
                h.editing.commit();
            } else {
                h.editing.cancel();
            }
            QVERIFY(documentOf(*column)->isEmpty());
            QCOMPARE(h.editing.columnPosition(2, false), 2);
            QCOMPARE(h.editing.editPosition(3), 3);
        }
    }

    // L4-C17, sur le vrai TextEditor.qml : la colonne montre le texte (liaison columnDocument), l'entrée
    // transparente ne le montre pas en double, le curseur et la sélection s'y placent comme dans le texte. La
    // validation ne place rien hors de la colonne vidée (avertissement « out of range » de QTextCursor).
    void verticalEditorShowsTheColumnNotTheInput()
    {
        QTest::failOnWarning(QRegularExpression(QStringLiteral("out of range")));
        EditorHarness e;
        QVERIFY(e.editor);
        QVERIFY(e.typeVertical());
        QQuickItem* input = e.item("textEdit");
        QQuickItem* column = e.item("textColumn");
        QQuickItem* caret = e.item("textColumnCaret");
        QVERIFY(input && column && caret);
        QTextDocument* shown = documentOf(*column);
        QCOMPARE(laidOutLines(*shown), 3);
        QVERIFY(column->isVisible());
        QCOMPARE(input->opacity(), 0.0);
        // Le curseur montré : au bout de la ligne de la lettre précédente.
        for (const auto& [position, line] : QList<std::pair<int, int>>{{1, 0}, {2, 1}, {3, 2}}) {
            QVERIFY(input->setProperty("cursorPosition", position));
            QCOMPARE(qRound(caret->y()), qRound(lineRect(*shown, line).y()));
            QVERIFY(caret->height() > 0);
        }
        // L'espace sélectionné : sa ligne seule.
        QVERIFY(QMetaObject::invokeMethod(input, "select", Q_ARG(int, 1), Q_ARG(int, 2)));
        QCOMPARE(column->property("selectionStart").toInt(), 2);
        QCOMPARE(column->property("selectionEnd").toInt(), 3);
        e.editing.commit();
        QVERIFY(shown->isEmpty());
    }

    // L4-C17, M1 : la colonne prend la souris comme le TextEdit d'entrée la prenait à plat (c84ef41) : un
    // double-clic sélectionne le mot, Maj+appui étend la sélection.
    void verticalEditorMouseSelectsLikeTheInput()
    {
        EditorHarness e;
        QVERIFY(e.editor);
        QVERIFY(e.typeVertical());
        QQuickItem* input = e.item("textEdit");
        QQuickItem* column = e.item("textColumn");
        QVERIFY(input && column);
        QTextDocument* shown = documentOf(*column);
        const qreal width = shown->textWidth();
        const auto at = [&](int line, qreal x) {
            return column->mapToScene(QPointF(x, lineRect(*shown, line).center().y())).toPoint();
        };
        QTest::mouseDClick(&e.window, Qt::LeftButton, Qt::NoModifier, at(2, width / 2));
        QCOMPARE(input->property("selectedText").toString(), QStringLiteral("B"));
        QTest::mouseClick(&e.window, Qt::LeftButton, Qt::NoModifier, at(0, 0.5));
        QCOMPARE(input->property("cursorPosition").toInt(), 0);
        QCOMPARE(input->property("selectedText").toString(), QString());
        QTest::mouseClick(&e.window, Qt::LeftButton, Qt::ShiftModifier, at(2, width - 0.5));
        QCOMPARE(input->property("selectedText").toString(), QStringLiteral("A B"));
    }

    // L4-C17, m3 : la pré-édition (touche morte, méthode de saisie) se voit dans la colonne, au curseur, sans
    // être du texte ; validée, elle devient la lettre composée.
    void verticalEditorShowsThePreedit()
    {
        EditorHarness e;
        QVERIFY(e.editor);
        QVERIFY(e.typeVertical());
        QQuickItem* input = e.item("textEdit");
        QQuickItem* column = e.item("textColumn");
        QVERIFY(input && column);
        QTextDocument* shown = documentOf(*column);
        QInputMethodEvent compose(QStringLiteral("^"), {});
        QCoreApplication::sendEvent(input, &compose);
        QCOMPARE(input->property("preeditText").toString(), QStringLiteral("^"));
        QCOMPARE(e.editing.plainText(), QStringLiteral("A B"));
        QVERIFY(shown->toPlainText().contains(QLatin1Char('^')));
        QCOMPARE(laidOutLines(*shown), 4);
        QCOMPARE(e.editing.editPosition(shown->characterCount() - 1), 3); // au bout : après « B »
        QCOMPARE(e.editing.columnPosition(3, false), 5);                  // le curseur, avant la pré-édition
        QInputMethodEvent commit;
        commit.setCommitString(QStringLiteral("ê"));
        QCoreApplication::sendEvent(input, &commit);
        QCOMPARE(e.editing.plainText(), QStringLiteral("A Bê"));
        QVERIFY(!shown->toPlainText().contains(QLatin1Char('^')));
        QCOMPARE(laidOutLines(*shown), 4);
    }

    // L4-C17, m5 : coût d'une frappe à 200 graphèmes, cadre et colonne montrée compris (médiane de cinq).
    // Mesure seule, sans seuil (construction de débogage, machine quelconque) ; le banc release reste au PC.
    void verticalKeystrokeCostAt200Graphemes()
    {
        Harness h;
        std::unique_ptr<QObject> column = columnTextEdit(h.engine);
        QVERIFY(column);
        QVERIFY(column->setProperty("width", 40)); // largeur posée, comme la liaison de TextEditor.qml
        h.editing.setColumnDocument(column->property("textDocument").value<QQuickTextDocument*>());
        QVERIFY(h.editing.beginAt({200, 200}, true));
        QString text;
        for (int i = 0; i < 199; ++i) {
            text += i % 5 == 4 ? QChar(u' ') : QChar(static_cast<char16_t>(u'a' + i % 26));
        }
        h.replaceAllText(text);
        std::vector<qint64> micros;
        for (int i = 0; i < 5; ++i) {
            QElapsedTimer timer;
            timer.start();
            h.typeChar(QLatin1Char('z'));
            micros.push_back(timer.nsecsElapsed() / 1000);
        }
        QCOMPARE(laidOutLines(*documentOf(*column)), 204);
        std::sort(micros.begin(), micros.end());
        qInfo("L4-C17 : une frappe à 200 graphèmes, %lld µs (médiane de 5)",
              static_cast<long long>(micros[2]));
    }

    // « Wii » validé : trois lignes, la colonne du peintre (la même mise en page,
    // render::layOutVerticalColumn).
    void verticalCommitIsTheColumnOfThePainter()
    {
        Harness h;
        QVERIFY(h.editing.beginAt({200, 200}, true));
        h.replaceAllText(QStringLiteral("Wii"));
        h.editing.commit();
        const auto* text = std::get_if<core::Text>(&core::body(h.controller.document().page(0).objects[0]));
        QVERIFY(text);
        QTextDocument column;
        column.setDocumentMargin(0);
        column.setHtml(QString::fromStdString(text->html));
        render::layOutVerticalColumn(column);
        QCOMPARE(text->bounds.height, std::ceil(column.size().height()));
        QCOMPARE(text->bounds.width, render::verticalColumnWidth(column));
        QFont font(QGuiApplication::font().family());
        font.setPointSizeF(14);
        const qreal line = QFontMetricsF(font).lineSpacing();
        QVERIFY2(text->bounds.height > 2.5 * line && text->bounds.height < 3.5 * line,
                 qPrintable(QStringLiteral("hauteur %1, ligne %2").arg(text->bounds.height).arg(line)));
    }

    // Une famille lue dans le document (« <img src=… > ») reste affichée telle quelle : les textes des listes
    // de police sont en texte brut, dans la barre à plat et dans celle de haut en bas.
    void fontFamilyIsShownAsPlainText()
    {
        const QString family = QStringLiteral("<img src=\"file:///inexistant.png\">");
        const QList<std::pair<const char*, const char*>> bars{{"VerticalTextBar", "vbar-font"},
                                                              {"TextFormatBar", "textFontCombo"}};
        for (const auto& [bar, combo] : bars) {
            QQmlEngine engine;
            QQmlComponent component(&engine);
            component.setData(
                QByteArray("import QtQuick\nimport ActCraft.Ui\nItem {\n  width: 900; height: 200\n"
                           "  Commands { id: cmd }\n  ") +
                    bar + QByteArray(" { objectName: \"bar\"; commands: cmd }\n}\n"),
                QUrl());
            std::unique_ptr<QObject> root(component.create());
            QVERIFY2(root, qPrintable(component.errorString()));
            QObject* box = root->findChild<QObject*>(QString::fromLatin1(combo));
            QVERIFY2(box, combo);
            QVERIFY(box->setProperty("displayText", family));
            auto* shown = box->property("contentItem").value<QObject*>();
            QVERIFY2(shown, combo);
            QCOMPARE(shown->property("text").toString(), family);
            QCOMPARE(shown->property("textFormat").toInt(), 0); // Text.PlainText
        }
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
