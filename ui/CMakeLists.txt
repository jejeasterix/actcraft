# Interface QML : boîte à outils, navigateurs, menus, thèmes.
qt_add_library(actcraft_ui STATIC)
add_library(ActCraft::ui ALIAS actcraft_ui)

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
# Le code C++ généré pour ce dialogue déclenche C4702 (code inatteignable, dans les en-têtes de Qt) sous /W4 /WX :
# il est chargé depuis sa source QML, sans précompilation (écran rarement ouvert).
set_source_files_properties(qml/AboutDialog.qml PROPERTIES QT_QML_SKIP_CACHEGEN TRUE)

# Même défaut (C4702) pour trois composants de la coquille qui appellent Commands.trigger : chargés depuis leur
# source QML. Les composants du chemin de dessin (page, vue, encre) restent précompilés.
set_source_files_properties(qml/TopBar.qml qml/CommandButton.qml qml/WidthPicker.qml
    PROPERTIES QT_QML_SKIP_CACHEGEN TRUE)

# Même défaut (C4702) pour les commandes de fichier et leurs dialogues (L2-T18) : chargés depuis leur source.
set_source_files_properties(qml/FileActions.qml qml/SaveChangesDialog.qml qml/PageSizeDialog.qml
    PROPERTIES QT_QML_SKIP_CACHEGEN TRUE)

# Même défaut (C4702) pour les composants de la coquille du socle 3-7 (L0-S0) : chargés depuis leur source.
set_source_files_properties(qml/SelectionOverlay.qml qml/ObjectBar.qml qml/TrashCan.qml qml/PageContextMenu.qml
    qml/TranslucencyPopup.qml qml/Presentation.qml qml/TextEditor.qml qml/TextFormatBar.qml
    qml/VerticalTextBar.qml qml/TextBarBase.qml qml/BarCombo.qml
    PROPERTIES QT_QML_SKIP_CACHEGEN TRUE)

# Lot 7 (présentation et outils) : ses sources QML et C++ sont listées dans ui/presentation.cmake (fichier de la
# session B) ; ce fichier-ci n'est plus modifié pour elles. Presentation.qml est déclaré ici (socle) : ne pas le
# relister dans presentation.cmake.
set(ACTCRAFT_UI_LOT7_QML "")
set(ACTCRAFT_UI_LOT7_SOURCES "")
include(${CMAKE_CURRENT_SOURCE_DIR}/presentation.cmake OPTIONAL)

# Lot 6 (L6-T1) : sources réservées (talons marqués), remplies par les tâches nommées dans leurs marqueurs.
# Les 22 fichiers QML du lot sont créés par L6-T2 : enregistrés ici par leur nom, ceux qui existent déjà seulement
# (CMake refuse un fichier absent) ; un nouveau fichier est pris au prochain configure (build.ps1 reconfigure).
set(ACTCRAFT_UI_LOT6_QML "")
foreach(_name PaletteFrame ShortcutBar ExtendedColors ColorLoupe ObjectBrowser PropertyBrowser PropertyEditor NotesBrowser ResourceBrowser LabelTip Dashboard ProfileChooser ProfileEditor FormatPage CommandsPage LaunchersPage SettingsPage NetworkPage ClassroomLockDialog SpellingDialog HelpViewer SaveSummaryDialog)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/qml/${_name}.qml)
        list(APPEND ACTCRAFT_UI_LOT6_QML qml/${_name}.qml)
    endif()
endforeach()
set(ACTCRAFT_UI_LOT6_SOURCES
    include/actcraft/ui/preferences.h
    include/actcraft/ui/profilecontroller.h
    include/actcraft/ui/colorpalette.h
    include/actcraft/ui/shortcutbar.h
    include/actcraft/ui/propertymodel.h
    include/actcraft/ui/objecttree.h
    include/actcraft/ui/notescontroller.h
    include/actcraft/ui/labeltips.h
    include/actcraft/ui/resourcelibrary.h
    include/actcraft/ui/designmode.h
    include/actcraft/ui/recentdocuments.h
    include/actcraft/ui/layouteditor.h
    include/actcraft/ui/settingsschema.h
    include/actcraft/ui/secretstore.h
    include/actcraft/ui/networksettings.h
    include/actcraft/ui/launchers.h
    include/actcraft/ui/classroomlock.h
    include/actcraft/ui/spelling.h
    include/actcraft/ui/helpviewer.h
    include/actcraft/ui/language.h
    src/preferences.cpp
    src/profilecontroller.cpp
    src/colorpalette.cpp
    src/shortcutbar.cpp
    src/propertymodel.cpp
    src/objecttree.cpp
    src/notescontroller.cpp
    src/labeltips.cpp
    src/resourcelibrary.cpp
    src/designmode.cpp
    src/recentdocuments.cpp
    src/layouteditor.cpp
    src/settingsschema.cpp
    src/networksettings.cpp
    src/launchers.cpp
    src/classroomlock.cpp
    src/spelling.cpp
    src/helpviewer.cpp
    src/language.cpp
)
if(WIN32)
    list(APPEND ACTCRAFT_UI_LOT6_SOURCES src/secretstore_win.cpp)
else()
    list(APPEND ACTCRAFT_UI_LOT6_SOURCES src/secretstore_other.cpp)
endif()
# Hachage et mandataire du lot 6 (Qt6::Network).
find_package(Qt6 6.12 REQUIRED COMPONENTS Network)

qt_add_qml_module(actcraft_ui
    URI ActCraft.Ui
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/AboutDialog.qml
        qml/TopBar.qml
        qml/ToolPalette.qml
        qml/PageArea.qml
        qml/CommandButton.qml
        qml/CommandEntry.qml
        qml/CommandMenu.qml
        qml/CommandSubmenu.qml
        qml/ColorSwatches.qml
        qml/WidthPicker.qml
        qml/ItemHighlight.qml
        qml/ScrollThumb.qml
        qml/FileActions.qml
        qml/SaveChangesDialog.qml
        qml/PageSizeDialog.qml
        qml/MessageBanner.qml
        qml/SelectionOverlay.qml
        qml/ObjectBar.qml
        qml/TranslucencyPopup.qml
        qml/TrashCan.qml
        qml/PageContextMenu.qml
        qml/ToolCursor.qml
        qml/Presentation.qml
        qml/BrowserPanel.qml
        qml/PageNavigator.qml
        qml/ShapePalette.qml
        qml/TextFormatBar.qml
        qml/TextEditor.qml
        qml/VerticalTextBar.qml
        qml/TextBarBase.qml
        qml/BarCombo.qml
        qml/GridDesigner.qml
        qml/BackgroundDialog.qml
        qml/PageExtensionHandle.qml
        qml/PageZonesOverlay.qml
        qml/ObjectEditMenu.qml
        qml/PenModifierBar.qml
        qml/ConnectorPanel.qml
        qml/DocumentTabBar.qml
        qml/FileExchangeActions.qml
        qml/PrintSetupDialog.qml
        qml/SheetPreview.qml
        qml/ActionBrowser.qml
        qml/DragDropActions.qml
        qml/ActionOverlays.qml
        qml/LinkConfirmDialog.qml
        qml/LinkDialog.qml
        qml/MediaLayer.qml
        qml/SoundController.qml
        qml/RecorderPalette.qml
        qml/RecordingIndicator.qml
        qml/WebFrameLayer.qml
        qml/WebFrameDialog.qml
        qml/PublishDialog.qml
        ${ACTCRAFT_UI_LOT7_QML}
        ${ACTCRAFT_UI_LOT6_QML}
    RESOURCES
        icons/menu.svg
        icons/outils.svg
        icons/page-precedente.svg
        icons/page-suivante.svg
        icons/stylo.svg
        icons/plein-ecran.svg
        icons/ajuster.svg
        icons/quitter.svg
        icons/aide.svg
        icons/a-propos.svg
        icons/selection.svg
        icons/marqueur.svg
        icons/gomme.svg
        icons/annuler.svg
        icons/retablir.svg
        icons/supprimer.svg
        icons/corbeille.svg
        icons/effacer.svg
        icons/deplacer.svg
        icons/rotation.svg
        icons/translucidite.svg
        icons/agrandir.svg
        icons/reduire.svg
        icons/rideau.svg
        icons/spot.svg
        icons/zoom-page.svg
        icons/effets.svg
        icons/regle.svg
        icons/equerre.svg
        icons/rapporteur.svg
        icons/compas.svg
        icons/origine.svg
        icons/des.svg
        icons/calculatrice.svg
        icons/horloge.svg
        icons/message.svg
        icons/forme-reconnue.svg
        icons/ecriture.svg
        icons/camera.svg
        icons/clavier.svg
        icons/bureau.svg
        icons/forme.svg
        icons/texte.svg
        icons/texte-vertical.svg
        icons/grille.svg
        icons/grille-masquer.svg
        icons/aimant.svg
        icons/createur-grille.svg
        icons/navigateurs.svg
        icons/page-avant.svg
        icons/page-apres.svg
        icons/page-couper.svg
        icons/page-copier.svg
        icons/page-coller.svg
        icons/page-dupliquer.svg
        icons/page-supprimer.svg
        icons/fond.svg
        icons/allonger.svg
        icons/zones.svg
        icons/gras.svg
        icons/italique.svg
        icons/souligne.svg
        icons/exposant.svg
        icons/indice.svg
        icons/aligner-gauche.svg
        icons/centrer.svg
        icons/aligner-droite.svg
        icons/interligne-plus.svg
        icons/interligne-moins.svg
        icons/retrait-plus.svg
        icons/retrait-moins.svg
        icons/puces.svg
        icons/symboles.svg
        icons/taille-plus.svg
        icons/taille-moins.svg
        icons/nav-ressources.svg
        icons/nav-objets.svg
        icons/nav-notes.svg
        icons/nav-proprietes.svg
        icons/nav-actions.svg
        icons/couper.svg
        icons/copier.svg
        icons/coller.svg
        icons/dupliquer.svg
        icons/grouper.svg
        icons/degrouper.svg
        icons/organiser.svg
        icons/aligner.svg
        icons/transformer.svg
        icons/verrouiller.svg
        icons/masquer.svg
        icons/glisser-copie.svg
        icons/points-forme.svg
        icons/pot-peinture.svg
        icons/connecteur.svg
        icons/modification-stylo.svg
        icons/menu-objet.svg
        icons/image.svg
        icons/media.svg
        icons/onglets.svg
        icons/exporter.svg
        icons/imprimer.svg
        icons/importer.svg
        icons/palette-gauche.svg
        icons/palette-droite.svg
        icons/palette-haut.svg
        icons/palette-bas.svg
        icons/palette-flottante.svg
        icons/palette-reduire.svg
        icons/palette-reduire-tout.svg
        icons/palette-epingler.svg
        icons/couleurs-plus.svg
        icons/pipette.svg
        icons/nav-objets-masque.svg
        icons/nav-objets-verrou.svg
        icons/notes-page.svg
        icons/profil.svg
        icons/profils.svg
        icons/conception.svg
        icons/accueil.svg
        icons/recents.svg
        icons/ressources-miennes.svg
        icons/ressources-partagees.svg
        icons/ressources-autre.svg
        icons/tampon.svg
        icons/classe-verrou.svg
        icons/orthographe.svg
        icons/aide-sommaire.svg
        icons/programme.svg
        icons/actions.svg
        icons/encre-revelatrice.svg
        icons/lien-fichier.svg
        icons/lien-site.svg
        icons/action-objet.svg
        icons/action-glisser.svg
        icons/conteneur.svg
        icons/limiteur.svg
        icons/historique-precedent.svg
        icons/historique-suivant.svg
        icons/son.svg
        icons/video.svg
        icons/lecteur-sons.svg
        icons/enregistrer-son.svg
        icons/enregistrer-ecran.svg
        icons/enregistrer-zone.svg
        icons/cadre-web.svg
        icons/diffuser.svg
    SOURCES
        include/actcraft/ui/viewtransform.h
        include/actcraft/ui/linkdialogs.h
        src/linkdialogs.cpp
        include/actcraft/ui/mediainsert.h
        src/mediainsert.cpp
        include/actcraft/ui/mediaitems.h
        src/mediaitems.cpp
        include/actcraft/ui/recorders.h
        src/recorders.cpp
        include/actcraft/ui/webpolicy.h
        src/webpolicy.cpp
        include/actcraft/ui/webframeitem.h
        src/webframeitem.cpp
        include/actcraft/ui/broadcastgate.h
        src/broadcastgate.cpp
        include/actcraft/ui/actionrunner.h
        src/actionrunner.cpp
        include/actcraft/ui/linklauncher.h
        src/linklauncher.cpp
        include/actcraft/ui/actionbrowser.h
        src/actionbrowser.cpp
        include/actcraft/ui/dragdropactions.h
        src/dragdropactions.cpp
        include/actcraft/ui/pagehistory.h
        src/pagehistory.cpp
        include/actcraft/ui/interactivityprops.h
        src/interactivityprops.cpp
        include/actcraft/ui/pageview.h
        include/actcraft/ui/documentcontroller.h
        include/actcraft/ui/commands.h
        src/uiforeigntypes.h
        src/penadapters.h
        src/penadapters.cpp
        src/viewtransform.cpp
        src/pageview.cpp
        src/documentcontroller.cpp
        include/actcraft/ui/documentio.h
        src/documentio.cpp
        src/commands.cpp
        include/actcraft/ui/appinfo.h
        src/appinfo.cpp
        include/actcraft/ui/toolhost.h
        src/toolhost.cpp
        src/tooladapters.h
        src/tooladapters.cpp
        include/actcraft/ui/pageoptions.h
        src/pageoptions.cpp
        include/actcraft/ui/pagethumbnails.h
        src/pagethumbnails.cpp
        include/actcraft/ui/pagescontroller.h
        src/pagescontroller.cpp
        include/actcraft/ui/textediting.h
        src/textediting.cpp
        include/actcraft/ui/objectediting.h
        src/objectediting.cpp
        include/actcraft/ui/insertion.h
        src/insertion.cpp
        include/actcraft/ui/documenttabs.h
        src/documenttabs.cpp
        include/actcraft/ui/fileexchange.h
        src/fileexchange.cpp
        include/actcraft/ui/printcontroller.h
        src/printcontroller.cpp
        ${ACTCRAFT_UI_LOT7_SOURCES}
        ${ACTCRAFT_UI_LOT6_SOURCES})

# Le code d'enregistrement QML généré inclut les en-têtes par leur seul nom : d'où le dossier privé.
# MSVC, C4702 (code inatteignable) dans les sources C++ que qmlcachegen produit : le code généré instancie
# QJSEngine::fromVariant / fromPrimitiveValue (QtQml/qjsengine.h, l. 96-112), dont les chaînes de `if constexpr` avec
# `return` laissent du code inatteignable pour tout type donné ; Qt protège fromVariant par QT_WARNING_PUSH, pas cette
# fonction. Avec /WX (avertissements = erreurs), n'importe quel QML qui lit une propriété de type primitif peut le
# déclencher (premier cas : ToolPalette.qml, `visible` du TrashCan). Défaut de Qt : on le tait seulement pour ces sources
# générés, jamais pour le reste du module.
if(MSVC)
    get_target_property(_actcraft_ui_qml actcraft_ui QT_QML_MODULE_QML_FILES)
    foreach(_qml IN LISTS _actcraft_ui_qml)
        get_filename_component(_name "${_qml}" NAME_WE)
        set_source_files_properties(
            "${CMAKE_CURRENT_BINARY_DIR}/.rcc/qmlcache/actcraft_ui_qml_${_name}_qml.cpp"
            PROPERTIES COMPILE_OPTIONS /wd4702)
    endforeach()
endif()

target_include_directories(actcraft_ui
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/actcraft/ui ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(actcraft_ui PUBLIC
    Qt6::Quick ActCraft::actions ActCraft::formats ActCraft::input ActCraft::render ActCraft::tools
    ActCraft::features ActCraft::export ActCraft::print Qt6::Concurrent Qt6::Network)
if(ACTCRAFT_MEDIA)
    target_link_libraries(actcraft_ui PUBLIC ActCraft::media)
endif()
if(ACTCRAFT_WEBFRAME)
    target_link_libraries(actcraft_ui PUBLIC Qt6::WebView)
endif()
actcraft_set_warnings(actcraft_ui)

# Version affichée par « À propos » (décision 31).
target_compile_definitions(actcraft_ui PRIVATE ACTCRAFT_VERSION="${PROJECT_VERSION}")

# Notices des composants tiers, affichées par « À propos » et livrées dans les paquets (décision 34).
set_source_files_properties(${PROJECT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md
    PROPERTIES QT_RESOURCE_ALIAS THIRD_PARTY_NOTICES.md)
qt_add_resources(actcraft_ui "ui_notices"
    PREFIX "/actcraft"
    FILES ${PROJECT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md)

# Orthographe (D6-9) : dictionnaire français (Grammalecte, MPL 2.0), copié au premier usage dans un dossier lisible
# par Hunspell (qui lit des chemins de fichiers).
set(ACTCRAFT_DICT_DIR ${PROJECT_SOURCE_DIR}/third_party/dictionaries/fr)
set_source_files_properties(${ACTCRAFT_DICT_DIR}/fr.aff PROPERTIES QT_RESOURCE_ALIAS dictionaries/fr.aff)
set_source_files_properties(${ACTCRAFT_DICT_DIR}/fr.dic PROPERTIES QT_RESOURCE_ALIAS dictionaries/fr.dic)
qt_add_resources(actcraft_ui "ui_dictionaries"
    PREFIX "/actcraft"
    FILES ${ACTCRAFT_DICT_DIR}/fr.aff ${ACTCRAFT_DICT_DIR}/fr.dic)

# Lot 6 : aide embarquée (L6-T35).
qt_add_resources(actcraft_ui "ui_lot6_data"
    PREFIX "/actcraft"
    BASE ${CMAKE_CURRENT_SOURCE_DIR}
    FILES help/index.html)

# Traductions (D6-11, L6-T36) : le .ts (produit par lupdate, cible update_translations) devient un .qm dans les
# ressources, sous /actcraft/i18n. Français livré : traduction identité.
find_package(Qt6 6.12 REQUIRED COMPONENTS LinguistTools)
qt_add_translations(actcraft_ui
    TS_FILES i18n/actcraft_fr.ts
    RESOURCE_PREFIX /actcraft/i18n)
