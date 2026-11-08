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
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    # Modèle à nous : raison de l'accès au micro (enregistreur de son, F-293), sans quoi macOS le refuse.
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/macos/Info.plist.in)

# APK Android (décisions 28 et 31) : identifiant, nom affiché, version. versionCode = majeur × 10000 + mineur × 100
# + correctif.
include(ActCraftVersion)
actcraft_version_code(ACTCRAFT_VERSION_CODE ${PROJECT_VERSION_MAJOR} ${PROJECT_VERSION_MINOR} ${PROJECT_VERSION_PATCH})
if(ANDROID)
    set_target_properties(actcraft PROPERTIES
        QT_ANDROID_PACKAGE_NAME io.github.jejeasterix.actcraft
        QT_ANDROID_APP_NAME ActCraft
        QT_ANDROID_VERSION_NAME ${PROJECT_VERSION}
        QT_ANDROID_VERSION_CODE ${ACTCRAFT_VERSION_CODE}
        # API cible = plateforme de compilation (ANDROID_PLATFORM de packaging/android/versions.env) ; le défaut de
        # Qt 6.12 (37) dépasserait la plateforme installée. Contrôlée par scripts/paquet-android.ps1.
        QT_ANDROID_TARGET_SDK_VERSION 36
        # Manifeste à nous (décision 28) : sans le marqueur %%INSERT_PERMISSIONS, androiddeployqt n'ajoute pas les
        # permissions par défaut de Qt (INTERNET, WRITE_EXTERNAL_STORAGE, ACCESS_NETWORK_STATE) ; celle
        # d'androidx.core (DYNAMIC_RECEIVER_NOT_EXPORTED_PERMISSION) y est ôtée par tools:node="remove".
        QT_ANDROID_PACKAGE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/android)
endif()
