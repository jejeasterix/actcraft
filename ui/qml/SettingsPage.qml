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
    // Sections du poste : `platforms` vide vaut toutes les plateformes.
    readonly property var shownSections: schema.sections.filter(function (section) {
        return section.platforms.length === 0 || section.platforms.indexOf(Qt.platform.os) >= 0
    })
    property string currentSection: shownSections.length > 0 ? shownSections[0].id : ""
    // Les éditeurs sont recréés quand le profil courant change, pas à chaque écriture d'un réglage.
    readonly property string profileId: root.profile ? root.profile.currentId : ""
    visible: false
    implicitWidth: 760
    implicitHeight: 480

    SettingsSchema {
        id: schema
        objectName: "settingsSchema"
        profile: root.profile
        preferences: root.preferences
    }

    Component.onCompleted: {
        // Talon L6 : rempli par L6-T36 (réglage de la langue).
        if (root.commands)
            root.commands.setHandler("settings.language", function () { console.info("settings.language") })
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 12

        ListView {
            objectName: "settingsSections"
            Layout.preferredWidth: 200
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
                            const profileId = root.profileId // dépendance : recréés au changement de profil
                            if (section === "" || profileId === undefined)
                                return []
                            return schema.entries(section).map(function (entry) {
                                return Object.assign({ section: section }, entry)
                            })
                        }
                        delegate: RowLayout {
                            id: row
                            required property var modelData
                            readonly property var current: schema.value(row.modelData.section, row.modelData.key)
                            Layout.fillWidth: true
                            spacing: 12

                            Label {
                                text: row.modelData.label
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                            Loader {
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
                                        model: row.modelData.choices
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
                                        implicitWidth: 260
                                        text: row.current
                                        onEditingFinished: schema.setValue(row.modelData.section, row.modelData.key, text)
                                    }
                                }
                            }
                        }
                    }

                    Label {
                        visible: entryRepeater.count === 0
                        text: qsTr("Cette section n'a pas encore de réglage.")
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
}
