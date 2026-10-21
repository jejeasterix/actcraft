#include <QtTest>

class TstCorePageedits : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L4-T1 : rempli par L4-T15."); }
};

QTEST_GUILESS_MAIN(TstCorePageedits)
#include "tst_core_pageedits.moc"
