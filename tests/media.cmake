# Tests du module media (construit si ACTCRAFT_MEDIA) : talons du lot 8.
actcraft_add_test(tst_media_player
    SOURCES tst_media_player.cpp
    LIBS ActCraft::media ActCraft::ui) # ui : LinkLauncher réel du repli (fallbackNeedsConfirmation)
actcraft_add_test(tst_media_soundrecorder
    SOURCES tst_media_soundrecorder.cpp
    LIBS ActCraft::media)
actcraft_add_test(tst_media_displayrecorder
    SOURCES tst_media_displayrecorder.cpp
    LIBS ActCraft::media)
# RUN_SERIAL : ces essais attendent en temps réel et mesurent l'écart entre les images. Posé sur hypothèse, sans
# reproduction (trou de 228 ms pour 200 ms, CI 42738454526) : docs/verifications/2026-11-09-tests-intermittents-windows.md.
set_tests_properties(tst_media_displayrecorder PROPERTIES RUN_SERIAL TRUE)
# Label « materiel » : framesAreCappedAtThirtyPerSecond exige la cadence d'encodage d'une vraie machine (21 images pour
# 1 550 ms sur macos-latest, rendu sans RHI, run 43303503126). Exclu du job macOS de GitHub (ci.yml, -LE materiel),
# joué sous Windows en CI et sur un Mac réel par scripts/tests-materiel.sh. Le label porte sur tout l'exécutable :
# ctest ne sait pas étiqueter un seul cas.
set_tests_properties(tst_media_displayrecorder PROPERTIES LABELS materiel)
