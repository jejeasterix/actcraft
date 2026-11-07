#include <actcraft/ui/language.h>

#include <QCoreApplication>
#include <QFile>
#include <QTranslator>
#include <QXmlStreamReader>
#include <QtTest>

using namespace actcraft::ui;

// Pseudo-langue de contrôle (L11-T19, D11-14) : chaque traduction est « ⟦ » + source + « ⟧ », générée au build
// des tests à partir de `ui/i18n/actcraft_fr.ts`, jamais livrée. Les chemins viennent de tests/ui.cmake.
class TstUiPseudolocale : public QObject
{
    Q_OBJECT

    struct Message
    {
        QString context;
        QString source;
        QStringList translations; // une forme, ou les formes plurielles
        bool unfinished = false;
    };

    static QList<Message> readTs(const QString& path)
    {
        QList<Message> messages;
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            return messages;
        QXmlStreamReader xml(&file);
        QString context;
        Message current;
        while (!xml.atEnd()) {
            xml.readNext();
            if (!xml.isStartElement())
                continue;
            const auto name = xml.name();
            if (name == QLatin1String("name"))
                context = xml.readElementText();
            else if (name == QLatin1String("message"))
                current = Message{context, {}, {}, false};
            else if (name == QLatin1String("source"))
                current.source = xml.readElementText();
            else if (name == QLatin1String("translation")) {
                current.unfinished = xml.attributes().value(QLatin1String("type")) == QLatin1String("unfinished");
                const QString whole = xml.readElementText(QXmlStreamReader::IncludeChildElements);
                current.translations << whole;
            } else if (name == QLatin1String("numerusform")) {
                // Les formes plurielles sont lues par readElementText ci-dessus : voir plus bas.
            }
            if (name == QLatin1String("translation"))
                messages.append(current);
        }
        return messages;
    }

    // Formes plurielles : relecture dédiée (readElementText concatène les formes).
    static QStringList forms(const QString& path, const QString& source)
    {
        QStringList out;
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            return out;
        QXmlStreamReader xml(&file);
        bool inMessage = false;
        QString currentSource;
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isStartElement() && xml.name() == QLatin1String("message")) {
                inMessage = true;
                currentSource.clear();
            } else if (xml.isStartElement() && xml.name() == QLatin1String("source") && inMessage) {
                currentSource = xml.readElementText();
            } else if (xml.isStartElement() && xml.name() == QLatin1String("numerusform") &&
                       currentSource == source) {
                out << xml.readElementText();
            }
        }
        return out;
    }

    static QString marked(const QString& source) { return QString::fromUtf8("⟦") + source + QString::fromUtf8("⟧"); }

private slots:
    void everySourceMarked()
    {
        const QList<Message> pseudo = readTs(QStringLiteral(ACTCRAFT_PSEUDO_TS));
        const QList<Message> french = readTs(QStringLiteral(ACTCRAFT_FR_TS));
        QVERIFY2(!french.isEmpty(), "actcraft_fr.ts illisible");
        QCOMPARE(pseudo.size(), french.size());
        int plural = 0;
        for (qsizetype i = 0; i < pseudo.size(); ++i) {
            const Message& m = pseudo.at(i);
            QCOMPARE(m.source, french.at(i).source);
            QVERIFY2(!m.unfinished, qPrintable(m.source));
            if (m.source.contains(QLatin1String("%n"))) {
                ++plural;
                const QStringList fs = forms(QStringLiteral(ACTCRAFT_PSEUDO_TS), m.source);
                QVERIFY2(!fs.isEmpty(), qPrintable(m.source));
                for (const QString& f : fs)
                    QCOMPARE(f, marked(m.source));
            } else {
                QCOMPARE(m.translations.value(0), marked(m.source));
            }
        }
        QVERIFY2(plural > 0, "aucune forme plurielle relevée");
    }

    void placeholdersKept()
    {
        const QList<Message> pseudo = readTs(QStringLiteral(ACTCRAFT_PSEUDO_TS));
        int withArg = 0;
        int withN = 0;
        for (const Message& m : pseudo) {
            const QString text = m.translations.value(0);
            for (int n = 1; n <= 9; ++n) {
                const QString token = QStringLiteral("%%1").arg(n);
                QCOMPARE(text.count(token), m.source.count(token));
            }
            QCOMPARE(text.count(QLatin1String("%n")), m.source.count(QLatin1String("%n")));
            withArg += m.source.contains(QLatin1String("%1"));
            withN += m.source.contains(QLatin1String("%n"));
        }
        QVERIFY(withArg > 0);
        QVERIFY(withN > 0);
    }

    void qmLoads()
    {
        QTranslator translator;
        QVERIFY(translator.load(QStringLiteral(ACTCRAFT_PSEUDO_QM)));
        QVERIFY(QCoreApplication::installTranslator(&translator));
        const QString shown =
            QCoreApplication::translate("AboutDialog", "Fermer");
        QCoreApplication::removeTranslator(&translator);
        QCOMPARE(shown, marked(QStringLiteral("Fermer")));
    }

    void installFileInstallsTheTranslator()
    {
        QVERIFY(!Language::installFile(*QCoreApplication::instance(), QStringLiteral("/inexistant/actcraft_xx.qm")));
        QCOMPARE(QCoreApplication::translate("AboutDialog", "Fermer"), QStringLiteral("Fermer"));
        QVERIFY(Language::installFile(*QCoreApplication::instance(), QStringLiteral(ACTCRAFT_PSEUDO_QM)));
        QCOMPARE(QCoreApplication::translate("AboutDialog", "Fermer"), marked(QStringLiteral("Fermer")));
        const auto translators = QCoreApplication::instance()->findChildren<QTranslator*>();
        for (QTranslator* t : translators)
            QCoreApplication::removeTranslator(t);
        QCOMPARE(QCoreApplication::translate("AboutDialog", "Fermer"), QStringLiteral("Fermer"));
    }

    void notShippedInResources()
    {
        QVERIFY(!QFile::exists(QStringLiteral(":/i18n/actcraft_xx.qm")));
        QVERIFY(!QFile::exists(QStringLiteral(":/actcraft/i18n/actcraft_xx.qm")));
        QVERIFY(!Language::available().contains(QStringLiteral("xx")));
    }
};

QTEST_MAIN(TstUiPseudolocale)
#include "tst_ui_pseudolocale.moc"
