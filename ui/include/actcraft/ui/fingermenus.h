#pragma once

#include <actcraft/ui/pageview.h>
#include <actcraft/ui/toolhost.h>

#include <QObject>
#include <QPointF>
#include <QPointer>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Menus au doigt (D11-7) : écoute PageView::touchHeld (un doigt tenu immobile). Avec le sélecteur comme outil
// effectif du contact (imposé par le mode du toucher, ou courant), demande le menu de l'objet sous le doigt
// (le doigt est dans le cadre de la sélection), sinon le menu de la page ; avec tout autre outil (encre,
// gomme, forme…) : rien. Rien non plus pour le second utilisateur du mode à deux (L11-C13, F-303) : les
// entrées de ces menus agissent sur la sélection du premier et ouvrent ses boîtes. Les menus eux-mêmes sont
// ouverts par FingerMenu.qml, sur les signaux ci-dessous (point en repère fenêtre).
class FingerMenus : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(actcraft::ui::PageView* view READ view WRITE setView NOTIFY wiringChanged)
    Q_PROPERTY(actcraft::ui::ToolHost* tools READ tools WRITE setTools NOTIFY wiringChanged)
    // "" : aucun menu au doigt ouvert ; "object" ou "page" : celui qui a été demandé en dernier.
    Q_PROPERTY(QString kind READ kind NOTIFY kindChanged)

public:
    explicit FingerMenus(QObject* parent = nullptr);

    PageView* view() const { return m_view; }
    void setView(PageView* view);
    ToolHost* tools() const { return m_tools; }
    void setTools(ToolHost* tools);
    QString kind() const { return m_kind; }

    // Le menu demandé a été refermé.
    Q_INVOKABLE void reset();

signals:
    void wiringChanged();
    void kindChanged();
    void objectMenuRequested(QPointF windowPoint);
    void pageMenuRequested(QPointF windowPoint);

private:
    void onTouchHeld(QPointF pagePoint, QPointF windowPoint, int user, const QString& tool);
    bool insideSelection(QPointF pagePoint) const;
    void setKind(const QString& kind);

    QPointer<PageView> m_view;
    QPointer<ToolHost> m_tools;
    QString m_kind;
};

} // namespace actcraft::ui
