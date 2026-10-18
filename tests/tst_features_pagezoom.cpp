// Talon L7-P0 : rempli par L7-P2.
#include <QtTest>

class TstFeaturesPageZoom : public QObject
{
    Q_OBJECT

private slots:
    void reserved() { QSKIP("Talon L7-P0 : rempli par L7-P2."); }
};

QTEST_GUILESS_MAIN(TstFeaturesPageZoom)
#include "tst_features_pagezoom.moc"
