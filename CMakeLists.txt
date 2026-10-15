cmake_minimum_required(VERSION 3.21)

project(ActCraft
    VERSION 0.1.0
    DESCRIPTION "Logiciel de tableau blanc interactif"
    LANGUAGES CXX)

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

find_package(Qt6 6.12 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Svg)
qt_standard_project_setup(REQUIRES 6.12)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ActCraftModule)

# Ordre = sens des dépendances : un module ne dépend que de ceux listés avant lui.
add_subdirectory(core)
add_subdirectory(formats)
add_subdirectory(actions)
add_subdirectory(input)
add_subdirectory(render)
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
