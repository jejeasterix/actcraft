#include <actcraft/bench/options.h>

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>

namespace actcraft::bench {

namespace {

// --sortie : un fichier « .json », ou un dossier. Toute autre extension, ou un fichier existant qui n'est pas
// « .json », serait pris pour un dossier et en créerait un du même nom : refusé.
bool outputAccepted(const QString& path, QString& error)
{
    const QFileInfo info(path);
    if (info.isDir() || info.suffix().compare(QLatin1String("json"), Qt::CaseInsensitive) == 0) {
        return true;
    }
    if (!info.suffix().isEmpty() || info.isFile()) {
        error = QStringLiteral("--sortie : un fichier .json ou un dossier (reçu : %1)").arg(path);
        return false;
    }
    return true;
}

} // namespace

bool parseBenchOptions(const QStringList& args, BenchOptions& out, QString& error)
{
    out = BenchOptions{};
#ifdef Q_OS_WIN
    out.requestedBackend = QStringLiteral("d3d11");
#else
    out.requestedBackend = QStringLiteral("auto");
#endif
    QSet<QString> seen;
    for (qsizetype i = 1; i < args.size(); ++i) {
        const QString option = args.at(i);
        // app/renderbackend.cpp garde la première occurrence d'une option : en accepter plusieurs ferait
        // mesurer un rendu et en noter un autre.
        if (seen.contains(option)) {
            error = QStringLiteral("%1 donné plusieurs fois").arg(option);
            return false;
        }
        seen.insert(option);
        const auto value = [&](QString& target) {
            if (i + 1 >= args.size()) {
                error = QStringLiteral("%1 attend une valeur").arg(option);
                return false;
            }
            target = args.at(++i);
            return true;
        };
        if (option == QLatin1String("--rendu")) {
            if (!value(out.requestedBackend)) {
                return false;
            }
        } else if (option == QLatin1String("--scenario")) {
            QString v;
            if (!value(v)) {
                return false;
            }
            if (v == QLatin1String("complet")) {
                out.scenarioSet = ScenarioSet::Complet;
            } else if (v == QLatin1String("court")) {
                out.scenarioSet = ScenarioSet::Court;
            } else {
                error = QStringLiteral("--scenario : complet ou court");
                return false;
            }
        } else if (option == QLatin1String("--sortie")) {
            if (!value(out.output) || !outputAccepted(out.output, error)) {
                return false;
            }
        } else if (option == QLatin1String("--sans-cache")) {
            out.cacheEnabled = false;
        } else if (option == QLatin1String("--taille")) {
            QString v;
            if (!value(v)) {
                return false;
            }
            if (v == QLatin1String("plein-ecran")) {
                out.fullScreen = true;
                out.windowSize = QSize();
                continue;
            }
            static const QRegularExpression size(QStringLiteral("^(\\d+)x(\\d+)$"));
            const auto match = size.match(v);
            const int w = match.hasMatch() ? match.captured(1).toInt() : 0;
            const int h = match.hasMatch() ? match.captured(2).toInt() : 0;
            if (w < 320 || h < 240) {
                error = QStringLiteral("--taille : plein-ecran ou LxH (au moins 320x240)");
                return false;
            }
            out.fullScreen = false;
            out.windowSize = QSize(w, h);
        } else if (option == QLatin1String("--captures")) {
            if (!value(out.capturesDir)) {
                return false;
            }
        } else {
            error = QStringLiteral("option inconnue : %1").arg(option);
            return false;
        }
    }
    return true;
}

QString resolveOutputPath(const BenchOptions& options, const QString& defaultDir, const QString& fileName)
{
    if (options.output.endsWith(QLatin1String(".json"), Qt::CaseInsensitive)) {
        return options.output;
    }
    return QDir(options.output.isEmpty() ? defaultDir : options.output).filePath(fileName);
}

} // namespace actcraft::bench
