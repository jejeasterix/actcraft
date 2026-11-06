// S11 local (L8-T19) : le paperboard réel n° 13, lu hors dépôt. Choisi par l'empreinte SHA-256 de ses octets,
// jamais par son nom ; sauté sans ACTCRAFT_SAMPLES_DIR.
#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/formats/flipchart.h>
#include <actcraft/ui/actionrunner.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/designmode.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/linklauncher.h>

#include <QCryptographicHash>
#include <QDirIterator>
#include <QFile>
#include <QTest>

#include <string>

using namespace actcraft;

namespace {
// Empreinte SHA-256 (hexadécimale) du paperboard n° 13, calculée sur le PC et à renseigner ici ; vide : le
// test se saute. Le fichier n'est jamais nommé ni copié.
constexpr const char* kPaperboardSha256 = "";
} // namespace

class TstFormatsSamplesInteractivity : public QObject
{
    Q_OBJECT
private slots:
    void toggleHiddenOnRealPaperboard()
    {
        const QString dir = qEnvironmentVariable("ACTCRAFT_SAMPLES_DIR");
        if (dir.isEmpty()) {
            QSKIP("ACTCRAFT_SAMPLES_DIR absent");
        }
        if (QByteArray(kPaperboardSha256).isEmpty()) {
            QSKIP("empreinte du paperboard n° 13 non renseignée");
        }
        QByteArray bytes;
        QDirIterator it(dir, {QStringLiteral("*.flipchart")}, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QFile file(it.next());
            if (!file.open(QIODevice::ReadOnly)) {
                continue;
            }
            const QByteArray data = file.readAll();
            if (QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex() == kPaperboardSha256) {
                bytes = data;
                break;
            }
        }
        if (bytes.isEmpty()) {
            QSKIP("paperboard d'empreinte voulue absent du dossier");
        }
        const formats::flipchart::FlipchartReadResult read = formats::flipchart::readFlipchart(bytes);
        QVERIFY2(read.ok, qPrintable(read.error));

        // Premier objet portant « masquer ou afficher » vers une cible présente sur sa page.
        std::size_t pageIndex = 0;
        const core::PageObject* source = nullptr;
        core::ObjectId target = 0;
        for (std::size_t p = 0; p < read.document.pageCount() && !source; ++p) {
            const core::Page& page = read.document.page(p);
            for (const core::PageObject& o : page.objects) {
                for (const core::Action& a : o.common.actions) {
                    if (a.catalogId != "object.toggle-hidden") {
                        continue;
                    }
                    for (const core::PageObject& t : page.objects) {
                        if (t.common.id == a.target && a.target != 0) {
                            source = &o;
                            target = a.target;
                            pageIndex = p;
                        }
                    }
                    if (source) {
                        break;
                    }
                }
                if (source) {
                    break;
                }
            }
        }
        QVERIFY2(source, "aucune action object.toggle-hidden avec cible");

        ui::DocumentController controller;
        ui::Commands commands;
        ui::DesignMode design;
        ui::LinkLauncher links;
        ui::ActionRunner runner;
        runner.setCommands(&commands);
        runner.setDocument(&controller);
        runner.setDesignMode(&design);
        runner.setLinks(&links);
        controller.replaceDocument(read.document, QUrl(), QStringLiteral("essai"));
        controller.showPage(static_cast<int>(pageIndex));

        const core::PointF c = core::boundsOnPage(*source).center();
        const auto hidden = [&] {
            for (const core::PageObject& o : controller.document().page(pageIndex).objects) {
                if (o.common.id == target) {
                    return !o.common.visible;
                }
            }
            return false;
        };
        const auto click = [&] {
            runner.setContactActive(true);
            runner.press(QPointF(c.x, c.y), Qt::LeftButton);
            runner.release(QPointF(c.x, c.y));
            runner.setContactActive(false);
            QCoreApplication::processEvents();
        };
        QVERIFY(!hidden());
        click();
        QVERIFY(hidden());
        click();
        QVERIFY(!hidden());
    }
};

QTEST_MAIN(TstFormatsSamplesInteractivity)
#include "tst_formats_samples_interactivity.moc"
