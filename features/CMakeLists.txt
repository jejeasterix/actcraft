# Fonctions activables ou retirables : règle, rapporteur, dés, horloge, rideau, spot…
# Logique pure (QtCore, QtGui), testée sans écran ; jamais Qt Quick ni QML (garde-fou ci-dessous).
# Sources lues par motif (lot 7, phase 1) : chaque fonction ajoute ses fichiers sans toucher à ce fichier.
# Code propre à une plateforme : <nom>_win.cpp (Windows) et son jumeau <nom>_other.cpp (ailleurs), toujours par paire.
file(GLOB _features_sources CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/actcraft/features/*.h)
if(WIN32)
    list(FILTER _features_sources EXCLUDE REGEX "_other\.cpp$")
else()
    list(FILTER _features_sources EXCLUDE REGEX "_win\.cpp$")
endif()

actcraft_add_module(features
    SOURCES
        ${_features_sources}
    DEPENDS
        ActCraft::core ActCraft::actions ActCraft::input)

if(WIN32)
    # Reconnaissance d'écriture par COM (handwriting_win.cpp) ; cadres des fenêtres (camera_win.cpp, phase 2).
    target_link_libraries(actcraft_features PRIVATE ole32 oleaut32 dwmapi)
endif()

# Règle du lot 7 : la logique de features/ ne dépend d'aucune interface Qt Quick, QML ou Widgets.
actcraft_forbid_links(actcraft_features Qt6::Quick Qt6::Qml Qt6::QuickControls2 Qt6::Widgets)
