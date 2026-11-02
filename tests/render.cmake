# Tests du module render : actcraft_add_test(...) (fonction définie dans tests/CMakeLists.txt).

actcraft_add_test(tst_render_engines
    SOURCES tst_render_engines.cpp
    LIBS ActCraft::render Qt6::Quick)

actcraft_add_test(tst_render_images
    SOURCES tst_render_images.cpp
    LIBS ActCraft::render Qt6::Quick)

actcraft_add_test(tst_render_fixedpage
    SOURCES tst_render_fixedpage.cpp
    LIBS ActCraft::render Qt6::Quick)

actcraft_add_test(tst_render_inkbuffer
    SOURCES tst_render_inkbuffer.cpp
    LIBS ActCraft::render Qt6::Quick)

actcraft_add_test(tst_render_ink_software
    SOURCES tst_render_ink_software.cpp
    LIBS ActCraft::render Qt6::Quick)

actcraft_add_test(tst_render_painter
    SOURCES tst_render_painter.cpp
    LIBS ActCraft::render Qt6::Gui)

actcraft_add_test(tst_render_painted
    SOURCES tst_render_painted.cpp
    LIBS ActCraft::render Qt6::Quick)

# Même exécutable à la densité 2 : le texte d'un document ne charge aucune ressource à la densité de l'écran.
add_test(NAME tst_render_painted_density COMMAND tst_render_painted htmlImagesNeverBlockTheScene)
get_test_property(tst_render_painted ENVIRONMENT _painted_env)
set_tests_properties(tst_render_painted_density PROPERTIES ENVIRONMENT "${_painted_env};QT_SCREEN_SCALE_FACTORS=2")

actcraft_add_test(tst_render_pendinglayer
    SOURCES tst_render_pendinglayer.cpp
    LIBS ActCraft::render Qt6::Quick)

actcraft_add_test(tst_render_liftlayer
    SOURCES tst_render_liftlayer.cpp
    LIBS ActCraft::render Qt6::Quick)

actcraft_add_test(tst_render_strokeshape
    SOURCES tst_render_strokeshape.cpp
    LIBS ActCraft::render Qt6::Quick)

actcraft_add_test(tst_render_gridoverlay
    SOURCES tst_render_gridoverlay.cpp
    LIBS ActCraft::render Qt6::Quick)

actcraft_add_test(tst_render_verticaltext
    SOURCES tst_render_verticaltext.cpp
    LIBS ActCraft::render Qt6::Quick)
