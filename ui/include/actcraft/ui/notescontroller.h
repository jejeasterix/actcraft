#pragma once

#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>

#include <QObject>
#include <QPointer>
#include <QQuickTextDocument>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Notes de la page affichée (F-830) : texte riche (HTML) lu dans la page courante et écrit par
// core::SetPageNotesCommand. La frappe continue se fusionne en une étape (SetPageNotesCommand::kMergeWindow).
// Écrire sans `commands`, sous le verrou des commandes ou pendant le rechargement d'une page ne change rien.
class NotesController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(
        actcraft::ui::DocumentController* document READ document WRITE setDocument NOTIFY wiringChanged)
    Q_PROPERTY(actcraft::ui::Commands* commands READ commands WRITE setCommands NOTIFY wiringChanged)
    Q_PROPERTY(QString html READ html WRITE setHtml NOTIFY htmlChanged)
    Q_PROPERTY(bool hasNotes READ hasNotes NOTIFY htmlChanged)
    Q_PROPERTY(bool editable READ isEditable NOTIFY editableChanged)
    // Vrai pendant un contact sur la vue : la relecture de la page est reportée à la fin du contact.
    Q_PROPERTY(bool contactActive READ contactActive WRITE setContactActive NOTIFY contactActiveChanged)

public:
    explicit NotesController(QObject* parent = nullptr);

    DocumentController* document() const noexcept;
    void setDocument(DocumentController* document);
    Commands* commands() const noexcept;
    void setCommands(Commands* commands);
    QString html() const;
    void setHtml(const QString& html);
    bool hasNotes() const noexcept; // texte visible, balises retirées, non vide
    bool isEditable() const noexcept;
    bool contactActive() const noexcept;
    void setContactActive(bool active);

    // Gras, italique ou souligné (`style` : « bold », « italic », « underline ») sur la sélection
    // [start, end) du document de l'éditeur ; sans sélection, le style du curseur suivant.
    Q_INVOKABLE void toggleStyle(QQuickTextDocument* editor, int start, int end, const QString& style);

signals:
    void wiringChanged();
    void htmlChanged();
    void editableChanged();
    void contactActiveChanged();

private:
    void refresh();
    void updateEditable();

    QPointer<DocumentController> m_document;
    QPointer<Commands> m_commands;
    QString m_html;
    bool m_hasNotes = false;
    bool m_editable = false;
    bool m_contactActive = false;
    bool m_dirty = false;   // relecture reportée par un contact
    bool m_loading = false; // le texte d'une page se recharge : une écriture n'est pas une frappe
};

} // namespace actcraft::ui
