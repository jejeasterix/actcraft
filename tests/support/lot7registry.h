#pragma once

// Registre des classes de test d'interface du lot 7 : un fichier par fonction, un seul exécutable
// (tst_ui_lot7). Chaque fichier inscrit sa classe sous une clé ; le main choisit la classe par sa première
// option. Pas d'initMain() (convention de QTEST_MAIN) : ce main ne l'appelle pas ; réglages globaux dans
// initTestCase().
#include <QObject>

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>

namespace lot7 {

using Factory = std::function<std::unique_ptr<QObject>()>;

inline std::map<std::string, Factory>& registry()
{
    static std::map<std::string, Factory> entries;
    return entries;
}

struct Registration
{
    Registration(const char* key, Factory factory) { registry().emplace(key, std::move(factory)); }
};

} // namespace lot7

#define ACTCRAFT_LOT7_TEST(key, Class)                                                                       \
    static const lot7::Registration lot7Registration##Class(key, [] { return std::make_unique<Class>(); });
