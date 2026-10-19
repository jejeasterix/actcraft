#include "support/lot7registry.h"

#include <QGuiApplication>
#include <QtQml/QQmlExtensionPlugin>
#include <QtTest>

#include <cstdio>
#include <memory>
#include <vector>

// Module QML statique, comme tests/tst_ui_shell.cpp:30 : une fois pour tout l'exécutable.
Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

// Exécutable unique des tests d'interface du lot 7 : tst_ui_lot7 <clé> [options de Qt Test].
int main(int argc, char** argv)
{
    if (argc < 2 || lot7::registry().count(argv[1]) == 0) {
        std::fprintf(stderr, "tst_ui_lot7 : clé attendue parmi :");
        for (const auto& entry : lot7::registry()) {
            std::fprintf(stderr, " %s", entry.first.c_str());
        }
        std::fprintf(stderr, "\n");
        return 2;
    }
    const lot7::Factory& factory = lot7::registry().at(argv[1]);
    std::vector<char*> arguments{argv[0]};
    for (int i = 2; i < argc; ++i) {
        arguments.push_back(argv[i]);
    }
    int count = static_cast<int>(arguments.size());
    arguments.push_back(nullptr); // argv[argc] nul, comme celui du système
    // Ce que QTEST_MAIN fait pour une application graphique (qtest.h de Qt 6.12, QTEST_QAPP_SETUP et
    // QTEST_SET_MAIN_SOURCE_PATH) ; aucune classe du lot n'a d'initMain().
    QGuiApplication app(count, arguments.data());
    app.setAttribute(Qt::AA_Use96Dpi, true);
    std::unique_ptr<QObject> test = factory();
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(test.get(), count, arguments.data());
}
