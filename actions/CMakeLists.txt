# Registre central des commandes nommées ; menus, raccourcis et barres décrits par fichiers de données.
actcraft_add_module(actions
    SOURCES
        src/actions.cpp
        src/commandregistry.cpp
        src/layout.cpp
        include/actcraft/actions/profile.h
        src/profile.cpp
        include/actcraft/actions/profilestore.h
        src/profilestore.cpp
        include/actcraft/actions/objectactions.h
        src/objectactions.cpp
    DEPENDS
        ActCraft::core)

# Déclarations des commandes et disposition par défaut (DA1), lues sous :/actcraft/actions.
qt_add_resources(actcraft_actions "actions_data"
    PREFIX "/actcraft/actions"
    BASE data
    FILES
        data/commands.json
        data/layout.json
        data/object-actions.json
        data/profiles/en-classe.json
        data/profiles/preparation.json
        data/profiles/lecture-ecriture.json
        data/profiles/calcul-geometrie.json
        data/profiles/son-image.json)
