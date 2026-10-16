#include "flipchartwriter.h"

#include <actcraft/formats/flipchart.h>
#include <actcraft/formats/flipchartdump.h>
#include <actcraft/render/pagepainter.h>

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QJsonDocument>
#include <QPainter>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <vector>

// actcraft-flipchart-dump : outil de preuve, jamais livré.
//
//   actcraft-flipchart-dump <fichier.flipchart> [--sortie <fichier.json>]
//     Export canonique du lecteur (oracle, décision 19). Le JSON seul va sur la sortie standard (ou dans
//     --sortie) ; les messages vont sur la sortie d'erreur, dont la dernière ligne est « DUMP OK : <n> pages
//     » (code 0) ou « DUMP EN ECHEC : <raison> » (code 1).
//
//   actcraft-flipchart-dump --miniatures <fichier.flipchart> [--connues "<fichier>:<page>;…"] [--comparaisons
//   <dossier>] actcraft-flipchart-dump --samples <dossier> [--connues "<fichier>:<page>;…"] [--comparaisons
//   <dossier>]
//     S4 (L2-T21) : rendu de chaque page par le peintre partagé, comparé à sa miniature .thm d'origine. Une
//     ligne « PAGE <fichier>:<n> écart <e> » par page, puis « MINIATURES OK : <n> pages, médiane <m>, <k>
//     au-dessus de 7 » (code 0 : médiane ≤ 2,0 et k ≤ 3) ou « MINIATURES EN ECHEC : <raison> » (code 1). Avec
//     --connues (page « * » : toutes celles du fichier), toute page au-dessus de 7 hors de la liste fait
//     échouer, et elle est nommée.
//     --samples : tous les .flipchart du dossier, par nom ; code 77 si le dossier est absent ou vide (test
//     sauté).
//     --comparaisons : rendu et miniature côte à côte, en PNG, pour chaque page au-dessus de 7 (sous .tmp/).
//
//   actcraft-flipchart-dump --ecrire-synthetique <fichier.flipchart> [--miniatures-alterees]
//     Écrit fctest::sampleFlipchart avec des miniatures JPEG rendues par notre peintre (chemin prouvé en CI,
//     pas la fidélité) ; --miniatures-alterees peint en noir le quart droit de chacune. « SYNTHETIQUE OK :
//     <fichier> ».

namespace {

namespace render = actcraft::render;
namespace flipchart = actcraft::formats::flipchart;

constexpr int kGapWidth = 80;
constexpr int kGapHeight = 60;
constexpr double kMedianLimit = 2.0;
constexpr double kPageLimit = 7.0;
constexpr int kPagesAboveLimit = 3;
constexpr int kSkipped = 77;

void printLine(const QString& line)
{
    std::printf("%s\n", line.toUtf8().constData()); // UTF-8 : accents intacts, comme actcraft-captures
    std::fflush(stdout);
}

QString number(double value)
{
    return QString::number(value, 'f', 2);
}

// Image ramenée à 80 × 60 par lissage, puis en RGB 8 bits : la documentation de QImage::scaled ne garantit
// pas le format rendu (Qt 6.12 garde le RGB 8 bits, vérifié), et thumbnailGap lit ses lignes comme des
// triplets RGB.
QImage gapImage(const QImage& image)
{
    return image.scaled(kGapWidth, kGapHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
        .convertToFormat(QImage::Format_RGB888);
}

// Écart moyen (0 à 255) entre deux images ramenées à 80 × 60 : moyenne des |a − b| sur les trois canaux et
// tous les pixels (mesure de S4, écrite par nous).
std::optional<double> thumbnailGap(const QImage& rendered, const QImage& thumbnail)
{
    const QImage a = gapImage(rendered);
    const QImage b = gapImage(thumbnail);
    if (a.format() != QImage::Format_RGB888 || b.format() != QImage::Format_RGB888 ||
        a.size() != QSize(kGapWidth, kGapHeight) || b.size() != a.size()) {
        return std::nullopt;
    }
    double total = 0;
    for (int y = 0; y < kGapHeight; ++y) {
        const uchar* la = a.constScanLine(y);
        const uchar* lb = b.constScanLine(y);
        for (int x = 0; x < kGapWidth * 3; ++x) {
            total += std::abs(static_cast<int>(la[x]) - static_cast<int>(lb[x]));
        }
    }
    return total / (kGapWidth * kGapHeight * 3.0);
}

QImage sideBySide(const QImage& rendered, const QImage& thumbnail)
{
    const QImage left = rendered.scaled(thumbnail.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QImage out(thumbnail.width() * 2 + 4, thumbnail.height(), QImage::Format_RGB32);
    out.fill(Qt::black);
    QPainter p(&out);
    p.drawImage(0, 0, left);
    p.drawImage(thumbnail.width() + 4, 0, thumbnail);
    p.end();
    return out;
}

struct PageGap
{
    QString file; // nom du fichier, sans dossier
    int page = 0; // à partir de 1
    double gap = 0;
};

// Pages à miniature d'un paperboard, rendues comme les miniatures de l'application (peintre partagé, échelle
// de miniature) à la largeur de leur .thm ; `withoutThumbnail` compte les autres.
bool pageGaps(const QString& path, std::vector<PageGap>& gaps, int& withoutThumbnail,
              const QString& comparisons, QString& error)
{
    const QString name = QFileInfo(path).fileName();
    QFile in(path);
    if (!in.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("ouverture impossible : %1").arg(path);
        return false;
    }
    const flipchart::FlipchartReadResult r = flipchart::readFlipchart(in.readAll());
    if (!r.ok) {
        error = QStringLiteral("%1 : %2").arg(name, r.error);
        return false;
    }
    const render::DecodedImages images = render::decodeImages(r.document);
    for (std::size_t i = 0; i < r.document.pageCount(); ++i) {
        const QImage thumbnail = i < r.thumbnails.size() ? QImage::fromData(r.thumbnails[i]) : QImage();
        if (thumbnail.isNull()) {
            ++withoutThumbnail;
            continue;
        }
        const actcraft::core::Page& page = r.document.page(i);
        const double scale = page.width > 0 ? thumbnail.width() / page.width : 1.0;
        render::PaintOptions options;
        options.thumbnail = true;
        const QImage rendered = render::renderPage(page, images.lookup(), scale, options);
        const std::optional<double> gap = thumbnailGap(rendered, thumbnail);
        if (!gap) {
            error = QStringLiteral("%1 page %2 : images de mesure invalides").arg(name).arg(i + 1);
            return false;
        }
        PageGap entry;
        entry.file = name;
        entry.page = static_cast<int>(i) + 1;
        entry.gap = *gap;
        printLine(
            QStringLiteral("PAGE %1:%2 écart %3").arg(entry.file).arg(entry.page).arg(number(entry.gap)));
        if (entry.gap > kPageLimit && !comparisons.isEmpty()) {
            const QString target =
                QDir(comparisons)
                    .filePath(
                        QStringLiteral("%1-p%2.png").arg(QFileInfo(path).completeBaseName()).arg(entry.page));
            if (!QDir().mkpath(comparisons) || !sideBySide(rendered, thumbnail).save(target, "PNG")) {
                error = QStringLiteral("écriture impossible : %1").arg(target);
                return false;
            }
        }
        gaps.push_back(entry);
    }
    return true;
}

struct KnownPage
{
    QString file;
    int page = 0; // 0 : toutes les pages du fichier (« * »)
};

bool parseKnown(const QString& text, std::vector<KnownPage>& known, QString& error)
{
    for (const QString& raw : text.split(QLatin1Char(';'), Qt::SkipEmptyParts)) {
        const QString item = raw.trimmed();
        const qsizetype colon = item.lastIndexOf(QLatin1Char(':'));
        KnownPage k;
        k.file = item.left(colon).trimmed();
        const QString page = item.mid(colon + 1).trimmed();
        bool ok = page == QLatin1String("*");
        if (!ok) {
            k.page = page.toInt(&ok);
            ok = ok && k.page >= 1;
        }
        if (colon <= 0 || k.file.isEmpty() || !ok) {
            error = QStringLiteral("page connue invalide « %1 » (attendu <fichier>:<page> ou <fichier>:*)")
                        .arg(item);
            return false;
        }
        known.push_back(k);
    }
    return true;
}

bool isKnown(const PageGap& g, const std::vector<KnownPage>& known)
{
    return std::any_of(known.begin(), known.end(), [&g](const KnownPage& k) {
        return k.file.compare(g.file, Qt::CaseInsensitive) == 0 && (k.page == 0 || k.page == g.page);
    });
}

// Synthèse et verdict de S4 ; dernière ligne de la sortie.
int verdict(const std::vector<PageGap>& gaps, int withoutThumbnail, bool hasKnownList,
            const std::vector<KnownPage>& known)
{
    if (gaps.empty()) {
        printLine(QStringLiteral("MINIATURES EN ECHEC : aucune page à miniature (%1 sans miniature)")
                      .arg(withoutThumbnail));
        return 1;
    }
    std::vector<double> sorted;
    sorted.reserve(gaps.size());
    for (const PageGap& g : gaps) {
        sorted.push_back(g.gap);
    }
    std::sort(sorted.begin(), sorted.end());
    const std::size_t n = sorted.size();
    const double median = n % 2 == 1 ? sorted[n / 2] : (sorted[n / 2 - 1] + sorted[n / 2]) / 2.0;
    QStringList unknown;
    int above = 0;
    for (const PageGap& g : gaps) {
        if (g.gap > kPageLimit) {
            ++above;
            if (hasKnownList && !isKnown(g, known)) {
                unknown << QStringLiteral("page %1:%2 au-dessus de 7 et non connue").arg(g.file).arg(g.page);
            }
        }
    }
    QString summary = QStringLiteral("%1 pages, médiane %2, %3 au-dessus de 7")
                          .arg(static_cast<int>(n))
                          .arg(number(median))
                          .arg(above);
    if (withoutThumbnail > 0) {
        summary += QStringLiteral(", %1 sans miniature").arg(withoutThumbnail);
    }
    if (!hasKnownList) {
        summary += QStringLiteral(", liste des pages connues absente");
    }
    printLine(QStringLiteral("SYNTHESE : %1").arg(summary));
    if (!unknown.isEmpty()) {
        printLine(QStringLiteral("MINIATURES EN ECHEC : %1").arg(unknown.join(QStringLiteral(" ; "))));
        return 1;
    }
    const bool ok = median <= kMedianLimit && above <= kPagesAboveLimit;
    printLine(QStringLiteral("MINIATURES %1 : %2")
                  .arg(ok ? QStringLiteral("OK") : QStringLiteral("EN ECHEC"), summary));
    return ok ? 0 : 1;
}

int writeSynthetic(const QString& path, bool altered)
{
    const auto fail = [](const QString& why) {
        printLine(QStringLiteral("SYNTHETIQUE EN ECHEC : %1").arg(why));
        return 1;
    };
    const flipchart::FlipchartReadResult r = flipchart::readFlipchart(fctest::sampleFlipchart());
    if (!r.ok) {
        return fail(r.error);
    }
    // Miniatures comme celles du format natif (documentio.cpp) : peintre partagé, images réduites au double
    // de la largeur de miniature, puis encodées en JPEG comme les .thm.
    const render::DecodedImages images = render::decodeImages(r.document, 2 * render::kThumbnailWidth);
    std::vector<QByteArray> jpegs;
    for (std::size_t i = 0; i < r.document.pageCount(); ++i) {
        QImage image = QImage::fromData(render::thumbnailPng(r.document.page(i), images.lookup()));
        if (image.isNull()) {
            return fail(QStringLiteral("miniature de la page %1 vide").arg(i + 1));
        }
        if (altered) {
            QPainter p(&image);
            p.fillRect(QRect(image.width() * 3 / 4, 0, image.width() - image.width() * 3 / 4, image.height()),
                       Qt::black);
        }
        // Qualité 100 : à 90, le sous-échantillonnage des couleurs du JPEG brouille les traits fins de la
        // grille (écart 4,47 sur la page 1, contre 0,00 en PNG et 0,39 à 100, mesuré) ; la preuve porte sur
        // le chemin.
        QByteArray jpeg;
        QBuffer buffer(&jpeg);
        buffer.open(QIODevice::WriteOnly);
        if (!image.save(&buffer, "JPG", 100)) {
            return fail(QStringLiteral("encodage JPEG impossible (greffon d'images absent ?)"));
        }
        jpegs.push_back(jpeg);
    }
    const QByteArray data = fctest::sampleFlipchart(jpegs);
    QFile out(path);
    if (!QDir().mkpath(QFileInfo(path).absolutePath()) || !out.open(QIODevice::WriteOnly) ||
        out.write(data) != data.size()) {
        return fail(QStringLiteral("écriture impossible : %1").arg(path));
    }
    printLine(QStringLiteral("SYNTHETIQUE OK : %1").arg(path));
    return 0;
}

// Valeur d'une option : argument suivant, vide s'il manque ou s'il est lui-même une option (ctest peut ne pas
// transmettre un argument vide, quand la variable d'environnement lue à la configuration est absente).
QString optionValue(const QStringList& args, const QString& name, bool& present)
{
    const qsizetype at = args.indexOf(name);
    present = at >= 0;
    if (!present || at + 1 >= args.size() || args.at(at + 1).startsWith(QLatin1String("--"))) {
        return {};
    }
    return args.at(at + 1);
}

int compareThumbnails(const QStringList& args)
{
    const auto fail = [](const QString& why) {
        printLine(QStringLiteral("MINIATURES EN ECHEC : %1").arg(why));
        return 1;
    };
    bool hasFile = false;
    bool hasSamples = false;
    bool unused = false;
    const QString file = optionValue(args, QStringLiteral("--miniatures"), hasFile);
    const QString samples = optionValue(args, QStringLiteral("--samples"), hasSamples);
    const QString knownText = optionValue(args, QStringLiteral("--connues"), unused);
    const QString comparisons = optionValue(args, QStringLiteral("--comparaisons"), unused);
    std::vector<KnownPage> known;
    QString error;
    if (!parseKnown(knownText, known, error)) {
        return fail(error);
    }
    QStringList files;
    if (hasSamples) {
        // Chemin vide : jamais QDir(""), qui désigne le dossier courant (le dossier de construction sous
        // ctest).
        const QDir dir(samples);
        if (!samples.isEmpty() && dir.exists()) {
            for (const QString& entry :
                 dir.entryList({QStringLiteral("*.flipchart")}, QDir::Files, QDir::Name)) {
                files << dir.filePath(entry);
            }
        }
        if (files.isEmpty()) {
            printLine(
                QStringLiteral("MINIATURES SAUTEES : aucun .flipchart dans « %1 » (ACTCRAFT_SAMPLES_DIR)")
                    .arg(samples));
            return kSkipped;
        }
    } else if (!file.isEmpty()) {
        files << file;
    } else {
        return fail(QStringLiteral("--miniatures <fichier> ou --samples <dossier> attendu"));
    }
    std::vector<PageGap> gaps;
    int withoutThumbnail = 0;
    for (const QString& f : files) {
        if (!pageGaps(f, gaps, withoutThumbnail, comparisons, error)) {
            return fail(error);
        }
    }
    return verdict(gaps, withoutThumbnail, !known.empty(), known);
}

int dump(const QStringList& args)
{
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
    const auto r = flipchart::readFlipchart(in.readAll());
    if (!r.ok) {
        return fail(r.error);
    }
    for (const QString& w : r.warnings) {
        std::fprintf(stderr, "AVERTISSEMENT %s\n", qPrintable(w));
    }
    const QByteArray json = QJsonDocument(flipchart::canonicalDump(r)).toJson(QJsonDocument::Indented);
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

} // namespace

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv); // le peintre dessine du texte : polices de la plateforme
    const QStringList args = QGuiApplication::arguments();
    const qsizetype synthetic = args.indexOf(QStringLiteral("--ecrire-synthetique"));
    if (synthetic >= 0) {
        if (synthetic + 1 >= args.size() || args.at(synthetic + 1).startsWith(QLatin1String("--"))) {
            printLine(QStringLiteral("SYNTHETIQUE EN ECHEC : --ecrire-synthetique <fichier> attendu"));
            return 1;
        }
        return writeSynthetic(args.at(synthetic + 1), args.contains(QStringLiteral("--miniatures-alterees")));
    }
    if (args.contains(QStringLiteral("--miniatures")) || args.contains(QStringLiteral("--samples"))) {
        return compareThumbnails(args);
    }
    return dump(args); // chemin d'origine, par défaut
}
