#pragma once

#include <actcraft/core/model.h>
#include <actcraft/ui/commands.h>           // types complets exigés par moc pour les propriétés
#include <actcraft/ui/documentcontroller.h> //
#include <actcraft/ui/toolhost.h>           //

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QList>
#include <QMetaObject>
#include <QPointer>
#include <QString>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

#include <vector>

namespace actcraft::ui {

// Navigateur d'objet (F-820, F-821), modèle à plat : quatre lignes de couche (`depth` 0 : du dessus,
// intermédiaire, inférieure, fond), chacune suivie des objets de la page affichée qui y sont, du plus haut au
// plus bas dans l'ordre de dessin (`depth` 1). Les membres d'un groupe sont portés par leur groupe : seul le
// groupe a sa ligne, dans la couche de l'objet Group (la plus basse de ses membres, makeGroupCommand), au
// rang du plus haut de ses membres dans cette couche (l'objet Group n'est pas dessiné). Règle D9, comme
// `object.lock` et `object.hide` : masquer ou verrouiller un groupe agit sur ses membres, et sa ligne est
// masquée (verrouillée) quand tous ses membres le sont, sélectionnée quand tous ses membres visibles le sont
// (un masqué n'est pas sélectionnable au canevas).
//
// Remise à jour : une seule par tour de boucle (`resetCount` + 1), reportée pendant un contact (D6-5). Elle
// suit l'observateur du contrôleur, `documentChanged` (émis après chaque `execute`, annulation et
// rétablissement, comme au remplacement du document : demande en double, regroupée), `pagesChanged` (page
// affichée), `ToolHost::selectionChanged`, et `DocumentController::objectFlagsChanged` : verrouiller ne rend
// aucun core::Change (objectflags.h), ce signal seul dit un verrou posé, ici ou par un autre chemin (L6-C2).
//
// Modifications fermées par défaut (B1) : sans `commands`, ou sous le verrou du registre, `move`, `setHidden`
// et `setLocked` rendent faux et ne touchent pas le document.
class ObjectTreeModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(
        actcraft::ui::DocumentController* document READ document WRITE setDocument NOTIFY sourcesChanged)
    Q_PROPERTY(actcraft::ui::ToolHost* tools READ tools WRITE setTools NOTIFY sourcesChanged)
    Q_PROPERTY(actcraft::ui::Commands* commands READ commands WRITE setCommands NOTIFY sourcesChanged)
    Q_PROPERTY(bool contactActive MEMBER m_contactActive NOTIFY contactActiveChanged)
    Q_PROPERTY(int resetCount READ resetCount NOTIFY reset)
public:
    enum Role
    {
        DepthRole = Qt::UserRole + 1, // 0 : couche ; 1 : objet
        LayerRole,                    // « top », « middle », « bottom », « background »
        ObjectIdRole,                 // 0 pour une couche
        NameRole,                     // nom de l'objet, ou libellé de son genre ; libellé de la couche
        KindRole, // « stroke », « shape », « text », « image », « group »… ; vide : couche
        HiddenRole,
        LockedRole,
        SelectedRole,
    };
    Q_ENUM(Role)

    explicit ObjectTreeModel(QObject* parent = nullptr);
    ~ObjectTreeModel() override; // retire l'observateur : il tient `this`

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Ligne d'objet → ToolHost::selectObjects (active l'outil Sélection, ne modifie pas le document). Ctrl
    // bascule ce que la ligne montre, relu sur la sélection courante : ligne allumée → l'objet (pour un
    // groupe : lui et ses membres) sort de la sélection ; éteinte → il y entre. Ligne de couche : rien.
    Q_INVOKABLE void select(int row, int modifiers);
    // La ligne `row` passe juste sous la ligne `beforeRow` − 1 (indices d'avant le déplacement ; `rowCount()`
    // : en fin). Même couche : core::ReorderObjectsCommand, seuls les objets de la couche bougent (comme
    // core::arrangedOrder), et la ligne déplacée arrive juste au-dessus de la ligne qui suit l'ancre (au fond
    // de la couche si l'ancre est la dernière). Autre couche : core::SetLayerCommand (sommet de la couche
    // d'arrivée). Une étape. Faux : ligne de couche, arrivée dans le fond, ligne déjà à sa place, refus.
    Q_INVOKABLE bool move(int row, int beforeRow);
    // core::SetObjectFlagCommand sur l'objet, ou sur le groupe et ses membres (D9) ; faux si la ligne est une
    // couche, si le drapeau a déjà cette valeur (sur tous les membres d'un groupe), ou refus.
    Q_INVOKABLE bool setHidden(int row, bool hidden); // une étape
    Q_INVOKABLE bool setLocked(int row, bool locked); // hors de la pile (aucun Change, règle du lot 5)

    DocumentController* document() const noexcept;
    void setDocument(DocumentController* document);
    ToolHost* tools() const noexcept;
    void setTools(ToolHost* tools);
    Commands* commands() const noexcept;
    void setCommands(Commands* commands);
    int resetCount() const noexcept;

signals:
    void sourcesChanged();
    void contactActiveChanged();
    void reset();

private:
    struct Row
    {
        int depth = 0;
        core::Layer layer = core::Layer::Middle;
        core::ObjectId id = 0;
        QString name;
        QString kind;
        bool hidden = false;
        bool locked = false;
        bool selected = false;
    };

    void requestReset();
    void schedule();
    void flush();
    void rebuild();
    void attachDocument();
    void detachDocument();
    bool canEdit() const;
    const Row* objectRow(int row) const; // nullptr : hors bornes ou ligne de couche
    bool setFlag(int row, int flag, bool value);

    std::vector<Row> m_rows;
    QPointer<DocumentController> m_document;
    QPointer<ToolHost> m_tools;
    QPointer<Commands> m_commands;
    QList<QMetaObject::Connection> m_documentConnections;
    QMetaObject::Connection m_selectionConnection;
    int m_observer = 0;
    bool m_contactActive = false;
    bool m_pending = false;   // une remise à jour est due
    bool m_scheduled = false; // un appel de flush() est en file
    int m_resets = 0;
};

} // namespace actcraft::ui
