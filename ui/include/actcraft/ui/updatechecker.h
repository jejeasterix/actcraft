#pragma once

#include <actcraft/ui/preferences.h>

#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringView>
#include <QTimer>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

#include <memory>

class QNetworkReply;

namespace actcraft::ui {

// Recherche de mises à jour (F-760, F-930 ; D11-13). Une requête GET sur l'API des releases du dépôt, et
// seulement sur demande : check() par la commande, start() au démarrage seulement si le réglage du poste
// `updates/check-at-start` est vrai (faux par défaut). Rien n'est téléchargé, aucun identifiant envoyé.
// Le gestionnaire interne suit le mandataire de l'application posé par NetworkSettings (lot 6).
// Réponse absente, refusée (404 d'un dépôt privé) ou illisible : Failed, « impossible de vérifier ».
class UpdateChecker : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(actcraft::ui::Preferences* preferences READ preferences WRITE setPreferences NOTIFY
                   preferencesChanged)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY stateChanged)
    Q_PROPERTY(QUrl releasePage READ releasePage NOTIFY stateChanged)
    Q_PROPERTY(bool checkAtStart READ checkAtStart WRITE setCheckAtStart NOTIFY checkAtStartChanged)
    Q_PROPERTY(int requestCount READ requestCount NOTIFY stateChanged)

public:
    enum class State
    {
        Idle,
        Checking,
        UpToDate,
        Available,
        Failed
    };
    Q_ENUM(State)

    static constexpr const char* kCheckAtStart = "updates/check-at-start";
    static constexpr const char* kEndpointVariable = "ACTCRAFT_UPDATES_URL";
    static constexpr const char* kDefaultEndpoint =
        "https://api.github.com/repos/jejeasterix/actcraft/releases/latest";

    explicit UpdateChecker(QObject* parent = nullptr);
    ~UpdateChecker() override;

    // Versions « x.y.z », préfixe « v » toléré ; faux si l'une ne se lit pas.
    static bool isNewer(QStringView candidate, QStringView current);
    // Point d'entrée : ACTCRAFT_UPDATES_URL si file: ou http://127.0.0.1 / localhost, sinon l'API des
    // releases du dépôt.
    static QUrl endpoint();
    void setNetworkAccessManager(QNetworkAccessManager* manager); // tests ; sinon un gestionnaire interne
    void setCurrentVersion(const QString& version);               // tests ; sinon la version construite
    Q_INVOKABLE void check();
    Q_INVOKABLE void start(); // appelé une fois au démarrage : check() seulement si checkAtStart

    Preferences* preferences() const;
    void setPreferences(Preferences* preferences);
    State state() const;
    QString latestVersion() const;
    QUrl releasePage() const;
    bool checkAtStart() const; // réglage updates/check-at-start, faux par défaut
    void setCheckAtStart(bool on);
    int requestCount() const;

signals:
    void preferencesChanged();
    void stateChanged();
    void checkAtStartChanged();

private:
    QNetworkAccessManager* manager();
    void finish(QNetworkReply* reply);
    void settle(State state, const QString& version = {}, const QUrl& page = {});

    QPointer<Preferences> m_preferences;
    QPointer<QNetworkAccessManager> m_manager;
    std::unique_ptr<QNetworkAccessManager> m_ownManager;
    QPointer<QNetworkReply> m_reply;
    QTimer m_deadline; // durée totale d'une vérification
    QString m_currentVersion;
    State m_state = State::Idle;
    QString m_latestVersion;
    QUrl m_releasePage;
    int m_requestCount = 0;
};

} // namespace actcraft::ui
