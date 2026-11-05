#include <actcraft/ui/classroomlock.h>

#include <QCryptographicHash>
#include <QFutureWatcher>
#include <QPasswordDigestor>
#include <QRandomGenerator>
#include <QThread>
#include <QThreadPool>
#include <QtConcurrent>

#include <array>
#include <cstring>

namespace actcraft::ui {

namespace {

QByteArray derive(const QString& password, const QByteArray& salt, int iterations)
{
    if (iterations < 1 || salt.isEmpty()) {
        return {};
    }
    QByteArray utf8 = password.toUtf8();
    const QByteArray key = QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Sha256, utf8, salt,
                                                              iterations, ClassroomLock::kKeyBytes);
    utf8.fill('\0');
    return key;
}

// Temps constant : toutes les positions sont lues, quel que soit le premier écart.
bool sameBytes(const QByteArray& a, const QByteArray& b) noexcept
{
    if (a.size() != b.size() || a.isEmpty()) {
        return false;
    }
    unsigned char diff = 0;
    for (qsizetype i = 0; i < a.size(); ++i) {
        diff |= static_cast<unsigned char>(a.at(i) ^ b.at(i));
    }
    return diff == 0;
}

QByteArray newSalt()
{
    std::array<quint32, ClassroomLock::kSaltBytes / sizeof(quint32)> words{};
    QRandomGenerator::system()->fillRange(words.data(), static_cast<qsizetype>(words.size()));
    QByteArray salt(ClassroomLock::kSaltBytes, Qt::Uninitialized);
    std::memcpy(salt.data(), words.data(), static_cast<std::size_t>(ClassroomLock::kSaltBytes));
    return salt;
}

QByteArray fromBase64(const QVariant& value)
{
    return QByteArray::fromBase64(value.toString().toLatin1());
}

} // namespace

ClassroomLock::ClassroomLock(QObject* parent) : ClassroomLock(kIterations, parent) {}

ClassroomLock::ClassroomLock(int iterations, QObject* parent)
    : QObject(parent), m_iterations(iterations > 0 ? iterations : kIterations)
{}

Commands* ClassroomLock::commands() const noexcept
{
    return m_commands;
}

void ClassroomLock::setCommands(Commands* commands)
{
    if (m_commands == commands) {
        return;
    }
    m_commands = commands;
    if (m_commands && m_locked) {
        m_commands->setLocked(true);
    }
    applyStoredState();
    emit commandsChanged();
}

Preferences* ClassroomLock::preferences() const noexcept
{
    return m_preferences;
}

void ClassroomLock::setPreferences(Preferences* preferences)
{
    if (m_preferences == preferences) {
        return;
    }
    m_preferences = preferences;
    applyStoredState();
    emit preferencesChanged();
    emit hasPasswordChanged();
}

bool ClassroomLock::isLocked() const noexcept
{
    return m_locked;
}

bool ClassroomLock::hasPassword() const
{
    const Stored s = stored();
    return s.hash.size() == kKeyBytes && !s.salt.isEmpty() && s.iterations > 0;
}

bool ClassroomLock::isBusy() const noexcept
{
    return m_busy;
}

Qt::HANDLE ClassroomLock::lastHashThread() const noexcept
{
    return m_lastHashThread;
}

bool ClassroomLock::createPassword(const QString& password, const QString& confirmation)
{
    // m3 : jamais pendant le verrou (empreinte effacée du fichier pendant qu'il tient).
    if (m_busy || m_locked || !m_preferences || hasPassword() || password.isEmpty() ||
        password != confirmation) {
        return false;
    }
    const QByteArray salt = newSalt();
    const int iterations = m_iterations;
    start(
        [password, salt, iterations] {
            HashResult r;
            r.thread = QThread::currentThreadId();
            r.salt = salt;
            r.iterations = iterations;
            r.hash = derive(password, salt, iterations);
            r.ok = r.hash.size() == kKeyBytes;
            return r;
        },
        [this](const HashResult& r) {
            if (!r.ok || !m_preferences || hasPassword()) {
                return false;
            }
            store(r.hash, r.salt, r.iterations);
            return true;
        },
        &ClassroomLock::passwordCreated);
    return true;
}

bool ClassroomLock::lock()
{
    if (!m_commands || !hasPassword()) {
        return false;
    }
    setLockedState(true);
    return true;
}

bool ClassroomLock::unlock(const QString& password)
{
    if (!m_locked || m_busy) {
        return false;
    }
    const Stored s = stored();
    start(
        [password, s] {
            HashResult r;
            r.thread = QThread::currentThreadId();
            r.ok = sameBytes(derive(password, s.salt, s.iterations), s.hash);
            return r;
        },
        [this](const HashResult& r) {
            if (!r.ok) {
                return false;
            }
            setLockedState(false);
            return true;
        },
        &ClassroomLock::unlocked);
    return true;
}

bool ClassroomLock::changePassword(const QString& oldPassword, const QString& newPassword)
{
    if (m_busy || !m_preferences || !hasPassword() || newPassword.isEmpty()) {
        return false;
    }
    const Stored s = stored();
    const QByteArray salt = newSalt();
    const int iterations = m_iterations;
    start(
        [oldPassword, newPassword, s, salt, iterations] {
            HashResult r;
            r.thread = QThread::currentThreadId();
            if (!sameBytes(derive(oldPassword, s.salt, s.iterations), s.hash)) {
                return r;
            }
            r.salt = salt;
            r.iterations = iterations;
            r.hash = derive(newPassword, salt, iterations);
            r.ok = r.hash.size() == kKeyBytes;
            return r;
        },
        [this](const HashResult& r) {
            if (!r.ok || !m_preferences) {
                return false;
            }
            store(r.hash, r.salt, r.iterations);
            return true;
        },
        &ClassroomLock::passwordReplaced);
    return true;
}

void ClassroomLock::start(std::function<HashResult()> job, std::function<bool(const HashResult&)> apply,
                          void (ClassroomLock::*finished)(bool))
{
    setBusy(true);
    auto* watcher = new QFutureWatcher<HashResult>(this);
    connect(watcher, &QFutureWatcher<HashResult>::finished, this,
            [this, watcher, apply = std::move(apply), finished] {
                const HashResult r = watcher->result();
                watcher->deleteLater();
                m_lastHashThread = r.thread;
                const bool ok = apply(r);
                setBusy(false); // avant le signal : son gestionnaire peut relancer une saisie
                emit(this->*finished)(ok);
            });
    watcher->setFuture(QtConcurrent::run(QThreadPool::globalInstance(), std::move(job)));
}

ClassroomLock::Stored ClassroomLock::stored() const
{
    Stored s;
    if (!m_preferences) {
        return s;
    }
    s.hash = fromBase64(m_preferences->value(QString::fromLatin1(kHashKey)));
    s.salt = fromBase64(m_preferences->value(QString::fromLatin1(kSaltKey)));
    bool ok = false;
    const int iterations = fromBase64(m_preferences->value(QString::fromLatin1(kIterationsKey))).toInt(&ok);
    s.iterations = ok && iterations > 0 ? iterations : 0;
    return s;
}

void ClassroomLock::store(const QByteArray& hash, const QByteArray& salt, int iterations)
{
    const bool had = hasPassword();
    m_preferences->setValue(QString::fromLatin1(kHashKey), QString::fromLatin1(hash.toBase64()));
    m_preferences->setValue(QString::fromLatin1(kSaltKey), QString::fromLatin1(salt.toBase64()));
    m_preferences->setValue(QString::fromLatin1(kIterationsKey),
                            QString::fromLatin1(QByteArray::number(iterations).toBase64()));
    if (!had) {
        emit hasPasswordChanged();
    }
}

void ClassroomLock::setLockedState(bool locked)
{
    if (m_commands) {
        m_commands->setLocked(locked);
    }
    if (m_preferences) {
        m_preferences->setValue(QString::fromLatin1(kLockedKey), locked);
    }
    if (m_locked != locked) {
        m_locked = locked;
        emit lockedChanged();
    }
}

// Verrou gardé (relance) : reposé seulement s'il existe un mot de passe pour le lever.
void ClassroomLock::applyStoredState()
{
    if (m_locked || !m_preferences || !hasPassword() ||
        !m_preferences->value(QString::fromLatin1(kLockedKey), false).toBool()) {
        return;
    }
    setLockedState(true);
}

void ClassroomLock::setBusy(bool busy)
{
    if (m_busy != busy) {
        m_busy = busy;
        emit busyChanged();
    }
}

} // namespace actcraft::ui
