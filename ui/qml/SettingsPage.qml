import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Onglet paramètres de l'éditeur de profils (F-930, F-935) : sections à gauche, réglages à droite, un éditeur par
// type. Le schéma lit root.profile et root.preferences (C2), jamais un identifiant de Main.qml. La page réseau
// (L6-T28) est construite avec cette page, sans Loader paresseux : ses réglages existent dès le démarrage.
Item {
    id: root
    objectName: "settingsPage"
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property PageView view: null
    property var profile: null
    property var preferences: null
    // Sections du poste (`platforms` vide : toutes) qui ont un réglage ; le réseau est édité par NetworkPage.
    readonly property var shownSections: schema.sections.filter(function (section) {
        const onThisPlatform = section.platforms.length === 0 || section.platforms.indexOf(Qt.platform.os) >= 0
        return onThisPlatform && (section.id === "network" || schema.entries(section.id).length > 0)
    })
    property string currentSection: shownSections.length > 0 ? shownSections[0].id : ""
    // Incrémentée à chaque changement des magasins : un éditeur relit sa valeur après une écriture, un
    // changement de profil ou « Revenir à l'origine » (le profil rechargé sous le même identifiant).
    property int revision: 0
    // Aucune taille implicite : le StackLayout de ProfileEditor.qml donne la place (comme CommandsPage.qml,
    // L6-T26), sans faire déborder les autres pages.
    visible: false

    SettingsSchema {
        id: schema
        objectName: "settingsSchema"
        profile: root.profile
        preferences: root.preferences
    }

    Connections {
        target: root.profile
        function onProfileChanged() { root.revision++ }
    }
    Connections {
        target: root.preferences
        function onChanged() { root.revision++ }
    }

    // Langue (L6-T36) : le réglage s'applique au redémarrage ; un changement montre le message, rien n'est rechargé.
    Language {
        id: language
        objectName: "language"
        preferences: root.preferences
    }

    Label {
        objectName: "restartMessage"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 8
        visible: language.restartPending
        wrapMode: Text.WordWrap
        text: qsTr("La langue choisie sera appliquée au prochain démarrage d'ActCraft.")
    }

    Component.onCompleted: {
        if (root.commands)
            root.commands.setHandler("settings.language", function (code) { language.change(code ? String(code) : "") })
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 12

        ListView {
            objectName: "settingsSections"
            Layout.preferredWidth: Math.min(200, root.width / 3)
            Layout.fillHeight: true
            clip: true
            model: root.shownSections
            delegate: ItemDelegate {
                required property var modelData
                objectName: "settingsSection-" + modelData.id
                width: ListView.view.width
                text: modelData.label
                highlighted: modelData.id === root.currentSection
                onClicked: root.currentSection = modelData.id
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            NetworkPage {
                id: networkPage
                anchors.left: parent.left
                anchors.right: parent.right
                visible: root.currentSection === "network"
                commands: root.commands
                documentController: root.documentController
                tools: root.tools
                view: root.view
                profile: root.profile
                preferences: root.preferences
            }

            // Recherche de mises à jour au démarrage (F-930, D11-13) : réglage du poste, Windows seulement, faux par
            // défaut. Cette instance ne fait aucune requête : elle lit et écrit `updates/check-at-start`, que lit la
            // coquille (appUpdates) au prochain démarrage. Rangé sous la page réseau : la recherche suit son mandataire.
            UpdateChecker {
                id: updateSetting
                objectName: "updateSetting"
                preferences: root.preferences
            }
            CheckBox {
                objectName: "setting-updates-check-at-start"
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: networkPage.bottom
                anchors.topMargin: 8
                visible: root.currentSection === "network" && Qt.platform.os === "windows"
                text: qsTr("Rechercher les mises à jour au démarrage")
                checked: updateSetting.checkAtStart
                onToggled: updateSetting.checkAtStart = checked
            }

            ScrollView {
                id: entriesView
                anchors.fill: parent
                visible: root.currentSection !== "network"
                clip: true

                ColumnLayout {
                    width: entriesView.availableWidth
                    spacing: 8

                    Repeater {
                        id: entryRepeater
                        // Chaque réglage porte sa section : un délégué ne relit jamais la section suivante.
                        model: {
                            const section = root.currentSection
                            if (section === "")
                                return []
                            return schema.entries(section).map(function (entry) {
                                return Object.assign({ section: section }, entry)
                            })
                        }
                        delegate: RowLayout {
                            id: row
                            required property var modelData
                            readonly property var current: {
                                const revision = root.revision // dépendance : relu à chaque changement
                                return schema.value(row.modelData.section, row.modelData.key)
                            }
                            Layout.fillWidth: true
                            spacing: 12

                            Label {
                                text: row.modelData.label
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                            Loader {
                                // Un champ de texte prend au plus la moitié de la ligne : le libellé reste lisible.
                                Layout.preferredWidth: row.modelData.type === "text" ? Math.min(260, entriesView.availableWidth / 2) : -1
                                sourceComponent: row.modelData.type === "bool" ? boolEditor
                                                 : row.modelData.type === "int" ? intEditor
                                                 : row.modelData.type === "choice" ? choiceEditor : textEditor

                                Component {
                                    id: boolEditor
                                    CheckBox {
                                        objectName: "setting-" + row.modelData.key
                                        checked: row.current
                                        onToggled: schema.setValue(row.modelData.section, row.modelData.key, checked)
                                    }
                                }
                                Component {
                                    id: intEditor
                                    SpinBox {
                                        objectName: "setting-" + row.modelData.key
                                        from: row.modelData.min
                                        to: row.modelData.max
                                        value: row.current
                                        editable: true
                                        onValueModified: schema.setValue(row.modelData.section, row.modelData.key, value)
                                    }
                                }
                                Component {
                                    id: choiceEditor
                                    ComboBox {
                                        objectName: "setting-" + row.modelData.key
                                        model: row.modelData.choiceLabels
                                        currentIndex: row.modelData.choices.indexOf(row.current)
                                        onActivated: function (index) {
                                            schema.setValue(row.modelData.section, row.modelData.key,
                                                            row.modelData.choices[index])
                                        }
                                    }
                                }
                                Component {
                                    id: textEditor
                                    TextField {
                                        objectName: "setting-" + row.modelData.key
                                        text: row.current
                                        // Un simple passage du focus n'écrit rien (aucune clé du poste créée vide).
                                        onEditingFinished: {
                                            if (text !== row.current)
                                                schema.setValue(row.modelData.section, row.modelData.key, text)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
