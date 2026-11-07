cmake_minimum_required(VERSION 3.21)

project(ActCraft
    VERSION 0.6.0
    DESCRIPTION "Logiciel libre de tableau blanc interactif"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Mesures du banc en RelWithDebInfo : MSVC y met /Ob1 (inlining limité) ; /Ob2 comme en Release
# (décision 28 du cadrage du prototype de mesure). Clang garde ses options.
if(MSVC)
    string(REPLACE "/Ob1" "/Ob2" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
endif()

# Mémoire graphique de Metal (bench/) : Objective-C++.
if(APPLE)
    enable_language(OBJCXX)
endif()

option(ACTCRAFT_BUILD_TESTS "Construire les tests" ON)
option(ACTCRAFT_BUILD_BENCH "Construire le banc de fluidité" ON)
option(ACTCRAFT_PRINT "Impression par QPrinter (Windows)" ${WIN32})
# Import PDF : automatique, ON si Qt PDF (module Pdf) est trouvé, OFF sinon (la construction passe sans).
find_package(Qt6 6.12 QUIET COMPONENTS Pdf)
if(Qt6Pdf_FOUND)
    set(_actcraft_pdf_default ON)
else()
    set(_actcraft_pdf_default OFF)
endif()
option(ACTCRAFT_IMPORT_PDF "Import PDF par Qt PDF" ${_actcraft_pdf_default})
message(STATUS "ACTCRAFT_IMPORT_PDF=${ACTCRAFT_IMPORT_PDF}")

# Lot 8 : lecteur et enregistreurs (Qt Multimedia) : REQUIS, ui/ inclut actcraft/media/* sans condition (une
# configuration sans lui passerait, puis échouerait à la compilation). Cadre web (Qt WebView) : ON si trouvé.
find_package(Qt6 6.12 QUIET COMPONENTS Multimedia)
if(NOT Qt6Multimedia_FOUND)
    message(FATAL_ERROR "Qt Multimedia 6.12 est requis (lecteur et enregistreurs). Windows, macOS officiel : "
            "ajouter « -m qtmultimedia » à la commande aqt (voir CLAUDE.md) ; Homebrew : brew install qt.")
endif()
option(ACTCRAFT_MEDIA "Lecteur et enregistreurs par Qt Multimedia (requis, ON)" ON)
if(NOT ACTCRAFT_MEDIA)
    message(FATAL_ERROR "ACTCRAFT_MEDIA=OFF n'est pas pris en charge : le module media est requis par ui/. "
            "Retirer l'option ou la passer à ON.")
endif()
find_package(Qt6 6.12 QUIET COMPONENTS WebView)
if(Qt6WebView_FOUND)
    set(_actcraft_webframe_default ON)
else()
    set(_actcraft_webframe_default OFF)
endif()
option(ACTCRAFT_WEBFRAME "Cadre web par Qt WebView" ${_actcraft_webframe_default})
message(STATUS "ACTCRAFT_MEDIA=${ACTCRAFT_MEDIA} ACTCRAFT_WEBFRAME=${ACTCRAFT_WEBFRAME}")

find_package(Qt6 6.12 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Svg Concurrent)
qt_standard_project_setup(REQUIRES 6.12)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ActCraftModule)

# Ordre = sens des dépendances : un module ne dépend que de ceux listés avant lui.
# Code tiers (DA3) : avant les modules qui l'utilisent.
add_subdirectory(third_party/miniz)
add_subdirectory(third_party/hunspell)
add_subdirectory(core)
if(ACTCRAFT_MEDIA)
    add_subdirectory(media)
endif()
add_subdirectory(formats)
add_subdirectory(actions)
add_subdirectory(input)
add_subdirectory(render)
add_subdirectory(export)
add_subdirectory(print)
add_subdirectory(tools)
add_subdirectory(features)
add_subdirectory(ui)
add_subdirectory(app)

if(ACTCRAFT_BUILD_BENCH)
    add_subdirectory(bench)
endif()

if(ACTCRAFT_BUILD_TESTS)
    find_package(Qt6 6.12 REQUIRED COMPONENTS Test)
    enable_testing()
    add_subdirectory(tests)
endif()

# Règle 4 du BRIEF : core/ ne dépend d'aucune interface graphique.
actcraft_forbid_links(actcraft_core Qt6::Gui Qt6::Quick Qt6::Qml Qt6::Widgets)
# DA4 : formats/ lit et écrit sans interface graphique (QtCore seulement) ; images et PDF vont dans export/ (DA12).
actcraft_forbid_links(actcraft_formats Qt6::Gui Qt6::Quick Qt6::Qml Qt6::Widgets)
