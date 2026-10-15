import QtQuick
import ActCraft.Ui

// Fenêtre d'essai du joueur de scénarios (tst_bench_uiscenario) : runCommand, raccourci F5, page vierge réelle.
Window {
    id: root
    width: 800
    height: 600
    visible: true
    color: "#3c3f45" // fond de la coquille : une capture de page vierge n'est jamais uniforme
    property string dernier: ""
    property int compteur: 0

    // Même contrat que la coquille : vrai si la commande existe et a été jouée.
    function runCommand(id, argument) {
        if (id === "inconnue")
            return false
        root.dernier = argument === undefined ? id : id + ":" + argument
        root.compteur += 1
        return true
    }

    Shortcut {
        sequence: "F5"
        context: Qt.ApplicationShortcut
        onActivated: root.compteur += 10
    }

    // F7 en double : raccourci ambigu, rien n'est joué.
    Shortcut {
        sequence: "F7"
        context: Qt.ApplicationShortcut
        onActivated: root.compteur += 100
    }
    Shortcut {
        sequence: "F7"
        context: Qt.ApplicationShortcut
        onActivated: root.compteur += 100
    }

    // Piège : un élément qui a le focus et ignore les touches. Le recevoir n'est pas l'accepter.
    Item {
        objectName: "piege"
        focus: true
    }

    DocumentController {
        fixedPage: fixed
    }

    PageView {
        id: view
        objectName: "pageView"
        anchors.fill: parent
        cacheEnabled: false
        content: page
        fixedPage: fixed
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
}
