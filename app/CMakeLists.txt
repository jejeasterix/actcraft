qt_add_executable(actcraft WIN32 MACOSX_BUNDLE
    main.cpp
    renderbackend.cpp
    renderbackend.h)

target_link_libraries(actcraft PRIVATE
    Qt6::Quick
    ActCraft::ui
    actcraft_uiplugin
    ActCraft::tools
    ActCraft::features
    ActCraft::formats)

actcraft_set_warnings(actcraft)

set_target_properties(actcraft PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER org.actcraft.ActCraft
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
