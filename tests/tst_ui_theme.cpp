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

const char* const kIcons[] = {
    "menu", "outils", "page-precedente", "page-suivante", "stylo", "plein-ecran", "ajuster", "quitter",
    "aide", "a-propos", "selection", "marqueur", "gomme", "annuler", "retablir", "supprimer", "corbeille",
    "effacer", "deplacer", "rotation", "translucidite", "agrandir", "reduire", "rideau", "spot", "zoom-page",
    "effets", "regle", "equerre", "rapporteur", "compas", "origine", "des", "calculatrice", "horloge",
    "message", "forme-reconnue", "ecriture", "camera", "clavier", "bureau",
    // Lot 4 (L4-T1) : 34 icônes à nous.
    "forme", "texte", "texte-vertical", "grille", "grille-masquer", "aimant", "createur-grille",
    "navigateurs", "page-avant", "page-apres", "page-couper", "page-copier", "page-coller", "page-dupliquer",
    "page-supprimer", "fond", "allonger", "zones", "gras", "italique", "souligne", "exposant", "indice",
    "aligner-gauche", "centrer", "aligner-droite", "interligne-plus", "interligne-moins", "retrait-plus",
    "retrait-moins", "puces", "symboles", "taille-plus", "taille-moins"};

// Luminance relative de WCAG 2.1 (composantes sRGB linéarisées).
double relativeLuminance(const QColor& color)
{
    const auto linear = [](double c) {
        return c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * linear(color.redF()) + 0.7152 * linear(color.greenF()) + 0.0722 * linear(color.blueF());
}

double contrastRatio(const QColor& a, const QColor& b)
{
    const double la = relativeLuminance(a);
    const double lb = relativeLuminance(b);
    return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}

// Seuils WCAG 2.1 AA : 4,5:1 pour un texte, 3:1 pour une icône, un indicateur d'état ou une bordure de
// contrôle.
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

    // La formule elle-même, sur des valeurs de référence : noir sur blanc vaut 21:1 ; #767676 est le gris le
    // plus clair qui passe 4,5:1 sur blanc, #777777 ne passe plus (une formule sans linéarisation sRGB les
    // ferait échouer tous les deux).
    void contrastFormulaMatchesWcagReferences()
    {
        QVERIFY(qAbs(contrastRatio(Qt::black, Qt::white) - 21.0) < 1e-9);
        QVERIFY(contrastRatio(QColor(0x76, 0x76, 0x76), Qt::white) >= kText);
        QVERIFY(contrastRatio(QColor(0x77, 0x77, 0x77), Qt::white) < kText);
    }

    // Paires de jetons distinctes que l'interface rend (L1-C5, table de docs/design/2026-10-14-direction-
    // visuelle.md), chacune au seuil WCAG 2.1 AA le plus exigeant de ses rôles. Les couleurs réellement
    // rendues, état par état (contrôles du style Basic compris), sont vérifiées par tst_ui_shell
    // (renderedColorsMeetWcagAa, keyboardFocusIsVisible, scrollThumbIsVisibleOnPageAndBackground).
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
            {"texte", "barre", kText},                  // menus de la barre, indicateur de page, info-bulles
            {"texte", "palette", kText},                // menus, dialogue ; poignée de la réglette
            {"texte", "surface", kText},                // zoom, bouton du dialogue, icône au repos
            {"texte", "surfaceSurvol", kText},          // survol, entrée surlignée (menus, zoom)
            {"texteDiscret", "palette", kText},         // raccourci d'un menu
            {"texteDiscret", "surfaceSurvol", kText},   // raccourci d'une entrée surlignée
            {"texteSurActive", "surfaceActive", kText}, // ouvert, appuyé, coché ; sélection de texte
            {"surfaceActive", "palette", kText},     // lien ; filet de surlignage ; anneau de focus ; coché
            {"surfaceActive", "barre", kUi},         // menu de la barre ouvert ou avec le focus
            {"surfaceActive", "surface", kUi},       // préréglage choisi à côté des autres ; focus du zoom
            {"surfaceActive", "surfaceSurvol", kUi}, // partie parcourue de la réglette ; filet sur l'entrée
            {"cadreSelection", "palette", kUi},      // cadre de la pastille choisie
            {"contour", "palette", kUi},             // bord des pastilles
            {"contour", "surface", kUi},             // flèche du zoom au repos
            {"barre", "page", kText},                // notices de « À propos » ; pouce sur la page
            {"texte", "fond", kUi},                  // bord du pouce sur le fond
            {"contour", "page", kUi},                // filet de l'indicateur de largeur sur la page
            {"page", "fond", kUi},                   // bord de la page
        };
        for (const auto& pair : pairs) {
            QTest::addRow("%s sur %s (%.1f)", pair.foreground, pair.background, pair.minimum)
                << QString::fromLatin1(pair.foreground) << QString::fromLatin1(pair.background)
                << pair.minimum;
        }
    }

    void themePairsMeetWcagAa()
    {
        QFETCH(QString, foreground);
        QFETCH(QString, background);
        QFETCH(double, minimum);
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nimport ActCraft.Ui\nQtObject { property QtObject theme: Theme }\n",
                          QUrl());
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
