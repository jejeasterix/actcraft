#include <QPainter>
#include <QPointingDevice>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>
#include <QSGRenderNode>
#include <QSGRendererInterface>
#include <QSignalSpy>
#include <QTabletEvent>
#include <QTest>
#include <QtQml/qqml.h>

#include <memory>

// Vérifications de comportements de Qt 6.12 dont dépend le prototype de mesure (cadrage, décisions 6, 7, 13).
// Les lignes « VERIF <clé>: oui|non » sont recopiées dans docs/verifications/2026-10-13-qt612.md.

namespace {

class PainterNode : public QSGRenderNode
{
public:
    PainterNode(QQuickWindow* window, QRectF rect, QColor color)
        : m_window(window), m_rect(rect), m_color(color)
    {}
    void render(const RenderState* state) override
    {
        auto* painter = static_cast<QPainter*>(
            m_window->rendererInterface()->getResource(m_window, QSGRendererInterface::PainterResource));
        if (!painter) {
            return;
        }
        // La région de découpe est en coordonnées du périphérique : la poser avant la transformation.
        if (const QRegion* clip = state->clipRegion(); clip && !clip->isEmpty()) {
            painter->setClipRegion(*clip, Qt::ReplaceClip);
        }
        painter->setTransform(matrix()->toTransform());
        painter->setOpacity(inheritedOpacity());
        painter->fillRect(m_rect, m_color);
    }
    StateFlags changedStates() const override { return {}; }
    RenderingFlags flags() const override { return BoundedRectRendering; }
    QRectF rect() const override { return m_rect; }
    void setColor(QColor color) { m_color = color; }

private:
    QQuickWindow* m_window;
    QRectF m_rect;
    QColor m_color;
};

class PainterItem : public QQuickItem
{
public:
    PainterItem() { setFlag(ItemHasContents); }

protected:
    QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData*) override
    {
        if (old) {
            return old;
        }
        return new PainterNode(window(), QRectF(0, 0, width(), height()), QColor(Qt::red));
    }
};

class GeometryItem : public QQuickItem
{
public:
    GeometryItem() { setFlag(ItemHasContents); }

protected:
    QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData*) override
    {
        if (old) {
            return old;
        }
        auto* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 4);
        geometry->setDrawingMode(QSGGeometry::DrawTriangleStrip);
        auto* v = geometry->vertexDataAsPoint2D();
        const auto w = static_cast<float>(width());
        const auto h = static_cast<float>(height());
        v[0].set(0, 0);
        v[1].set(w, 0);
        v[2].set(0, h);
        v[3].set(w, h);
        auto* material = new QSGFlatColorMaterial;
        material->setColor(QColor(Qt::green));
        auto* node = new QSGGeometryNode;
        node->setGeometry(geometry);
        node->setMaterial(material);
        node->setFlags(QSGNode::OwnsGeometry | QSGNode::OwnsMaterial);
        return node;
    }
};

class TabletProbeItem : public QQuickItem
{
public:
    TabletProbeItem() { setAcceptedMouseButtons(Qt::LeftButton); }
    int tabletEvents = 0;
    qreal firstPressure = -1;

protected:
    bool event(QEvent* e) override
    {
        switch (e->type()) {
        case QEvent::TabletPress:
        case QEvent::TabletMove:
        case QEvent::TabletRelease: {
            auto* tablet = static_cast<QTabletEvent*>(e);
            if (tabletEvents == 0) {
                firstPressure = tablet->pressure();
            }
            ++tabletEvents;
            tablet->accept();
            return true;
        }
        default:
            break;
        }
        return QQuickItem::event(e);
    }
    void mousePressEvent(QMouseEvent* e) override { e->accept(); }
};

class TabletFilter : public QObject
{
public:
    int tabletEvents = 0;
    qreal firstPressure = -1;

protected:
    bool eventFilter(QObject* watched, QEvent* e) override
    {
        if (e->type() == QEvent::TabletPress || e->type() == QEvent::TabletMove ||
            e->type() == QEvent::TabletRelease) {
            if (tabletEvents == 0) {
                firstPressure = static_cast<QTabletEvent*>(e)->pressure();
            }
            ++tabletEvents;
        }
        return QObject::eventFilter(watched, e);
    }
};

bool sameColor(QColor a, QColor b)
{
    return qAbs(a.red() - b.red()) <= 3 && qAbs(a.green() - b.green()) <= 3 && qAbs(a.blue() - b.blue()) <= 3;
}

} // namespace

// Source C++ à QSGRenderNode, comme la page fixe en rendu logiciel (T6), pour vérifier la couche sur ce cas.
class ColorPainterItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor)

public:
    ColorPainterItem() { setFlag(ItemHasContents); }
    QColor color() const { return m_color; }
    void setColor(QColor color)
    {
        m_color = color;
        update();
    }

protected:
    QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData*) override
    {
        auto* node = old ? static_cast<PainterNode*>(old)
                         : new PainterNode(window(), QRectF(0, 0, width(), height()), m_color);
        node->setColor(m_color);
        node->markDirty(QSGNode::DirtyMaterial);
        return node;
    }

private:
    QColor m_color = Qt::red;
};

// Vrai si un ShaderEffectSource `live: false` garde son image, se redessine sur scheduleUpdate() et émet
// scheduledUpdateCompleted, avec pour source l'élément décrit par `source` (id src, propriété color).
static bool staticLayerWorks(const QByteArray& source, QString& details)
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(
        "import QtQuick\nimport Verif\nItem {\n    width: 200; height: 200\n    " + source +
            "\n    ShaderEffectSource {\n        objectName: \"cache\"; width: 100; height: 100\n"
            "        sourceItem: src; live: false; hideSource: true; textureSize: Qt.size(200, 200)\n"
            "    }\n}\n",
        QUrl());
    std::unique_ptr<QObject> root(component.create());
    if (!root) {
        details = component.errorString();
        return false;
    }
    QQuickWindow window;
    window.setColor(Qt::white);
    window.resize(200, 200);
    auto* rootItem = qobject_cast<QQuickItem*>(root.get());
    rootItem->setParentItem(window.contentItem());
    window.show();
    if (!QTest::qWaitForWindowExposed(&window)) {
        details = QStringLiteral("fenêtre non exposée");
        return false;
    }
    const bool firstRed = sameColor(window.grabWindow().pixelColor(50, 50), QColor(Qt::red));
    auto* src = rootItem->findChild<QObject*>(QStringLiteral("src"));
    auto* cache = rootItem->findChild<QObject*>(QStringLiteral("cache"));
    if (!src || !cache) {
        details = QStringLiteral("src ou cache introuvable");
        return false;
    }
    src->setProperty("color", QColor(Qt::green));
    QTest::qWait(100);
    const bool stillRed = sameColor(window.grabWindow().pixelColor(50, 50), QColor(Qt::red));
    QSignalSpy completed(cache, SIGNAL(scheduledUpdateCompleted()));
    QMetaObject::invokeMethod(cache, "scheduleUpdate");
    const bool signalled = completed.isValid() && completed.wait(2000);
    const bool nowGreen = sameColor(window.grabWindow().pixelColor(50, 50), QColor(Qt::green));
    details = QStringLiteral("premier=%1 fige=%2 signal=%3 redessin=%4")
                  .arg(firstRed)
                  .arg(stillRed)
                  .arg(signalled)
                  .arg(nowGreen);
    return firstRed && stillRed && signalled && nowGreen;
}

class TstQtVerifications : public QObject
{
    Q_OBJECT

private slots:
    void renderNodePaintsInSoftware()
    {
        QQuickWindow window;
        window.setColor(Qt::white);
        window.resize(200, 200);
        auto* item = new PainterItem;
        item->setSize(QSizeF(100, 100));
        item->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QCOMPARE(window.rendererInterface()->graphicsApi(), QSGRendererInterface::Software);
        const QImage image = window.grabWindow();
        const bool painted = sameColor(image.pixelColor(50, 50), QColor(Qt::red));
        qInfo("VERIF render-node-logiciel: %s", painted ? "oui" : "non");
        QVERIFY(painted);
        QVERIFY(sameColor(image.pixelColor(150, 150), QColor(Qt::white)));
    }

    void geometryNodeInSoftware()
    {
        QQuickWindow window;
        window.setColor(Qt::white);
        window.resize(200, 200);
        auto* item = new GeometryItem;
        item->setSize(QSizeF(100, 100));
        item->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        const QImage image = window.grabWindow();
        const bool drawn = sameColor(image.pixelColor(50, 50), QColor(Qt::green));
        qInfo("VERIF geometrie-logiciel: %s", drawn ? "oui" : "non");
    }

    void initTestCase() { qmlRegisterType<ColorPainterItem>("Verif", 1, 0, "ColorPainter"); }

    void staticLayerInSoftware()
    {
        QString details;
        const bool works = staticLayerWorks(
            "Rectangle { id: src; objectName: \"src\"; width: 100; height: 100; color: \"red\" }", details);
        qInfo("VERIF couche-logicielle: %s (%s)", works ? "oui" : "non", qPrintable(details));
    }

    void staticLayerOverRenderNodeInSoftware()
    {
        QString details;
        const bool works = staticLayerWorks(
            "ColorPainter { id: src; objectName: \"src\"; width: 100; height: 100; color: \"red\" }",
            details);
        qInfo("VERIF couche-rendernode-logiciel: %s (%s)", works ? "oui" : "non", qPrintable(details));
    }

    void tabletEventReachesQuickItem()
    {
        QQuickWindow window;
        window.resize(200, 200);
        auto* item = new TabletProbeItem;
        item->setSize(QSizeF(200, 200));
        item->setParentItem(window.contentItem());
        TabletFilter filter;
        window.installEventFilter(&filter);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        QPointingDevice stylus(QStringLiteral("verif-stylus"), 4242, QInputDevice::DeviceType::Stylus,
                               QPointingDevice::PointerType::Pen,
                               QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 3);
        const auto post = [&](QEvent::Type type, QPointF pos, qreal pressure, Qt::MouseButton button,
                              Qt::MouseButtons buttons) {
            QCoreApplication::postEvent(&window, new QTabletEvent(type, &stylus, pos, window.mapToGlobal(pos),
                                                                  pressure, 0, 0, 0, 0, 0, Qt::NoModifier,
                                                                  button, buttons));
        };
        post(QEvent::TabletPress, QPointF(50, 50), 0.7, Qt::LeftButton, Qt::LeftButton);
        post(QEvent::TabletMove, QPointF(60, 60), 0.4, Qt::NoButton, Qt::LeftButton);
        post(QEvent::TabletRelease, QPointF(60, 60), 0.0, Qt::LeftButton, Qt::NoButton);
        QTRY_COMPARE_WITH_TIMEOUT(filter.tabletEvents, 3, 2000);
        QVERIFY(qAbs(filter.firstPressure - 0.7) < 0.01);

        const bool delivered = item->tabletEvents == 3 && qAbs(item->firstPressure - 0.7) < 0.01;
        qInfo("VERIF tablette-element: %s (recus=%d pression=%.2f)", delivered ? "oui" : "non",
              item->tabletEvents, item->firstPressure);
    }
};

QTEST_MAIN(TstQtVerifications)
#include "tst_qt_verifications.moc"
