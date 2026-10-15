#include "support/flipchartwriter.h"

#include <actcraft/formats/flipchart.h>
#include <actcraft/formats/flipchartdump.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSet>
#include <QStandardPaths>
#include <QTest>

#include <algorithm>
#include <numeric>
#include <vector>

using namespace actcraft::formats::flipchart;
using namespace Qt::StringLiterals;

namespace {

// Python 3 utilisable : ACTCRAFT_PYTHON, sinon « python » puis « python3 » du PATH, chacun essayé (sous
// Windows, l'alias du Microsoft Store est un python.exe qui ne lance rien).
QString findPython()
{
    QStringList candidates;
    if (const QString forced = qEnvironmentVariable("ACTCRAFT_PYTHON"); !forced.isEmpty()) {
        candidates << forced;
    }
    for (const QString& name : {u"python"_s, u"python3"_s}) {
        if (const QString path = QStandardPaths::findExecutable(name); !path.isEmpty()) {
            candidates << path;
        }
    }
    for (const QString& exe : candidates) {
        QProcess probe;
        probe.start(exe, {u"-I"_s, u"-c"_s, u"import sys; print(sys.version_info[0])"_s});
        if (probe.waitForFinished(30000) && probe.exitStatus() == QProcess::NormalExit &&
            probe.exitCode() == 0 && probe.readAllStandardOutput().trimmed() == "3") {
            return exe;
        }
    }
    return {};
}

// Racine de l'ancien dépôt : ACTCRAFT_REFERENCE_DIR, sinon le parent de ACTCRAFT_SAMPLES_DIR (oracle réel).
QString referenceRoot()
{
    if (const QString root = qEnvironmentVariable("ACTCRAFT_REFERENCE_DIR"); !root.isEmpty()) {
        return root;
    }
    const QString samples = qEnvironmentVariable("ACTCRAFT_SAMPLES_DIR");
    return samples.isEmpty() ? QString() : QDir(samples).absoluteFilePath(u".."_s);
}

// Écart « enfants-ranges-par-couche » : notre lecteur range la page par couche, stable (contrat du modèle :
// Page::objects est rangé par couche, model.h ; nativejson.cpp le rétablit au rechargement), avec un
// avertissement par page dérangée ; la référence garde l'ordre de l'arbre (parent puis enfants). Les
// références (membres, cibles, extrémités) suivent les objets déplacés.
void sortByLayer(QJsonObject& dump)
{
    QJsonArray pages = dump.value(u"pages"_s).toArray();
    int moved = 0;
    for (qsizetype p = 0; p < pages.size(); ++p) {
        QJsonObject page = pages.at(p).toObject();
        const QJsonArray objects = page.value(u"objects"_s).toArray();
        std::vector<qsizetype> order(static_cast<std::size_t>(objects.size()));
        std::iota(order.begin(), order.end(), qsizetype{0});
        std::stable_sort(order.begin(), order.end(), [&objects](qsizetype a, qsizetype b) {
            return objects.at(a).toObject().value(u"layer"_s).toInt() <
                   objects.at(b).toObject().value(u"layer"_s).toInt();
        });
        if (std::is_sorted(order.begin(), order.end())) {
            continue;
        }
        ++moved;
        std::vector<int> newIndex(order.size());
        for (std::size_t i = 0; i < order.size(); ++i) {
            newIndex[static_cast<std::size_t>(order[i])] = static_cast<int>(i);
        }
        const auto remap = [&newIndex](const QJsonValue& v) {
            const int i = v.toInt(-1);
            return i < 0 ? -1 : newIndex[static_cast<std::size_t>(i)];
        };
        QJsonArray sorted;
        for (const qsizetype old : order) {
            QJsonObject o = objects.at(old).toObject();
            if (o.contains(u"members"_s)) {
                std::vector<int> members;
                for (const QJsonValue& m : o.value(u"members"_s).toArray()) {
                    members.push_back(remap(m));
                }
                std::sort(members.begin(), members.end());
                QJsonArray list;
                for (const int m : members) {
                    list.append(m);
                }
                o.insert(u"members"_s, list);
            }
            for (const QString& key : {u"objectA"_s, u"objectB"_s}) {
                if (o.contains(key)) {
                    o.insert(key, remap(o.value(key)));
                }
            }
            QJsonArray actions = o.value(u"actions"_s).toArray();
            for (qsizetype i = 0; i < actions.size(); ++i) {
                QJsonObject a = actions.at(i).toObject();
                a.insert(u"target"_s, remap(a.value(u"target"_s)));
                actions.replace(i, a);
            }
            o.insert(u"actions"_s, actions);
            sorted.append(o);
        }
        page.insert(u"objects"_s, sorted);
        pages.replace(p, page);
    }
    dump.insert(u"pages"_s, pages);
    dump.insert(u"warnings"_s, dump.value(u"warnings"_s).toInt() + moved);
}

// Écart « objet-non-lu-signale » : notre lecteur signale chaque objet de genre non lu (ici la formule et le
// texte vertical) ; la référence le garde en silence. Rien d'autre ne diffère : ses enfants sont lus des deux
// côtés.
void unreadObjectsWarn(QJsonObject& dump)
{
    dump.insert(u"warnings"_s, dump.value(u"warnings"_s).toInt() + 2);
}

// Écarts attendus, nommés. Chacun transforme l'export de la référence en ce que notre lecteur doit produire ;
// le reste doit être identique, et chaque écart doit être observé (sinon il n'a plus lieu d'être déclaré).
struct NamedGap
{
    const char* file;
    const char* name;
    void (*adjust)(QJsonObject&);
};

const NamedGap kGaps[] = {
    {"imbrication", "enfants-ranges-par-couche", sortByLayer},
    {"non-lus", "objet-non-lu-signale", unreadObjectsWarn},
};

// Ce que l'export de la référence doit contenir : sans cela, « 0 écart » ne prouverait rien.
const char* const kRequired[] = {
    "annotation",
    "line",
    "shape",
    "image",
    "text",
    "group",
    "connector",
    "media",
    "plume-0",
    "plume-1",
    "plume-2",
    "plume-3",
    "plume-4",
    "plume-5",
    "bout-0",
    "bout-1",
    "bout-2",
    "jointure-0",
    "jointure-1",
    "jointure-2",
    "trait-0",
    "trait-1",
    "trait-2",
    "degrade-1",
    "degrade-2",
    "degrade-3",
    "degrade-4",
    "degrade-5",
    "motif",
    "motif-opaque",
    "translucide",
    "invisible",
    "fond-degrade",
    "fond-image",
    "fond-degrade-image",
    "grille-2-niveaux",
    "enfant",
    "petit-enfant",
    "enfant-d-objet-non-lu",
    "connecteur-relie",
    "ligne-points",
    "media-complet",
    "groupe-membres",
    "image-transparente",
    "police-taille-0",
    "police-graisse-0",
};

void collectFeatures(const QJsonObject& dump, QSet<QString>& f)
{
    for (const QJsonValue& pv : dump.value(u"pages"_s).toArray()) {
        const QJsonObject page = pv.toObject();
        const QJsonObject bg = page.value(u"background"_s).toObject();
        const bool gradient = bg.value(u"gradient"_s).toInt() != 0;
        const bool image = !bg.value(u"image"_s).toString().isEmpty();
        if (gradient) {
            f << u"fond-degrade"_s;
        }
        if (image) {
            f << u"fond-image"_s;
        }
        if (gradient && image) {
            f << u"fond-degrade-image"_s;
        }
        if (page.value(u"grid"_s).toObject().value(u"levels"_s).toArray().size() >= 2) {
            f << u"grille-2-niveaux"_s;
        }
        for (const QJsonValue& ov : page.value(u"objects"_s).toArray()) {
            const QJsonObject o = ov.toObject();
            const QString kind = o.value(u"kind"_s).toString();
            const QString name = o.value(u"name"_s).toString();
            f << kind;
            if (name.startsWith(u"enfant"_s)) {
                f << u"enfant"_s;
            }
            if (name.startsWith(u"petit-enfant"_s)) {
                f << u"petit-enfant"_s;
            }
            if (name.contains(u"non-lu"_s)) {
                f << u"enfant-d-objet-non-lu"_s;
            }
            const int t = o.value(u"translucency"_s).toInt();
            if (t > 0 && t < 255) {
                f << u"translucide"_s;
            } else if (t == 255) {
                f << u"invisible"_s;
            }
            const QJsonObject pen = o.value(u"pen"_s).toObject();
            if (!pen.isEmpty()) {
                f << u"bout-%1"_s.arg(pen.value(u"cap"_s).toInt());
                if (pen.contains(u"style"_s)) {
                    f << u"plume-%1"_s.arg(pen.value(u"style"_s).toInt())
                      << u"jointure-%1"_s.arg(pen.value(u"join"_s).toInt());
                }
            }
            if (kind == u"annotation"_s) {
                f << u"trait-%1"_s.arg(o.value(u"strokeMode"_s).toInt());
            }
            const QJsonObject brush = o.value(u"brush"_s).toObject();
            if (brush.value(u"kind"_s).toString() == u"gradient"_s) {
                f << u"degrade-%1"_s.arg(brush.value(u"gradient"_s).toInt());
            } else if (brush.value(u"kind"_s).toString() == u"pattern"_s) {
                f << (brush.value(u"opaque"_s).toBool() ? u"motif-opaque"_s : u"motif"_s);
            }
            if (kind == u"connector"_s && o.value(u"objectA"_s).toInt() >= 0 &&
                o.value(u"objectB"_s).toInt() >= 0) {
                f << u"connecteur-relie"_s;
            }
            if (kind == u"line"_s && o.value(u"points"_s).toArray().size() >= 3) {
                f << u"ligne-points"_s;
            }
            if (kind == u"media"_s && !o.value(u"media"_s).toString().isEmpty() &&
                !o.value(u"image"_s).toString().isEmpty()) {
                f << u"media-complet"_s;
            }
            if (kind == u"group"_s && !o.value(u"members"_s).toArray().isEmpty()) {
                f << u"groupe-membres"_s;
            }
            if (kind == u"image"_s && o.value(u"transparent"_s).toBool()) {
                f << u"image-transparente"_s;
            }
            for (const QJsonValue& rv : o.value(u"runs"_s).toArray()) {
                const QJsonObject r = rv.toObject();
                const QString text = r.value(u"text"_s).toString();
                if (text.startsWith(u"taille 0"_s) && r.value(u"size"_s).toDouble() == 12) {
                    f << u"police-taille-0"_s;
                }
                if (text.endsWith(u"graisse 0"_s) && r.value(u"weight"_s).toInt() == 400) {
                    f << u"police-graisse-0"_s;
                }
            }
        }
    }
}

} // namespace

// Oracle synthétique (L2-C4) : les paperboards de notre écrivain (fctest::synth::oracleCases) sont lus par
// notre lecteur et par la référence Python (tools/oracle_flipchart.py de l'ancien dépôt), puis comparés par
// compareCanonical. Génération, script et comparaison en un seul test. Local seulement : sauté (et signalé)
// sans l'ancien dépôt ou sans Python 3. Fichiers et JSON sous <dossier des exécutables>/oracle-synth/, jamais
// commités.
class TstFormatsOracleSynth : public QObject
{
    Q_OBJECT

private slots:
    void syntheticFlipchartsMatchThePythonReader()
    {
        const QString root = referenceRoot();
        const QString script =
            root.isEmpty() ? QString() : QDir(root).absoluteFilePath(u"tools/oracle_flipchart.py"_s);
        if (script.isEmpty() || !QFileInfo::exists(script)) {
            QSKIP("ancien dépôt absent (ACTCRAFT_REFERENCE_DIR ou ACTCRAFT_SAMPLES_DIR) : oracle synthétique "
                  "local "
                  "seulement");
        }
        const QString python = findPython();
        if (python.isEmpty()) {
            QSKIP("Python 3 introuvable (ACTCRAFT_PYTHON, python, python3) : oracle synthétique local "
                  "seulement");
        }
        const QDir work(QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(u"oracle-synth"_s));
        const QString in = work.absoluteFilePath(u"in"_s);
        const QString out = work.absoluteFilePath(u"out"_s);
        QDir(in).removeRecursively();
        QVERIFY(QDir().mkpath(in));
        const auto cases = fctest::synth::oracleCases();
        for (const auto& [name, bytes] : cases) {
            QFile f(QDir(in).filePath(name + u".flipchart"_s));
            QVERIFY(f.open(QIODevice::WriteOnly));
            QCOMPARE(f.write(bytes), bytes.size());
        }

        QProcess py;
        py.start(python, {u"-I"_s, script, u"--samples"_s, in, u"--out"_s, out});
        QVERIFY2(py.waitForFinished(180000), "script de l'oracle : plus de 3 minutes");
        const QByteArray stdErr = py.readAllStandardError();
        QVERIFY2(py.exitStatus() == QProcess::NormalExit && py.exitCode() == 0,
                 qPrintable(u"la référence refuse un paperboard synthétique :\n"_s +
                            QString::fromLocal8Bit(stdErr)));
        QCOMPARE(QDir(out).entryList({u"*.json"_s}, QDir::Files).size(),
                 static_cast<qsizetype>(cases.size()));

        QSet<QString> features;
        QStringList failures;
        QStringList observed;
        int pages = 0;
        for (const auto& [name, bytes] : cases) {
            const FlipchartReadResult r = readFlipchart(bytes);
            if (!r.ok) {
                failures << u"%1 : %2"_s.arg(name, r.error);
                continue;
            }
            pages += static_cast<int>(r.document.pageCount());
            QFile file(QDir(out).filePath(name + u".json"_s));
            QJsonParseError parse{};
            const QJsonDocument parsed = file.open(QIODevice::ReadOnly)
                                             ? QJsonDocument::fromJson(file.readAll(), &parse)
                                             : QJsonDocument();
            if (parse.error != QJsonParseError::NoError || !parsed.isObject()) {
                failures << u"%1 : JSON de la référence absent ou illisible"_s.arg(name);
                continue;
            }
            QJsonObject expected = parsed.object();
            if (expected.take(u"source"_s).toString() !=
                QString::fromStdString(actcraft::core::sha256Hex(bytes))) {
                failures << u"%1 : JSON écrit pour un autre paperboard (empreinte)"_s.arg(name);
                continue;
            }
            collectFeatures(expected, features);
            const QJsonObject actual = canonicalDump(r);
            std::vector<const NamedGap*> gaps;
            for (const NamedGap& g : kGaps) {
                if (name == QLatin1String(g.file)) {
                    gaps.push_back(&g);
                }
            }
            QJsonObject adjusted = expected;
            for (const NamedGap* g : gaps) {
                g->adjust(adjusted);
            }
            for (const QString& diff : compareCanonical(adjusted, actual, 60)) {
                failures << u"%1 : %2"_s.arg(name, diff);
            }
            for (const NamedGap* g : gaps) {
                QJsonObject without = expected;
                for (const NamedGap* other : gaps) {
                    if (other != g) {
                        other->adjust(without);
                    }
                }
                if (compareCanonical(without, actual, 1).isEmpty()) {
                    failures << u"%1 : écart attendu « %2 » non observé"_s.arg(name, QLatin1String(g->name));
                } else {
                    observed << u"%1/%2"_s.arg(name, QLatin1String(g->name));
                }
            }
        }
        for (const char* feature : kRequired) {
            if (!features.contains(QLatin1String(feature))) {
                failures << u"couverture : « %1 » absent de l'export de la référence"_s.arg(
                    QLatin1String(feature));
            }
        }
        for (const QString& f : failures.mid(0, 80)) {
            qWarning("ECART %s", qPrintable(f));
        }
        QVERIFY2(failures.isEmpty(), qPrintable(u"%1 écart(s)"_s.arg(failures.size())));
        qInfo("ORACLE SYNTHETIQUE OK : %d paperboards, %d pages, 0 écart ; écarts attendus nommés : %s",
              static_cast<int>(cases.size()), pages, qPrintable(observed.join(u", "_s)));
    }
};

QTEST_GUILESS_MAIN(TstFormatsOracleSynth)
#include "tst_formats_oracle_synth.moc"
