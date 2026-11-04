#include <actcraft/ui/colorpalette.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/preferences.h>
#include <actcraft/ui/profilecontroller.h>

#include <QColor>
#include <QImage>
#include <QSet>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include <memory>

using namespace actcraft;

// Palette étendue, couleurs ajoutées et copieur de couleur (L6-T8). Chaque fonction a ses profils d'essai.
class TstUiColors : public QObject
{
    Q_OBJECT
    std::unique_ptr<QTemporaryDir> m_dir;

    QString ini() const { return m_dir->filePath(QStringLiteral("poste.ini")); }

    std::unique_ptr<ui::ProfileController> controller(ui::Preferences* prefs, ui::Commands* commands)
    {
        actions::ProfileStore store(m_dir->filePath(QStringLiteral("profiles")),
                                    QStringLiteral(":/actcraft/actions/profiles"),
                                    ui::ProfileController::builtinOrder());
        auto c = std::make_unique<ui::ProfileController>(std::move(store), prefs, commands);
        c->start();
        return c;
    }

    static QStringList sixteen()
    {
        QStringList out;
        for (int i = 0; i < 16; ++i) {
            out << QColor::fromHsv(i * 20, 255, 200).name();
        }
        return out;
    }

private slots:
    void init() { m_dir = std::make_unique<QTemporaryDir>(); }
    void cleanup() { m_dir.reset(); }

    void extendedGridIsSixBySix()
    {
        const QStringList grid = ui::ColorPalette::extendedGrid();
        QCOMPARE(grid.size(), 36);
        QCOMPARE(QSet<QString>(grid.begin(), grid.end()).size(), 36);
        for (const QString& color : grid) {
            QVERIFY2(QColor(color).isValid(), qPrintable(color));
            QCOMPARE(color.size(), 7);
        }
        ui::ColorPalette palette;
        QCOMPARE(palette.extended(), grid);
    }

    void addedColorIsKept()
    {
        ui::Commands commands;
        ui::Preferences prefs(ini(), nullptr);
        auto profile = controller(&prefs, &commands);
        ui::ColorPalette palette;
        palette.setProfile(profile.get());
        QSignalSpy changed(&palette, &ui::ColorPalette::changed);
        palette.add(QStringLiteral("#12AB34"));
        QCOMPARE(palette.extraCount(), 1);
        QVERIFY(changed.count() >= 1);
        QVERIFY(profile->extraColors().contains(QStringLiteral("#12AB34"), Qt::CaseInsensitive));
        profile->saveNow();
        // Un second contrôleur sur le même dossier relit la couleur ajoutée.
        ui::Preferences prefs2(ini(), nullptr);
        auto second = controller(&prefs2, &commands);
        QCOMPARE(second->currentId(), profile->currentId());
        QVERIFY(second->extraColors().contains(QStringLiteral("#12AB34"), Qt::CaseInsensitive));
        // La couleur ajoutée est visible dans les pastilles.
        QVERIFY(palette.swatches().contains(QStringLiteral("#12AB34"), Qt::CaseInsensitive));
        QCOMPARE(palette.swatches().last().toUpper(), QStringLiteral("#12AB34"));
    }

    void duplicatesIgnored()
    {
        ui::Commands commands;
        ui::Preferences prefs(ini(), nullptr);
        auto profile = controller(&prefs, &commands);
        ui::ColorPalette palette;
        palette.setProfile(profile.get());
        palette.setProperty("baseColors", sixteen());
        palette.add(QStringLiteral("#12ab34"));
        palette.add(QStringLiteral("#12ab34"));
        palette.add(QStringLiteral("#12AB34"));
        palette.add(sixteen().first().toUpper()); // une couleur de base
        palette.add(QStringLiteral("pas une couleur"));
        QCOMPARE(palette.extraCount(), 1);
        QCOMPARE(palette.swatches().size(), 16);
    }

    void swatchCountFollowsTheProfile()
    {
        ui::Commands commands;
        ui::Preferences prefs(ini(), nullptr);
        auto profile = controller(&prefs, &commands);
        ui::ColorPalette palette;
        palette.setProfile(profile.get());
        QStringList base = sixteen();
        for (int i = 0; i < 12; ++i) {
            base << QColor::fromHsv(i * 30, 120, 90).name();
        }
        palette.setProperty("baseColors", base);
        profile->setSwatches(8);
        QCOMPARE(palette.swatches().size(), 8);
        profile->setSwatches(24);
        QCOMPARE(palette.swatches().size(), 24);
        QCOMPARE(palette.swatches(), base.mid(0, 24));
        profile->setSwatches(16);
        profile->setSwatches(10); // refusée : le profil garde 16
        QCOMPARE(profile->swatches(), 16);
        QCOMPARE(palette.swatches().size(), 16);
        // Une couleur ajoutée prend la dernière place, même quand la palette est pleine.
        palette.add(QStringLiteral("#12AB34"));
        QCOMPARE(palette.swatches().size(), 16);
        QCOMPARE(palette.swatches().last().toUpper(), QStringLiteral("#12AB34"));
    }

    void loupeSamplesOnceOnOpen()
    {
        ui::ColorPalette palette;
        QVERIFY(palette.colorAt(0, 0).isEmpty()); // aucune capture
        QSignalSpy requested(&palette, &ui::ColorPalette::snapshotRequested);
        palette.beginPick();
        QCOMPARE(requested.count(), 1);
        QImage image(4, 4, QImage::Format_ARGB32);
        image.fill(Qt::blue);
        image.setPixelColor(1, 2, Qt::red);
        palette.setSnapshot(image);
        QCOMPARE(palette.snapshotCount(), 1);
        QCOMPARE(palette.colorAt(1, 2), QStringLiteral("#ff0000"));
        QCOMPARE(palette.colorAt(0, 0), QStringLiteral("#0000ff"));
        QVERIFY(palette.colorAt(9, 9).isEmpty());
        QVERIFY(palette.colorAt(-1, 0).isEmpty());
        for (int i = 0; i < 20; ++i) {
            palette.colorAt(i % 4, i % 3);
        }
        QCOMPARE(palette.snapshotCount(), 1);
        QCOMPARE(requested.count(), 1);
    }
};

QTEST_MAIN(TstUiColors)
#include "tst_ui_colors.moc"
