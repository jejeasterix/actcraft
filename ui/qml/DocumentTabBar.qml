import QtQuick
import ActCraft.Ui

// Talon L5-T2 : barre d'onglets sous la ligne supérieure, invisible tant qu'un seul document est ouvert. Le
// corps vient de L5-T21. Sans hauteur tant qu'elle est invisible : l'interface garde sa disposition.
Item {
    id: root
    visible: false
    height: visible ? implicitHeight : 0

    required property DocumentTabs tabs
    required property Commands commands
    signal popupOpened()
    signal popupClosed()
}
