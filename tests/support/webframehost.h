#pragma once

#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/webframeitem.h>

#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTest>
#include <QUrl>

#include <memory>

// WebFrameLayer dans une fenêtre hors écran, sans Main.qml. ACTCRAFT_SOURCE_DIR : posé par tests/ui.cmake.
struct WebFrameHost
{
    QQmlEngine engine;
    QQuickWindow window;
    actcraft::ui::DocumentController document;
    std::unique_ptr<QQuickItem> root;
    QQuickItem* layer = nullptr;
    actcraft::ui::WebFrameItems* frames = nullptr;

    // `commands` : posées avant la fin de la création, comme Main.qml les donne à la couche.
    bool start(actcraft::ui::Commands* commands = nullptr)
    {
        document.newDocument(1024, 768);
        QQmlComponent component(
            &engine, QUrl::fromLocalFile(QStringLiteral(ACTCRAFT_SOURCE_DIR "/tests/qml/WebFrameHost.qml")));
        root.reset(qobject_cast<QQuickItem*>(component.beginCreate(engine.rootContext())));
        if (root && commands)
            root->setProperty("commands", QVariant::fromValue(commands));
        if (root)
            component.completeCreate();
        if (!root) {
            qWarning() << component.errors();
            return false;
        }
        root->setProperty("documentController", QVariant::fromValue(&document));
        window.resize(1024, 768);
        root->setParentItem(window.contentItem());
        window.show();
        if (!QTest::qWaitForWindowExposed(&window))
            return false;
        layer = root->findChild<QQuickItem*>(QStringLiteral("webFrameLayer"));
        frames =
            layer ? layer->findChild<actcraft::ui::WebFrameItems*>(QStringLiteral("appWebFrames")) : nullptr;
        return frames != nullptr;
    }
    void useFakeView()
    {
        layer->setProperty("viewSource", QUrl::fromLocalFile(QStringLiteral(
                                             ACTCRAFT_SOURCE_DIR "/tests/qml/FakeWebFrameView.qml")));
    }
};
