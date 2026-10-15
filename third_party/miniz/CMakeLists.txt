# miniz 3.0.2 (MIT) : archives zip du format natif .actcraft (DA3). Code tiers tel que publié : ni nos
# avertissements, ni clang-format (scripts/format.ps1 exclut third_party/).
add_library(actcraft_miniz STATIC miniz.c miniz.h)
add_library(ActCraft::miniz ALIAS actcraft_miniz)
target_include_directories(actcraft_miniz SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Sans E/S de fichiers (Qt s'en charge), sans horodatage (archives reproductibles), sans noms zlib (Qt embarque
# zlib : aucun conflit de symboles).
target_compile_definitions(actcraft_miniz PUBLIC MINIZ_NO_STDIO MINIZ_NO_TIME MINIZ_NO_ZLIB_COMPATIBLE_NAMES)
set_target_properties(actcraft_miniz PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    target_compile_options(actcraft_miniz PRIVATE /W0 /utf-8)
else()
    target_compile_options(actcraft_miniz PRIVATE -w)
endif()
