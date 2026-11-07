#include <QtTest>

class TstUiPageinput : public QObject
{
    Q_OBJECT
private slots:
    void reserved() { QSKIP("Talon L11 : rempli par L11-T5."); }
};

QTEST_MAIN(TstUiPageinput)
#include "tst_ui_pageinput.moc"
