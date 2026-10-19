#include "support/lot7registry.h"

#include <QtTest>

namespace {

// Tout le fichier dans cet espace de noms anonyme (exécutable unique : aucun nom global hors du registre).
class TstUiEquation : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L7-S8 : rempli par L7-T26."); }
};

} // namespace

ACTCRAFT_LOT7_TEST("equation", TstUiEquation)
#include "tst_ui_equation.moc"
