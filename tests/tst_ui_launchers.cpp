#include <actcraft/actions/profile.h>
#include <actcraft/ui/launchers.h>
#include <actcraft/ui/preferences.h>
#include <actcraft/ui/profilecontroller.h>
#include <actcraft/ui/secretstore.h>

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageAuthenticationCode>
#include <QRandomGenerator>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest>

#include <memory>
#include <optional>
#include <utility>

using namespace actcraft;
using ui::Launchers;
using ui::SecretStore;

// Raccourcis vers des programmes (L6-T29). Le poste a sa clé dans le trousseau (L6-T28) ; hors Windows, le
// trousseau est indisponible (DA10 : plateforme lue à l'exécution) : aucune signature ne vérifie, et chaque
// lancement passe par la confirmation. Les fonctions comparent donc à SecretStore::available() plutôt que de
// sauter : le refus, la confirmation et le lancement sans interpréteur se prouvent partout.
namespace {

constexpr int kLaunchWaitMs = 5000; // le programme d'essai écrit son témoin en moins de 5 s
constexpr int kQuietMs = 1500;      // délai laissé à un lancement fautif pour se montrer

// Espaces et « & » : un interpréteur (cmd /c) ou une ligne de commande découpée couperait ce chemin.
const QString kWitnessDir = QStringLiteral("dossier temoin & co");

QByteArray randomKey()
{
    QByteArray key(Launchers::kKeyBytes, '\0');
    for (char& byte : key)
        byte = static_cast<char>(QRandomGenerator::system()->bounded(256));
    return key;
}

QByteArray readFile(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

bool stayedQuiet(const QString& path)
{
    QTest::qWait(kQuietMs);
    return !QFile::exists(path);
}

} // namespace

class TstUiLaunchers : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void addShowsThePath();
    void runStartsTheProgram();
    void signedOnThisWorkstation();
    void foreignSignatureAsksFirst();
    void refusedForeignRunsNothing();
    void acceptedForeignIsResigned();
    void removeDeletes();
    void tamperedArgumentsAskAgain();
    void confirmationShowsPathAndArguments();
    void signatureIsUnambiguous();
    void batAndCmdRefused();
    void unsafePathRefused();
    void executableDocumentsRefused();
    void documentTakesNoArguments();
    void duplicateIdKeepsFirst();
    void changedSinceRequestRunsNothing();

private:
    QString witness(const QString& name = QStringLiteral("lance.txt")) const
    {
        return QDir(m_dir->filePath(kWitnessDir)).filePath(name);
    }
    QString keyName(const QString& suffix = QString())
    {
        // Clé propre au test, effacée par cleanup() même en échec : la clé du poste n'est jamais touchée.
        const QString name = m_keyNames.constFirst() + suffix;
        if (!m_keyNames.contains(name))
            m_keyNames.append(name);
        return name;
    }
    // Entrée absente : map vide, la fonction échoue sans arrêter les suivantes.
    static QVariantMap entryOf(const Launchers& launchers, qsizetype index)
    {
        const QVariantList entries = launchers.entries();
        return index < entries.size() ? entries.at(index).toMap() : QVariantMap();
    }
    QVariantMap entryAt(qsizetype index) const { return entryOf(*m_launchers, index); }
    QJsonArray storedEntries() const
    {
        return m_profile->profile().launchers.value(QStringLiteral("entries")).toArray();
    }
    QJsonObject storedEntry(const QString& id) const
    {
        const QJsonArray entries = storedEntries();
        for (qsizetype i = 0; i < entries.size(); ++i) {
            if (entries.at(i).toObject().value(QStringLiteral("id")).toString() == id)
                return entries.at(i).toObject();
        }
        return {};
    }
    void writeEntries(const QJsonArray& entries)
    {
        m_profile->editProfile().launchers.insert(QStringLiteral("entries"), entries);
        m_profile->touch();
    }
    // Retouche à la main d'un champ du profil (éditeur de texte, profil importé).
    void editStored(const QString& id, const QString& field, const QJsonValue& value)
    {
        QJsonArray entries = storedEntries();
        for (qsizetype i = 0; i < entries.size(); ++i) {
            QJsonObject entry = entries.at(i).toObject();
            if (entry.value(QStringLiteral("id")).toString() == id) {
                entry.insert(field, value);
                entries.replace(i, entry);
            }
        }
        writeEntries(entries);
    }
    // Raccourci venu du profil d'un autre poste : signé par une autre clé, et qui se dit sûr.
    void putForeign(const QString& id, const QString& path, const QStringList& arguments,
                    const QString& toolTip)
    {
        QJsonArray entries = storedEntries();
        const QByteArray signature = Launchers::signatureOf(path, arguments, toolTip, randomKey());
        entries.append(QJsonObject{
            {QStringLiteral("id"), id},
            {QStringLiteral("path"), path},
            {QStringLiteral("arguments"), QJsonArray::fromStringList(arguments)},
            {QStringLiteral("toolTip"), toolTip},
            {QStringLiteral("signature"), QString::fromLatin1(signature.toBase64())},
            {QStringLiteral("trusted"), true},
        });
        writeEntries(entries);
    }
    // Fichier qui existe et ne s'exécute pas : le lancement l'ouvrirait par QDesktopServices::openUrl.
    QString document(const QString& name) const
    {
        const QString path = m_dir->filePath(name);
        QFile file(path);
        if (file.open(QIODevice::WriteOnly))
            file.write("essai");
        return path;
    }
    // Refusé avant toute confirmation : run rend kFailed avec un message, et une acceptation ne lance rien.
    void expectRefused(const QString& id)
    {
        QVERIFY(!id.isEmpty()); // sinon « introuvable » passerait pour un refus
        QSignalSpy failed(m_launchers.get(), &Launchers::failed);
        QSignalSpy asked(m_launchers.get(), &Launchers::confirmationNeeded);
        const int result = m_launchers->run(id);
        QVERIFY2(result == Launchers::kFailed,
                 qPrintable(QStringLiteral("%1 : run rend %2").arg(id).arg(result)));
        QCOMPARE(failed.count(), 1);
        QVERIFY(!failed.at(0).at(0).toString().isEmpty());
        QCOMPARE(asked.count(), 0);
        QVERIFY(!m_launchers->confirm(id, true));
    }

    QString m_probe;
    std::unique_ptr<QTemporaryDir> m_dir;
    std::unique_ptr<ui::Preferences> m_prefs;
    std::unique_ptr<ui::ProfileController> m_profile;
    std::unique_ptr<Launchers> m_launchers;
    QStringList m_keyNames;
};

void TstUiLaunchers::initTestCase()
{
    QCoreApplication::setOrganizationName(QStringLiteral("ActCraftEssais"));
    QCoreApplication::setApplicationName(QStringLiteral("tst_ui_launchers"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    // Le programme d'essai est construit à côté du test (launcher_probe.exe sous Windows).
    m_probe = QStandardPaths::findExecutable(QStringLiteral("launcher_probe"),
                                             {QCoreApplication::applicationDirPath()});
    QVERIFY2(!m_probe.isEmpty(), qPrintable(QCoreApplication::applicationDirPath()));
}

void TstUiLaunchers::init()
{
    m_dir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_dir->isValid());
    QVERIFY(QDir(m_dir->path()).mkpath(kWitnessDir));
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_dir->path());
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, m_dir->path());
    m_prefs = std::make_unique<ui::Preferences>(m_dir->filePath(QStringLiteral("poste.ini")), nullptr);
    actions::ProfileStore store(m_dir->filePath(QStringLiteral("profiles")),
                                QStringLiteral(":/actcraft/actions/profiles"),
                                ui::ProfileController::builtinOrder());
    m_profile = std::make_unique<ui::ProfileController>(std::move(store), m_prefs.get(), nullptr);
    m_profile->start();
    m_keyNames = {
        QStringLiteral("essai-raccourcis-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces))};
    m_launchers = std::make_unique<Launchers>(m_profile.get(), m_keyNames.constFirst());
}

void TstUiLaunchers::cleanup()
{
    m_launchers.reset();
    m_profile.reset();
    m_prefs.reset();
    for (const QString& name : std::as_const(m_keyNames))
        SecretStore::remove(name);
    m_keyNames.clear();
    m_dir.reset();
}

void TstUiLaunchers::addShowsThePath()
{
    QSignalSpy changed(m_launchers.get(), &Launchers::entriesChanged);
    const QString id = m_launchers->add(m_probe, {witness()}, QStringLiteral("Essai"));
    QVERIFY(!id.isEmpty());
    QVERIFY(changed.count() >= 1);
    QCOMPARE(m_launchers->entries().size(), 1);
    const QVariantMap entry = entryAt(0);
    QCOMPARE(entry.value(QStringLiteral("id")).toString(), id);
    QCOMPARE(entry.value(QStringLiteral("path")).toString(), m_probe);
    QCOMPARE(entry.value(QStringLiteral("arguments")).toStringList(), QStringList{witness()});
    QCOMPARE(entry.value(QStringLiteral("toolTip")).toString(), QStringLiteral("Essai"));
    QCOMPARE(entry.value(QStringLiteral("trusted")).toBool(), SecretStore::available());

    // Gardé dans le profil, donc transportable avec lui.
    QCOMPARE(storedEntries().size(), 1);
    QCOMPARE(storedEntry(id).value(QStringLiteral("path")).toString(), m_probe);

    // Sans chemin, rien n'est créé.
    QVERIFY(m_launchers->add(QStringLiteral("  "), {}, QString()).isEmpty());
    QCOMPARE(m_launchers->entries().size(), 1);
}

void TstUiLaunchers::runStartsTheProgram()
{
    const QString id = m_launchers->add(m_probe, {witness()}, QStringLiteral("Essai"));
    QSignalSpy asked(m_launchers.get(), &Launchers::confirmationNeeded);
    const int result = m_launchers->run(id);
    if (SecretStore::available()) {
        QCOMPARE(result, Launchers::kStarted);
        QCOMPARE(asked.count(), 0);
    } else {
        // Sans trousseau, aucune signature ne vérifie : la confirmation précède le lancement.
        QCOMPARE(result, Launchers::kConfirmationNeeded);
        QVERIFY(m_launchers->confirm(id, true));
    }
    // Le témoin, sous un chemin à espaces et « & », est écrit : le paramètre est arrivé entier.
    QTRY_COMPARE_WITH_TIMEOUT(readFile(witness()), QByteArrayLiteral("lance"), kLaunchWaitMs);
}

void TstUiLaunchers::signedOnThisWorkstation()
{
    const QStringList arguments{witness()};
    const QString id = m_launchers->add(m_probe, arguments, QStringLiteral("Essai"));
    const std::optional<QByteArray> key = SecretStore::read(keyName());
    if (SecretStore::available()) {
        // Clé de 32 octets créée au premier usage, rangée dans le trousseau, jamais dans le profil.
        QVERIFY(key.has_value());
        QCOMPARE(key->size(), qsizetype(Launchers::kKeyBytes));
        const QByteArray stored =
            QByteArray::fromBase64(storedEntry(id).value(QStringLiteral("signature")).toString().toLatin1());
        QCOMPARE(stored, Launchers::signatureOf(m_probe, arguments, QStringLiteral("Essai"), *key));
        const QByteArray json = actions::profileToJson(m_profile->profile());
        QVERIFY(!json.contains(key->toBase64()));
        QVERIFY(!json.contains(key->toHex()));
    } else {
        QVERIFY(!key.has_value());
    }

    // Redémarrage : un nouvel objet sur le même poste fait toujours confiance, la clé est gardée.
    Launchers restarted(m_profile.get(), keyName());
    QCOMPARE(entryOf(restarted, 0).value(QStringLiteral("trusted")).toBool(), SecretStore::available());
    QSignalSpy asked(&restarted, &Launchers::confirmationNeeded);
    QCOMPARE(restarted.run(id),
             SecretStore::available() ? Launchers::kStarted : Launchers::kConfirmationNeeded);
    QCOMPARE(asked.count(), SecretStore::available() ? 0 : 1);
    if (SecretStore::available())
        QTRY_COMPARE_WITH_TIMEOUT(readFile(witness()), QByteArrayLiteral("lance"), kLaunchWaitMs);

    // Un autre poste (une autre clé) ne lui fait pas confiance.
    Launchers elsewhere(m_profile.get(), keyName(QStringLiteral("-autre-poste")));
    QVERIFY(!entryOf(elsewhere, 0).value(QStringLiteral("trusted")).toBool());
}

void TstUiLaunchers::foreignSignatureAsksFirst()
{
    // Le poste a sa clé (raccourci à lui), le profil importé en apporte un signé ailleurs.
    m_launchers->add(m_probe, {witness(QStringLiteral("a-moi.txt"))}, QStringLiteral("À moi"));
    putForeign(QStringLiteral("etranger"), m_probe, {witness()}, QStringLiteral("Étranger"));
    QCOMPARE(entryAt(0).value(QStringLiteral("trusted")).toBool(), SecretStore::available());
    // « trusted » écrit dans le profil n'est pas cru : seule la signature compte.
    QVERIFY(!entryAt(1).value(QStringLiteral("trusted")).toBool());

    QSignalSpy asked(m_launchers.get(), &Launchers::confirmationNeeded);
    QCOMPARE(m_launchers->run(QStringLiteral("etranger")), Launchers::kConfirmationNeeded);
    QCOMPARE(asked.count(), 1);
    QCOMPARE(asked.at(0).at(0).toString(), QStringLiteral("etranger"));
    QCOMPARE(asked.at(0).at(1).toString(), m_probe);
    QVERIFY(stayedQuiet(witness()));
}

void TstUiLaunchers::refusedForeignRunsNothing()
{
    putForeign(QStringLiteral("etranger"), m_probe, {witness()}, QStringLiteral("Étranger"));
    const QJsonValue before = storedEntry(QStringLiteral("etranger")).value(QStringLiteral("signature"));
    QCOMPARE(m_launchers->run(QStringLiteral("etranger")), Launchers::kConfirmationNeeded);
    QVERIFY(!m_launchers->confirm(QStringLiteral("etranger"), false));
    // La réponse vaut pour une seule demande : une acceptation tardive ne lance rien.
    QVERIFY(!m_launchers->confirm(QStringLiteral("etranger"), true));
    QVERIFY(stayedQuiet(witness()));
    // Pas signé à nouveau : la demande revient au lancement suivant.
    QCOMPARE(storedEntry(QStringLiteral("etranger")).value(QStringLiteral("signature")), before);
    QSignalSpy asked(m_launchers.get(), &Launchers::confirmationNeeded);
    QCOMPARE(m_launchers->run(QStringLiteral("etranger")), Launchers::kConfirmationNeeded);
    QCOMPARE(asked.count(), 1);
    QVERIFY(!m_launchers->confirm(QStringLiteral("etranger"), false));
}

void TstUiLaunchers::acceptedForeignIsResigned()
{
    const QString id = QStringLiteral("etranger");
    const QStringList arguments{witness()};
    putForeign(id, m_probe, arguments, QStringLiteral("Étranger"));
    QCOMPARE(m_launchers->run(id), Launchers::kConfirmationNeeded);
    QVERIFY(m_launchers->confirm(id, true));
    QTRY_COMPARE_WITH_TIMEOUT(readFile(witness()), QByteArrayLiteral("lance"), kLaunchWaitMs);
    QVERIFY(QFile::remove(witness()));

    QCOMPARE(entryAt(0).value(QStringLiteral("trusted")).toBool(), SecretStore::available());
    QSignalSpy asked(m_launchers.get(), &Launchers::confirmationNeeded);
    const int again = m_launchers->run(id);
    if (SecretStore::available()) {
        // Signé à nouveau par la clé du poste : lancé sans demander.
        QCOMPARE(again, Launchers::kStarted);
        QCOMPARE(asked.count(), 0);
        QTRY_COMPARE_WITH_TIMEOUT(readFile(witness()), QByteArrayLiteral("lance"), kLaunchWaitMs);
        const QByteArray key = SecretStore::read(keyName()).value_or(QByteArray());
        QCOMPARE(
            QByteArray::fromBase64(storedEntry(id).value(QStringLiteral("signature")).toString().toLatin1()),
            Launchers::signatureOf(m_probe, arguments, QStringLiteral("Étranger"), key));
    } else {
        // Sans trousseau, rien pour signer : la demande revient.
        QCOMPARE(again, Launchers::kConfirmationNeeded);
        QCOMPARE(asked.count(), 1);
        QVERIFY(!SecretStore::read(keyName()).has_value());
        QVERIFY(!m_launchers->confirm(id, false));
    }
}

void TstUiLaunchers::removeDeletes()
{
    const QString first = m_launchers->add(m_probe, {QStringLiteral("un")}, QStringLiteral("A"));
    const QString second = m_launchers->add(m_probe, {QStringLiteral("deux")}, QStringLiteral("B"));
    QVERIFY(!first.isEmpty());
    QVERIFY(first != second);
    QSignalSpy changed(m_launchers.get(), &Launchers::entriesChanged);
    QVERIFY(m_launchers->remove(first));
    QVERIFY(changed.count() >= 1);
    QCOMPARE(m_launchers->entries().size(), 1);
    QCOMPARE(entryAt(0).value(QStringLiteral("id")).toString(), second);
    QCOMPARE(storedEntries().size(), 1);
    QVERIFY(!actions::profileToJson(m_profile->profile()).contains(first.toUtf8()));
    QVERIFY(!m_launchers->remove(first));
    QSignalSpy failed(m_launchers.get(), &Launchers::failed);
    QCOMPARE(m_launchers->run(first), Launchers::kFailed);
    QCOMPARE(failed.count(), 1);
}

void TstUiLaunchers::tamperedArgumentsAskAgain()
{
    const QString id = m_launchers->add(m_probe, {witness()}, QStringLiteral("Essai"));
    QCOMPARE(entryAt(0).value(QStringLiteral("trusted")).toBool(), SecretStore::available());

    // Paramètres changés à la main dans le profil : la signature ne vérifie plus.
    const QString other = witness(QStringLiteral("autre.txt"));
    editStored(id, QStringLiteral("arguments"), QJsonArray{other});
    QVERIFY(!entryAt(0).value(QStringLiteral("trusted")).toBool());
    QSignalSpy asked(m_launchers.get(), &Launchers::confirmationNeeded);
    QCOMPARE(m_launchers->run(id), Launchers::kConfirmationNeeded);
    QCOMPARE(asked.count(), 1);
    QCOMPARE(asked.at(0).at(2).toStringList(), QStringList{other});
    QVERIFY(!m_launchers->confirm(id, false));

    // Remis tels quels, ils vérifient de nouveau : c'est bien le contenu qui est signé.
    editStored(id, QStringLiteral("arguments"), QJsonArray{witness()});
    QCOMPARE(entryAt(0).value(QStringLiteral("trusted")).toBool(), SecretStore::available());

    // Chemin et info-bulle sont signés aussi.
    editStored(id, QStringLiteral("toolTip"), QStringLiteral("Calculatrice"));
    QVERIFY(!entryAt(0).value(QStringLiteral("trusted")).toBool());
    QCOMPARE(m_launchers->run(id), Launchers::kConfirmationNeeded);
    QVERIFY(!m_launchers->confirm(id, false));
    editStored(id, QStringLiteral("toolTip"), QStringLiteral("Essai"));
    editStored(id, QStringLiteral("path"), m_probe + QStringLiteral("-autre"));
    QVERIFY(!entryAt(0).value(QStringLiteral("trusted")).toBool());
    QCOMPARE(m_launchers->run(id), Launchers::kConfirmationNeeded);
    QVERIFY(!m_launchers->confirm(id, false));
    QVERIFY(stayedQuiet(other));
    QVERIFY(!QFile::exists(witness()));
}

void TstUiLaunchers::confirmationShowsPathAndArguments()
{
    const QStringList arguments{QStringLiteral("/i"), QStringLiteral("http://exemple.test/x.msi")};
    putForeign(QStringLiteral("msi"), QStringLiteral("msiexec"), arguments, QStringLiteral("Installer"));
    QSignalSpy asked(m_launchers.get(), &Launchers::confirmationNeeded);
    QCOMPARE(m_launchers->run(QStringLiteral("msi")), Launchers::kConfirmationNeeded);
    QCOMPARE(asked.count(), 1);
    QCOMPARE(asked.at(0).at(0).toString(), QStringLiteral("msi"));
    QCOMPARE(asked.at(0).at(1).toString(), QStringLiteral("msiexec"));
    QCOMPARE(asked.at(0).at(2).toStringList(), arguments);
    QVERIFY(!m_launchers->confirm(QStringLiteral("msi"), false));
}

// Le test qui distingue : une concaténation (chemin + « » + paramètres) signerait pareil `["a b", []]` et
// `["a", ["b"]]`, et laisserait un texte passer du chemin aux paramètres sans nouvelle confirmation.
void TstUiLaunchers::signatureIsUnambiguous()
{
    const QByteArray key(Launchers::kKeyBytes, 'k');
    const QString none;
    const QByteArray split = Launchers::signatureOf(QStringLiteral("a b"), {}, none, key);
    const QByteArray moved = Launchers::signatureOf(QStringLiteral("a"), {QStringLiteral("b")}, none, key);
    QCOMPARE(split.size(), 32);
    QVERIFY(split != moved);
    QVERIFY(
        Launchers::signatureOf(QStringLiteral("a"), {QStringLiteral("b c")}, none, key) !=
        Launchers::signatureOf(QStringLiteral("a"), {QStringLiteral("b"), QStringLiteral("c")}, none, key));
    QVERIFY(Launchers::signatureOf(QStringLiteral("a"), {QStringLiteral("b")}, none, key) !=
            Launchers::signatureOf(QStringLiteral("a"), {}, QStringLiteral("b"), key));
    QVERIFY(Launchers::signatureOf(QStringLiteral("a"), {}, none, key) !=
            Launchers::signatureOf(QStringLiteral("a"), {}, none, randomKey()));

    // Message exact des amendements 1 et 2 : JSON compact [chemin, [paramètres], info-bulle].
    QCOMPARE(split, QMessageAuthenticationCode::hash(QByteArrayLiteral(R"(["a b",[],""])"), key,
                                                     QCryptographicHash::Sha256));
    QCOMPARE(moved, QMessageAuthenticationCode::hash(QByteArrayLiteral(R"(["a",["b"],""])"), key,
                                                     QCryptographicHash::Sha256));
}

// cmd.exe interpréterait un .bat ou un .cmd (et ses paramètres) : refusé avant toute confirmation, même
// signé, casse ignorée, et malgré un point ou une espace final que Windows retire du nom.
void TstUiLaunchers::batAndCmdRefused()
{
    const QStringList arguments{QStringLiteral("&"), QStringLiteral("calc")};
    QStringList ids;
    ids << m_launchers->add(QStringLiteral("C:/essai/Script.BAT"), arguments, QStringLiteral("Script"))
        << m_launchers->add(QStringLiteral("C:/essai/lance.Cmd"), arguments, QStringLiteral("Script"));
    putForeign(QStringLiteral("point"), QStringLiteral("C:/essai/point.bat."), arguments, QString());
    putForeign(QStringLiteral("espace"), QStringLiteral("C:/essai/espace.cmd "), arguments, QString());
    ids << QStringLiteral("point") << QStringLiteral("espace");
    for (const QString& id : std::as_const(ids)) {
        QVERIFY(!id.isEmpty());
        QSignalSpy failed(m_launchers.get(), &Launchers::failed);
        QSignalSpy asked(m_launchers.get(), &Launchers::confirmationNeeded);
        QCOMPARE(m_launchers->run(id), Launchers::kFailed);
        QCOMPARE(failed.count(), 1);
        QVERIFY(!failed.at(0).at(0).toString().isEmpty());
        QCOMPARE(asked.count(), 0);
        QVERIFY(!m_launchers->confirm(id, true));
    }
}

// Amendement 10 (a), le test qui distingue : le suffixe seul se contournait. `"C:\scripts\sauve.bat"` (copié
// depuis l'Explorateur) se lit `bat"`, `t.bat<TAB>x` se lit `bat\tx`, `n.bat<NUL>.exe` se lit `exe` alors que
// la ligne de commande de Windows s'arrête au NUL : chacun partirait par cmd.exe avec ses paramètres, et un
// guillemet glisse dans la ligne des paramètres que la boîte ne montre pas. Sans la garde, venus d'un autre
// poste, ils demanderaient confirmation au lieu d'être refusés.
void TstUiLaunchers::unsafePathRefused()
{
    const QStringList arguments{QStringLiteral("&"), QStringLiteral("calc")};
    const QList<std::pair<QString, QString>> paths{
        {QStringLiteral("guillemets"), QStringLiteral(R"("C:\scripts\sauve.bat")")},
        {QStringLiteral("parametres-caches"),
         QStringLiteral(R"("C:\Windows\System32\msiexec.exe" /i http://x/y.msi /qn)")},
        {QStringLiteral("tabulation"), QStringLiteral("C:/essai/t.bat\tx")},
        {QStringLiteral("nul"), QStringLiteral("C:/essai/n.bat") + QChar(u'\0') + QStringLiteral(".exe")},
        {QStringLiteral("suppression"),
         QStringLiteral("C:/essai/d.bat") + QChar(u'\x7f') + QStringLiteral(".exe")},
        {QStringLiteral("inferieur"), QStringLiteral("C:/essai/a<b.exe")},
        {QStringLiteral("superieur"), QStringLiteral("C:/essai/a>b.exe")},
        {QStringLiteral("barre"), QStringLiteral("C:/essai/a|b.exe")},
        {QStringLiteral("question"), QStringLiteral("C:/essai/a?.exe")},
        {QStringLiteral("etoile"), QStringLiteral("C:/essai/*.exe")},
    };
    for (const auto& [id, path] : paths)
        putForeign(id, path, arguments, QString());
    // Signé sur ce poste, il est refusé de même : la garde précède la confiance.
    const QString signedId =
        m_launchers->add(paths.constFirst().second, arguments, QStringLiteral("Sauvegarde"));
    QVERIFY(!signedId.isEmpty());
    expectRefused(signedId);
    for (const auto& [id, path] : paths) {
        if (QTest::currentTestFailed())
            return;
        expectRefused(id);
    }
}

// Amendement 10 (b) : openUrl laisse Windows exécuter tout type associé (scripts, application HTML, raccourci
// dont la boîte ne peut pas montrer la commande). Ces types sont refusés avant toute confirmation, casse
// ignorée, après retrait des points et espaces finaux ; un document ordinaire demande seulement confirmation.
void TstUiLaunchers::executableDocumentsRefused()
{
    QStringList names{QStringLiteral("script.vbs"), QStringLiteral("page.HTA"),
                      QStringLiteral("raccourci.lnk"), QStringLiteral("script.ps1."),
                      QStringLiteral("adresse.Url ")};
    const QStringList types =
        QStringLiteral("bat cmd com scr pif vbs vbe js jse wsf wsh hta ps1 psm1 msi msp msc "
                       "cpl reg lnk url inf scf")
            .split(QLatin1Char(' '));
    for (const QString& type : types)
        names << QStringLiteral("type.") + type;
    QStringList ids;
    for (const QString& name : std::as_const(names)) {
        ids << QStringLiteral("document %1").arg(name);
        putForeign(ids.constLast(), document(name), {}, QString());
    }
    ids << m_launchers->add(document(QStringLiteral("signe.vbs")), {}, QStringLiteral("Signé"));
    for (const QString& id : std::as_const(ids)) {
        if (QTest::currentTestFailed())
            return;
        expectRefused(id);
    }
    if (QTest::currentTestFailed())
        return;

    putForeign(QStringLiteral("notes"), document(QStringLiteral("notes.txt")), {}, QString());
    QSignalSpy asked(m_launchers.get(), &Launchers::confirmationNeeded);
    QCOMPARE(m_launchers->run(QStringLiteral("notes")), Launchers::kConfirmationNeeded);
    QCOMPARE(asked.count(), 1);
    QVERIFY(!m_launchers->confirm(QStringLiteral("notes"), false));
}

// Amendement 10 (c) : openUrl ne transmet aucun paramètre. Un document qui en porte est refusé avant la
// demande, pour que la boîte ne montre jamais ce qui ne partira pas ; sans paramètre, il demande
// confirmation.
void TstUiLaunchers::documentTakesNoArguments()
{
    const QString notes = document(QStringLiteral("notes.txt"));
    putForeign(QStringLiteral("avec"), notes, {QStringLiteral("/p")}, QString());
    expectRefused(QStringLiteral("avec"));
    if (QTest::currentTestFailed())
        return;
    expectRefused(m_launchers->add(notes, {QStringLiteral("/p")}, QStringLiteral("Notes")));
    if (QTest::currentTestFailed())
        return;

    putForeign(QStringLiteral("sans"), notes, {}, QString());
    QSignalSpy asked(m_launchers.get(), &Launchers::confirmationNeeded);
    QCOMPARE(m_launchers->run(QStringLiteral("sans")), Launchers::kConfirmationNeeded);
    QCOMPARE(asked.count(), 1);
    QCOMPARE(asked.at(0).at(1).toString(), notes);
    QVERIFY(asked.at(0).at(2).toStringList().isEmpty());
    QVERIFY(!m_launchers->confirm(QStringLiteral("sans"), false));
}

// Amendement 10 (d) : un identifiant en double (profil retouché ou réimporté) ne montre que la première
// entrée, celle que run lance : l'info-bulle d'une seconde entrée ne couvre pas le lancement de la première.
void TstUiLaunchers::duplicateIdKeepsFirst()
{
    const QString id = QStringLiteral("double");
    const QString first = witness(QStringLiteral("premier.txt"));
    const QString second = witness(QStringLiteral("second.txt"));
    putForeign(id, m_probe, {first}, QStringLiteral("Premier"));
    putForeign(id, m_probe, {second}, QStringLiteral("Calculatrice"));
    QCOMPARE(storedEntries().size(), 2);
    QCOMPARE(m_launchers->entries().size(), 1);
    QCOMPARE(entryAt(0).value(QStringLiteral("toolTip")).toString(), QStringLiteral("Premier"));
    QCOMPARE(entryAt(0).value(QStringLiteral("arguments")).toStringList(), QStringList{first});

    QSignalSpy asked(m_launchers.get(), &Launchers::confirmationNeeded);
    QCOMPARE(m_launchers->run(id), Launchers::kConfirmationNeeded);
    QCOMPARE(asked.count(), 1);
    QCOMPARE(asked.at(0).at(2).toStringList(), QStringList{first});
    QVERIFY(m_launchers->confirm(id, true));
    QTRY_COMPARE_WITH_TIMEOUT(readFile(first), QByteArrayLiteral("lance"), kLaunchWaitMs);
    QVERIFY(!QFile::exists(second));
    // Signée à nouveau (avec trousseau), la première seule : la seconde reste écartée.
    QCOMPARE(m_launchers->entries().size(), 1);
    QCOMPARE(entryAt(0).value(QStringLiteral("trusted")).toBool(), SecretStore::available());
}

// Garde de confirm : ce qui part est ce qui a été montré. Retouché entre la demande et l'acceptation, le
// raccourci ne part ni sous la forme montrée ni sous la nouvelle.
void TstUiLaunchers::changedSinceRequestRunsNothing()
{
    const QString id = QStringLiteral("etranger");
    putForeign(id, m_probe, {witness()}, QStringLiteral("Étranger"));
    QCOMPARE(m_launchers->run(id), Launchers::kConfirmationNeeded);
    const QString other = witness(QStringLiteral("autre.txt"));
    editStored(id, QStringLiteral("arguments"), QJsonArray{other});
    QSignalSpy failed(m_launchers.get(), &Launchers::failed);
    QVERIFY(!m_launchers->confirm(id, true));
    QCOMPARE(failed.count(), 1);
    QVERIFY(stayedQuiet(witness()));
    QVERIFY(!QFile::exists(other));
    // La demande est consommée : une seconde acceptation ne lance rien.
    QVERIFY(!m_launchers->confirm(id, true));
}

QTEST_MAIN(TstUiLaunchers)
#include "tst_ui_launchers.moc"
