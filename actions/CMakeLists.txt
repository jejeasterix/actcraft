# Registre central des commandes nommées ; menus, raccourcis et barres décrits par fichiers de données.
actcraft_add_module(actions
    SOURCES
        src/actions.cpp
        src/commandregistry.cpp
        src/layout.cpp
    DEPENDS
        ActCraft::core)

# Déclarations des commandes et disposition par défaut (DA1), lues sous :/actcraft/actions.
qt_add_resources(actcraft_actions "actions_data"
    PREFIX "/actcraft/actions"
    BASE data
    FILES
        data/commands.json
        data/layout.json)
