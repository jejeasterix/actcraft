# Tests du lot 7 (présentation et outils) : actcraft_add_test(...) (fonction définie dans tests/CMakeLists.txt).
# Inclus en dernier par tests/CMakeLists.txt (OPTIONAL). Phase 1 (L7-P0) : logique pure de features/, sans écran ;
# la phase 2 ajoute ici les tests d'interface et de captures du lot.
actcraft_add_test(tst_features_curtain SOURCES tst_features_curtain.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_spotlight SOURCES tst_features_spotlight.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_pagetools SOURCES tst_features_pagetools.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_pagezoom SOURCES tst_features_pagezoom.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_transitions SOURCES tst_features_transitions.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_mathtools SOURCES tst_features_mathtools.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_dice SOURCES tst_features_dice.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_calculator SOURCES tst_features_calculator.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_countdown SOURCES tst_features_countdown.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_ticker SOURCES tst_features_ticker.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_shapefinder SOURCES tst_features_shapefinder.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_recognitionsession SOURCES tst_features_recognitionsession.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_handwriting SOURCES tst_features_handwriting.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_camera SOURCES tst_features_camera.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_keyboard SOURCES tst_features_keyboard.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_desktopwheel SOURCES tst_features_desktopwheel.cpp LIBS ActCraft::features)
