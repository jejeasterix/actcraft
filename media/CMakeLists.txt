# Lecteur et enregistreurs (Qt Multimedia). Ne dépend que de core et de Qt : aucun ui/.
set(ACTCRAFT_MEDIA_SOURCES
    include/actcraft/media/media.h
    src/media.cpp
    include/actcraft/media/player.h
    src/player.cpp
    include/actcraft/media/soundrecorder.h
    src/soundrecorder.cpp
    include/actcraft/media/displayrecorder.h)
if(WIN32)
    list(APPEND ACTCRAFT_MEDIA_SOURCES src/displayrecorder_win.cpp)
else()
    list(APPEND ACTCRAFT_MEDIA_SOURCES src/displayrecorder_other.cpp)
endif()
actcraft_add_module(media
    SOURCES ${ACTCRAFT_MEDIA_SOURCES}
    DEPENDS ActCraft::core Qt6::Multimedia)
