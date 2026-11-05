# Hunspell 1.7.2 (correcteur orthographique ; triple licence MPL 1.1 / GPL 2.0 / LGPL 2.1, option MPL 1.1 retenue,
# décision H6-6b) : sources de src/hunspell/ telles que publiées, sans les outils ni les scripts du paquet.
# Code tiers : ni clang-format (scripts/format.ps1 exclut third_party/), ni nos avertissements (cible hors
# actcraft_set_warnings : niveau par défaut du compilateur, sans /W4 /WX).
add_library(hunspell STATIC
    affentry.cxx affixmgr.cxx csutil.cxx filemgr.cxx hashmgr.cxx hunspell.cxx hunzip.cxx phonet.cxx
    replist.cxx suggestmgr.cxx)
add_library(ActCraft::hunspell ALIAS hunspell)
target_include_directories(hunspell SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(hunspell PUBLIC HUNSPELL_STATIC)
target_compile_features(hunspell PRIVATE cxx_std_11)
set_target_properties(hunspell PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    target_compile_options(hunspell PRIVATE /utf-8)
endif()
