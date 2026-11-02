import QtQuick
import QtQuick.Layouts
import ActCraft.Ui

// Fenêtre d'essai de la mise en page du joueur de scénarios (tst_bench_uiscenario) : une vue de page et un
// panneau latéral que deux commandes ouvrent et ferment, comme la palette des formes de la coquille. La vue
// change de largeur à la prochaine image, pas à la commande.
Window {
    id: root
    width: 800
    height: 600
    visible: true
    color: "#3c3f45"

    function runCommand(id, argument) {
        if (id === "panneau.ouvre")
            panneau.visible = true
        else if (id === "panneau.ferme")
            panneau.visible = false
        else
            return false
        return true
    }

    DocumentController {
        id: doc
        fixedPage: fixed
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        PageView {
            id: view
            objectName: "pageView"
            Layout.fillWidth: true
            Layout.fillHeight: true
            cacheEnabled: false
            content: page
            fixedPage: fixed
            documentController: doc
            ink: ink

            Item {
                id: page
                width: 1024
                height: 768

                FixedPage {
                    id: fixed
                    objectName: "fixedPage"
                    anchors.fill: parent
                }

                InkLayer {
                    id: ink
                    anchors.fill: parent
                }
            }
        }

        Rectangle {
            id: panneau
            objectName: "panneau"
            visible: false
            Layout.preferredWidth: 156
            Layout.fillHeight: true
            color: "#808080"
        }
    }
}
