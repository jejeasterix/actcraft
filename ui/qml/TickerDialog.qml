import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import ActCraft.Ui

// Fenêtre d'édition du message défilant (F-291, L7-T19) : boîte flottante (FloatingPanel : couverture de la
// page). Chaque réglage (texte, position, boucle, vitesse, police, taille, ombre, couleurs, fond) s'applique au
// contrôleur dès qu'il est modifié. « Essayer » joue le message sans rien garder ; « Appliquer » garde les
// réglages comme défaut (QSettings, tools/ticker) et ferme la boîte ; « Pause » fige ou reprend le défilement ;
// « Ouvrir » et « Enregistrer » lisent et écrivent un fichier de message. Le dialogue de fichiers ne fait que
// donner une URL : le contrôleur le demande (chooseOpen, chooseSave) et lit ou écrit lui-même ; un choix injecté
// (setFileChooser) le remplace dans les tests. Le texte du message est du texte brut. Ouverte par open().
FloatingPanel {
    id: root
    property TickerController controller: null
    readonly property int rowHeight: 40
    readonly property var swatchColors: ["#1F2937", "#000000", "#FFFFFF", "#DC2626", "#2563EB", "#16A34A",
                                         "#F59E0B", "#7C3AED"]
    readonly property var families: [qsTr("Police de l'interface")].concat(Qt.fontFamilies())

    title: qsTr("Message défilant")
    visible: false
    width: 400
    height: Theme.tailleBouton + column.implicitHeight + 2 * Theme.espace2
    x: parent ? Math.max(0, parent.width - width - 24) : 0
    y: 24

    // Remplit les contrôles depuis le contrôleur (aucune liaison : une saisie ne les défait pas).
    function load() {
        const c = root.controller
        if (!c) {
            return
        }
        textField.text = c.text
        positionBox.currentIndex = c.position
        loopBox.checked = c.loop
        speedBox.value = c.speed
        fontBox.currentIndex = Math.max(0, root.families.indexOf(c.fontFamily))
        sizeBox.value = Math.round(c.fontPointSize)
        shadowBox.value = c.shadowOffset
        screenBox.checked = c.screenBackground
        textColors.current = c.textColor
        shadowColors.current = c.shadowColor
        backgroundColors.current = c.backgroundColor
    }

    function open() {
        root.visible = true
        root.load()
        textField.forceActiveFocus()
        textField.selectAll()
    }

    // Garde les réglages comme défaut et ferme la boîte.
    function apply() {
        if (root.controller) {
            root.controller.keepAsDefault()
        }
        root.close()
    }

    // Joue le message avec les réglages courants, sans rien garder.
    function tryOut() {
        if (root.controller) {
            root.controller.start()
        }
    }

    Connections {
        target: root.controller
        function onSettingsChanged() {
            if (root.visible) {
                root.load()
            }
        }
        // Un dialogue natif est demandé seulement si aucun choix n'est injecté.
        function onFileDialogRequested(save) {
            if (save) {
                saveChooser.open()
            } else {
                openChooser.open()
            }
        }
    }

    FileDialog {
        id: openChooser
        title: qsTr("Ouvrir un message")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Messages ActCraft (*.json)"), qsTr("Tous les fichiers (*)")]
        onAccepted: if (root.controller) root.controller.openUrl(selectedFile)
    }
    FileDialog {
        id: saveChooser
        title: qsTr("Enregistrer le message sous")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: [qsTr("Messages ActCraft (*.json)")]
        onAccepted: if (root.controller) root.controller.saveToUrl(selectedFile)
    }

    // Une rangée de couleurs à choisir.
    component Swatches: Row {
        id: swatches
        property color current
        property string tag
        signal picked(color value)
        spacing: Theme.espace1
        Repeater {
            model: root.swatchColors
            delegate: Rectangle {
                required property int index
                required property string modelData
                objectName: swatches.tag + index
                width: 30
                height: 30
                radius: 4
                color: modelData
                border.width: Qt.colorEqual(modelData, swatches.current) ? 3 : 1
                border.color: Qt.colorEqual(modelData, swatches.current) ? Theme.surfaceActive : Theme.contour
                MouseArea {
                    anchors.fill: parent
                    onClicked: swatches.picked(parent.modelData)
                }
            }
        }
    }

    component Caption: Label {
        width: 100
        height: root.rowHeight
        verticalAlignment: Text.AlignVCenter
        textFormat: Text.PlainText
        color: Theme.texte
        font.pixelSize: Theme.taillePolice
    }

    Column {
        id: column
        width: parent ? parent.width : 0
        spacing: Theme.espace2

        Row {
            spacing: Theme.espace2
            Caption { text: qsTr("Message") }
            TextField {
                id: textField
                objectName: "tickerTextField"
                width: column.width - 100 - Theme.espace2
                height: root.rowHeight
                color: Theme.texte
                font.pixelSize: Theme.taillePolice
                selectByMouse: true
                background: Rectangle {
                    radius: Theme.rayon
                    color: Theme.surface
                    border.color: textField.activeFocus ? Theme.surfaceActive : Theme.contour
                    border.width: textField.activeFocus ? 2 : 1
                }
                KeyNavigation.tab: positionBox
                // Un texte que le contrôleur refuse (trop long) est remplacé par celui qu'il garde.
                onTextEdited: {
                    if (root.controller) {
                        root.controller.text = text
                        if (root.controller.text !== text) {
                            text = root.controller.text
                        }
                    }
                }
                onAccepted: root.tryOut()
                Keys.onEscapePressed: root.close()
            }
        }

        Row {
            spacing: Theme.espace2
            Caption { text: qsTr("Position") }
            ComboBox {
                id: positionBox
                objectName: "tickerPosition"
                width: 130
                height: root.rowHeight
                model: [qsTr("Haut"), qsTr("Centre"), qsTr("Bas")]
                font.pixelSize: Theme.taillePolice
                KeyNavigation.tab: loopBox
                KeyNavigation.backtab: textField
                onActivated: if (root.controller) root.controller.position = currentIndex
            }
            CheckBox {
                id: loopBox
                objectName: "tickerLoop"
                height: root.rowHeight
                text: qsTr("En boucle")
                font.pixelSize: Theme.taillePolice
                palette.windowText: Theme.texte
                KeyNavigation.tab: speedBox
                KeyNavigation.backtab: positionBox
                onToggled: if (root.controller) root.controller.loop = checked
            }
        }

        Row {
            spacing: Theme.espace2
            Caption { text: qsTr("Vitesse") }
            SpinBox {
                id: speedBox
                objectName: "tickerSpeed"
                height: root.rowHeight
                from: 1
                to: 10
                font.pixelSize: Theme.taillePolice
                KeyNavigation.tab: fontBox
                KeyNavigation.backtab: loopBox
                onValueModified: if (root.controller) root.controller.speed = value
            }
        }

        Row {
            spacing: Theme.espace2
            Caption { text: qsTr("Police") }
            ComboBox {
                id: fontBox
                objectName: "tickerFont"
                width: column.width - 100 - Theme.espace2
                height: root.rowHeight
                model: root.families
                font.pixelSize: Theme.taillePolice
                KeyNavigation.tab: sizeBox
                KeyNavigation.backtab: speedBox
                onActivated: if (root.controller) root.controller.fontFamily = currentIndex === 0 ? "" : currentText
            }
        }

        Row {
            spacing: Theme.espace2
            Caption { text: qsTr("Taille") }
            SpinBox {
                id: sizeBox
                objectName: "tickerSize"
                height: root.rowHeight
                from: 8
                to: 200
                font.pixelSize: Theme.taillePolice
                KeyNavigation.tab: shadowBox
                KeyNavigation.backtab: fontBox
                onValueModified: if (root.controller) root.controller.fontPointSize = value
            }
        }

        Row {
            spacing: Theme.espace2
            Caption { text: qsTr("Ombre (px)") }
            SpinBox {
                id: shadowBox
                objectName: "tickerShadowOffset"
                height: root.rowHeight
                from: 0
                to: 20
                font.pixelSize: Theme.taillePolice
                KeyNavigation.tab: screenBox
                KeyNavigation.backtab: sizeBox
                onValueModified: if (root.controller) root.controller.shadowOffset = value
            }
        }

        Row {
            spacing: Theme.espace2
            Caption { text: qsTr("Texte") }
            Swatches {
                id: textColors
                tag: "tickerTextColor"
                anchors.verticalCenter: parent.verticalCenter
                onPicked: value => { if (root.controller) root.controller.textColor = value }
            }
        }

        Row {
            spacing: Theme.espace2
            Caption { text: qsTr("Ombre") }
            Swatches {
                id: shadowColors
                tag: "tickerShadowColor"
                anchors.verticalCenter: parent.verticalCenter
                onPicked: value => { if (root.controller) root.controller.shadowColor = value }
            }
        }

        CheckBox {
            id: screenBox
            objectName: "tickerScreenBackground"
            width: parent.width
            height: root.rowHeight
            text: qsTr("Fond : l'écran (transparent)")
            font.pixelSize: Theme.taillePolice
            palette.windowText: Theme.texte
            KeyNavigation.tab: tryButton
            KeyNavigation.backtab: shadowBox
            onToggled: if (root.controller) root.controller.screenBackground = checked
        }

        Row {
            spacing: Theme.espace2
            enabled: !screenBox.checked
            opacity: enabled ? 1 : 0.4
            Caption { text: qsTr("Fond") }
            Swatches {
                id: backgroundColors
                tag: "tickerBackgroundColor"
                anchors.verticalCenter: parent.verticalCenter
                onPicked: value => { if (root.controller) root.controller.backgroundColor = value }
            }
        }

        Row {
            spacing: Theme.espace2
            Button {
                id: tryButton
                objectName: "tickerTry"
                width: (column.width - 2 * Theme.espace2) / 3
                height: root.rowHeight
                text: qsTr("Essayer")
                KeyNavigation.tab: applyButton
                KeyNavigation.backtab: screenBox
                onClicked: root.tryOut()
            }
            Button {
                id: applyButton
                objectName: "tickerApply"
                width: (column.width - 2 * Theme.espace2) / 3
                height: root.rowHeight
                text: qsTr("Appliquer")
                KeyNavigation.tab: pauseButton
                KeyNavigation.backtab: tryButton
                onClicked: root.apply()
            }
            Button {
                id: pauseButton
                objectName: "tickerPause"
                width: (column.width - 2 * Theme.espace2) / 3
                height: root.rowHeight
                text: root.controller && root.controller.paused ? qsTr("Reprendre") : qsTr("Pause")
                enabled: root.controller ? root.controller.running : false
                KeyNavigation.tab: openButton
                KeyNavigation.backtab: applyButton
                onClicked: if (root.controller) root.controller.togglePause()
            }
        }

        Row {
            spacing: Theme.espace2
            Button {
                id: openButton
                objectName: "tickerOpenFile"
                width: (column.width - Theme.espace2) / 2
                height: root.rowHeight
                text: qsTr("Ouvrir un message…")
                KeyNavigation.tab: saveButton
                KeyNavigation.backtab: pauseButton
                onClicked: if (root.controller) root.controller.chooseOpen()
            }
            Button {
                id: saveButton
                objectName: "tickerSaveFile"
                width: (column.width - Theme.espace2) / 2
                height: root.rowHeight
                text: qsTr("Enregistrer le message…")
                KeyNavigation.backtab: openButton
                onClicked: if (root.controller) root.controller.chooseSave()
            }
        }

        Label {
            id: fileError
            objectName: "tickerFileError"
            width: parent.width
            visible: text.length > 0
            textFormat: Text.PlainText
            wrapMode: Text.WordWrap
            color: "#E57373"
            font.pixelSize: Theme.taillePolice
            Connections {
                target: root.controller
                function onFileFinished(ok, message) {
                    fileError.text = ok ? "" : message
                }
            }
        }
    }
}
