#pragma once

#include <actcraft/features/screenkeyboard.h>

#include <QObject>
#include <QPointer>
#include <QQuickWindow>
#include <QString>
#include <QVariantList>
#include <QWindow>
#include <QtQml/qqmlregistration.h>

#include <functional>

namespace actcraft::ui {

// Clavier à l'écran (F-294), L7-T29.
//
// Fonction à fenêtre propre (décision 4 bis) : la racine ScreenKeyboard.qml crée la fenêtre du clavier depuis
// son Component, SANS parent (createObject(null) : un Window enfant d'un Item prend la fenêtre principale
// pour parent et serait réduit avec elle), et la confie au contrôleur (`window`). La fenêtre ne prend jamais
// le focus (Qt::WindowDoesNotAcceptFocus | Qt::WindowStaysOnTopHint | Qt::Tool) ; si une plateforme le lui
// donne quand même, le contrôleur le rend à la fenêtre d'ActCraft qui l'avait.
//
// Chaque touche (`press`) passe par features::ScreenKeyboard (majuscule, verrouillage) puis part :
// - si une fenêtre d'ActCraft autre que le clavier a le focus, en QKeyEvent (appui puis relâché) vers
//   cette fenêtre, qui le remet à son objet de focus (QGuiApplication::focusObject) ;
// - sinon (une autre application est au premier plan), à l'injecteur : par défaut
//   features::sendUnicodeToForeground (Windows : SendInput) ; les tests en posent un autre et ne l'appellent
//   jamais vers une autre application.
// Aucune frappe n'est journalisée.
class KeyboardController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool open READ isOpen WRITE setOpen NOTIFY openChanged)
    Q_PROPERTY(Layout layout READ layout WRITE setLayout NOTIFY layoutChanged)
    Q_PROPERTY(QVariantList rows READ rows NOTIFY layoutChanged)
    Q_PROPERTY(bool shifted READ shifted NOTIFY modifiersChanged)
    Q_PROPERTY(bool capsLocked READ capsLocked NOTIFY modifiersChanged)
    Q_PROPERTY(QQuickWindow* window READ window WRITE setWindow NOTIFY windowChanged)

public:
    enum class Layout
    {
        Compact,
        Standard,
        Extended
    };
    Q_ENUM(Layout)

    // Envoie un texte à l'application au premier plan ; faux si rien n'est parti.
    using Injector = std::function<bool(const QString&)>;

    explicit KeyboardController(QObject* parent = nullptr);
    ~KeyboardController() override;

    bool isOpen() const noexcept;
    void setOpen(bool open); // la racine crée ou détruit la fenêtre
    Layout layout() const noexcept;
    void setLayout(Layout layout);
    // Lignes de touches de la disposition, prêtes pour QML : chacune une liste de {id, label, width}.
    QVariantList rows() const;
    bool shifted() const noexcept;
    bool capsLocked() const noexcept;
    QQuickWindow* window() const noexcept; // nulle clavier fermé
    void setWindow(QQuickWindow* window);
    void setInjector(Injector injector);

    Q_INVOKABLE void toggle();
    // Tape la touche `id` ; faux si elle est absente de la disposition ou si rien n'est parti.
    Q_INVOKABLE bool press(const QString& id);

signals:
    void openChanged();
    void layoutChanged();
    void modifiersChanged();
    void windowChanged();

private:
    QWindow* focusTarget() const; // fenêtre d'ActCraft qui reçoit la frappe, nulle hors d'ActCraft
    bool sendToFocus(const features::KeyOutput& output);
    void onFocusWindowChanged(QWindow* focus);

    features::ScreenKeyboard m_keyboard;
    bool m_open = false;
    QPointer<QQuickWindow> m_window;
    // Dernière fenêtre d'ActCraft (hors clavier) qui a eu le focus ; nulle quand le focus a quitté ActCraft.
    QPointer<QWindow> m_lastFocus;
    Injector m_injector;
};

} // namespace actcraft::ui
