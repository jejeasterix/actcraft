// L11-T14 : portail de documents (D11-9). Les permissions gardées d'Android passent par une PermissionBackend
// factice : une URL content:// vaut par sa permission, jamais par un chemin tiré de l'URL. Dossiers
// temporaires et réglages d'essai seulement.
#include <actcraft/ui/documentportal.h>
#include <actcraft/ui/preferences.h>
#include <actcraft/ui/recentdocuments.h>

#include <QFile>
#include <QHash>
#include <QSet>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

using namespace actcraft;

namespace {

// Permissions du système, tenues en mémoire : `held` dit lesquelles sont encore valides.
class FakeBackend final : public ui::PermissionBackend
{
public:
    bool take(const QUrl& url) override
    {
        taken << url.toString();
        held.insert(url.toString());
        return true;
    }
    bool holds(const QUrl& url) const override { return held.contains(url.toString()); }
    bool reachable(const QUrl& url) const override { return !silent.contains(url.toString()); }
    QString displayName(const QUrl& url) const override
    {
        return silent.contains(url.toString()) ? QString() : names.value(url.toString());
    }
    void release(const QUrl& url) override
    {
        released << url.toString();
        held.remove(url.toString());
    }

    QSet<QString> held;
    QSet<QString> silent;          // fournisseur muet (carte démontée, document retiré)
    QHash<QString, QString> names; // OpenableColumns.DISPLAY_NAME
    QStringList taken;
    QStringList released;
};

QUrl touch(const QTemporaryDir& dir, const QString& name)
{
    QFile file(dir.filePath(name));
    if (!file.open(QIODevice::WriteOnly)) {
        return {};
    }
    file.write("x");
    return QUrl::fromLocalFile(file.fileName());
}

// Document rendu par le sélecteur du système : le nom affiché est le dernier segment décodé du chemin.
QUrl picked(const QString& name)
{
    return QUrl(
        QStringLiteral("content://com.android.externalstorage.documents/document/primary%3ADocuments%2F") +
        name);
}

QStringList names(const ui::RecentDocuments& list)
{
    QStringList out;
    for (const QVariant& entry : list.entries()) {
        out << entry.toMap().value(QStringLiteral("name")).toString();
    }
    return out;
}

QStringList stored(const ui::Preferences& preferences)
{
    return preferences.value(QString::fromLatin1(ui::Preferences::kRecent)).toStringList();
}

} // namespace

class TstUiDocumentportal : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase()
    {
        QVERIFY(m_settings.isValid());
        QStandardPaths::setTestModeEnabled(true);
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings.path());
        QSettings probe;
        QCOMPARE(probe.format(), QSettings::IniFormat);
        QVERIFY(probe.fileName().startsWith(m_settings.path()));
    }

    void cleanup()
    {
        ui::Preferences preferences;
        preferences.remove(QString::fromLatin1(ui::Preferences::kRecent));
    }

    void contentUrlKeptOnAdd()
    {
        FakeBackend backend;
        ui::DocumentPortal portal;
        portal.setBackend(&backend);
        ui::RecentDocuments list;
        list.setPortal(&portal);
        const QUrl doc = picked(QStringLiteral("cours.actcraft"));
        list.note(doc);
        QCOMPARE(backend.taken, QStringList{doc.toString()}); // lecture et écriture gardées dès l'ajout
        QCOMPARE(names(list), QStringList{QStringLiteral("cours.actcraft")});
        QVERIFY(portal.keep(doc));
        QVERIFY(portal.canOpen(doc));
    }

    void localFileNeedsNoPermission()
    {
        QTemporaryDir dir;
        FakeBackend backend; // ne tient aucune permission
        ui::DocumentPortal portal;
        portal.setBackend(&backend);
        ui::RecentDocuments list;
        list.setPortal(&portal);
        const QUrl here = touch(dir, QStringLiteral("ici.actcraft"));
        const QUrl gone = QUrl::fromLocalFile(dir.filePath(QStringLiteral("absent.actcraft")));
        QVERIFY(portal.keep(here));
        QVERIFY(portal.canOpen(here));
        QVERIFY(!portal.canOpen(gone));
        list.note(here);
        list.note(gone);
        QVERIFY(backend.taken.isEmpty());
        QCOMPARE(names(list), QStringList{QStringLiteral("ici.actcraft")});
        portal.forget(here);
        QVERIFY(backend.released.isEmpty());
    }

    void lostPermissionRemovesTheEntry()
    {
        QTemporaryDir dir;
        FakeBackend backend;
        ui::DocumentPortal portal;
        portal.setBackend(&backend);
        ui::Preferences preferences;
        // URL rendue par DocumentsUI sur l'émulateur : son dernier segment (« 13 ») n'est pas le nom.
        const QUrl doc(QStringLiteral("content://com.android.providers.downloads.documents/document/13"));
        backend.names.insert(doc.toString(), QStringLiteral("cours.actcraft"));
        const QUrl card = picked(QStringLiteral("carte.actcraft")); // sur une carte bientôt démontée
        const QUrl here = touch(dir, QStringLiteral("ici.actcraft"));
        {
            ui::RecentDocuments list;
            list.setPortal(&portal);
            list.setPreferences(&preferences);
            list.note(card);
            list.note(doc);
            list.note(here);
            QCOMPARE(names(list), (QStringList{"ici.actcraft", "cours.actcraft", "carte.actcraft"}));
        }
        // Document retiré : le fournisseur se tait et la permission est perdue. Carte démontée : il se tait,
        // la permission reste.
        backend.silent.insert(doc.toString());
        backend.held.remove(doc.toString());
        backend.silent.insert(card.toString());

        // Relance : l'entrée perdue est retirée (oubliée) avec le nom gardé ; la carte est cachée, pas
        // oubliée, sa permission gardée.
        ui::RecentDocuments list;
        list.setPortal(&portal);
        QSignalSpy removed(&list, &ui::RecentDocuments::staleRemoved);
        list.setPreferences(&preferences);
        QCOMPARE(removed.count(), 1);
        QCOMPARE(removed.at(0).at(0).toString(), QStringLiteral("cours.actcraft"));
        QCOMPARE(names(list), QStringList{QStringLiteral("ici.actcraft")});
        QCOMPARE(stored(preferences), (QStringList{here.toString(), card.toString()}));
        QCOMPARE(backend.released, QStringList{doc.toString()});

        // La carte revient : l'entrée aussi, avec son nom ; le message ne se répète pas.
        backend.silent.remove(card.toString());
        list.refresh();
        QCOMPARE(removed.count(), 1);
        QCOMPARE(names(list), (QStringList{"ici.actcraft", "carte.actcraft"}));
    }

    void forgetReleases()
    {
        FakeBackend backend;
        ui::DocumentPortal portal;
        portal.setBackend(&backend);
        const QUrl doc = picked(QStringLiteral("cours.actcraft"));
        QVERIFY(portal.keep(doc));
        portal.forget(doc);
        QCOMPARE(backend.released, QStringList{doc.toString()});
        QVERIFY(!portal.canOpen(doc));

        // La liste rend les permissions qu'elle ne montre plus : effacée, ou poussée au-delà de dix.
        ui::RecentDocuments list;
        list.setPortal(&portal);
        list.note(doc);
        backend.released.clear();
        list.clear(); // le document encore ouvert garde sa permission
        QVERIFY(backend.released.isEmpty());
        backend.released.clear();
        for (int i = 0; i <= ui::RecentDocuments::kMax; ++i) {
            list.note(picked(QStringLiteral("f%1.actcraft").arg(i)));
        }
        QCOMPARE(backend.released, QStringList{picked(QStringLiteral("f0.actcraft")).toString()});
    }

    void insertImageUsesThePortal()
    {
        QTemporaryDir dir;
        FakeBackend backend;
        ui::DocumentPortal portal;
        portal.setBackend(&backend);
        QSignalSpy refused(&portal, &ui::DocumentPortal::refused);
        const QUrl image(QStringLiteral(
            "content://com.android.providers.media.documents/document/image%3A42%2Fphoto.png"));
        backend.held.insert(image.toString()); // permission accordée par le sélecteur pour cette lecture
        QVERIFY(portal.admit(image));
        QVERIFY(backend.taken.isEmpty()); // une image se lit une fois : aucune permission gardée
        backend.held.clear();
        QVERIFY(!portal.admit(image));
        QCOMPARE(refused.count(), 1);
        QCOMPARE(refused.at(0).at(0).toString(), QStringLiteral("photo.png"));
        const QUrl pdf = touch(dir, QStringLiteral("cours.pdf"));
        QVERIFY(portal.admit(pdf));
        QVERIFY(!portal.admit(QUrl::fromLocalFile(dir.filePath(QStringLiteral("absent.pdf")))));
        QCOMPARE(refused.count(), 2);
    }

    void noPathFromContentUrl()
    {
        // Une URL content:// dont le chemin désigne un vrai fichier : sa validité ne vient pas de ce fichier.
        QTemporaryDir dir;
        const QUrl real = touch(dir, QStringLiteral("leurre.actcraft"));
        const QUrl decoy(QStringLiteral("content://leurre") + real.path());
        const QUrl heldNowhere(QStringLiteral("content://leurre") +
                               dir.filePath(QStringLiteral("nulle-part.actcraft")));
        FakeBackend backend;
        backend.held.insert(heldNowhere.toString());
        ui::DocumentPortal portal;
        portal.setBackend(&backend);
        QVERIFY(!portal.canOpen(decoy));      // fichier présent au chemin, permission absente
        QVERIFY(portal.canOpen(heldNowhere)); // aucun fichier au chemin, permission tenue

        ui::Preferences preferences;
        preferences.setValue(QString::fromLatin1(ui::Preferences::kRecent),
                             QStringList{decoy.toString(), heldNowhere.toString()});
        ui::RecentDocuments list;
        list.setPortal(&portal);
        list.setPreferences(&preferences);
        QCOMPARE(names(list), QStringList{QStringLiteral("nulle-part.actcraft")});
        const QVariantMap entry = list.entries().value(0).toMap();
        QVERIFY(entry.value(QStringLiteral("url")).toUrl().toLocalFile().isEmpty());
        QVERIFY(!entry.value(QStringLiteral("when")).isValid());
        QCOMPARE(list.summary(nullptr, heldNowhere).value(QStringLiteral("bytes")).toLongLong(), 0);
    }

private:
    QTemporaryDir m_settings;
};

QTEST_MAIN(TstUiDocumentportal)
#include "tst_ui_documentportal.moc"
