#pragma once

#include <actcraft/ui/commands.h>
#include <actcraft/ui/preferences.h>

#include <QByteArray>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QtQml/qqmlregistration.h>

#include <functional>

namespace actcraft::ui {

// Mode classe protégé (F-298, D6-7) : aucun mot de passe par défaut ; le premier verrouillage le fait créer.
// Seuls l'empreinte PBKDF2-SHA256, le sel et le nombre d'itérations sont gardés, en Base64, sous
// `classroom/*` des réglages du poste ; l'état verrouillé y est gardé aussi. Verrouiller pose le verrou du
// registre (Commands::setLocked, que QML ne peut pas appeler) ; le lever demande le mot de passe, comparé en
// temps constant. Le calcul de l'empreinte tourne sur QThreadPool::globalInstance() et rend son résultat par
// signal. Mot de passe oublié : supprimer les clés `classroom/*` (procédure de l'aide).
class ClassroomLock : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(Commands* commands READ commands WRITE setCommands NOTIFY commandsChanged)
    Q_PROPERTY(Preferences* preferences READ preferences WRITE setPreferences NOTIFY preferencesChanged)
    Q_PROPERTY(bool locked READ isLocked NOTIFY lockedChanged)
    Q_PROPERTY(bool hasPassword READ hasPassword NOTIFY hasPasswordChanged)
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)

public:
    static constexpr int kIterations = 200000;
    static constexpr int kSaltBytes = 16;
    static constexpr int kKeyBytes = 32;
    static constexpr const char* kHashKey = "classroom/hash";
    static constexpr const char* kSaltKey = "classroom/salt";
    static constexpr const char* kIterationsKey = "classroom/iterations";
    static constexpr const char* kLockedKey = "classroom/locked";

    explicit ClassroomLock(QObject* parent = nullptr);
    ClassroomLock(int iterations, QObject* parent); // tests : 1 000 itérations

    Commands* commands() const noexcept;
    void setCommands(Commands* commands);
    Preferences* preferences() const noexcept;
    void setPreferences(Preferences* preferences);
    bool isLocked() const noexcept;
    bool hasPassword() const;
    bool isBusy() const noexcept;
    // Tests : fil du dernier calcul d'empreinte (nul avant le premier).
    Qt::HANDLE lastHashThread() const noexcept;

    // Faux sans rien lancer : calcul en cours, mot de passe déjà créé, vide, saisies différentes, ou réglages
    // absents. Sinon, passwordCreated(bool) à la fin.
    Q_INVOKABLE bool createPassword(const QString& password, const QString& confirmation);
    // Faux sans mot de passe (aucun défaut) ou sans commandes ; vrai si le verrou est posé.
    Q_INVOKABLE bool lock();
    // Faux sans rien lancer si rien n'est verrouillé ou qu'un calcul est en cours ; sinon unlocked(bool).
    Q_INVOKABLE bool unlock(const QString& password);
    // L'ancien d'abord ; faux sans rien lancer si le nouveau est vide ; sinon passwordReplaced(bool).
    Q_INVOKABLE bool changePassword(const QString& oldPassword, const QString& newPassword);

signals:
    void commandsChanged();
    void preferencesChanged();
    void lockedChanged();
    void hasPasswordChanged();
    void busyChanged();
    void passwordCreated(bool ok);
    void unlocked(bool ok);
    void passwordReplaced(bool ok);

private:
    struct Stored
    {
        QByteArray hash;
        QByteArray salt;
        int iterations = 0;
    };
    struct HashResult
    {
        QByteArray hash;
        QByteArray salt;
        int iterations = 0;
        bool ok = false;
        Qt::HANDLE thread = nullptr;
    };
    // Calcul sur le fil de la réserve ; `apply` (fil principal) dit le succès, émis par `finished` après
    // busy.
    void start(std::function<HashResult()> job, std::function<bool(const HashResult&)> apply,
               void (ClassroomLock::*finished)(bool));
    Stored stored() const;
    void store(const QByteArray& hash, const QByteArray& salt, int iterations);
    void setLockedState(bool locked);
    void applyStoredState();
    void setBusy(bool busy);

    QPointer<Commands> m_commands;
    QPointer<Preferences> m_preferences;
    int m_iterations = kIterations;
    bool m_locked = false;
    bool m_busy = false;
    Qt::HANDLE m_lastHashThread = nullptr;
};

} // namespace actcraft::ui
