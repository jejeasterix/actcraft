# Lot 7 (présentation et outils) : sources QML et C++ du lot, lues par ui/CMakeLists.txt (crochet du socle L0-S0,
# avant qt_add_qml_module). Ce fichier ne fait que remplir les deux listes. Presentation.qml est déclaré par le
# socle : ne pas le relister ici.
set(ACTCRAFT_UI_LOT7_QML
    qml/FloatingPanel.qml
    qml/CurtainOverlay.qml
    qml/SpotlightOverlay.qml
    qml/PageZoomOverlay.qml
    qml/PerPageTools.qml
    qml/TransitionLayer.qml
    qml/PageEffectsPicker.qml
    qml/MathToolsLayer.qml
    qml/MathToolZone.qml
    qml/MathToolMenuButton.qml
    qml/RulerTool.qml
    qml/SetSquareTool.qml
    qml/ProtractorTool.qml
    qml/CompassTool.qml
    qml/OriginMarker.qml
    qml/DicePanel.qml
    qml/CalculatorPanel.qml
    qml/ClockPanel.qml
    qml/CountdownDialog.qml
    qml/OtherToolsHost.qml
    qml/TickerOverlay.qml
    qml/TickerDialog.qml
    qml/RecognitionAlternatives.qml
    qml/EquationEditor.qml
    qml/CameraOverlay.qml
    qml/CameraPalette.qml
    qml/ScreenKeyboard.qml
    qml/DesktopAnnotation.qml
    qml/DesktopToolsWheel.qml)
# Pas de QT_QML_SKIP_CACHEGEN : le socle tait C4702 dans le C++ de qmlcachegen de tous les QML du module, ceux du
# lot 7 compris (ui/CMakeLists.txt, bloc « MSVC, C4702 »).

# Sources C++ du lot par noms exacts : chaque tâche crée son contrôleur, CMake le voit à la construction suivante
# (CONFIGURE_DEPENDS) ; aucun .cpp talon vide (LNK4221). Seuls ces noms sont pris, jamais un fichier du socle.
file(GLOB ACTCRAFT_UI_LOT7_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/presentationdocument.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/presentationdocument.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/curtaincontroller.*
    ${CMAKE_CURRENT_SOURCE_DIR}/src/spotlightcontroller.*
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pagezoomcontroller.*
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pagetoolscontroller.*
    ${CMAKE_CURRENT_SOURCE_DIR}/src/transitioncontroller.*
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mathtoolscontroller.*
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dicecontroller.*
    ${CMAKE_CURRENT_SOURCE_DIR}/src/calculatorcontroller.*
    ${CMAKE_CURRENT_SOURCE_DIR}/src/clockcontroller.*
    ${CMAKE_CURRENT_SOURCE_DIR}/src/presentationcommands.*
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tickercontroller.*
    ${CMAKE_CURRENT_SOURCE_DIR}/src/recognitioncontroller.*
    ${CMAKE_CURRENT_SOURCE_DIR}/src/equationcontroller.*
    ${CMAKE_CURRENT_SOURCE_DIR}/src/equationpreviewitem.*
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cameracontroller.*
    ${CMAKE_CURRENT_SOURCE_DIR}/src/keyboardcontroller.*
    ${CMAKE_CURRENT_SOURCE_DIR}/src/desktopcontroller.*)
