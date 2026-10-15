qt_add_executable(actcraft WIN32 MACOSX_BUNDLE
    main.cpp
    renderbackend.cpp
    renderbackend.h
    startup.cpp
    startup.h)

target_link_libraries(actcraft PRIVATE
    Qt6::Quick
    ActCraft::ui
    actcraft_uiplugin
    ActCraft::tools
    ActCraft::features
    ActCraft::formats)

actcraft_set_warnings(actcraft)

# `actcraft --version` (décision 31, S1).
target_compile_definitions(actcraft PRIVATE ACTCRAFT_VERSION="${PROJECT_VERSION}")

set_target_properties(actcraft PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER org.actcraft.ActCraft
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

# APK Android (décisions 28 et 31) : identifiant, nom affiché, version. versionCode = majeur × 10000 + mineur × 100
# + correctif.
if(ANDROID)
    math(EXPR ACTCRAFT_VERSION_CODE
        "${PROJECT_VERSION_MAJOR} * 10000 + ${PROJECT_VERSION_MINOR} * 100 + ${PROJECT_VERSION_PATCH}")
    set_target_properties(actcraft PROPERTIES
        QT_ANDROID_PACKAGE_NAME io.github.jejeasterix.actcraft
        QT_ANDROID_APP_NAME ActCraft
        QT_ANDROID_VERSION_NAME ${PROJECT_VERSION}
        QT_ANDROID_VERSION_CODE ${ACTCRAFT_VERSION_CODE})
endif()
