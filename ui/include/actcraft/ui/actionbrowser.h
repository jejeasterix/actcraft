#pragma once

#include <actcraft/core/model.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/toolhost.h>

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QMetaObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

#include <vector>

namespace actcraft::actions {
struct ObjectActionInfo;
}

namespace actcraft::ui {

// Onglet « Sélection » du navigateur d'actions (L8-T11, F-850, F-851, D8-10). Une ligne par action du
// catalogue (actions/data/object-actions.json) de la catégorie choisie ; « Appliquer » pose l'action choisie
// sur tous les objets sélectionnés en une étape, « Retirer les actions existantes » les retire en une étape.
// Fermé par défaut : sans `commands`, ou sous le verrou de classe, rien ne change et `refused` est émis.
// Toute modification passe par DocumentController::execute ; aucune ne change un pixel du cache
// (SetObjectActionsCommand). La remise à jour est regroupée et reportée pendant un contact (D6-5).
class ActionBrowserModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(actcraft::ui::Commands* commands READ commands WRITE setCommands NOTIFY sourcesChanged)
    Q_PROPERTY(
        actcraft::ui::DocumentController* document READ document WRITE setDocument NOTIFY sourcesChanged)
    Q_PROPERTY(actcraft::ui::ToolHost* toolHost READ toolHost WRITE setToolHost NOTIFY sourcesChanged)
    Q_PROPERTY(bool contactActive READ contactActive WRITE setContactActive NOTIFY contactActiveChanged)
    Q_PROPERTY(int category READ category WRITE setCategory NOTIFY categoryChanged) // 0 : toutes
    Q_PROPERTY(QString currentId READ currentId WRITE setCurrentId NOTIFY currentIdChanged)
    Q_PROPERTY(QVariantMap parameters READ parameters WRITE setParameters NOTIFY parametersChanged)
    Q_PROPERTY(QVariantList propertyRows READ propertyRows NOTIFY currentIdChanged)
    Q_PROPERTY(quint64 target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(bool picking READ picking NOTIFY pickingChanged)
    Q_PROPERTY(bool enabled READ enabled NOTIFY enabledChanged)

public:
    enum Role
    {
        IdRole = Qt::UserRole + 1,
        LabelRole,
        IconRole,
        ExistingRole, // tous les objets sélectionnés portent déjà cette action
    };
    Q_ENUM(Role)

    explicit ActionBrowserModel(QObject* parent = nullptr);
    ~ActionBrowserModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Commands* commands() const noexcept;
    void setCommands(Commands* commands);
    DocumentController* document() const noexcept;
    void setDocument(DocumentController* document);
    ToolHost* toolHost() const noexcept;
    void setToolHost(ToolHost* tools);
    bool contactActive() const noexcept;
    void setContactActive(bool active);
    int category() const noexcept;
    void setCategory(int category);
    QString currentId() const;
    void setCurrentId(const QString& id);
    QVariantMap parameters() const;
    void setParameters(const QVariantMap& parameters);
    QVariantList propertyRows() const;
    quint64 target() const noexcept;
    void setTarget(quint64 target);
    bool picking() const noexcept;
    bool enabled() const noexcept; // sélection non vide, `commands` posé, pas de verrou

    // « Toutes », « Commandes », « Pages », « Objets », « Document » : l'indice est `category`.
    Q_INVOKABLE QStringList categoryLabels() const;
    Q_INVOKABLE bool apply();          // une étape pour tous les objets sélectionnés
    Q_INVOKABLE bool removeExisting(); // une étape ; faux si aucun objet sélectionné ne porte d'action
    // Le prochain clic sur la page (ToolHost, outil Sélection) choisit l'objet visé ; la sélection d'avant
    // revient ensuite, hors contact.
    Q_INVOKABLE void pickTarget();
    Q_INVOKABLE void cancelPick();

signals:
    void sourcesChanged();
    void contactActiveChanged();
    void categoryChanged();
    void currentIdChanged();
    void parametersChanged();
    void targetChanged();
    void pickingChanged();
    void enabledChanged();
    void refused(const QString& reason);

private:
    struct Row
    {
        const actions::ObjectActionInfo* info = nullptr;
        bool existing = false;
    };

    void attachDocument();
    void detachDocument();
    void requestRefresh();
    void schedule();
    void flush();
    void onSelectionChanged();
    void restoreAfterPick();
    void updateEnabled();
    bool allowed(); // fermé par défaut ; émet refused
    std::vector<core::ObjectId> selectedOnPage() const;
    const actions::ObjectActionInfo* currentInfo() const;

    QPointer<Commands> m_commands;
    QPointer<DocumentController> m_document;
    QPointer<ToolHost> m_tools;
    QMetaObject::Connection m_commandsConnection;
    QMetaObject::Connection m_selectionConnection;
    QList<QMetaObject::Connection> m_documentConnections;
    int m_observer = 0;
    bool m_contactActive = false;
    bool m_pending = false;
    bool m_scheduled = false;
    int m_category = 0;
    QString m_currentId;
    QVariantMap m_parameters;
    quint64 m_target = 0;
    bool m_picking = false;
    bool m_restorePending = false;
    std::vector<core::ObjectId> m_beforePick;
    bool m_hasSelection = false;
    bool m_enabled = false;
    std::vector<Row> m_rows;
};

} // namespace actcraft::ui
