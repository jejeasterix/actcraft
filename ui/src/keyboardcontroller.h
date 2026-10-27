#pragma once

#include <actcraft/features/screenkeyboard.h>

#include <QObject>
#include <QPointer>
#include <QQuickWindow>
#include <QString>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <functional>

namespace actcraft::ui {

// Clavier à l'écran (F-294), L7-T29.
//
// Fonction à fenêtre propre (décision 4 bis) : la racine ScreenKeyboard.qml crée la fenêtre du clavier depuis
// son Component, SANS parent (createObject(null) : un Window enfant d'un Item prend la fenêtre principale
// pour parent et serait réduit avec elle), et la confie au contrôleur (`window`). La fenêtre ne prend jamais
// le focus (Qt::WindowDoesNotAcceptFocus | Qt::WindowStaysOnTopHint | Qt::Tool).
//
// Chaque touche (`press`) passe par features::ScreenKeyboard (majuscule, verrouillage) puis part :
// - si une fenêtre d'ActCraft autre que le clavier a le focus, en QKeyEvent (appui puis relâché) vers
//   son objet de focus (QGuiApplication::focusObject) ;
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
    Q_PROPERTY(QVariantList keys READ keys NOTIFY layoutChanged)
    Q_PROPERTY(int rowCount READ rowCount NOTIFY layoutChanged)
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
    void setOpen(bool open);
    Layout layout() const noexcept;
    void setLayout(Layout layout);
    // Touches de la disposition, prêtes pour QML : {id, label, width, row}.
    QVariantList keys() const;
    int rowCount() const noexcept;
    bool shifted() const noexcept;
    bool capsLocked() const noexcept;
    QQuickWindow* window() const noexcept; // nulle clavier fermé
    void setWindow(QQuickWindow* window);
    void setInjector(Injector injector);

    Q_INVOKABLE void toggle();
    // Tape la touche `id` ; faux si elle n'a rien envoyé (identifiant absent, aucune cible).
    Q_INVOKABLE bool press(const QString& id);

signals:
    void openChanged();
    void layoutChanged();
    void modifiersChanged();
    void windowChanged();

private:
    bool sendToFocus(const features::KeyOutput& output);
    void onFocusWindowChanged(QWindow* focus);

    features::ScreenKeyboard m_keyboard;
    bool m_open = false;
    QPointer<QQuickWindow> m_window;
    QPointer<QWindow> m_lastFocus; // dernière fenêtre d'ActCraft (hors clavier) qui a eu le focus
    Injector m_injector;
};

} // namespace actcraft::ui
