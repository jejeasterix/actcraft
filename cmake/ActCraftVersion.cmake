# Règle du versionCode (décision 31) : majeur × 10000 + mineur × 100 + correctif (0.1.0 → 100, 1.0.0 → 10000).
# Une seule définition : app/CMakeLists.txt (APK Android) et tests/app.cmake (tst_app_versioninfo) l'appellent.
function(actcraft_version_code out major minor patch)
    math(EXPR code "${major} * 10000 + ${minor} * 100 + ${patch}")
    set(${out} ${code} PARENT_SCOPE)
endfunction()
