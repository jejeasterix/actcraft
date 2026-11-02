import QtQuick
import ActCraft.Ui

// Talon L5-T2 : boîte d'impression (Windows). Le corps vient de L5-T28 ; la boîte émettra popupOpened et
// popupClosed (contrat du lot 4).
Item {
    id: root
    visible: false

    required property PrintController printing
    signal popupOpened()
    signal popupClosed()

    function open() {
    }
}
