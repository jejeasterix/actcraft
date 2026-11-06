#pragma once

#include <actcraft/core/model.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QMetaObject>
#include <QPointF>
#include <QPointer>
#include <QString>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

namespace actcraft::actions {
struct ObjectActionInfo;
}

namespace actcraft::ui {

// Actions par glisser-déposer (L8-T13, F-892, D8-11). Une ligne par entrée du catalogue marquée « drag-drop »
// (dragDropList(), jamais recopiée ici) ; une icône déposée sur un objet lui donne l'action en une étape
// (SetObjectActionsCommand), l'icône glissée de l'objet vers la corbeille la lui retire en une étape. Seules
// les actions de glisser-déposer sont touchées : une action au clic de l'objet reste, au dépôt comme au retrait.
// Un objet porte au plus une action de glisser-déposer (un nouveau dépôt remplace la précédente). Fermé par
// défaut : sans `commands`, ou sous le verrou de classe, rien ne change et `refused` est émis. Aucun pixel
// de page ne change : la surimpression (overlays) est une surcouche QML, jamais dans le cache.
class DragDropActions : public QAbstractListModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(DragDropActionsModel) // DragDropActions.qml est le plateau d'icônes
    Q_PROPERTY(actcraft::ui::Commands* commands READ commands WRITE setCommands NOTIFY sourcesChanged)
    Q_PROPERTY(
        actcraft::ui::DocumentController* document READ document WRITE setDocument NOTIFY sourcesChanged)
    // Un élément par objet de la page courante qui porte une action de glisser-déposer : objectId, actionId,
    // icon, label, x, y, width, height (emprise de page).
    Q_PROPERTY(QVariantList overlays READ overlays NOTIFY overlaysChanged)

public:
    enum Role
    {
        IdRole = Qt::UserRole + 1,
        LabelRole,
        IconRole,
    };
    Q_ENUM(Role)

    explicit DragDropActions(QObject* parent = nullptr);
    ~DragDropActions() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Commands* commands() const noexcept;
    void setCommands(Commands* commands);
    DocumentController* document() const noexcept;
    void setDocument(DocumentController* document);
    QVariantList overlays() const;

    Q_INVOKABLE bool dropOn(quint64 objectId, const QString& actionId);
    // Dépôt en un point de page : l'objet sélectionnable le plus haut, à `tolerance` écran près (zoom).
    Q_INVOKABLE bool dropAt(QPointF pagePoint, const QString& actionId, qreal zoom);
    Q_INVOKABLE bool dropOnTrash(quint64 objectId);
    Q_INVOKABLE QVariantList overlayIcons() const; // calculé à l'appel

signals:
    void sourcesChanged();
    void overlaysChanged();
    void refused(const QString& reason);

private:
    void attachDocument();
    void detachDocument();
    void requestRefresh();
    void refresh();
    bool allowed();
    const core::PageObject* find(core::ObjectId id) const;

    QPointer<Commands> m_commands;
    QPointer<DocumentController> m_document;
    QList<QMetaObject::Connection> m_documentConnections;
    QList<const actions::ObjectActionInfo*> m_rows;
    QVariantList m_overlays;
    int m_observer = 0;
    bool m_scheduled = false;
};

} // namespace actcraft::ui
