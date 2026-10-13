# Vérifications de comportements de Qt 6.12 : actcraft_add_test(...) (fonction définie dans tests/CMakeLists.txt).

actcraft_add_test(tst_qt_verifications
    SOURCES tst_qt_verifications.cpp
    LIBS Qt6::Quick Qt6::Qml)
