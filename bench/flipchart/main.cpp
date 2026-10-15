#include <actcraft/formats/flipchart.h>
#include <actcraft/formats/flipchartdump.h>

#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>

#include <cstdio>

// actcraft-flipchart-dump <fichier.flipchart> [--sortie <fichier.json>] : export canonique du lecteur
// (oracle, décision 19). Outil de preuve, jamais livré.
// Dernière ligne : « DUMP OK : <n> pages » (0) ou « DUMP EN ECHEC : <raison> » (1).
int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    const QStringList args = QCoreApplication::arguments();
    const auto fail = [](const QString& why) {
        std::printf("DUMP EN ECHEC : %s\n", qPrintable(why));
        return 1;
    };
    if (args.size() < 2 || args.at(1).startsWith(QLatin1String("--"))) {
        return fail(
            QStringLiteral("usage : actcraft-flipchart-dump <fichier.flipchart> [--sortie <fichier.json>]"));
    }
    QFile in(args.at(1));
    if (!in.open(QIODevice::ReadOnly)) {
        return fail(QStringLiteral("ouverture impossible : %1").arg(args.at(1)));
    }
    const auto r = actcraft::formats::flipchart::readFlipchart(in.readAll());
    if (!r.ok) {
        return fail(r.error);
    }
    for (const QString& w : r.warnings) {
        std::printf("AVERTISSEMENT %s\n", qPrintable(w));
    }
    const QByteArray json = QJsonDocument(actcraft::formats::flipchart::canonicalDump(r.document))
                                .toJson(QJsonDocument::Indented);
    const qsizetype at = args.indexOf(QStringLiteral("--sortie"));
    if (at > 0 && at + 1 < args.size()) {
        QFile out(args.at(at + 1));
        if (!out.open(QIODevice::WriteOnly) || out.write(json) != json.size()) {
            return fail(QStringLiteral("écriture impossible : %1").arg(args.at(at + 1)));
        }
    } else {
        std::fwrite(json.constData(), 1, static_cast<std::size_t>(json.size()), stdout);
    }
    std::printf("DUMP OK : %d pages\n", static_cast<int>(r.document.pageCount()));
    return 0;
}
