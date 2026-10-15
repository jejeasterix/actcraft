import QtQuick
import ActCraft.Ui

// Fond d'une entrée de menu ou de la liste du zoom (L1-C5). Surlignée (survol, flèches du clavier), appuyée ou
// avec le focus : fond « surfaceSurvol » et filet d'accent de 2 px. Le fond seul ne se détache du menu qu'à
// 1,48:1 ; le filet, à 6,30:1, porte l'état (WCAG 1.4.11). Le texte reste clair sur le fond sombre.
Rectangle {
    required property var control // MenuItem ou ItemDelegate
    readonly property bool lit: control.highlighted || control.down || control.visualFocus
    x: 1
    y: 1
    width: control.width - 2
    height: control.height - 2
    radius: Theme.rayon
    color: lit ? Theme.surfaceSurvol : "transparent"
    border.width: lit ? 2 : 0
    border.color: Theme.surfaceActive
}
