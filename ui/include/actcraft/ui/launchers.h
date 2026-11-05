#pragma once

#include <actcraft/ui/profilecontroller.h> // type complet exigé par moc pour la propriété

#include <QByteArray>
#include <QHash>
#include <QJsonArray>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <optional>

namespace actcraft::ui {

// Raccourcis vers des programmes ou des fichiers (F-920, N1-A6). Gardés dans le profil (Profile::launchers :
// { "entries": [{id, path, arguments, toolTip, signature}] }), donc transportables, et signés par HMAC-SHA256
// avec une clé de 32 octets propre au poste, créée au premier usage et rangée dans le trousseau (SecretStore,
// L6-T28). La confiance n'est jamais lue du profil : elle est recalculée à chaque lecture. Une signature qui
// ne vérifie pas (profil d'un autre poste, chemin, paramètres ou info-bulle modifiés à la main, trousseau
// absent) demande une confirmation avant tout lancement ; acceptée, le raccourci est signé à nouveau.
// Lancement sans interpréteur : QProcess::startDetached(programme, paramètres en liste) pour un exécutable,
// QDesktopServices::openUrl pour un fichier ; .bat et .cmd refusés (cmd.exe les interpréterait).
class Launchers : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(actcraft::ui::ProfileController* profile READ profile WRITE setProfile NOTIFY profileChanged)
    // [{id, path, arguments, toolTip, trusted}] ; trusted : signature vérifiée par la clé du poste.
    Q_PROPERTY(QVariantList entries READ entries NOTIFY entriesChanged)

public:
    static constexpr int kStarted = 0;
    static constexpr int kConfirmationNeeded = 1;
    static constexpr int kFailed = 2;
    static constexpr int kKeyBytes = 32;
    static constexpr char kKeyName[] = "launchers-key"; // trousseau : « ActCraft/launchers-key »

    explicit Launchers(QObject* parent = nullptr);                                     // QML
    Launchers(ProfileController* profile, QString keyName, QObject* parent = nullptr); // tests

    ProfileController* profile() const;
    void setProfile(ProfileController* profile);
    QVariantList entries() const;

    // Identifiant du raccourci créé, vide si le chemin est vide ou s'il n'y a pas de profil.
    Q_INVOKABLE QString add(const QString& path, const QStringList& arguments, const QString& toolTip);
    Q_INVOKABLE bool remove(const QString& id); // faux si rien n'a été effacé
    // kStarted : lancé ; kConfirmationNeeded : signal confirmationNeeded émis, rien n'est lancé avant
    // confirm() ; kFailed : signal failed émis.
    Q_INVOKABLE int run(const QString& id);
    // Répond à la dernière demande pour `id` ; vrai si le programme est lancé. Sans demande en cours, ou si
    // le raccourci a changé depuis, rien n'est lancé.
    Q_INVOKABLE bool confirm(const QString& id, bool accepted);

    // HMAC-SHA256 du message signé :
    // QJsonDocument(QJsonArray{path, QJsonArray::fromStringList(arguments), toolTip}).toJson(Compact).
    static QByteArray signatureOf(const QString& path, const QStringList& arguments, const QString& toolTip,
                                  const QByteArray& key);

signals:
    void profileChanged();
    void entriesChanged();
    void confirmationNeeded(const QString& id, const QString& path, const QStringList& arguments);
    void failed(const QString& message);

private:
    struct Entry
    {
        QString id;
        QString path;
        QStringList arguments;
        QString toolTip;
        QByteArray signature;
    };

    QJsonArray storedItems() const;
    void store(const QJsonArray& items); // dans le profil, puis touch()
    QList<Entry> readEntries() const;
    std::optional<Entry> find(const QString& id) const;
    QByteArray key(bool create) const; // vide : trousseau indisponible ou clé absente sans création
    static bool trusted(const Entry& entry, const QByteArray& key);
    void sign(const Entry& entry, const QByteArray& key);
    bool launch(const Entry& entry);

    QPointer<ProfileController> m_profile;
    QString m_keyName;
    QHash<QString, Entry> m_pending; // demandes de confirmation en cours, telles que montrées
};

} // namespace actcraft::ui
