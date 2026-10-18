#pragma once

#include <actcraft/core/model.h>
#include <actcraft/render/fixedpageitem.h> // type complet exigé par moc pour la propriété fixedPage

#include <QPointer>
#include <QQuickItem>
#include <QtQml/qqmlregistration.h>

#include <vector>

namespace actcraft::render {

// Couche soulevée (lot 3, DA11) : dessine les copies des objets soulevés pendant un geste, avec les moteurs
// de la page fixe (createObjectNode), sous une transformation supplémentaire qui ne reconstruit aucun nœud.
class LiftLayerItem : public QQuickItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(LiftLayer)
    Q_PROPERTY(
        actcraft::render::FixedPageItem* fixedPage READ fixedPage WRITE setFixedPage NOTIFY fixedPageChanged)
    Q_PROPERTY(int objectCount READ objectCount NOTIFY objectsChanged)

public:
    explicit LiftLayerItem(QQuickItem* parent = nullptr);
    FixedPageItem* fixedPage() const noexcept;
    void setFixedPage(FixedPageItem* page);
    void setObjects(std::vector<core::PageObject> objects); // remplace ; transformation remise à l'identité
    void setExtraTransform(const core::Transform& extra);
    void clear();
    int objectCount() const noexcept;
    int builtCount() const noexcept; // crochet de test : nœuds d'objet construits depuis la création

signals:
    void fixedPageChanged();
    void objectsChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData* data) override;

private:
    QPointer<FixedPageItem> m_fixedPage;
    std::vector<core::PageObject> m_objects;
};

} // namespace actcraft::render
