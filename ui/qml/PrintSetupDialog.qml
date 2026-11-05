import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Boîte d'impression (F-670 à F-679) : imprimante, couleur ou gris, lignes et colonnes, cadres, numéros,
// plage, copies, marge, notes, aperçu de la feuille. Libellés à nous. Les réglages vivent dans PrintController ;
// « Enregistrer les réglages et quitter » les garde pour la prochaine ouverture. La boîte émet popupOpened et
// popupClosed (contrat du lot 4).
//
// Branchement (Main.qml) : `exchange: appExchange` et `fileExchangeActions: fileExchangeActions`. Sans
// `exchange`, « Exporter en PDF » est grisé ; sans `fileExchangeActions`, il ne propose pas de nom de fichier.
Popup {
    id: dialog
    objectName: "printSetupDialog"

    required property PrintController printing
    property var exchange: null
    property var fileExchangeActions: null // son dialogue d'export et le nom de fichier proposé
    signal popupOpened()
    signal popupClosed()

    modal: true
    anchors.centerIn: Overlay.overlay
    width: 780
    height: 640
    padding: 12
    closePolicy: Popup.CloseOnEscape

    readonly property var rangeKeys: ["all", "current", "from-to"]
    readonly property var noteKeys: ["none", "with-pages", "only"]
    property string message: ""

    function exportTo(url) {
        if (exchange)
            exchange.exportPdf(url)
    }
    // Les contrôles et le contrôleur se suivent dans les deux sens (affectations, pas de liaison : une saisie
    // ne casse pas la suite).
    function sync() {
        rowsBox.value = printing.rows
        columnsBox.value = printing.columns
        framesBox.checked = printing.frames
        numbersBox.checked = printing.pageNumbers
        rangeBox.currentIndex = Math.max(0, rangeKeys.indexOf(printing.range))
        fromBox.value = printing.from
        toBox.value = printing.to
        copiesBox.value = printing.copies
        marginBox.value = Math.round(printing.marginMm)
        colorRadio.checked = !printing.gray
        grayRadio.checked = printing.gray
        notesBox.currentIndex = Math.max(0, noteKeys.indexOf(printing.notes))
        noteBreakBox.checked = printing.noteBreak
        noteSeparatorBox.checked = printing.noteSeparator
        const at = printing.printers.indexOf(printing.printer)
        printerBox.currentIndex = at < 0 ? 0 : at + 1
    }
    onOpened: {
        message = ""
        sync()
        popupOpened()
    }
    onClosed: popupClosed()

    Connections {
        target: dialog.printing
        function onSettingsChanged() { if (dialog.opened) dialog.sync() }
        function onFailed(text) { dialog.message = text }
        function onFinished(text) { dialog.message = text }
    }

    // Fond du thème, comme GridDesigner : les libellés suivent la palette claire sur sombre de Main.qml
    // (un fond clair fixe les rendait illisibles, contraste ≈ 1,07 relevé à l'écran en D3D11).
    background: Rectangle {
        color: Theme.palette
        radius: Theme.rayon
        border.color: Theme.contour
    }

    contentItem: RowLayout {
        spacing: 16

        GridLayout {
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: 330
            columns: 2
            columnSpacing: 10
            rowSpacing: 2

            Label { text: qsTr("Imprimante") }
            ComboBox {
                id: printerBox
                objectName: "printerBox"
                Layout.fillWidth: true
                model: [qsTr("Imprimante par défaut")].concat(dialog.printing.printers)
                onActivated: dialog.printing.printer = currentIndex === 0 ? "" : dialog.printing.printers[currentIndex - 1]
            }
            Label { text: qsTr("Couleur") }
            RowLayout {
                RadioButton {
                    id: colorRadio
                    objectName: "colorRadio"
                    text: qsTr("En couleur")
                    onClicked: dialog.printing.gray = false
                }
                RadioButton {
                    id: grayRadio
                    objectName: "grayRadio"
                    text: qsTr("En gris")
                    onClicked: dialog.printing.gray = true
                }
            }
            Label { text: qsTr("Pages par feuille : lignes") }
            SpinBox {
                id: rowsBox; objectName: "rowsBox"; from: 1; to: 20; editable: true
                // Cible du clic nommé des scénarios (« tap »), qui vise le centre du bouton « plus ».
                Component.onCompleted: up.indicator.objectName = "rowsBoxPlus"
                onValueChanged: if (dialog.printing.rows !== value) dialog.printing.rows = value
            }
            Label { text: qsTr("Pages par feuille : colonnes") }
            SpinBox {
                id: columnsBox; objectName: "columnsBox"; from: 1; to: 20; editable: true
                onValueChanged: if (dialog.printing.columns !== value) dialog.printing.columns = value
            }
            Label { text: qsTr("Cadres") }
            CheckBox {
                id: framesBox; objectName: "framesBox"; text: qsTr("Entourer chaque page")
                onToggled: dialog.printing.frames = checked
            }
            Label { text: qsTr("Numéros") }
            CheckBox {
                id: numbersBox; objectName: "numbersBox"; text: qsTr("Numéroter les pages")
                onToggled: dialog.printing.pageNumbers = checked
            }
            Label { text: qsTr("Plage") }
            ComboBox {
                id: rangeBox
                objectName: "rangeBox"
                Layout.fillWidth: true
                model: [qsTr("Toutes les pages"), qsTr("Page affichée"), qsTr("De… à…")]
                onActivated: dialog.printing.range = dialog.rangeKeys[currentIndex]
            }
            Label { text: qsTr("De la page"); enabled: rangeBox.currentIndex === 2 }
            SpinBox {
                id: fromBox; objectName: "fromBox"; from: 1; to: 9999; editable: true
                // Cible du clic nommé des scénarios (« tap »), qui vise le centre du bouton « plus ».
                Component.onCompleted: up.indicator.objectName = "fromBoxPlus"
                enabled: rangeBox.currentIndex === 2
                onValueChanged: if (dialog.printing.from !== value) dialog.printing.from = value
            }
            Label { text: qsTr("à la page"); enabled: rangeBox.currentIndex === 2 }
            SpinBox {
                id: toBox; objectName: "toBox"; from: 1; to: 9999; editable: true
                // Cible du clic nommé des scénarios (« tap »), qui vise le centre du bouton « plus ».
                Component.onCompleted: up.indicator.objectName = "toBoxPlus"
                enabled: rangeBox.currentIndex === 2
                onValueChanged: if (dialog.printing.to !== value) dialog.printing.to = value
            }
            Label { text: qsTr("Copies") }
            SpinBox {
                id: copiesBox; objectName: "copiesBox"; from: 1; to: 999; editable: true
                onValueChanged: if (dialog.printing.copies !== value) dialog.printing.copies = value
            }
            Label { text: qsTr("Marge (mm)") }
            SpinBox {
                id: marginBox; objectName: "marginBox"; from: 0; to: 100; editable: true
                onValueChanged: if (Math.round(dialog.printing.marginMm) !== value) dialog.printing.marginMm = value
            }
            Label { text: qsTr("Notes") }
            ComboBox {
                id: notesBox
                objectName: "notesBox"
                Layout.fillWidth: true
                model: [qsTr("Sans les notes"), qsTr("Avec les pages"), qsTr("Notes seules")]
                onActivated: dialog.printing.notes = dialog.noteKeys[currentIndex]
            }
            ColumnLayout {
                Layout.columnSpan: 2
                CheckBox {
                    id: noteBreakBox; objectName: "noteBreakBox"; text: qsTr("Une feuille par page")
                    enabled: notesBox.currentIndex !== 0
                    onToggled: dialog.printing.noteBreak = checked
                }
                CheckBox {
                    id: noteSeparatorBox; objectName: "noteSeparatorBox"; text: qsTr("Trait entre les pages")
                    enabled: notesBox.currentIndex !== 0
                    onToggled: dialog.printing.noteSeparator = checked
                }
            }
            Label {
                Layout.columnSpan: 2
                Layout.fillWidth: true
                objectName: "messageLabel"
                text: dialog.message
                wrapMode: Text.Wrap
                color: Theme.texteDiscret
            }
            Item { Layout.fillHeight: true; Layout.columnSpan: 2 }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            SheetPreview {
                Layout.fillWidth: true
                Layout.fillHeight: true
                printing: dialog.printing
                visible: true
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                Button {
                    objectName: "exportButton"
                    text: qsTr("Exporter en PDF…")
                    enabled: dialog.exchange !== null
                    onClicked: if (dialog.fileExchangeActions) dialog.fileExchangeActions.chooseExportPdf()
                }
                Button {
                    objectName: "printButton"
                    text: qsTr("Imprimer")
                    highlighted: true
                    onClicked: {
                        // Avec ou sans QPrinter : les feuilles des réglages (imprimante, ou PDF ouvert).
                        if (dialog.printing.print(""))
                            dialog.close()
                    }
                }
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                Button {
                    objectName: "saveAndQuitButton"
                    text: qsTr("Enregistrer les réglages et quitter")
                    onClicked: {
                        dialog.printing.saveSettings()
                        dialog.close()
                    }
                }
                Button {
                    objectName: "cancelButton"
                    text: qsTr("Annuler")
                    onClicked: dialog.close()
                }
            }
        }
    }
}
