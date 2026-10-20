#pragma once

#include <actcraft/input/routing.h>
#include <actcraft/render/liftlayeritem.h> // types complets exigés par moc pour les propriétés
#include <actcraft/tools/userstate.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>

#include <QColor>
#include <QObject>
#include <QPointer>
#include <QRectF>
#include <QString>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include <memory>

namespace actcraft::tools {
class SelectTool;
class Tool;
class UserState;
} // namespace actcraft::tools

namespace actcraft::ui {

struct ToolAdapters; // tooladapters.h

// Outils de l'utilisateur (DA6, DA-A1) : registre, état d'utilisateur, adaptateurs des interfaces d'outil,
// commandes du lot 3, mémoire par outil (F-136, QSettings).
class ToolHost : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Tools)
    Q_PROPERTY(actcraft::ui::Commands* commands READ commands WRITE setCommands NOTIFY wiringChanged)
    Q_PROPERTY(actcraft::ui::DocumentController* documentController READ documentController WRITE
                   setDocumentController NOTIFY wiringChanged)
    Q_PROPERTY(actcraft::ui::PageView* view READ view WRITE setView NOTIFY wiringChanged)
    Q_PROPERTY(
        actcraft::render::LiftLayerItem* liftLayer READ liftLayer WRITE setLiftLayer NOTIFY wiringChanged)
    Q_PROPERTY(QString currentTool READ currentTool NOTIFY currentToolChanged)
    Q_PROPERTY(QColor color READ color NOTIFY settingsChanged)
    Q_PROPERTY(qreal width READ width NOTIFY settingsChanged)
    Q_PROPERTY(int selectionCount READ selectionCount NOTIFY selectionChanged)
    Q_PROPERTY(bool handlesVisible READ handlesVisible NOTIFY selectionChanged)
    Q_PROPERTY(QVariantMap selectionFrame READ selectionFrame NOTIFY selectionChanged)
    Q_PROPERTY(QRectF trashArea READ trashArea WRITE setTrashArea NOTIFY trashAreaChanged) // scène
    Q_PROPERTY(bool trashHover READ trashHover NOTIFY trashHoverChanged)
    // Vide : réglages de l'application ; tests : chemin d'un fichier .ini.
    Q_PROPERTY(QString settingsFile READ settingsFile WRITE setSettingsFile NOTIFY settingsFileChanged)

public:
    explicit ToolHost(QObject* parent = nullptr);
    ~ToolHost() override;

    Commands* commands() const noexcept;
    void setCommands(Commands* commands);
    DocumentController* documentController() const noexcept;
    void setDocumentController(DocumentController* controller);
    PageView* view() const noexcept;
    void setView(PageView* view);
    render::LiftLayerItem* liftLayer() const noexcept;
    void setLiftLayer(render::LiftLayerItem* layer);
    QString currentTool() const;
    QColor color() const;
    qreal width() const;
    int selectionCount() const;
    bool handlesVisible() const;
    QVariantMap selectionFrame() const;
    QRectF trashArea() const noexcept;
    void setTrashArea(const QRectF& area);
    bool trashHover() const noexcept;
    QString settingsFile() const;
    void setSettingsFile(const QString& path);

    // « select », « pen », « highlighter », « eraser » : outil courant de l'utilisateur 1, commande tool.<id>
    // cochée, réglages de l'outil rendus à la vue (couleur et épaisseur de l'encre).
    Q_INVOKABLE void activate(const QString& toolId);
    Q_INVOKABLE void setColor(const QColor& color); // outil courant (F-134)
    Q_INVOKABLE void setWidth(qreal width);
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void deleteSelection();
    Q_INVOKABLE void enlargeSelection();
    Q_INVOKABLE void reduceSelection();
    Q_INVOKABLE void setSelectionTranslucency(int value);
    Q_INVOKABLE void cancelGesture();

    // Outil courant de l'utilisateur ; nullptr tant que le contrôleur, la vue et la couche ne sont pas
    // branchés.
    tools::Tool* activeTool(input::UserId user = input::kFirstUser) const;
    tools::UserState& user(input::UserId id = input::kFirstUser); // un utilisateur jusqu'au lot 11 (D-S10)

signals:
    void wiringChanged();
    void currentToolChanged();
    void settingsChanged();
    void selectionChanged();
    void trashAreaChanged();
    void trashHoverChanged();
    void settingsFileChanged();

private:
    // Adaptateurs et outils ; nullptr tant que contrôleur, vue (et son encre) et couche ne sont pas branchés.
    ToolAdapters* adapters() const;
    void discardAdapters() const; // jette les adaptateurs, l'outil occupé abandonnant d'abord son geste
    tools::Tool* toolFor(const QString& id) const;      // créé à la première demande
    tools::Tool* existingTool(const QString& id) const; // jamais créé
    tools::SelectTool* selectTool() const;              // l'outil Sélectionner, créé à la première demande
    tools::InkSettings settingsOf(const QString& id) const; // réglages de l'utilisateur, ou ceux par défaut
    // Réglages rangés chez l'utilisateur, écrits dans le fichier et rendus à la vue.
    void storeSettings(const QString& id, const tools::InkSettings& settings);
    void ensureSettings(); // QSettings → UserState (F-136), une fois par fichier
    void writeSettings(const QString& toolId) const;
    void applyToView(); // couleur (alpha du marqueur) et épaisseur rendues à PageView
    void syncToolCommands();
    void showSelection(const QVariantMap& frame, bool handlesVisible);
    void setTrashHoverState(bool hover);
    void refreshSelection();
    void relayModifiers(int key, int modifiers);
    QString shownInkTool() const;
    QString shownWidthTool() const;
    bool eventFilter(QObject* watched, QEvent* event) override; // Maj et Ctrl (L3-T8)

    QPointer<Commands> m_commands;
    QPointer<DocumentController> m_controller;
    QPointer<PageView> m_view;
    QPointer<render::LiftLayerItem> m_liftLayer;
    QRectF m_trashArea;
    bool m_trashHover = false;
    QString m_settingsFile;
    std::unique_ptr<tools::UserState> m_user;
    mutable std::unique_ptr<ToolAdapters> m_adapters;
    QString m_lastInk = QStringLiteral("pen"); // dernier outil d'encre (L3-T0)
    QVariantMap m_selectionFrame;
    bool m_handlesVisible = false;
    bool m_settingsLoaded = false;
    bool m_filtering = false;
    int m_observer = 0;
};

} // namespace actcraft::ui
