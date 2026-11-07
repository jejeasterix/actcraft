#pragma once

#include <actcraft/ui/preferences.h>

#include <QObject>
#include <QPointer>
#include <QQuickItem>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Mesures de la mise en page (D11-5, D11-6) : lit la taille logique de la fenêtre (dp) et le réglage du poste
// `workstation/touch-ui`, rend les seuils (mise en page étroite, compacte, portrait) et la taille des cibles
// tactiles. Les seuils et les tailles sont des constantes ci-dessous. Tout est en pixels logiques : aucune
// taille physique n'entre ici.
class LayoutMetrics : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(actcraft::ui::Preferences* preferences READ preferences WRITE setPreferences NOTIFY
                   preferencesChanged)
    Q_PROPERTY(qreal windowWidth READ windowWidth WRITE setWindowWidth NOTIFY windowWidthChanged)
    Q_PROPERTY(qreal windowHeight READ windowHeight WRITE setWindowHeight NOTIFY windowHeightChanged)
    Q_PROPERTY(bool narrow READ narrow NOTIFY metricsChanged FINAL)
    Q_PROPERTY(bool compact READ compact NOTIFY metricsChanged FINAL)
    Q_PROPERTY(bool portrait READ portrait NOTIFY metricsChanged FINAL)
    Q_PROPERTY(bool touchUi READ touchUi NOTIFY metricsChanged FINAL)
    Q_PROPERTY(qreal targetSize READ targetSize NOTIFY metricsChanged FINAL)
    Q_PROPERTY(qreal spacing READ spacing NOTIFY metricsChanged FINAL)

public:
    static constexpr qreal kNarrowBelow = 960;  // navigateurs en tiroir, palette repliable
    static constexpr qreal kCompactBelow = 600; // menus de la ligne supérieure en débordement
    static constexpr qreal kTouchTarget = 48;   // cible tactile (touchUi)
    static constexpr qreal kPointerTarget = 44; // taille du lot 6
    static constexpr qreal kTouchSpacing = 8;
    static constexpr qreal kPointerSpacing = 4;
    static constexpr const char* kTouchUiKey = "workstation/touch-ui";

    explicit LayoutMetrics(QObject* parent = nullptr);

    Preferences* preferences() const noexcept;
    void setPreferences(Preferences* preferences);
    qreal windowWidth() const noexcept;
    void setWindowWidth(qreal windowWidth);
    qreal windowHeight() const noexcept;
    void setWindowHeight(qreal windowHeight);

    // Une largeur nulle (fenêtre pas encore dimensionnée) n'est ni étroite ni compacte.
    bool narrow() const noexcept;
    bool compact() const noexcept;
    bool portrait() const noexcept;
    bool touchUi() const;
    qreal targetSize() const;
    qreal spacing() const;

    // Ordre des enfants d'une ligne : `stackBefore` et `stackAfter` de QQuickItem ne sont pas appelables
    // depuis le QML (Qt 6.12). Le tiroir des navigateurs retrouve par là sa place à côté de la page.
    Q_INVOKABLE void stackItemBefore(QQuickItem* item, QQuickItem* sibling) const;
    Q_INVOKABLE void stackItemAfter(QQuickItem* item, QQuickItem* sibling) const;

signals:
    void preferencesChanged();
    void windowWidthChanged();
    void windowHeightChanged();
    void metricsChanged();

private:
    QPointer<Preferences> m_preferences;
    qreal m_windowWidth = 0;
    qreal m_windowHeight = 0;
};

} // namespace actcraft::ui
