#include "presentationdocument.h"

#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/replace.h>
#include <actcraft/core/undo.h>

#include <QObject>
#include <QTimer>

#include <algorithm>
#include <memory>
#include <utility>

namespace actcraft::ui {

namespace {

// Identifiants écrits par la commande elle-même, à la fin de son redo() (revue contre le socle, tour 2, m1) :
// execute() détruit une commande qui n'a rien changé, donc aucun pointeur vers elle n'est lu après. La
// cellule survit à la commande ; vide si rien n'a été appliqué.
using IdCell = std::shared_ptr<std::vector<core::ObjectId>>;

IdCell newCell()
{
    return std::make_shared<std::vector<core::ObjectId>>();
}

// core::AddObjectCommand, identifiant copié dans la cellule.
class ReportedAdd final : public core::UndoCommand
{
public:
    ReportedAdd(std::size_t page, core::PageObject object, IdCell ids)
        : m_command(page, std::move(object)), m_ids(std::move(ids))
    {}
    void redo(core::Document& document, core::Changes& changes) override
    {
        const std::size_t before = changes.size();
        m_command.redo(document, changes);
        m_ids->clear();
        if (changes.size() > before) {
            m_ids->push_back(m_command.id());
        }
    }
    void undo(core::Document& document, core::Changes& changes) override
    {
        m_command.undo(document, changes);
    }

private:
    core::AddObjectCommand m_command;
    IdCell m_ids;
};

// core::ReplaceObjectsCommand (un groupe), identifiants des ajouts copiés dans la cellule.
class ReportedReplace final : public core::UndoCommand
{
public:
    ReportedReplace(std::size_t page, std::vector<core::ObjectId> removed,
                    std::vector<core::PageObject> added, IdCell ids)
        : m_command(page, std::move(removed), std::move(added)), m_ids(std::move(ids))
    {}
    void redo(core::Document& document, core::Changes& changes) override
    {
        const std::size_t before = changes.size();
        m_command.redo(document, changes);
        m_ids->clear();
        if (changes.size() > before) {
            *m_ids = m_command.addedIds();
        }
    }
    void undo(core::Document& document, core::Changes& changes) override
    {
        m_command.undo(document, changes);
    }

private:
    core::ReplaceObjectsCommand m_command;
    IdCell m_ids;
};

// Page ajoutée en fin et objet posé dessus, en une étape (insertObjectOnNewPage). Le cache voit la page
// insérée puis l'objet ; ni l'un ni l'autre n'est la page affichée, qui change ensuite (showAddedPage).
class AddObjectOnNewPage final : public core::UndoCommand
{
public:
    AddObjectOnNewPage(std::size_t index, double width, double height, core::PageObject object, IdCell ids)
        : m_page(index, width, height), m_object(index, std::move(object)), m_ids(std::move(ids))
    {}
    void redo(core::Document& document, core::Changes& changes) override
    {
        m_ids->clear();
        core::Changes applied;
        m_page.redo(document, applied);
        const std::size_t pageOnly = applied.size();
        m_object.redo(document, applied);
        if (applied.size() == pageOnly) {
            // Objet refusé (identifiants épuisés) : la page repart, rien n'est rendu, aucune étape.
            core::Changes undone;
            m_page.undo(document, undone);
            return;
        }
        m_ids->push_back(m_object.id());
        changes.insert(changes.end(), applied.begin(), applied.end());
    }
    void undo(core::Document& document, core::Changes& changes) override
    {
        m_object.undo(document, changes);
        m_page.undo(document, changes);
    }

private:
    core::AddPageCommand m_page;
    core::AddObjectCommand m_object;
    IdCell m_ids;
};

// Image d'une ressource neuve (insertImage) : ressource ajoutée et objet inséré, sur une page neuve au
// besoin, en une étape. L'annulation retire l'objet, la page qu'elle a ajoutée, et la ressource si sa clé
// était absente avant redo() : l'empreinte revient et le fichier enregistré ne garde pas l'image.
class InsertImage final : public core::UndoCommand
{
public:
    InsertImage(std::size_t page, bool newPage, double width, double height, QByteArray data,
                std::string extension, core::ImageObject image, IdCell ids)
        : m_page(page), m_newPage(newPage), m_width(width), m_height(height), m_data(std::move(data)),
          m_extension(std::move(extension)), m_object(std::move(image)), m_ids(std::move(ids))
    {}
    void redo(core::Document& document, core::Changes& changes) override
    {
        m_ids->clear();
        // Vérifié d'abord : page cible, puis identifiant ; refusé, rien n'est touché ni rendu.
        if (m_newPage ? m_page > document.pageCount() : m_page >= document.pageCount()) {
            return;
        }
        if (m_object.common.id == 0) {
            m_object.common.id = document.allocateId(); // gardé pour les rétablissements
            if (m_object.common.id == 0) {
                return; // identifiants épuisés
            }
        }
        const std::size_t resources = document.resources().size();
        m_key = document.addResource(m_data, m_extension);
        m_ownsResource = document.resources().size() > resources;
        std::get<core::ImageObject>(core::body(m_object)).resource = m_key;
        if (m_newPage) {
            core::Page page;
            page.width = m_width;
            page.height = m_height;
            document.insertPage(m_page, std::move(page));
            changes.push_back(core::Change{core::Change::Kind::PageInserted, m_page, 0, core::RectF{}});
        }
        core::insertAtTopOfLayer(document.page(m_page), m_object);
        m_inserted = true;
        changes.push_back(core::Change{core::Change::Kind::ObjectInserted, m_page, m_object.common.id,
                                       core::boundsOnPage(m_object)});
        m_ids->push_back(m_object.common.id);
    }
    void undo(core::Document& document, core::Changes& changes) override
    {
        if (!m_inserted) {
            return;
        }
        core::Page& page = document.page(m_page);
        const std::size_t index = core::indexOf(page, m_object.common.id);
        if (index < page.objects.size()) {
            page.objects.erase(page.objects.begin() + static_cast<std::ptrdiff_t>(index));
        }
        changes.push_back(core::Change{core::Change::Kind::ObjectRemoved, m_page, m_object.common.id,
                                       core::boundsOnPage(m_object)});
        if (m_newPage) {
            document.takePage(m_page);
            changes.push_back(core::Change{core::Change::Kind::PageRemoved, m_page, 0, core::RectF{}});
        }
        if (m_ownsResource) {
            document.removeResource(m_key);
        }
        m_inserted = false;
    }

private:
    std::size_t m_page;
    bool m_newPage;
    double m_width;
    double m_height;
    QByteArray m_data;
    std::string m_extension;
    core::PageObject m_object;
    IdCell m_ids;
    std::string m_key;
    bool m_ownsResource = false;
    bool m_inserted = false;
};

// Retour à la page de départ quand l'insertion sur une page neuve est annulée (revue contre le socle, tour 2,
// M3). Observateur à usage unique : il ne garde que des valeurs, le contrôleur et la cellule de son état
// (jeton, connexion ; aucune référence vers DocumentInsertions), et se retire à son premier Undo qui retire
// `added`. Il ne ramène que si la page retirée était la page affichée (pendant l'appel, currentIndex() est
// encore l'indice passé au cache). La navigation est différée : pendant la diffusion, les observateurs
// suivants doivent lire l'indice passé au cache (contrat de addChangeObserver), et pageBack() émettrait
// pageAboutToChange au milieu de l'annulation (un trait en cours y serait versé, effaçant la branche à
// rétablir). Elle est bornée (pageBack ne fait que reculer). Un document remplacé retire l'observateur
// (documentReplaced, jamais documentChanged : émis après chaque trait) ; retiré sur Undo, il coupe aussi
// cette connexion.
struct ReturnWatch
{
    int token = 0;
    QMetaObject::Connection replaced;
};

void watchReturn(DocumentController& controller, std::size_t start, std::size_t added)
{
    auto watch = std::make_shared<ReturnWatch>();
    watch->token = controller.addChangeObserver([&controller, start, added,
                                                 watch](const core::Changes& applied,
                                                        DocumentController::ChangeOrigin origin) {
        if (origin != DocumentController::ChangeOrigin::Undo) {
            return;
        }
        const bool removed = std::any_of(applied.begin(), applied.end(), [added](const core::Change& change) {
            return change.kind == core::Change::Kind::PageRemoved && change.page == added;
        });
        if (!removed) {
            return;
        }
        const bool wasShown = static_cast<std::size_t>(controller.currentIndex()) == added;
        QObject::disconnect(watch->replaced);
        controller.removeChangeObserver(watch->token);
        if (!wasShown) {
            return;
        }
        QTimer::singleShot(0, &controller, [&controller, start] {
            while (static_cast<std::size_t>(controller.currentIndex()) > start) {
                controller.pageBack();
            }
        });
    });
    watch->replaced = QObject::connect(
        &controller, &DocumentController::documentReplaced, &controller,
        [&controller, watch] { controller.removeChangeObserver(watch->token); }, Qt::SingleShotConnection);
}

// La page ajoutée (la dernière) est affichée par la seule voie publique : nextPage(), qui n'ajoute une page
// que sur la dernière (la boucle s'arrête avant) ; puis le retour à l'annulation est armé.
void showAddedPage(DocumentController& controller, std::size_t start, std::size_t added)
{
    while (controller.currentIndex() < controller.pageCount() - 1) {
        controller.nextPage();
    }
    watchReturn(controller, start, added);
}

} // namespace

DocumentInsertions::DocumentInsertions(DocumentController& controller) : m_controller(controller) {}

core::ObjectId DocumentInsertions::insertObject(core::PageObject object)
{
    const IdCell ids = newCell();
    m_controller.execute(std::make_unique<ReportedAdd>(shownIndex(), std::move(object), ids));
    return ids->empty() ? 0 : ids->front();
}

core::ObjectId DocumentInsertions::insertObjectOnNewPage(core::PageObject object)
{
    const core::Document& document = m_controller.document();
    const core::Page& last = document.page(document.pageCount() - 1);
    const std::size_t start = shownIndex();
    const std::size_t added = document.pageCount();
    const IdCell ids = newCell();
    m_controller.execute(
        std::make_unique<AddObjectOnNewPage>(added, last.width, last.height, std::move(object), ids));
    if (ids->empty()) {
        return 0;
    }
    showAddedPage(m_controller, start, added);
    return ids->front();
}

core::ObjectId DocumentInsertions::insertText(const QString& plainText, const QRectF& bounds)
{
    core::Text text;
    text.bounds = core::RectF{bounds.x(), bounds.y(), bounds.width(), bounds.height()};
    // Texte brut échappé (jamais interprété) ; police des nouveaux textes (N2-3) : sans empattement du
    // système, 14 pt ; retours à la ligne gardés.
    text.html =
        QStringLiteral("<p style=\"font-family:sans-serif; font-size:14pt; white-space:pre-wrap\">%1</p>")
            .arg(plainText.toHtmlEscaped())
            .toStdString();
    core::PageObject object = std::move(text);
    object.common.layer = core::Layer::Middle;
    return insertObject(std::move(object));
}

core::ObjectId DocumentInsertions::insertImage(QByteArray data, std::string extension,
                                               core::ImageObject image, bool onNewPage)
{
    const core::Document& document = m_controller.document();
    const core::Page& last = document.page(document.pageCount() - 1);
    const std::size_t start = shownIndex();
    const std::size_t page = onNewPage ? document.pageCount() : start;
    const IdCell ids = newCell();
    m_controller.execute(std::make_unique<InsertImage>(page, onNewPage, last.width, last.height,
                                                       std::move(data), std::move(extension),
                                                       std::move(image), ids));
    if (ids->empty()) {
        return 0;
    }
    if (onNewPage) {
        showAddedPage(m_controller, start, page);
    }
    return ids->front();
}

std::vector<core::ObjectId> DocumentInsertions::replaceObjects(std::vector<core::ObjectId> removed,
                                                               std::vector<core::PageObject> added)
{
    const IdCell ids = newCell();
    m_controller.execute(
        std::make_unique<ReportedReplace>(shownIndex(), std::move(removed), std::move(added), ids));
    return *ids;
}

void DocumentInsertions::setPagePresentation(core::PagePresentation value)
{
    m_controller.execute(std::make_unique<core::SetPagePresentationCommand>(shownIndex(), std::move(value)));
}

const core::Page* DocumentInsertions::shownPage() const noexcept
{
    return &m_controller.document().page(shownIndex());
}

std::size_t DocumentInsertions::shownIndex() const noexcept
{
    return static_cast<std::size_t>(m_controller.currentIndex());
}

} // namespace actcraft::ui
