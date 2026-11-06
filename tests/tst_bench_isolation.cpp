// L8-C14 : les captures et le banc ne touchent jamais aux ressources réelles du poste.
// Le mode test de QStandardPaths ne déplace pas DocumentsLocation (et sur macOS HOME est ignoré) : les
// racines « Mes ressources » et partagées d'essai doivent être posées explicitement sous le dossier de
// sortie.
#include <actcraft/bench/testroots.h>
#include <actcraft/ui/resourcelibrary.h>

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

using actcraft::ui::ResourceLibrary;

class TstBenchIsolation : public QObject
{
    Q_OBJECT
private slots:
    void cleanup() { ResourceLibrary::setRootPathsForTests(QString(), QString()); }

    void rootsLieSousLeDossierDeSortie()
    {
        QTemporaryDir out;
        QVERIFY(out.isValid());
        actcraft::bench::useIsolatedResourceRoots(out.path());
        const QString real = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                                 .filePath(QStringLiteral("ActCraft/Ressources"));
        QVERIFY(ResourceLibrary::mineRoot() != real);
        QVERIFY2(ResourceLibrary::mineRoot().startsWith(out.path()), qPrintable(ResourceLibrary::mineRoot()));
        QVERIFY2(ResourceLibrary::sharedRoot().startsWith(out.path()),
                 qPrintable(ResourceLibrary::sharedRoot()));
    }

    void videLesRacinesDEssaiSeulement()
    {
        QTemporaryDir out;
        QVERIFY(out.isValid());
        const QString stale = QDir(out.path()).filePath(QStringLiteral("mes-ressources/vieux.txt"));
        QVERIFY(QDir().mkpath(QFileInfo(stale).absolutePath()));
        QFile f(stale);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.close();
        actcraft::bench::useIsolatedResourceRoots(out.path());
        QVERIFY(!QFile::exists(stale)); // le scénario se rejoue sans nettoyage à la main
    }
};

QTEST_GUILESS_MAIN(TstBenchIsolation)
#include "tst_bench_isolation.moc"
