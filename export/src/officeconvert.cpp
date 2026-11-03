#include <actcraft/export/officeconvert.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QPromise>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QThread>
#include <QUrl>

#ifdef Q_OS_WIN
#include <QSettings>
#endif

#include <atomic>
#include <functional>
#include <memory>

namespace actcraft::exporting {

namespace {

constexpr int kDefaultTimeoutMs = 120000;
constexpr const char* kScriptResource = ":/actcraft/export/pptx-to-pdf.ps1";
std::atomic<int> g_timeoutMs{0};

ConvertResult failure(const QString& error)
{
    ConvertResult result;
    result.error = error;
    return result;
}

// Un fil à nous, pas le groupe de fils : QFuture::result() peut exécuter une tâche encore en file sur le
// fil qui attend (même raison que detail::runAsync de pageexport.cpp).
QFuture<ConvertResult> runAsync(std::function<ConvertResult()> work)
{
    auto promise = std::make_shared<QPromise<ConvertResult>>();
    promise->start();
    QFuture<ConvertResult> future = promise->future();
    QThread* thread = QThread::create([promise, work = std::move(work)]() {
        promise->addResult(work());
        promise->finish();
    });
    QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
    return future;
}

QString libreOfficeProgram()
{
    const QString onPath = QStandardPaths::findExecutable(QStringLiteral("soffice"));
    if (!onPath.isEmpty()) {
        return onPath;
    }
    QStringList known;
#if defined(Q_OS_MACOS)
    known << QStringLiteral("/Applications/LibreOffice.app/Contents/MacOS/soffice");
#elif defined(Q_OS_WIN)
    for (const char* variable : {"ProgramFiles", "ProgramFiles(x86)"}) {
        const QString root = qEnvironmentVariable(variable);
        if (!root.isEmpty()) {
            known << root + QStringLiteral("/LibreOffice/program/soffice.exe");
        }
    }
#endif
    for (const QString& path : known) {
        if (QFileInfo::exists(path)) {
            return path;
        }
    }
    return {};
}

#ifdef Q_OS_WIN
bool powerPointInstalled()
{
    const QSettings registry(QStringLiteral("HKEY_CLASSES_ROOT\\PowerPoint.Application"),
                             QSettings::NativeFormat);
    return !registry.childGroups().isEmpty() || !registry.childKeys().isEmpty();
}

QString windowsPowerShell()
{
    const QString root = qEnvironmentVariable("SystemRoot", QStringLiteral("C:/Windows"));
    for (const QString& candidate :
         {root + QStringLiteral("/System32/powershell.exe"),
          root + QStringLiteral("/System32/WindowsPowerShell/v1.0/powershell.exe")}) {
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}
#endif

Converter powerPointVia(const QString& program)
{
    Converter converter;
    converter.program = program;
    // Le chemin du script (copie dans le dossier temporaire) est ajouté par convertToPdf, après « -File ».
    converter.arguments = {QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"),
                           QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"),
                           QStringLiteral("-File")};
    converter.name = QStringLiteral("PowerPoint");
    converter.pathsByEnvironment = true;
    converter.scriptResource = QString::fromLatin1(kScriptResource);
    return converter;
}

QString replaced(QString text, const QString& in, const QString& out, const QString& outdir,
                 const QString& profileUrl)
{
    text.replace(QStringLiteral("{profileurl}"), profileUrl);
    text.replace(QStringLiteral("{in}"), in);
    text.replace(QStringLiteral("{outdir}"), outdir);
    text.replace(QStringLiteral("{out}"), out);
    return text;
}

ConvertResult runConversion(const Converter& converter, const QString& presentationPath)
{
    const QFileInfo input(presentationPath);
    if (!input.isFile()) {
        return failure(QStringLiteral("La présentation est introuvable."));
    }
    if (converter.program.isEmpty()) {
        return failure(QStringLiteral("Aucun convertisseur de présentation n'est disponible."));
    }
    QTemporaryDir dir;
    if (!dir.isValid()) {
        return failure(QStringLiteral("Le dossier temporaire de la conversion n'a pas pu être créé."));
    }
    dir.setAutoRemove(false); // le PDF doit survivre à l'appel ; supprimé en cas d'échec ci-dessous
    const QString outDir = QFileInfo(dir.path()).absoluteFilePath();
    const QString inPath = input.absoluteFilePath();
    const QString outPath = QDir(outDir).filePath(input.completeBaseName() + QStringLiteral(".pdf"));
    const auto fail = [&dir](const QString& error) {
        QDir(dir.path()).removeRecursively();
        return failure(error);
    };

    // Profil LibreOffice propre à cette conversion (-env:UserInstallation={profileurl}) : sans lui, avec
    // LibreOffice déjà ouvert, la commande part à l'instance ouverte et ne produit rien. Supprimé ensuite.
    const QString profileDir = QDir(outDir).filePath(QStringLiteral("profil-lo"));
    const QString profileUrl = QUrl::fromLocalFile(profileDir).toString();
    struct ProfileCleanup
    {
        QString path;
        ~ProfileCleanup() { QDir(path).removeRecursively(); }
    } profileCleanup{profileDir};

    QProcess process;
    QStringList arguments;
    if (converter.pathsByEnvironment) {
        QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
        environment.insert(QStringLiteral("ACTCRAFT_CONVERT_IN"), inPath);
        environment.insert(QStringLiteral("ACTCRAFT_CONVERT_OUT"), outPath);
        process.setProcessEnvironment(environment);
        arguments = converter.arguments;
        if (!converter.scriptResource.isEmpty()) {
            const QString script = QDir(outDir).filePath(QStringLiteral("pptx-to-pdf.ps1"));
            if (!QFile::copy(converter.scriptResource, script)) {
                return fail(QStringLiteral("Le script de conversion n'a pas pu être copié."));
            }
            QFile::setPermissions(script, QFile::ReadOwner | QFile::WriteOwner);
            arguments << script;
        }
    } else {
        for (const QString& argument : converter.arguments) {
            arguments << replaced(argument, inPath, outPath, outDir, profileUrl);
        }
    }
    process.setProgram(converter.program);
    process.setArguments(arguments); // liste : jamais d'interpréteur de commandes
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(QIODevice::ReadOnly);
    if (!process.waitForStarted(10000)) {
        return fail(QStringLiteral("Le convertisseur « %1 » n'a pas pu être lancé.")
                        .arg(converter.name.isEmpty() ? converter.program : converter.name));
    }
    const int configured = g_timeoutMs.load();
    const int timeout = configured > 0 ? configured : kDefaultTimeoutMs;
    if (!process.waitForFinished(timeout)) {
        process.kill();
        process.waitForFinished(5000);
        return fail(QStringLiteral("La conversion a dépassé le délai de %1 secondes.")
                        .arg(QString::number(timeout / 1000.0, 'g', 3)));
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        const QString detail = QString::fromLocal8Bit(process.readAll()).trimmed().left(300);
        return fail(QStringLiteral("La conversion a échoué (code %1).%2")
                        .arg(process.exitCode())
                        .arg(detail.isEmpty() ? QString() : QStringLiteral(" ") + detail));
    }
    if (QFileInfo(outPath).size() <= 0) {
        return fail(QStringLiteral("Le convertisseur n'a produit aucun PDF."));
    }
    ConvertResult result;
    result.ok = true;
    result.pdfPath = outPath; // l'appelant supprime le dossier parent une fois le PDF importé
    return result;
}

} // namespace

namespace detail {
// Couture de test : délai du convertisseur en millisecondes, 0 = 120 s.
void setConvertTimeoutMs(int milliseconds)
{
    g_timeoutMs.store(milliseconds);
}
} // namespace detail

std::optional<Converter> findConverter()
{
    // (1) Variable d'environnement : programme puis arguments séparés par « | ».
    const QString fromEnv = qEnvironmentVariable("ACTCRAFT_OFFICE_CONVERTER");
    if (!fromEnv.isEmpty()) {
        QStringList parts = fromEnv.split(QLatin1Char('|'));
        Converter converter;
        converter.program = parts.takeFirst();
        converter.arguments = parts;
        converter.name = QFileInfo(converter.program).fileName();
        return converter;
    }
    // (2) Couture de diagnostic : voie PowerPoint prise directement, sans contrôle de présence.
    const QString fakeShell = qEnvironmentVariable("ACTCRAFT_POWERSHELL");
    if (!fakeShell.isEmpty()) {
        return powerPointVia(fakeShell);
    }
    // (3) LibreOffice.
    if (const QString soffice = libreOfficeProgram(); !soffice.isEmpty()) {
        Converter converter;
        converter.program = soffice;
        converter.arguments = {
            QStringLiteral("--headless"),   QStringLiteral("-env:UserInstallation={profileurl}"),
            QStringLiteral("--convert-to"), QStringLiteral("pdf"),
            QStringLiteral("--outdir"),     QStringLiteral("{outdir}"),
            QStringLiteral("{in}")};
        converter.name = QStringLiteral("LibreOffice");
        return converter;
    }
#ifdef Q_OS_WIN
    // (4) PowerPoint, par le chemin absolu de powershell.exe.
    if (powerPointInstalled()) {
        if (const QString shell = windowsPowerShell(); !shell.isEmpty()) {
            return powerPointVia(shell);
        }
    }
#endif
    return std::nullopt;
}

QFuture<ConvertResult> convertToPdf(const Converter& converter, const QString& presentationPath)
{
    return runAsync([converter, presentationPath]() { return runConversion(converter, presentationPath); });
}

} // namespace actcraft::exporting
