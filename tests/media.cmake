# Tests du module media (construit si ACTCRAFT_MEDIA) : talons du lot 8.
actcraft_add_test(tst_media_player
    SOURCES tst_media_player.cpp
    LIBS ActCraft::media ActCraft::ui) # ui : LinkLauncher réel du repli (fallbackNeedsConfirmation)
# Label « materiel » : la lecture exige une sortie audio qui consomme le son. Sur macos-latest, la sortie listée ne
# consomme rien et la position reste à 0, même avec la sortie virtuelle BlackHole (runs 43303503126, 43313320173).
# Exclu du job macOS de GitHub, joué sous Windows en CI et sur un Mac réel par scripts/tests-materiel.sh.
set_tests_properties(tst_media_player PROPERTIES LABELS materiel)
actcraft_add_test(tst_media_soundrecorder
    SOURCES tst_media_soundrecorder.cpp
    LIBS ActCraft::media)
# Deux entrées ctest pour un exécutable : framesAreCappedAtThirtyPerSecond porte seul le label « materiel », les
# autres cas restent dans le job macOS de GitHub. Une fonction ajoutée au test va dans _displayrecorder_others, sinon
# tst_media_displayrecorder_functions rougit.
set(_displayrecorder_capped framesAreCappedAtThirtyPerSecond)
set(_displayrecorder_others notBuiltOutsideWindows fullScreenFiveSeconds areaIsCropped areaIsClippedToEvenSides
    pauseExcludesThePause cancelDeletesTheFile stopHandsTheFileOnceClosed engineFailureLeavesNothing
    engineReleasedBetweenRecordings destroyedWhileRecordingLeavesNothing cancelAfterStopBeforeFileClosed
    abandonedEngineThatNeverClosesIsReleased cropFrameKeepsOnlyTheZone)
actcraft_add_test(tst_media_displayrecorder
    SOURCES tst_media_displayrecorder.cpp
    LIBS ActCraft::media
    ARGS ${_displayrecorder_others})
get_test_property(tst_media_displayrecorder ENVIRONMENT _displayrecorder_env)
actcraft_register_test(tst_media_displayrecorder_${_displayrecorder_capped} tst_media_displayrecorder
    ${_displayrecorder_capped})
# RUN_SERIAL : ces essais attendent en temps réel et mesurent l'écart entre les images. Posé sur hypothèse, sans
# reproduction (trou de 228 ms pour 200 ms, CI 42738454526) : docs/verifications/2026-11-09-tests-intermittents-windows.md.
set_tests_properties(tst_media_displayrecorder tst_media_displayrecorder_${_displayrecorder_capped} PROPERTIES
    RUN_SERIAL TRUE)
# Label « materiel » : framesAreCappedAtThirtyPerSecond exige la cadence d'encodage d'une vraie machine (21 images pour
# 1 550 ms sur macos-latest, rendu sans RHI, run 43303503126). Exclu du job macOS de GitHub (ci.yml, -LE materiel),
# joué sous Windows en CI et sur un Mac réel par scripts/tests-materiel.sh.
set_tests_properties(tst_media_displayrecorder_${_displayrecorder_capped} PROPERTIES
    LABELS materiel
    ENVIRONMENT "${_displayrecorder_env}")
list(JOIN _displayrecorder_others "," _displayrecorder_all)
add_test(NAME tst_media_displayrecorder_functions
    COMMAND ${CMAKE_COMMAND}
        -DEXE=$<TARGET_FILE:tst_media_displayrecorder>
        "-DFUNCTIONS=${_displayrecorder_all},${_displayrecorder_capped}"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/fonctions_declarees.cmake)
set_tests_properties(tst_media_displayrecorder_functions PROPERTIES
    ENVIRONMENT "${_displayrecorder_env}"
    PASS_REGULAR_EXPRESSION "FONCTIONS DECLAREES OK")
