#pragma once

#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/toolhost.h>

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QMetaObject>
#include <QPointer>
#include <QString>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

#include <vector>

namespace actcraft::ui {

// Modèle du navigateur de propriétés (L6-T12, F-570 à F-576, F-581, F-583, F-840) : liste à deux colonnes
// groupée en sections selon la sélection de ToolHost (A5). Rien de sélectionné : propriétés de la page
// affichée ; un objet : celles qui s'appliquent à lui (core::readProperty) ; plusieurs objets : celles qui
// s'appliquent à tous, valeur affichée si elle est la même pour tous, sinon invalide.
//
// Le modèle ne modifie le document que par DocumentController::execute (une commande de L6-T11 par
// setValue) et ne sélectionne rien. Remise à jour regroupée (une par tour de boucle) et reportée pendant un
// contact (D6-5). Un changement de drapeau d'objet (verrou, masqué, copie au glissement) ne rend aucun
// core::Change : il se suit par DocumentController::objectFlagsChanged, quel que soit le chemin (L6-C2).
class PropertyModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(
        actcraft::ui::DocumentController* document READ document WRITE setDocument NOTIFY sourcesChanged)
    Q_PROPERTY(actcraft::ui::ToolHost* tools READ tools WRITE setTools NOTIFY sourcesChanged)
    Q_PROPERTY(actcraft::ui::Commands* commands READ commands WRITE setCommands NOTIFY sourcesChanged)
    Q_PROPERTY(QString title READ title NOTIFY refreshed) // « Page », « Trait », « Forme », « 3 objets »…
    Q_PROPERTY(bool contactActive READ contactActive WRITE setContactActive NOTIFY contactActiveChanged)
    Q_PROPERTY(bool editable READ editable NOTIFY editableChanged)
    Q_PROPERTY(int refreshCount READ refreshCount NOTIFY refreshed)

public:
    enum Role
    {
        SectionRole = Qt::UserRole +
            1, // identification, appearance, outline, fill, position, label, misc, page
        SectionLabelRole,
        KeyRole,     // identifiant de la propriété (kebab-case)
        LabelRole,   // libellé français
        EditorRole,  // text, number, color, slider, choice, bool, keywords
        ValueRole,   // QVariant ; invalide si les objets diffèrent. choice : indice dans ChoicesRole
        ChoicesRole, // QStringList pour « choice »
        MinRole,
        MaxRole,
    };
    Q_ENUM(Role)

    explicit PropertyModel(QObject* parent = nullptr);
    ~PropertyModel() override;

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Exécute une commande de L6-T11 (ou SetLayerCommand, SetObjectFlagCommand, SetTranslucencyCommand pour
    // couche, visibilité, verrou et translucidité) sur la page affichée ou les objets sélectionnés. Faux sans
    // document ni commandes, verrou de classe posé, ligne inconnue ou valeur illisible. Une modification
    // ponctuelle (tout éditeur sauf « slider ») est close aussitôt (closeMerge) : elle reste une étape à
    // elle. Seule la réglette glissée fusionne ; DocumentController::closeMerge la clôt au relâcher.
    Q_INVOKABLE bool setValue(int row, const QVariant& value);
    Q_INVOKABLE int rowOf(const QString& key) const; // -1 si absente

    DocumentController* document() const noexcept;
    void setDocument(DocumentController* document);
    ToolHost* tools() const noexcept;
    void setTools(ToolHost* tools);
    Commands* commands() const noexcept;
    void setCommands(Commands* commands);
    QString title() const;
    bool contactActive() const noexcept;
    void setContactActive(bool active);
    bool editable() const;
    int refreshCount() const noexcept;

signals:
    void sourcesChanged();
    void refreshed();
    void contactActiveChanged();
    void editableChanged();

private:
    struct Row;
    void requestRefresh(); // regroupe : un seul refresh() par tour de boucle, rien pendant un contact
    void schedule();
    void flush();
    void rebuild();
    void attachDocument();
    void detachDocument();
    void updateEditable();
    void onObjectFlagsChanged(int page, const QList<quint64>& ids);

    std::vector<Row> m_rows;
    QPointer<DocumentController> m_document;
    QPointer<ToolHost> m_tools;
    QPointer<Commands> m_commands;
    int m_observer = 0;
    QList<QMetaObject::Connection> m_documentConnections;
    QMetaObject::Connection m_selectionConnection;
    QMetaObject::Connection m_commandsConnection;
    bool m_contactActive = false;
    bool m_pending = false;
    bool m_scheduled = false;
    bool m_editable = false;
    int m_refreshes = 0;
    QString m_title;
};

} // namespace actcraft::ui
