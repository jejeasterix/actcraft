#include <actcraft/export/officeconvert.h>
#include <actcraft/export/pdfexport.h>
#include <actcraft/export/pdfimport.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>

#include <chrono>

using namespace actcraft;

namespace actcraft::exporting::detail {
void setConvertTimeoutMs(int milliseconds); // couture de test (officeconvert.cpp), 0 = 120 s
} // namespace actcraft::exporting::detail

namespace {

// Programmes factices : l'exécutable du test lui-même (voir main).
constexpr const char* kFakeCopy = "--fake-copy";   // --fake-copy <source> <destination>
constexpr const char* kFakeSleep = "--fake-sleep"; // attend longtemps

QString selfProgram()
{
    return QCoreApplication::applicationFilePath();
}

// Écrit l'environnement et les arguments reçus (ACTCRAFT_FAKE_DUMP), puis sort sans lancer les tests.
int dumpAndExit(int argc, char** argv)
{
    QJsonArray args;
    for (int i = 1; i < argc; ++i) {
        args.append(QString::fromLocal8Bit(argv[i]));
    }
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QJsonObject root;
    root["args"] = args;
    root["in"] = env.value(QStringLiteral("ACTCRAFT_CONVERT_IN"), QStringLiteral("<absente>"));
    root["out"] = env.value(QStringLiteral("ACTCRAFT_CONVERT_OUT"), QStringLiteral("<absente>"));
    root["scriptExists"] = argc > 1 && QFile::exists(QString::fromLocal8Bit(argv[argc - 1]));
    QFile file(env.value(QStringLiteral("ACTCRAFT_FAKE_DUMP")));
    if (!file.open(QIODevice::WriteOnly)) {
        return 3;
    }
    file.write(QJsonDocument(root).toJson());
    return 0;
}

QJsonObject readDump(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QJsonDocument::fromJson(file.readAll()).object();
}

QString makeFile(const QTemporaryDir& dir, const QString& name)
{
    const QString path = dir.filePath(name);
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write("présentation d'essai");
    }
    return path;
}

// Remet l'environnement du processus à zéro, quoi qu'il arrive dans le test.
struct EnvGuard
{
    ~EnvGuard()
    {
        qunsetenv("ACTCRAFT_OFFICE_CONVERTER");
        qunsetenv("ACTCRAFT_POWERSHELL");
        qunsetenv("ACTCRAFT_FAKE_DUMP");
        exporting::detail::setConvertTimeoutMs(0);
    }
};

} // namespace

class TestExportOfficeConvert : public QObject
{
    Q_OBJECT
private slots:
    void envConverterWins()
    {
        EnvGuard guard;
        qputenv("ACTCRAFT_POWERSHELL", "powershell-factice"); // viendrait après la variable du convertisseur
        qputenv("ACTCRAFT_OFFICE_CONVERTER", "mon-convertisseur|--vers|pdf|{in}|{out}");
        const auto found = exporting::findConverter();
        QVERIFY(found.has_value());
        QCOMPARE(found->program, QStringLiteral("mon-convertisseur"));
        QCOMPARE(found->arguments,
                 (QStringList{"--vers", "pdf", QStringLiteral("{in}"), QStringLiteral("{out}")}));
        QVERIFY(!found->pathsByEnvironment);
        QVERIFY(found->scriptResource.isEmpty());
    }

    void argumentsSubstituted()
    {
        EnvGuard guard;
        QTemporaryDir dir;
        const QString presentation = makeFile(dir, "cours.pptx");
        const QString dump = dir.filePath("dump.json");
        qputenv("ACTCRAFT_FAKE_DUMP", dump.toLocal8Bit());
        qputenv("ACTCRAFT_OFFICE_CONVERTER", (selfProgram() + "|a|{outdir}|{in}|{out}|z").toLocal8Bit());
        const auto found = exporting::findConverter();
        QVERIFY(found.has_value());
        QVERIFY(!found->pathsByEnvironment);
        const auto result = exporting::convertToPdf(*found, presentation).result();
        QVERIFY(!result.ok); // le factice n'écrit pas de PDF
        QVERIFY(!result.error.isEmpty());
        const QJsonObject seen = readDump(dump);
        const QJsonArray args = seen["args"].toArray();
        QCOMPARE(args.size(), 5);
        QCOMPARE(args[0].toString(), QStringLiteral("a"));
        QCOMPARE(args[3].toString().endsWith(".pdf"), true);
        QCOMPARE(args[2].toString(), presentation);
        QCOMPARE(args[4].toString(), QStringLiteral("z"));
        const QString outdir = args[1].toString();
        QVERIFY(!outdir.isEmpty());
        QCOMPARE(QFileInfo(args[3].toString()).absolutePath(), QFileInfo(outdir).absoluteFilePath());
        QVERIFY(!args.toVariantList().contains(QStringLiteral("{in}")));
        QCOMPARE(seen["in"].toString(), QStringLiteral("<absente>")); // pas de variables hors voie PowerPoint
    }

    void pathsPassedByEnvironment()
    {
        EnvGuard guard;
        QTemporaryDir dir;
        // Un chemin qu'une ligne de commande ou un script interpolé casserait.
        const QString presentation = makeFile(dir, "l'essai;$x.pptx");
        QVERIFY(QFileInfo::exists(presentation));
        const QString dump = dir.filePath("dump.json");
        qputenv("ACTCRAFT_FAKE_DUMP", dump.toLocal8Bit());
        qputenv("ACTCRAFT_POWERSHELL", selfProgram().toLocal8Bit());
        const auto found = exporting::findConverter();
        QVERIFY(found.has_value());
        QVERIFY(found->pathsByEnvironment);
        QCOMPARE(found->program, selfProgram());
        QVERIFY(!found->scriptResource.isEmpty());
        QVERIFY(QFile::exists(found->scriptResource)); // ressource Qt livrée
        exporting::convertToPdf(*found, presentation).waitForFinished();
        const QJsonObject seen = readDump(dump);
        QVERIFY2(!seen.isEmpty(), "le programme factice n'a rien reçu");
        const QJsonArray args = seen["args"].toArray();
        QCOMPARE(args.size(), 6);
        QCOMPARE(args[0].toString(), QStringLiteral("-NoProfile"));
        QCOMPARE(args[1].toString(), QStringLiteral("-NonInteractive"));
        QCOMPARE(args[2].toString(), QStringLiteral("-ExecutionPolicy"));
        QCOMPARE(args[3].toString(), QStringLiteral("Bypass"));
        QCOMPARE(args[4].toString(), QStringLiteral("-File"));
        QVERIFY(args[5].toString().endsWith("pptx-to-pdf.ps1"));
        QVERIFY(seen["scriptExists"].toBool());
        for (const QJsonValue& arg : args) {
            QVERIFY(!arg.toString().contains("essai"));
        }
        QCOMPARE(seen["in"].toString(), presentation);
        QVERIFY(seen["out"].toString().endsWith(".pdf"));
    }

    void timeoutReported()
    {
        EnvGuard guard;
        QTemporaryDir dir;
        const QString presentation = makeFile(dir, "lent.pptx");
        exporting::detail::setConvertTimeoutMs(600);
        exporting::Converter slow;
        slow.program = selfProgram();
        slow.arguments = {kFakeSleep};
        QElapsedTimer timer;
        timer.start();
        const auto result = exporting::convertToPdf(slow, presentation).result();
        QVERIFY(!result.ok);
        QVERIFY2(result.error.contains(QStringLiteral("délai")), qPrintable(result.error));
        QVERIFY2(timer.elapsed() < 10000, "le convertisseur lent n'a pas été arrêté");
    }

    void missingConverterReported()
    {
        EnvGuard guard;
        QTemporaryDir dir;
        const QString presentation = makeFile(dir, "x.pptx");
        exporting::Converter missing;
        missing.program = dir.filePath("n-existe-pas");
        const auto absent = exporting::convertToPdf(missing, presentation).result();
        QVERIFY(!absent.ok);
        QVERIFY(!absent.error.isEmpty());
        QVERIFY(absent.pdfPath.isEmpty());
        // Présentation introuvable : erreur à nous, sans lancer le convertisseur.
        exporting::Converter fine;
        fine.program = selfProgram();
        fine.arguments = {kFakeSleep};
        const auto none = exporting::convertToPdf(fine, dir.filePath("absente.pptx")).result();
        QVERIFY(!none.ok);
        QVERIFY(!none.error.isEmpty());
    }

    void fakeConverterChainGivesPages()
    {
        EnvGuard guard;
        QTemporaryDir dir;
        core::Document document(1024, 768);
        document.appendPage();
        const QUrl pdfUrl = QUrl::fromLocalFile(dir.filePath("source.pdf"));
        QVERIFY(exporting::exportPdf(document, pdfUrl).result().ok);
        const QString presentation = makeFile(dir, "cours.pptx");
        qputenv("ACTCRAFT_OFFICE_CONVERTER",
                (selfProgram() + "|" + kFakeCopy + "|" + pdfUrl.toLocalFile() + "|{out}").toLocal8Bit());
        const auto found = exporting::findConverter();
        QVERIFY(found.has_value());
        const auto converted = exporting::convertToPdf(*found, presentation).result();
        QVERIFY2(converted.ok, qPrintable(converted.error));
        QVERIFY(QFileInfo::exists(converted.pdfPath));
        QFile produced(converted.pdfPath);
        QVERIFY(produced.open(QIODevice::ReadOnly));
        QCOMPARE(exporting::countPdfPages(produced.readAll()), 2);
        if (exporting::pdfImportAvailable()) {
            const auto imported = exporting::importPdf(QUrl::fromLocalFile(converted.pdfPath)).result();
            QVERIFY2(imported.ok, qPrintable(imported.error));
            QCOMPARE(imported.document.pageCount(), std::size_t{2});
        }
        produced.close();
        QDir(QFileInfo(converted.pdfPath).absolutePath()).removeRecursively();
    }
};

int main(int argc, char** argv)
{
    if (argc >= 4 && QByteArray(argv[1]) == kFakeCopy) {
        return QFile::copy(QString::fromLocal8Bit(argv[2]), QString::fromLocal8Bit(argv[3])) ? 0 : 4;
    }
    if (argc >= 2 && QByteArray(argv[1]) == kFakeSleep) {
        QThread::sleep(std::chrono::seconds(60));
        return 0;
    }
    if (qEnvironmentVariableIsSet("ACTCRAFT_FAKE_DUMP")) {
        return dumpAndExit(argc, argv);
    }
    QCoreApplication app(argc, argv);
    TestExportOfficeConvert test;
    return QTest::qExec(&test, argc, argv);
}
#include "tst_export_officeconvert.moc"
