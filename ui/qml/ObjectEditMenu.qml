import QtQuick
import ActCraft.Ui

// Talon L5-T2 : menu d'édition d'objet (commands.objectMenu), ouvert par object.menu et par le clic droit sur
// un objet. Le corps vient de L5-T18 ; le contrat du lot 4 s'applique alors : popupOpened et popupClosed sont
// émis à l'ouverture et à la fermeture (relayés par Main jusqu'à openPopups).
Item {
    id: root
    visible: false

    required property Commands commands
    required property ObjectEditing editing
    required property Tools tools
    signal popupOpened()
    signal popupClosed()

    // Ouvre le menu à la sélection (L5-T18).
    function popupAtSelection() {
    }
}
