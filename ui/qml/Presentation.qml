import QtQuick
import ActCraft.Ui

// Racine des fonctions du lot 7 : une racine par fonction (contrôleur, surcouche ou panneau, gestionnaires de
// commandes). Une surcouche qui prend l'entrée se déclare couverture : pageArea.view.addInputCover(<item>).
// Seul le parent est posé ici : chaque racine décide de sa géométrie. Une fonction du lot 7 ajoute sa racine ici, à la suite des autres.
// Racine QtObject et non Item : la propriété `layer` figée par le socle est une propriété FINAL de Item. Les racines
// sont déclarées dans `data` et posées par `parent:` ; jamais enfants visuels de cette racine.
QtObject {
    id: presentation
    default property list<QtObject> data
    property Commands commands: null
    property DocumentController documentController: null
    property Tools tools: null
    property Item pageArea: null
    property Item pageOverlays: null
    property Item layer: null
    // Contrôleur des autres outils (navigateur web…), pour le bandeau de Main.qml.
    readonly property QtObject otherToolsController: otherToolsRoot.controller

    // Racines assignées par nom : une propriété par défaut déclarée dans ce fichier ne sert qu'aux objets
    // déclarés là où le type est utilisé (Main.qml), pas aux enfants écrits ici (Qt : « Cannot assign to
    // non-existent default property »).
    data: [
        CurtainOverlay { id: curtainRoot; objectName: "curtainRoot"; z: 10; parent: presentation.pageOverlays
            commands: presentation.commands; documentController: presentation.documentController
            tools: presentation.tools; pageArea: presentation.pageArea },
        SpotlightOverlay { id: spotlightRoot; objectName: "spotlightRoot"; z: 20; parent: presentation.pageOverlays
            commands: presentation.commands; documentController: presentation.documentController
            tools: presentation.tools; pageArea: presentation.pageArea },
        PageZoomOverlay { id: pageZoomRoot; objectName: "pageZoomRoot"
            parent: presentation.pageArea ? presentation.pageArea.presentationSlot : null
            commands: presentation.commands; documentController: presentation.documentController
            tools: presentation.tools; pageArea: presentation.pageArea },
        PerPageTools { id: pageToolsRoot; objectName: "pageToolsRoot"
            commands: presentation.commands; documentController: presentation.documentController
            tools: presentation.tools; pageArea: presentation.pageArea
            curtain: curtainRoot.controller; spotlight: spotlightRoot.controller },
        TransitionLayer { id: transitionRoot; objectName: "transitionRoot"; z: 10; parent: presentation.layer
            commands: presentation.commands; documentController: presentation.documentController
            tools: presentation.tools; pageArea: presentation.pageArea; curtain: curtainRoot.controller
            spotlight: spotlightRoot.controller; desktop: desktopRoot.controller },
        PageEffectsPicker { id: effectsRoot; objectName: "effectsRoot"; z: 50; parent: presentation.layer
            commands: presentation.commands; documentController: presentation.documentController
            tools: presentation.tools; pageArea: presentation.pageArea; transitions: transitionRoot.controller },
        MathToolsLayer { id: mathToolsRoot; objectName: "mathToolsRoot"; z: 30; parent: presentation.pageOverlays
            commands: presentation.commands; documentController: presentation.documentController
            tools: presentation.tools; pageArea: presentation.pageArea },
        DicePanel { id: diceRoot; objectName: "diceRoot"; z: 30; parent: presentation.layer
            commands: presentation.commands; documentController: presentation.documentController
            tools: presentation.tools; pageArea: presentation.pageArea },
        CalculatorPanel { id: calculatorRoot; objectName: "calculatorRoot"; z: 31; parent: presentation.layer
            commands: presentation.commands; documentController: presentation.documentController
            tools: presentation.tools; pageArea: presentation.pageArea },
        ClockPanel { id: clockRoot; objectName: "clockRoot"; z: 32; parent: presentation.layer
            commands: presentation.commands; documentController: presentation.documentController
            tools: presentation.tools; pageArea: presentation.pageArea },
        TickerOverlay { id: tickerRoot; objectName: "tickerRoot"; z: 20; parent: presentation.layer
            commands: presentation.commands; documentController: presentation.documentController
            tools: presentation.tools; pageArea: presentation.pageArea },
        RecognitionAlternatives { id: recognitionRoot; objectName: "recognitionRoot"; z: 40; parent: presentation.layer
            commands: presentation.commands; documentController: presentation.documentController
            tools: presentation.tools; pageArea: presentation.pageArea },
        EquationEditor { id: equationRoot; objectName: "equationRoot"; z: 41; parent: presentation.layer
            commands: presentation.commands; documentController: presentation.documentController
            tools: presentation.tools; pageArea: presentation.pageArea },
        CameraOverlay { id: cameraRoot; objectName: "cameraRoot"
            commands: presentation.commands; documentController: presentation.documentController
            tools: presentation.tools; pageArea: presentation.pageArea },
        ScreenKeyboard { id: keyboardRoot; objectName: "keyboardRoot"
            commands: presentation.commands; documentController: presentation.documentController
            tools: presentation.tools; pageArea: presentation.pageArea },
        DesktopAnnotation { id: desktopRoot; objectName: "desktopRoot"
            commands: presentation.commands; documentController: presentation.documentController
            tools: presentation.tools; pageArea: presentation.pageArea },
        OtherToolsHost { id: otherToolsRoot; objectName: "otherToolsRoot"
            commands: presentation.commands; documentController: presentation.documentController
            tools: presentation.tools; pageArea: presentation.pageArea }
    ]
}
