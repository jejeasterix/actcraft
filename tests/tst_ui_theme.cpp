#include <QColor>
#include <QFile>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSvgRenderer>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <algorithm>
#include <cmath>
#include <memory>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

namespace {

const char* const kIcons[] = {"menu",        "outils",  "page-precedente", "page-suivante", "stylo",
                              "plein-ecran", "ajuster", "quitter",         "aide",          "a-propos"};

// Luminance relative de WCAG 2.1 (composantes sRGB linéarisées).
double relativeLuminance(const QColor& color)
{
    const auto linear = [](double c) { return c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4); };
    return 0.2126 * linear(color.redF()) + 0.7152 * linear(color.greenF()) + 0.0722 * linear(color.blueF());
}

double contrastRatio(const QColor& a, const QColor& b)
{
    const double la = relativeLuminance(a);
    const double lb = relativeLuminance(b);
    return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}

// Seuils WCAG 2.1 AA : 4,5:1 pour un texte, 3:1 pour une icône, un indicateur d'état ou une bordure de contrôle.
constexpr double kText = 4.5;
constexpr double kUi = 3.0;

} // namespace

// Thème et icônes à nous (décision 23).
class TstUiTheme : public QObject
{
    Q_OBJECT

private slots:
    void themeSingletonExposesTokens()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nimport ActCraft.Ui\nQtObject {\n"
                          "  property color fond: Theme.fond\n"
                          "  property color palette: Theme.palette\n"
                          "  property color texte: Theme.texte\n"
                          "  property int bouton: Theme.tailleBouton\n"
                          "  property int largeurPalette: Theme.largeurPalette\n"
                          "  property int hauteurBarre: Theme.hauteurBarre\n"
                          "}\n",
                          QUrl());
        std::unique_ptr<QObject> tokens(component.create());
        QVERIFY2(tokens, qPrintable(component.errorString()));
        QVERIFY(tokens->property("fond").value<QColor>().isValid());
        QVERIFY(tokens->property("palette").value<QColor>().isValid());
        QVERIFY(tokens->property("texte").value<QColor>().isValid());
        QVERIFY(tokens->property("bouton").toInt() >= 44); // cible tactile
        QVERIFY(tokens->property("largeurPalette").toInt() >= 96);
        QVERIFY(tokens->property("hauteurBarre").toInt() >= 32);
    }

    // La formule elle-même, sur des valeurs de référence : noir sur blanc vaut 21:1 ; #767676 est le gris le plus
    // clair qui passe 4,5:1 sur blanc, #777777 ne passe plus (sans linéarisation sRGB, les deux passeraient).
    void contrastFormulaMatchesWcagReferences()
    {
        QVERIFY(qAbs(contrastRatio(Qt::black, Qt::white) - 21.0) < 1e-9);
        QVERIFY(contrastRatio(QColor(0x76, 0x76, 0x76), Qt::white) >= kText);
        QVERIFY(contrastRatio(QColor(0x77, 0x77, 0x77), Qt::white) < kText);
    }

    // Chaque paire premier plan / fond que l'interface rend réellement (L1-C5, table des contrastes de
    // docs/design/2026-10-14-direction-visuelle.md) tient le seuil WCAG 2.1 AA de son rôle.
    void themePairsMeetWcagAa_data()
    {
        QTest::addColumn<QString>("foreground");
        QTest::addColumn<QString>("background");
        QTest::addColumn<double>("minimum");
        const struct
        {
            const char* foreground;
            const char* background;
            double minimum;
        } pairs[] = {
            // Textes : menus de la barre, indicateur de page, menus surgissants, zoom, dialogue.
            {"texte", "barre", kText},
            {"texte", "palette", kText},
            {"texte", "surface", kText},
            {"texte", "surfaceSurvol", kText},
            {"texteDiscret", "palette", kText},
            {"texteDiscret", "surface", kText},
            {"texteDiscret", "surfaceSurvol", kText},
            {"texteSurActive", "surfaceActive", kText},
            {"surfaceActive", "palette", kText}, // lien du dialogue « À propos »
            // Les icônes (« texte » au repos et survolées, « texteSurActive » sur l'accent) sont couvertes par les
            // paires de texte ci-dessus, plus exigeantes. Indicateurs d'état : coché, appuyé, focus contre ce qui
            // les entoure (la rainure de la réglette est « surfaceSurvol ») ; cadre de la pastille choisie ; bord
            // des pastilles sombres et flèche du zoom ; poignée de la réglette.
            {"surfaceActive", "barre", kUi},
            {"surfaceActive", "surface", kUi},
            {"surfaceActive", "surfaceSurvol", kUi},
            {"cadreSelection", "palette", kUi},
            {"contour", "palette", kUi},
            {"contour", "surface", kUi},
            {"texte", "surfaceSurvol", kUi},
            // Sur la zone de la page : pouce des barres de défilement (sombre cerclé de clair) et filet de
            // l'indicateur de largeur, sur la page comme sur le fond ; bord de la page contre le fond. « barre » sur
            // « page » est aussi le texte des notices du dialogue « À propos » (seuil de texte).
            {"barre", "page", kText},
            {"texte", "fond", kUi},
            {"contour", "page", kUi},
            {"page", "fond", kUi},
        };
        for (const auto& pair : pairs) {
            QTest::addRow("%s sur %s (%.1f)", pair.foreground, pair.background, pair.minimum)
                << QString::fromLatin1(pair.foreground) << QString::fromLatin1(pair.background) << pair.minimum;
        }
    }

    void themePairsMeetWcagAa()
    {
        QFETCH(QString, foreground);
        QFETCH(QString, background);
        QFETCH(double, minimum);
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nimport ActCraft.Ui\nQtObject { property QtObject theme: Theme }\n", QUrl());
        std::unique_ptr<QObject> holder(component.create());
        QVERIFY2(holder, qPrintable(component.errorString()));
        const QObject* theme = holder->property("theme").value<QObject*>();
        QVERIFY(theme);
        const QColor fg = theme->property(qPrintable(foreground)).value<QColor>();
        const QColor bg = theme->property(qPrintable(background)).value<QColor>();
        QVERIFY2(fg.isValid(), qPrintable(QStringLiteral("jeton absent : ") + foreground));
        QVERIFY2(bg.isValid(), qPrintable(QStringLiteral("jeton absent : ") + background));
        const double ratio = contrastRatio(fg, bg);
        QVERIFY2(ratio >= minimum, qPrintable(QStringLiteral("%1 (%2) sur %3 (%4) : %5:1 < %6:1")
                                                  .arg(foreground, fg.name(), background, bg.name())
                                                  .arg(ratio, 0, 'f', 2)
                                                  .arg(minimum, 0, 'f', 1)));
    }

    void iconsAreOurSvgOnA24Grid_data()
    {
        QTest::addColumn<QString>("name");
        for (const char* name : kIcons) {
            QTest::newRow(name) << QString::fromLatin1(name);
        }
    }

    void iconsAreOurSvgOnA24Grid()
    {
        QFETCH(QString, name);
        const QString path = QStringLiteral(":/qt/qml/ActCraft/Ui/icons/%1.svg").arg(name);
        QVERIFY2(QFile::exists(path), qPrintable(path));
        QSvgRenderer renderer(path);
        QVERIFY(renderer.isValid());
        QCOMPARE(renderer.viewBoxF(), QRectF(0, 0, 24, 24));
    }
};

QTEST_MAIN(TstUiTheme)
#include "tst_ui_theme.moc"
