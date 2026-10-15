#include <actcraft/formats/flipchart.h>
#include <actcraft/formats/flipchartdump.h>

#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>

#include <cstdio>

// actcraft-flipchart-dump <fichier.flipchart> [--sortie <fichier.json>] : export canonique du lecteur
// (oracle, décision 19). Outil de preuve, jamais livré. Le JSON seul va sur la sortie standard (ou dans
// --sortie) ; les messages vont sur la sortie d'erreur, dont la dernière ligne est
// « DUMP OK : <n> pages » (code 0) ou « DUMP EN ECHEC : <raison> » (code 1).
int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    const QStringList args = QCoreApplication::arguments();
    const auto fail = [](const QString& why) {
        std::fprintf(stderr, "DUMP EN ECHEC : %s\n", qPrintable(why));
        return 1;
    };
    const QString usage =
        QStringLiteral("usage : actcraft-flipchart-dump <fichier.flipchart> [--sortie <fichier.json>]");
    QString input;
    QString output;
    for (qsizetype i = 1; i < args.size(); ++i) {
        if (args.at(i) == QLatin1String("--sortie")) {
            if (i + 1 >= args.size() || args.at(i + 1).startsWith(QLatin1String("--")) || !output.isEmpty()) {
                return fail(usage);
            }
            output = args.at(++i);
        } else if (args.at(i).startsWith(QLatin1String("--")) || !input.isEmpty()) {
            return fail(usage);
        } else {
            input = args.at(i);
        }
    }
    if (input.isEmpty()) {
        return fail(usage);
    }
    QFile in(input);
    if (!in.open(QIODevice::ReadOnly)) {
        return fail(QStringLiteral("ouverture impossible : %1").arg(input));
    }
    const auto r = actcraft::formats::flipchart::readFlipchart(in.readAll());
    if (!r.ok) {
        return fail(r.error);
    }
    for (const QString& w : r.warnings) {
        std::fprintf(stderr, "AVERTISSEMENT %s\n", qPrintable(w));
    }
    const QByteArray json =
        QJsonDocument(actcraft::formats::flipchart::canonicalDump(r)).toJson(QJsonDocument::Indented);
    if (!output.isEmpty()) {
        QFile out(output);
        if (!out.open(QIODevice::WriteOnly) || out.write(json) != json.size()) {
            return fail(QStringLiteral("écriture impossible : %1").arg(output));
        }
    } else if (std::fwrite(json.constData(), 1, static_cast<std::size_t>(json.size()), stdout) !=
               static_cast<std::size_t>(json.size())) {
        return fail(QStringLiteral("écriture impossible sur la sortie standard"));
    }
    std::fflush(stdout);
    std::fprintf(stderr, "DUMP OK : %d pages\n", static_cast<int>(r.document.pageCount()));
    return 0;
}
