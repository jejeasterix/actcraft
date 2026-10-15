# Tests du lecteur .flipchart (DA4) : actcraft_add_test(...) (fonction définie dans tests/CMakeLists.txt).
# Les paperboards des tests sont écrits par tests/support/flipchartwriter.h ; samples/ n'est lu que par
# tst_formats_oracle, en place, et seulement si ACTCRAFT_SAMPLES_DIR est posé (décision 19).
actcraft_add_test(tst_formats_bamboo
    SOURCES tst_formats_bamboo.cpp
    LIBS ActCraft::formats ActCraft::miniz Qt6::Gui)
if(WIN32)
    target_link_libraries(tst_formats_bamboo PRIVATE psapi) # GetProcessMemoryInfo (peakmemory.h)
endif()
actcraft_add_test(tst_formats_objectstream
    SOURCES tst_formats_objectstream.cpp
    LIBS ActCraft::formats Qt6::Gui)
actcraft_add_test(tst_formats_flipchart
    SOURCES tst_formats_flipchart.cpp
    LIBS ActCraft::formats Qt6::Gui)
if(WIN32)
    target_link_libraries(tst_formats_flipchart PRIVATE psapi) # GetProcessMemoryInfo (peakmemory.h)
endif()
