#include <actcraft/core/edits.h>
#include <actcraft/core/hittest.h>
#include <actcraft/core/objectedits.h>
#include <actcraft/core/textedits.h>
#include <actcraft/ui/textediting.h>

#include <QAbstractTextDocumentLayout>
#include <QByteArray>
#include <QColor>
#include <QFont>
#include <QGuiApplication>
#include <QImage>
#include <QList>
#include <QObject>
#include <QSizeF>
#include <QStringList>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextFragment>
#include <QTextFrame>
#include <QTextFrameFormat>
#include <QTextImageFormat>
#include <QTextList>
#include <QTextListFormat>
#include <QTextTable>
#include <QTextTableCell>
#include <QUrl>

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace actcraft::ui {

using ResourceObserver = std::function<void(const QUrl&, const QVariant&)>;

QString safeTextHtml(const QString& html, const ResourceObserver& observer);
std::unique_ptr<QTextDocument> isolatedTextDocument(ResourceObserver observer);

namespace {

constexpr qreal kSizeStep = 2;     // D8 : taille ±2 points
constexpr qreal kMinPointSize = 8; // D8 : minimum 8 (et non 4)
constexpr qreal kSpacingStep = 2;  // D8 : 2 points d'espace au-dessus du paragraphe
constexpr int kIndentStep = 1;     // D8 : un niveau

QString placeholderText()
{
    return QStringLiteral("Votre texte"); // D7 : texte de départ, le nôtre
}

// Document isolé de l'édition (analyse et mesure) : la règle du peintre (render/src/pagepainter.cpp:45-92,
// NoImageHandler et IsolatedTextDocument), recopiée à l'identique parce que ces classes y sont internes. Tout
// écart se voit dans tst_ui_text (editingLoadsNoExternalResource), qui reprend le crible du peintre
// (tst_render_painter, htmlLoadsNoExternalResource).
// Toute ressource demandée (image, fond, feuille de style, toutes URL et tous schémas) reçoit une valeur vide
// mais valide : QTextDocument ne se rabat alors ni sur un fournisseur, ni sur la lecture directe du fichier.
// Images du HTML : ni lues ni dessinées, leur place seule est gardée. Le gestionnaire de Qt chercherait
// « nom@2x.png » sur le disque avant toute ressource (chemin UNC : résolution réseau), puis lirait le fichier
// quand la ressource est nulle. L'observateur (tests) voit chaque demande et sa réponse.
class NoImageHandler final : public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

public:
    QSizeF intrinsicSize(QTextDocument*, int, const QTextFormat& format) override
    {
        const QTextImageFormat image = format.toImageFormat();
        return QSizeF(std::max(0.0, image.width()), std::max(0.0, image.height()));
    }
    void drawObject(QPainter*, const QRectF&, QTextDocument*, int, const QTextFormat&) override {}
};

class AnalysisDocument final : public QTextDocument
{
public:
    explicit AnalysisDocument(ResourceObserver observer) : m_observer(std::move(observer))
    {
        setDocumentMargin(0);
        setResourceProvider([](const QUrl&) { return QVariant(QByteArray()); });
        documentLayout()->registerHandler(QTextFormat::ImageObject, &m_images);
    }

protected:
    QVariant loadResource(int type, const QUrl& name) override
    {
        QVariant answer;
        if (type == QTextDocument::ImageResource) {
            QImage blank(1, 1, QImage::Format_ARGB32_Premultiplied);
            blank.fill(Qt::transparent);
            answer = blank;
        } else if (type == QTextDocument::StyleSheetResource) {
            answer = QString();
        } else {
            answer = QByteArray();
        }
        if (m_observer) {
            m_observer(name, answer);
        }
        return answer;
    }

private:
    ResourceObserver m_observer; // par valeur (Q4a-m4) : le document peut survivre à l'appelant
    NoImageHandler m_images;
};

template <typename Format> bool clearImageUrl(Format& format)
{
    if (!format.hasProperty(QTextFormat::BackgroundImageUrl)) {
        return false;
    }
    format.clearProperty(QTextFormat::BackgroundImageUrl);
    return true;
}

bool isDataUrl(const QString& name)
{
    return name.startsWith(QStringLiteral("data:"), Qt::CaseInsensitive);
}

// Cadres et tableaux (N5) : image de fond du cadre, du tableau et de chaque cellule.
void cleanFrames(QTextFrame* frame)
{
    if (!frame) {
        return;
    }
    QTextFrameFormat format = frame->frameFormat();
    if (clearImageUrl(format)) {
        frame->setFrameFormat(format);
    }
    if (auto* table = qobject_cast<QTextTable*>(frame)) {
        for (int row = 0; row < table->rows(); ++row) {
            for (int column = 0; column < table->columns(); ++column) {
                QTextTableCell cell = table->cellAt(row, column);
                QTextCharFormat cellFormat = cell.format();
                if (clearImageUrl(cellFormat)) {
                    cell.setFormat(cellFormat);
                }
            }
        }
    }
    const QList<QTextFrame*> children = frame->childFrames();
    for (QTextFrame* child : children) {
        cleanFrames(child);
    }
}

} // namespace

// Pour tst_ui_text (L4-C13) : le document isolé de safeTextHtml et de measuredBounds, sans en-tête public.
std::unique_ptr<QTextDocument> isolatedTextDocument(ResourceObserver observer)
{
    return std::make_unique<AnalysisDocument>(std::move(observer));
}

// Bloquant 5 (amendements 1 à 3) : le HTML d'un texte vient du fichier ouvert. Rien n'en est chargé ; les
// images hors `data:`, les ancres et les images de fond (bloc, cadre, tableau, cellule, caractères) sont
// retirées.
QString safeTextHtml(const QString& html, const ResourceObserver& observer)
{
    AnalysisDocument document(observer);
    document.setHtml(html);
    cleanFrames(document.rootFrame());
    struct Range
    {
        int position = 0;
        int length = 0;
    };
    std::vector<Range> images;
    std::vector<std::pair<Range, QTextCharFormat>> formats;
    for (QTextBlock block = document.begin(); block.isValid(); block = block.next()) {
        QTextBlockFormat blockFormat = block.blockFormat();
        QTextCharFormat blockChars = block.charFormat();
        const bool blockDirty = clearImageUrl(blockFormat);
        const bool charsDirty = clearImageUrl(blockChars);
        if (blockDirty || charsDirty) {
            QTextCursor cursor(block);
            if (blockDirty) {
                cursor.setBlockFormat(blockFormat);
            }
            if (charsDirty) {
                cursor.setBlockCharFormat(blockChars);
            }
        }
        for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment fragment = it.fragment();
            if (!fragment.isValid()) {
                continue;
            }
            QTextCharFormat format = fragment.charFormat();
            if (format.isImageFormat() && !isDataUrl(format.toImageFormat().name())) {
                images.push_back({fragment.position(), fragment.length()});
                continue;
            }
            bool dirty = clearImageUrl(format);
            if (format.isAnchor() || format.hasProperty(QTextFormat::AnchorHref) ||
                format.hasProperty(QTextFormat::AnchorName)) {
                format.setAnchor(false);
                format.clearProperty(QTextFormat::AnchorHref);
                format.clearProperty(QTextFormat::AnchorName);
                dirty = true;
            }
            if (dirty) {
                formats.push_back({{fragment.position(), fragment.length()}, format});
            }
        }
    }
    // Après le parcours : changer un format pendant l'itération fusionnerait ou couperait les fragments.
    for (const auto& [range, format] : formats) {
        QTextCursor cursor(&document);
        cursor.setPosition(range.position);
        cursor.setPosition(range.position + range.length, QTextCursor::KeepAnchor);
        cursor.setCharFormat(format);
    }
    // De la fin vers le début : les positions relevées restent justes.
    std::sort(images.begin(), images.end(),
              [](const Range& a, const Range& b) { return a.position > b.position; });
    for (const Range& image : images) {
        QTextCursor cursor(&document);
        cursor.setPosition(image.position);
        cursor.setPosition(image.position + image.length, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
    }
    return document.toHtml();
}

TextEditing::TextEditing(QObject* parent) : QObject(parent) {}

DocumentController* TextEditing::documentController() const noexcept
{
    return m_controller.data();
}

void TextEditing::setDocumentController(DocumentController* controller)
{
    if (m_controller == controller) {
        return;
    }
    commit();
    if (m_controller) {
        disconnect(m_controller, nullptr, this, nullptr);
    }
    m_controller = controller;
    if (m_controller) {
        // V6 : les dépôts forcés passent par DEUX signaux (ui/src/documentcontroller.cpp:322-332).
        // Enregistrer, annuler, rétablir, ouvrir, supprimer, effacer : editAboutToApply ; changer de page (et
        // replaceDocument) : pageAboutToChange, sans editAboutToApply. Comme ToolHost
        // (ui/src/toolhost.cpp:121-122), le texte se valide sur les deux, avant que dropRemaining ne remette
        // le texte soulevé tel qu'il était.
        connect(m_controller, &DocumentController::editAboutToApply, this, &TextEditing::commit);
        connect(m_controller, &DocumentController::pageAboutToChange, this, &TextEditing::commit);
    }
    emit wiringChanged();
}

PageView* TextEditing::view() const noexcept
{
    return m_view.data();
}

void TextEditing::setView(PageView* view)
{
    if (m_view == view) {
        return;
    }
    m_view = view;
    emit wiringChanged();
}

QQuickTextDocument* TextEditing::textDocument() const noexcept
{
    return m_document.data();
}

void TextEditing::setTextDocument(QQuickTextDocument* document)
{
    if (m_document == document) {
        return;
    }
    commit();
    if (m_watched) {
        disconnect(m_watched, nullptr, this, nullptr);
    }
    m_document = document;
    m_watched = document ? document->textDocument() : nullptr;
    if (m_watched) {
        // Frappe : le cadre suit le document ; rien n'est écrit dans le paperboard avant la validation.
        connect(m_watched, &QTextDocument::contentsChanged, this, &TextEditing::updateFrame);
    }
    emit wiringChanged();
}

int TextEditing::selectionStart() const noexcept
{
    return m_selectionStart;
}

void TextEditing::setSelectionStart(int position)
{
    if (m_selectionStart == position) {
        return;
    }
    m_selectionStart = position;
    emit formatChanged();
}

int TextEditing::selectionEnd() const noexcept
{
    return m_selectionEnd;
}

void TextEditing::setSelectionEnd(int position)
{
    if (m_selectionEnd == position) {
        return;
    }
    m_selectionEnd = position;
    emit formatChanged();
}

bool TextEditing::isActive() const noexcept
{
    return m_active;
}

bool TextEditing::isVertical() const noexcept
{
    return m_vertical;
}

QRectF TextEditing::frame() const noexcept
{
    return m_frame;
}

QTextDocument* TextEditing::editDocument() const
{
    return m_document ? m_document->textDocument() : nullptr;
}

QTextCursor TextEditing::selectionCursor() const
{
    QTextDocument* doc = editDocument();
    if (!doc) {
        return {};
    }
    QTextCursor cursor(doc);
    const int last = std::max(0, doc->characterCount() - 1);
    cursor.setPosition(std::clamp(m_selectionStart, 0, last));
    cursor.setPosition(std::clamp(m_selectionEnd, 0, last), QTextCursor::KeepAnchor);
    return cursor;
}

QTextCharFormat TextEditing::currentCharFormat() const
{
    const QTextCursor cursor = selectionCursor();
    return cursor.isNull() ? QTextCharFormat() : cursor.charFormat();
}

QString TextEditing::plainText() const
{
    const QTextDocument* doc = editDocument();
    return doc ? doc->toPlainText() : QString();
}

QString TextEditing::fontFamily() const
{
    const QStringList families = currentCharFormat().fontFamilies().toStringList();
    if (!families.isEmpty()) {
        return families.first();
    }
    const QTextDocument* doc = editDocument();
    return doc ? doc->defaultFont().family() : QString();
}

qreal TextEditing::fontSize() const
{
    const qreal size = currentCharFormat().fontPointSize();
    if (size > 0) {
        return size;
    }
    const QTextDocument* doc = editDocument();
    const qreal fallback = doc ? doc->defaultFont().pointSizeF() : 0;
    return fallback > 0 ? fallback : m_nextSize;
}

bool TextEditing::bold() const
{
    return currentCharFormat().fontWeight() >= QFont::Bold;
}

bool TextEditing::italic() const
{
    return currentCharFormat().fontItalic();
}

bool TextEditing::underline() const
{
    return currentCharFormat().fontUnderline();
}

QTextCharFormat TextEditing::newTextFormat() const
{
    QTextCharFormat format;
    format.setFontFamilies(
        QStringList{m_nextFamily.isEmpty() ? QGuiApplication::font().family() : m_nextFamily});
    format.setFontPointSize(m_nextSize);
    format.setForeground(QColor(Qt::black)); // D7 : noir, sans fond ni contour
    return format;
}

// P17, P20, D7. Le patron évident (une commande par frappe) est le mauvais : rien n'entre dans le paperboard
// avant commit ; un texte existant est soulevé (un retrait du cache), il revient en une étape.
bool TextEditing::beginAt(QPointF pagePoint, bool vertical)
{
    QTextDocument* doc = editDocument();
    if (!m_controller || !doc) {
        return false;
    }
    if (m_active) {
        if (m_frame.contains(pagePoint)) {
            return true; // clic dans le texte : l'édition continue
        }
        commit(); // P20 : un clic hors du texte valide, sans poser un autre texte
        return false;
    }
    const std::size_t page = static_cast<std::size_t>(std::max(0, m_controller->currentIndex()));
    const core::Page& shown = m_controller->document().page(page);
    const double zoom = m_view && m_view->zoom() > 0 ? m_view->zoom() : 1.0;
    const core::PointF at{pagePoint.x(), pagePoint.y()};
    const std::optional<core::ObjectId> hit = core::objectAt(shown, at, core::kHitToleranceScreen / zoom);
    std::optional<core::Text> existing;
    core::RectF onPage;
    if (hit) {
        for (const core::PageObject& object : shown.objects) {
            const auto* text = std::get_if<core::Text>(&core::body(object));
            if (object.common.id == *hit && text && !object.common.locked) { // P17 : verrouillé, pas édité
                existing = *text;
                onPage = core::boundsOnPage(object);
            }
        }
    }
    m_page = page;
    m_geometryChanged = false;
    if (existing) {
        // Copies prises avant le soulèvement, qui retire l'objet de la page. Seul le HTML assaini entre dans
        // le TextEdit (bloquant 5).
        doc->setHtml(safeTextHtml(QString::fromStdString(existing->html), {}));
        m_editing = *hit;
        m_vertical = existing->vertical;
        m_objectBounds = existing->bounds;
        m_fixedWidth = existing->bounds.width;
        m_frame = QRectF(onPage.x, onPage.y, onPage.width, onPage.height);
        m_controller->liftObjects({*hit});
        const int end = std::max(0, doc->characterCount() - 1);
        m_selectionStart = end;
        m_selectionEnd = end;
    } else {
        doc->clear();
        QTextCursor cursor(doc);
        const QTextCharFormat format = newTextFormat();
        cursor.setBlockCharFormat(format);
        cursor.insertText(placeholderText(), format);
        m_editing = 0;
        m_vertical = vertical;
        m_objectBounds = core::RectF{pagePoint.x(), pagePoint.y(), 0, 0};
        m_fixedWidth = 0;
        m_frame = QRectF(pagePoint, QSizeF());
        m_selectionStart = 0; // D7 : « Votre texte » tout sélectionné
        m_selectionEnd = static_cast<int>(placeholderText().size());
    }
    m_initialHtml = doc->toHtml();
    m_active = true;
    updateFrame();
    emit activeChanged();
    emit formatChanged();
    emit frameChanged();
    emit focusRequested();
    return true;
}

// Mesure du document du peintre (render/src/pagepainter.cpp, paintText) : marge 0, largeur = celle de
// l'emprise.
core::RectF TextEditing::measuredBounds(const QString& safeHtml) const
{
    AnalysisDocument measure{ResourceObserver()};
    measure.setHtml(safeHtml);
    const qreal width = m_fixedWidth > 0 ? m_fixedWidth : std::ceil(measure.idealWidth()) + 1;
    measure.setTextWidth(width);
    return core::RectF{m_objectBounds.x, m_objectBounds.y, width, std::ceil(measure.size().height())};
}

void TextEditing::finish()
{
    m_active = false;
    m_editing = 0;
    m_fixedWidth = 0;
    m_geometryChanged = false;
    m_initialHtml.clear();
}

// F-252, D7 : une étape au plus. Texte neuf vide : oublié, sans étape ; texte existant vidé : supprimé en une
// étape ; texte inchangé : revient sans étape.
void TextEditing::commit()
{
    if (!m_active) {
        return;
    }
    QTextDocument* doc = editDocument();
    const core::ObjectId id = m_editing;
    const std::size_t page = m_page;
    bool empty = true;
    bool unchanged = false;
    core::Text text;
    if (doc) {
        const QString live = doc->toHtml();
        empty = doc->toPlainText().trimmed().isEmpty();
        unchanged = live == m_initialHtml && !m_geometryChanged;
        const QString safe = safeTextHtml(live, {}); // un collage riche ne fait pas entrer de ressource
        text.bounds = measuredBounds(safe);
        text.html = safe.toStdString();
        text.vertical = m_vertical;
    }
    finish(); // avant le contrôleur : ses signaux peuvent revenir ici
    if (m_controller) {
        if (id == 0) {
            if (!empty) {
                core::PageObject object = std::move(text);
                object.common.layer = core::Layer::Middle; // F-253 : couche intermédiaire
                m_controller->execute(std::make_unique<core::AddObjectCommand>(page, std::move(object)));
            }
        } else if (empty) {
            m_controller->dropLifted(std::make_unique<core::RemoveObjectsCommand>(page, std::vector{id}));
        } else if (unchanged) {
            m_controller->dropLifted(nullptr);
        } else {
            m_controller->dropLifted(std::make_unique<core::SetTextCommand>(page, id, std::move(text)));
        }
    }
    if (doc) {
        doc->clear();
    }
    emit activeChanged();
    emit formatChanged();
    emit frameChanged();
}

// Abandon (pas Échap, qui valide : D7) : le texte soulevé revient tel qu'il était, sans étape.
void TextEditing::cancel()
{
    if (!m_active) {
        return;
    }
    const core::ObjectId id = m_editing;
    finish();
    if (id != 0 && m_controller) {
        m_controller->dropLifted(nullptr);
    }
    if (QTextDocument* doc = editDocument()) {
        doc->clear();
    }
    emit activeChanged();
    emit formatChanged();
    emit frameChanged();
}

void TextEditing::mergeFormat(const QTextCharFormat& format)
{
    QTextCursor cursor = selectionCursor();
    if (cursor.isNull()) {
        return;
    }
    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::WordUnderCursor);
    }
    cursor.mergeCharFormat(format);
    if (cursor.block().length() <= 1) {
        cursor.mergeBlockCharFormat(format); // paragraphe vide : la frappe suivante prend le format
    }
}

void TextEditing::changeBlocks(const std::function<void(QTextBlockFormat&)>& change)
{
    QTextDocument* doc = editDocument();
    if (!doc) {
        return;
    }
    const QTextCursor selection = selectionCursor();
    const QTextBlock last = doc->findBlock(selection.selectionEnd());
    QTextCursor edit(doc);
    edit.beginEditBlock();
    for (QTextBlock block = doc->findBlock(selection.selectionStart()); block.isValid();
         block = block.next()) {
        QTextBlockFormat format = block.blockFormat();
        change(format);
        QTextCursor(block).setBlockFormat(format);
        if (block == last) {
            break;
        }
    }
    edit.endEditBlock();
}

void TextEditing::rememberFormat()
{
    m_nextSize = fontSize();
    m_nextFamily = fontFamily();
}

// F-100, D8 : sur la sélection du TextEdit.
void TextEditing::apply(const QString& command, const QVariant& argument)
{
    QTextDocument* doc = editDocument();
    if (!m_active || !doc) {
        return;
    }
    QTextCharFormat format;
    if (command == QLatin1String("text.bold")) {
        format.setFontWeight(bold() ? QFont::Normal : QFont::Bold);
        mergeFormat(format);
    } else if (command == QLatin1String("text.italic")) {
        format.setFontItalic(!italic());
        mergeFormat(format);
    } else if (command == QLatin1String("text.underline")) {
        format.setFontUnderline(!underline());
        mergeFormat(format);
    } else if (command == QLatin1String("text.superscript") || command == QLatin1String("text.subscript")) {
        const auto wanted = command == QLatin1String("text.superscript") ? QTextCharFormat::AlignSuperScript
                                                                         : QTextCharFormat::AlignSubScript;
        format.setVerticalAlignment(
            currentCharFormat().verticalAlignment() == wanted ? QTextCharFormat::AlignNormal : wanted);
        mergeFormat(format);
    } else if (command == QLatin1String("text.size-up") || command == QLatin1String("text.size-down") ||
               command == QLatin1String("text.size")) {
        qreal size = fontSize();
        if (command == QLatin1String("text.size-up")) {
            size += kSizeStep;
        } else if (command == QLatin1String("text.size-down")) {
            size -= kSizeStep;
        } else {
            size = argument.toReal();
        }
        format.setFontPointSize(std::max(kMinPointSize, size));
        mergeFormat(format);
        rememberFormat();
    } else if (command == QLatin1String("text.font")) {
        const QString family = argument.toString();
        if (family.isEmpty()) {
            return;
        }
        format.setFontFamilies(QStringList{family});
        mergeFormat(format);
        rememberFormat();
    } else if (command == QLatin1String("text.color") || command == QLatin1String("text.background")) {
        const QColor color = argument.value<QColor>();
        if (!color.isValid()) {
            return;
        }
        if (command == QLatin1String("text.color")) {
            format.setForeground(color);
        } else {
            format.setBackground(color);
        }
        mergeFormat(format);
    } else if (command == QLatin1String("text.align-left") || command == QLatin1String("text.align-center") ||
               command == QLatin1String("text.align-right")) {
        const Qt::Alignment alignment = command == QLatin1String("text.align-left")     ? Qt::AlignLeft
                                        : command == QLatin1String("text.align-center") ? Qt::AlignHCenter
                                                                                        : Qt::AlignRight;
        changeBlocks([alignment](QTextBlockFormat& block) { block.setAlignment(alignment); });
    } else if (command == QLatin1String("text.spacing-up") || command == QLatin1String("text.spacing-down")) {
        const qreal step = command == QLatin1String("text.spacing-up") ? kSpacingStep : -kSpacingStep;
        changeBlocks(
            [step](QTextBlockFormat& block) { block.setTopMargin(std::max(0.0, block.topMargin() + step)); });
    } else if (command == QLatin1String("text.indent") || command == QLatin1String("text.outdent")) {
        const int step = command == QLatin1String("text.indent") ? kIndentStep : -kIndentStep;
        changeBlocks(
            [step](QTextBlockFormat& block) { block.setIndent(std::max(0, block.indent() + step)); });
    } else if (command == QLatin1String("text.bullets")) {
        QTextCursor cursor = selectionCursor();
        if (QTextList* list = cursor.currentList()) {
            const QTextBlock last = doc->findBlock(cursor.selectionEnd());
            for (QTextBlock block = doc->findBlock(cursor.selectionStart()); block.isValid();
                 block = block.next()) {
                if (block.textList() == list) {
                    list->remove(block);
                    QTextBlockFormat plain = block.blockFormat();
                    plain.setIndent(0);
                    QTextCursor(block).setBlockFormat(plain);
                }
                if (block == last) {
                    break;
                }
            }
        } else {
            cursor.createList(QTextListFormat::ListDisc); // D8 : une sorte de puces au lot 4
        }
    } else if (command == QLatin1String("text.select-all")) {
        m_selectionStart = 0;
        m_selectionEnd = std::max(0, doc->characterCount() - 1);
        emit focusRequested();
    } else if (command == QLatin1String("text.symbol")) {
        const QString symbol = argument.toString();
        if (symbol.isEmpty()) {
            return;
        }
        QTextCursor cursor = selectionCursor();
        cursor.insertText(symbol);
        m_selectionStart = cursor.position();
        m_selectionEnd = cursor.position();
    } else {
        return;
    }
    updateFrame();
    emit formatChanged();
}

void TextEditing::updateFrame()
{
    const QTextDocument* doc = editDocument();
    if (!m_active || !doc) {
        return;
    }
    const qreal width = m_fixedWidth > 0 ? m_fixedWidth : doc->idealWidth();
    const QRectF next(m_frame.topLeft(), QSizeF(width, doc->size().height()));
    if (next != m_frame) {
        m_frame = next;
        emit frameChanged();
    }
}

void TextEditing::setFrameWidth(qreal width)
{
    if (!m_active || !(width > 0)) {
        return;
    }
    m_fixedWidth = width;
    m_geometryChanged = true;
    updateFrame();
    emit frameChanged(); // fixedWidth a pu changer sans que le cadre change
}

void TextEditing::moveFrame(QPointF delta)
{
    if (!m_active || (delta.x() == 0 && delta.y() == 0)) {
        return;
    }
    // Texte d'ActCraft ou placé par translation : un déplacement de page est un déplacement de l'objet.
    m_objectBounds.x += delta.x();
    m_objectBounds.y += delta.y();
    m_frame.translate(delta);
    m_geometryChanged = true;
    emit frameChanged();
}

} // namespace actcraft::ui

#include "textediting.moc"
