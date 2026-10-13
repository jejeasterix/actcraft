import QtQuick
import ActCraft.Ui

Window {
    id: root
    width: 1280
    height: 800
    visible: true
    // Rendu effectif dans le titre (décision 19) : aucun texte dans la scène.
    title: qsTr("ActCraft — rendu : %1").arg(root.apiName(GraphicsInfo.api))
    color: "#3c3f45"

    // Fixée avant le chargement par l'application ou le banc (--sans-cache, décision 8).
    property bool cacheEnabled: true

    PageView {
        id: view
        objectName: "pageView"
        anchors.fill: parent
        cacheEnabled: root.cacheEnabled
        content: pageContent
        fixedPage: fixedPageItem
        ink: inkLayer
        // Branché dès la création de la vue (un bloc Connections déclaré plus bas ne l'est qu'en fin de
        // composant et perdrait les premières demandes).
        onCacheRefreshRequested: cache.scheduleUpdate()

        // Page au format d'un paperboard (1024 x 768), transformée par la vue.
        Item {
            id: pageContent
            objectName: "page"
            width: 1024
            height: 768

            FixedPage {
                id: fixedPageItem
                objectName: "fixedPage"
                anchors.fill: parent
                Component.onCompleted: loadGeneratedPage()
            }

            // Cache du contenu fixe (décision 7) : redessiné seulement sur demande de la vue.
            ShaderEffectSource {
                id: cache
                objectName: "pageCache"
                anchors.fill: parent
                visible: view.cacheEnabled
                sourceItem: view.cacheEnabled ? fixedPageItem : null
                hideSource: view.cacheEnabled
                live: false
                textureSize: view.cacheTextureSize
                onScheduledUpdateCompleted: view.notifyCacheUpdated()
            }

            InkLayer {
                id: inkLayer
                objectName: "ink"
                anchors.fill: parent
            }
        }
    }

    function apiName(api) {
        switch (api) {
        case GraphicsInfo.Software: return "logiciel"
        case GraphicsInfo.Direct3D11: return "Direct3D 11"
        case GraphicsInfo.Direct3D12: return "Direct3D 12"
        case GraphicsInfo.Metal: return "Metal"
        case GraphicsInfo.Vulkan: return "Vulkan"
        case GraphicsInfo.OpenGL: return "OpenGL"
        default: return "inconnu"
        }
    }
}
