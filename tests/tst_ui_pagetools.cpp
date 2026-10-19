#include "support/lot7registry.h"

#include <QtTest>

namespace {

// Tout le fichier dans cet espace de noms anonyme (exécutable unique : aucun nom global hors du registre).
class TstUiPageTools : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L7-S8 : rempli par L7-T6."); }
};

} // namespace

ACTCRAFT_LOT7_TEST("pagetools", TstUiPageTools)
#include "tst_ui_pagetools.moc"
