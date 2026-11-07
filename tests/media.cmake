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
