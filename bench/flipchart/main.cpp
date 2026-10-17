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
#include <QImageWriter>
#include <QJsonDocument>
#include <QPainter>

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <vector>

// actcraft-flipchart-dump : outil de preuve, jamais livré.
//
// actcraft-flipchart-dump <fichier.flipchart> [--sortie <fichier.json>]
//   Export canonique du lecteur (oracle, décision 19). Le JSON seul va sur la sortie standard (ou dans
//   --sortie) ; les messages vont sur la sortie d'erreur, dont la dernière ligne est « DUMP OK : <n> pages »
//   (code 0) ou « DUMP EN ECHEC : <raison> » (code 1).
//
// actcraft-flipchart-dump --miniatures <fichier.flipchart> [options]
// actcraft-flipchart-dump --samples <dossier> --pages-attendues <n> [options]
//   S4 (L2-T21) : rendu de chaque page par le peintre partagé, comparé à sa miniature .thm d'origine. Une
//   ligne « PAGE <fichier>:<n> écart <e> » par page (ou « sans miniature », « miniature illisible »), une
//   ligne « AVERTISSEMENT » par avertissement du lecteur, « SYNTHESE : … », puis « MINIATURES OK : <n> pages,
//   médiane <m>, <k> au-dessus de 7 » (code 0 : médiane ≤ 2,0, k ≤ 3) ou « MINIATURES EN ECHEC : <raison> »
//   (code 1). Échoue aussi si une page lue manque à la mesure, ou si le nombre de pages lues diffère de
//   --pages-attendues (obligatoire avec --samples). --samples : tous les .flipchart du dossier, par nom ;
//   code 77 si le dossier est absent ou vide (test sauté).
//   --connues "<fichier>:<page>;…" : toute page au-dessus de 7 hors de la liste fait échouer et est nommée ;
//   « <fichier>:* » excuse une page quelconque du fichier, une seule.
//   --comparaisons <dossier> : rendu et miniature côte à côte (PNG) pour chaque page au-dessus de 7 ; dossier
//   sous .tmp/ ou build/ du dépôt seulement (les PNG contiennent les vraies miniatures).
//   Miniature JPEG : notre rendu passe d'abord par le même codec (QImageWriter, JPEG), à la qualité estimée
//   par la table de quantification de la luminance de la miniature (table de la norme mise à l'échelle par la
//   formule de libjpeg ; 50, la qualité relevée sur les .thm, si la table manque), et le JPEG écrit doit
//   avoir le même échantillonnage des couleurs que la miniature, sinon la page échoue. Sans cela, l'écart
//   mesure aussi la compression : un rendu parfait s'écarte de sa propre version JPEG de qualité 50 de 7,80
//   sur une page d'aplats saturés (mesuré, L2-C7). Miniature d'un autre format (PNG…) : comparée sans
//   recompression.
//   --sans-recompression : rendu comparé tel quel, même à une miniature JPEG (preuve que la règle compte).
//
// actcraft-flipchart-dump --ecrire-synthetique <fichier.flipchart> [variante]
//   Écrit fctest::sampleFlipchart avec des miniatures JPEG rendues par notre peintre (chemin prouvé en CI,
//   pas la fidélité), « SYNTHETIQUE OK : <fichier> ». Variantes : --miniatures-alterees (quart droit noir),
//   --miniatures-trouees (page 1 vide, page 2 indécodable), --miniatures-geantes (2000 px de large),
//   --miniatures-q50 (JPEG de qualité 50, comme les .thm, au lieu de 100) ; --miniatures-alterees avec
//   --miniatures-q50 : quart droit noir ET qualité 50.

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

// --- Miniatures JPEG : rendu recompressé comme la miniature avant la mesure
// ---------------------------------------------------------------------------------------

constexpr int kDefaultJpegQuality = 50; // qualité relevée sur les .thm (tables de la norme à 50)

// Table de quantification de la luminance de la norme JPEG (UIT-T T.81, annexe K, tableau K.1).
constexpr std::array<int, 64> kStandardLumaTable = {
    16, 11,  10,  16, 24, 40, 51, 61, 12,  12,  14,  19,  26, 58, 60, 55,  14,  13,  16,  24, 40, 57,
    69, 56,  14,  17, 22, 29, 51, 87, 80,  62,  18,  22,  37, 56, 68, 109, 103, 77,  24,  35, 55, 64,
    81, 104, 113, 92, 49, 64, 78, 87, 103, 121, 120, 101, 72, 92, 95, 98,  112, 100, 103, 99};

// Ce que la mesure lit d'un JPEG : table 0 (luminance) et échantillonnage de chaque composante (h × 16 + v).
struct JpegLayout
{
    bool isJpeg = false;
    std::vector<int> luma;     // 64 valeurs dans l'ordre du fichier ; vide si absente
    std::vector<int> sampling; // dans l'ordre du SOF
};

// Marqueurs lus jusqu'au début des données compressées (SOS), bornes vérifiées : fichier non fiable.
JpegLayout jpegLayout(const QByteArray& data)
{
    JpegLayout out;
    const auto at = [&data](qsizetype i) { return static_cast<int>(static_cast<uchar>(data.at(i))); };
    if (data.size() < 4 || at(0) != 0xFF || at(1) != 0xD8) {
        return out;
    }
    out.isJpeg = true;
    qsizetype i = 2;
    while (i + 4 <= data.size() && at(i) == 0xFF) {
        const int marker = at(i + 1);
        if (marker == 0xFF) { // remplissage
            ++i;
            continue;
        }
        if (marker == 0x01 || (marker >= 0xD0 && marker <= 0xD8)) { // marqueurs sans longueur
            i += 2;
            continue;
        }
        if (marker == 0xD9 || marker == 0xDA) {
            break;
        }
        const qsizetype length = (at(i + 2) << 8) | at(i + 3);
        const qsizetype end = i + 2 + length;
        if (length < 2 || end > data.size()) {
            break;
        }
        const qsizetype segment = i + 4;
        if (marker == 0xDB) {
            for (qsizetype j = segment; j < end;) {
                const bool wide = (at(j) >> 4) != 0;
                const int id = at(j) & 0x0F;
                const qsizetype size = wide ? 128 : 64;
                if (j + 1 + size > end) {
                    break;
                }
                if (id == 0) {
                    out.luma.clear();
                    for (qsizetype k = 0; k < 64; ++k) {
                        out.luma.push_back(wide ? (at(j + 1 + 2 * k) << 8) | at(j + 2 + 2 * k)
                                                : at(j + 1 + k));
                    }
                }
                j += 1 + size;
            }
        } else if ((marker == 0xC0 || marker == 0xC1 || marker == 0xC2) && segment + 6 <= end) {
            const int components = at(segment + 5);
            out.sampling.clear();
            for (int k = 0; k < components && segment + 8 + 3 * k <= end; ++k) {
                out.sampling.push_back(at(segment + 7 + 3 * k));
            }
        }
        i = end;
    }
    return out;
}

// Somme de la table de la norme mise à l'échelle pour une qualité (formule de libjpeg, valeurs de 1 à 255).
int scaledLumaSum(int quality)
{
    const int scale = quality < 50 ? 5000 / quality : 200 - 2 * quality;
    int sum = 0;
    for (const int v : kStandardLumaTable) {
        sum += std::clamp((v * scale + 50) / 100, 1, 255);
    }
    return sum;
}

// Ordre en zigzag du fichier : la valeur k d'une table de quantification est celle de la case kZigzag[k] de
// la table de la norme (donnée ligne par ligne).
constexpr std::array<int, 64> kZigzag = {0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,
                                         12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6,  7,  14, 21, 28,
                                         35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
                                         58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63};

// Table de la norme mise à l'échelle pour une qualité (même formule que scaledLumaSum), dans l'ordre du
// fichier.
std::vector<int> scaledLuma(int quality)
{
    const int scale = quality < 50 ? 5000 / quality : 200 - 2 * quality;
    std::vector<int> out;
    for (const int natural : kZigzag) {
        out.push_back(
            std::clamp((kStandardLumaTable[static_cast<std::size_t>(natural)] * scale + 50) / 100, 1, 255));
    }
    return out;
}

struct QualityEstimate
{
    int quality = kDefaultJpegQuality;
    bool usual = false; // la table de la miniature est exactement une table standard de cette qualité
};

// Qualité dont la table mise à l'échelle a la somme la plus proche de celle de la miniature (exacte pour un
// JPEG de libjpeg ; la plus haute en cas d'égalité). `usual` faux : table absente ou qui ne correspond à
// aucune qualité standard (la qualité estimée n'est alors qu'une approximation).
QualityEstimate estimatedQuality(const std::vector<int>& luma)
{
    QualityEstimate out;
    if (luma.size() != kStandardLumaTable.size()) {
        return out;
    }
    int sum = 0;
    for (const int v : luma) {
        sum += v;
    }
    int bestDiff = INT_MAX;
    for (int q = 100; q >= 1; --q) {
        const int diff = std::abs(scaledLumaSum(q) - sum);
        if (diff < bestDiff) {
            out.quality = q;
            bestDiff = diff;
        }
    }
    out.usual = out.quality >= 1 && out.quality <= 100 && scaledLuma(out.quality) == luma;
    return out;
}

QString samplingText(const std::vector<int>& sampling)
{
    QStringList parts;
    for (const int s : sampling) {
        parts << QStringLiteral("%1x%2").arg(s >> 4).arg(s & 0x0F);
    }
    return parts.isEmpty() ? QStringLiteral("inconnu") : parts.join(QLatin1Char(','));
}

// Rendu passé par le codec JPEG de Qt comme la miniature : même qualité, même échantillonnage (vérifié sur le
// JPEG écrit, le codec ne le laisse pas choisir).
std::optional<QImage> asThumbnailJpeg(const QImage& rendered, const JpegLayout& thumbnail, int quality,
                                      QString& why)
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    QImageWriter writer(&buffer, "jpeg");
    writer.setQuality(quality);
    if (!writer.write(rendered)) {
        why = QStringLiteral("recompression JPEG impossible (%1)").arg(writer.errorString());
        return std::nullopt;
    }
    buffer.close();
    const JpegLayout written = jpegLayout(bytes);
    if (written.sampling != thumbnail.sampling) {
        why = QStringLiteral(
                  "échantillonnage %1 de la miniature non reproduit par le codec à la qualité %2 (%3)")
                  .arg(samplingText(thumbnail.sampling))
                  .arg(quality)
                  .arg(samplingText(written.sampling));
        return std::nullopt;
    }
    QImage image = QImage::fromData(bytes, "JPEG");
    if (image.isNull()) {
        why = QStringLiteral("rendu recompressé illisible");
        return std::nullopt;
    }
    return image;
}

struct PageGap
{
    QString file; // nom du fichier, sans dossier
    int page = 0; // à partir de 1
    double gap = 0;
};

// Pages lues qui manquent à la mesure, et avertissements du lecteur : comptés et imprimés, jamais tus.
struct Tally
{
    int pagesRead = 0;
    int withoutThumbnail = 0;    // .thm absent ou vide
    int unreadableThumbnail = 0; // .thm présent mais indécodable
    int warnings = 0;
};

// Rendu borné comme render::thumbnailPng : la miniature d'un fichier non fiable ne commande pas la mémoire.
constexpr int kMaxRenderWidth = 4 * render::kThumbnailWidth;
constexpr int kMaxRenderHeight = render::kThumbnailMaxHeight;

// Pages d'un paperboard rendues comme les miniatures de l'application (peintre partagé, échelle de miniature)
// à la largeur de leur .thm.
bool pageGaps(const QString& path, std::vector<PageGap>& gaps, Tally& tally, const QString& comparisons,
              bool recompress, QString& error)
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
    for (const QString& w : r.warnings) {
        printLine(QStringLiteral("AVERTISSEMENT %1 : %2").arg(name, w));
    }
    tally.warnings += static_cast<int>(r.warnings.size());
    tally.pagesRead += static_cast<int>(r.document.pageCount());
    const render::DecodedImages images = render::decodeImages(r.document);
    for (std::size_t i = 0; i < r.document.pageCount(); ++i) {
        const int pageNumber = static_cast<int>(i) + 1;
        const QByteArray bytes = i < r.thumbnails.size() ? r.thumbnails[i] : QByteArray();
        if (bytes.isEmpty()) {
            ++tally.withoutThumbnail;
            printLine(QStringLiteral("PAGE %1:%2 sans miniature").arg(name).arg(pageNumber));
            continue;
        }
        const QImage thumbnail = QImage::fromData(bytes);
        if (thumbnail.isNull()) {
            ++tally.unreadableThumbnail;
            printLine(QStringLiteral("PAGE %1:%2 miniature illisible").arg(name).arg(pageNumber));
            continue;
        }
        const actcraft::core::Page& page = r.document.page(i);
        if (!std::isfinite(page.width) || !std::isfinite(page.height) || page.width <= 0 ||
            page.height <= 0) {
            error = QStringLiteral("%1 page %2 : taille de page invalide").arg(name).arg(pageNumber);
            return false;
        }
        const double scale = thumbnail.width() / page.width;
        const double width = page.width * scale;
        const double height = page.height * scale;
        if (!std::isfinite(height) || width > kMaxRenderWidth || height > kMaxRenderHeight) {
            error = QStringLiteral("%1 page %2 : rendu de %3 × %4 refusé (au plus %5 × %6)")
                        .arg(name)
                        .arg(pageNumber)
                        .arg(QString::number(width, 'f', 0))
                        .arg(std::isfinite(height) ? QString::number(height, 'f', 0) : QStringLiteral("∞"))
                        .arg(kMaxRenderWidth)
                        .arg(kMaxRenderHeight);
            return false;
        }
        render::PaintOptions options;
        options.thumbnail = true;
        QImage rendered = render::renderPage(page, images.lookup(), scale, options);
        const JpegLayout layout = jpegLayout(bytes);
        QString qualityText;
        if (layout.isJpeg) {
            const QualityEstimate estimate = estimatedQuality(layout.luma);
            qualityText = QStringLiteral(" qualité %1").arg(estimate.quality);
            if (!estimate.usual) {
                qualityText += QStringLiteral(", qualité inhabituelle (table hors des qualités standard)");
            }
        }
        if (layout.isJpeg && recompress) {
            QString why;
            std::optional<QImage> jpeg =
                asThumbnailJpeg(rendered, layout, estimatedQuality(layout.luma).quality, why);
            if (!jpeg) {
                error = QStringLiteral("%1 page %2 : %3").arg(name).arg(pageNumber).arg(why);
                return false;
            }
            rendered = std::move(*jpeg);
        }
        const std::optional<double> gap = thumbnailGap(rendered, thumbnail);
        if (!gap) {
            error = QStringLiteral("%1 page %2 : images de mesure invalides").arg(name).arg(pageNumber);
            return false;
        }
        PageGap entry;
        entry.file = name;
        entry.page = pageNumber;
        entry.gap = *gap;
        printLine(QStringLiteral("PAGE %1:%2 écart %3%4")
                      .arg(entry.file)
                      .arg(entry.page)
                      .arg(number(entry.gap), qualityText));
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
    int page = 0; // 0 : une page quelconque du fichier (« * »)
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

bool sameFile(const QString& a, const QString& b)
{
    return a.compare(b, Qt::CaseInsensitive) == 0;
}

// Page au-dessus de 7 excusée par la liste : « fichier:n » excuse la page n ; « fichier:* » excuse UNE page
// quelconque du fichier, la première rencontrée (`starUsed` le retient), jamais toutes.
bool excused(const PageGap& g, const std::vector<KnownPage>& known, std::vector<bool>& starUsed)
{
    for (const KnownPage& k : known) {
        if (k.page == g.page && sameFile(k.file, g.file)) {
            return true;
        }
    }
    for (std::size_t i = 0; i < known.size(); ++i) {
        if (known[i].page == 0 && !starUsed[i] && sameFile(known[i].file, g.file)) {
            starUsed[i] = true;
            return true;
        }
    }
    return false;
}

// Synthèse et verdict de S4 ; dernière ligne de la sortie. Toute page lue qui manque à la mesure fait
// échouer ; `expectedPages` > 0 : le nombre de pages lues doit l'égaler (pages écartées par le lecteur).
int verdict(const std::vector<PageGap>& gaps, const Tally& tally, int expectedPages,
            const std::vector<KnownPage>& known)
{
    const bool hasKnownList = !known.empty();
    std::vector<double> sorted;
    sorted.reserve(gaps.size());
    for (const PageGap& g : gaps) {
        sorted.push_back(g.gap);
    }
    std::sort(sorted.begin(), sorted.end());
    const std::size_t n = sorted.size();
    double median = 0;
    if (n > 0) {
        median = n % 2 == 1 ? sorted[n / 2] : (sorted[n / 2 - 1] + sorted[n / 2]) / 2.0;
    }
    QStringList unknown;
    std::vector<bool> starUsed(known.size(), false);
    const QString limit = QString::number(kPageLimit);
    int above = 0;
    for (const PageGap& g : gaps) {
        if (g.gap > kPageLimit) {
            ++above;
            if (hasKnownList && !excused(g, known, starUsed)) {
                unknown << QStringLiteral("page %1:%2 au-dessus de %3 et non connue")
                               .arg(g.file)
                               .arg(g.page)
                               .arg(limit);
            }
        }
    }
    // Pages de la liste redescendues sous 7 : signalées (la liste est à réviser), sans faire échouer.
    for (std::size_t i = 0; i < known.size(); ++i) {
        if (known[i].page > 0) {
            for (const PageGap& g : gaps) {
                if (g.page == known[i].page && sameFile(known[i].file, g.file) && g.gap <= kPageLimit) {
                    printLine(QStringLiteral("CONNUE SOUS %1 : %2:%3 (%4)")
                                  .arg(limit, g.file)
                                  .arg(g.page)
                                  .arg(number(g.gap)));
                }
            }
        } else if (!starUsed[i]) {
            double worst = -1;
            for (const PageGap& g : gaps) {
                if (sameFile(known[i].file, g.file)) {
                    worst = std::max(worst, g.gap);
                }
            }
            if (worst >= 0) {
                printLine(QStringLiteral("CONNUE SOUS %1 : %2:* (écart le plus haut %3)")
                              .arg(limit, known[i].file, number(worst)));
            }
        }
    }
    QString summary = QStringLiteral("%1 pages, médiane %2, %3 au-dessus de %4")
                          .arg(static_cast<int>(n))
                          .arg(number(median))
                          .arg(above)
                          .arg(limit);
    if (tally.withoutThumbnail > 0) {
        summary += QStringLiteral(", %1 sans miniature").arg(tally.withoutThumbnail);
    }
    if (tally.unreadableThumbnail > 0) {
        summary += QStringLiteral(", %1 miniature(s) illisible(s)").arg(tally.unreadableThumbnail);
    }
    if (tally.warnings > 0) {
        summary += QStringLiteral(", %1 avertissement(s) de lecture").arg(tally.warnings);
    }
    if (!hasKnownList) {
        summary += QStringLiteral(", liste des pages connues absente");
    }
    printLine(QStringLiteral("SYNTHESE : %1").arg(summary));
    const auto fail = [](const QString& why) {
        printLine(QStringLiteral("MINIATURES EN ECHEC : %1").arg(why));
        return 1;
    };
    if (expectedPages > 0 && tally.pagesRead != expectedPages) {
        return fail(QStringLiteral("%1 pages lues, %2 attendues").arg(tally.pagesRead).arg(expectedPages));
    }
    const int outside = tally.withoutThumbnail + tally.unreadableThumbnail;
    if (outside > 0) {
        return fail(QStringLiteral("%1 pages hors de la mesure (%2 sans miniature, %3 miniature illisible)")
                        .arg(outside)
                        .arg(tally.withoutThumbnail)
                        .arg(tally.unreadableThumbnail));
    }
    if (n == 0) {
        return fail(QStringLiteral("aucune page à miniature"));
    }
    if (!unknown.isEmpty()) {
        return fail(unknown.join(QStringLiteral(" ; ")));
    }
    const bool ok = median <= kMedianLimit && above <= kPagesAboveLimit;
    printLine(QStringLiteral("MINIATURES %1 : %2")
                  .arg(ok ? QStringLiteral("OK") : QStringLiteral("EN ECHEC"), summary));
    return ok ? 0 : 1;
}

// Variantes du paperboard synthétique, pour prouver que la mesure et son verdict voient chaque défaut.
enum class Variant
{
    Plain,
    Altered,       // quart droit de chaque miniature peint en noir
    Holes,         // page 1 à miniature vide, page 2 à miniature indécodable
    Huge,          // miniatures de 2000 px de large
    Jpeg50,        // miniatures JPEG de qualité 50, comme les .thm
    AlteredJpeg50, // les deux : un défaut doit survivre à la recompression q50
};

int writeSynthetic(const QString& path, Variant variant)
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
        if (variant == Variant::Altered || variant == Variant::AlteredJpeg50) {
            QPainter p(&image);
            p.fillRect(QRect(image.width() * 3 / 4, 0, image.width() - image.width() * 3 / 4, image.height()),
                       Qt::black);
        } else if (variant == Variant::Huge) {
            image = image.scaled(2000, 1500, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }
        // Qualité 100 : sans recompression du rendu, le sous-échantillonnage des couleurs du JPEG à 90
        // brouillait les traits fins de la grille (écart 4,47 sur la page 1, contre 0,00 en PNG et 0,39 à
        // 100, mesuré) ; la preuve porte sur le chemin. Qualité 50 (variante) : celle des .thm, pour prouver
        // la recompression.
        QByteArray jpeg;
        QBuffer buffer(&jpeg);
        buffer.open(QIODevice::WriteOnly);
        if (!image.save(&buffer, "JPG",
                        variant == Variant::Jpeg50 || variant == Variant::AlteredJpeg50 ? 50 : 100)) {
            return fail(QStringLiteral("encodage JPEG impossible (greffon d'images absent ?)"));
        }
        jpegs.push_back(jpeg);
    }
    if (variant == Variant::Holes && jpegs.size() == 2) {
        jpegs[0].clear();
        jpegs[1] = QByteArrayLiteral("pas une image");
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

// Les comparaisons contiennent les vraies miniatures (dépôt public) : seulement sous .tmp/ ou build/ du
// dépôt.
bool isUnderWorkArea(const QString& dir)
{
#ifdef Q_OS_WIN
    constexpr Qt::CaseSensitivity cs = Qt::CaseInsensitive;
#else
    constexpr Qt::CaseSensitivity cs = Qt::CaseSensitive;
#endif
    const QString target = QDir::cleanPath(QFileInfo(dir).absoluteFilePath());
    const QString root = QDir::cleanPath(QStringLiteral(ACTCRAFT_SOURCE_DIR));
    for (const QString& sub : {QStringLiteral(".tmp"), QStringLiteral("build")}) {
        const QString base = root + QLatin1Char('/') + sub;
        if (target.compare(base, cs) == 0 || target.startsWith(base + QLatin1Char('/'), cs)) {
            return true;
        }
    }
    return false;
}

int compareThumbnails(const QStringList& args)
{
    const auto fail = [](const QString& why) {
        printLine(QStringLiteral("MINIATURES EN ECHEC : %1").arg(why));
        return 1;
    };
    bool hasFile = false;
    bool hasSamples = false;
    bool hasExpected = false;
    bool unused = false;
    const QString file = optionValue(args, QStringLiteral("--miniatures"), hasFile);
    const QString samples = optionValue(args, QStringLiteral("--samples"), hasSamples);
    const QString knownText = optionValue(args, QStringLiteral("--connues"), unused);
    const QString comparisons = optionValue(args, QStringLiteral("--comparaisons"), unused);
    const QString expectedText = optionValue(args, QStringLiteral("--pages-attendues"), hasExpected);
    const bool recompress = !args.contains(QStringLiteral("--sans-recompression"));
    std::vector<KnownPage> known;
    QString error;
    if (!parseKnown(knownText, known, error)) {
        return fail(error);
    }
    if (!comparisons.isEmpty() && !isUnderWorkArea(comparisons)) {
        return fail(QStringLiteral("dossier de comparaisons hors de .tmp/ et de build/ du dépôt (%1)")
                        .arg(comparisons));
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
    int expectedPages = 0;
    if (hasExpected || hasSamples) {
        bool ok = false;
        expectedPages = expectedText.toInt(&ok);
        if (!ok || expectedPages < 1) {
            return fail(
                QStringLiteral("--pages-attendues <n> attendu (n ≥ 1 ; ACTCRAFT_THM_PAGES pour --samples)"));
        }
    }
    std::vector<PageGap> gaps;
    Tally tally;
    for (const QString& f : files) {
        if (!pageGaps(f, gaps, tally, comparisons, recompress, error)) {
            return fail(error);
        }
    }
    return verdict(gaps, tally, expectedPages, known);
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
    bool hasSynthetic = false;
    const QString syntheticFile = optionValue(args, QStringLiteral("--ecrire-synthetique"), hasSynthetic);
    if (hasSynthetic) {
        if (syntheticFile.isEmpty()) {
            printLine(QStringLiteral("SYNTHETIQUE EN ECHEC : --ecrire-synthetique <fichier> attendu"));
            return 1;
        }
        Variant variant = Variant::Plain;
        if (args.contains(QStringLiteral("--miniatures-alterees")) &&
            args.contains(QStringLiteral("--miniatures-q50"))) {
            variant = Variant::AlteredJpeg50;
        } else if (args.contains(QStringLiteral("--miniatures-alterees"))) {
            variant = Variant::Altered;
        } else if (args.contains(QStringLiteral("--miniatures-trouees"))) {
            variant = Variant::Holes;
        } else if (args.contains(QStringLiteral("--miniatures-geantes"))) {
            variant = Variant::Huge;
        } else if (args.contains(QStringLiteral("--miniatures-q50"))) {
            variant = Variant::Jpeg50;
        }
        return writeSynthetic(syntheticFile, variant);
    }
    if (args.contains(QStringLiteral("--miniatures")) || args.contains(QStringLiteral("--samples"))) {
        return compareThumbnails(args);
    }
    return dump(args); // chemin d'origine, par défaut
}
