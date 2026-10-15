# Lecture et écriture des formats : .flipchart (lecture, DA4) et format natif ouvert .actcraft (DA3). QtCore
# seulement (CMakeLists.txt racine : actcraft_forbid_links). Deux listes de sources, une par famille, pour que
# les deux chantiers avancent sans toucher le même fichier.
include(${CMAKE_CURRENT_SOURCE_DIR}/native.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/flipchart.cmake)
actcraft_add_module(formats
    SOURCES
        src/formats.cpp
        ${ACTCRAFT_FORMATS_NATIVE_SOURCES}
        ${ACTCRAFT_FORMATS_FLIPCHART_SOURCES}
    DEPENDS
        ActCraft::core)
target_link_libraries(actcraft_formats PRIVATE ActCraft::miniz)
