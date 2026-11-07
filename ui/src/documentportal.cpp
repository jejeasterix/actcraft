#include <actcraft/ui/documentportal.h>

#include <QFileInfo>

#ifdef Q_OS_ANDROID
#include <QJniEnvironment>
#include <QJniObject>
#include <QtCore/qcoreapplication_platform.h>
#endif

namespace actcraft::ui {

namespace {

#ifdef Q_OS_ANDROID
// Intent.FLAG_GRANT_READ_URI_PERMISSION, FLAG_GRANT_WRITE_URI_PERMISSION ; PackageManager.PERMISSION_GRANTED.
constexpr jint kRead = 0x1;
constexpr jint kWrite = 0x2;
constexpr jint kGranted = 0;
const jobject kNone = nullptr;
constexpr char kUriFlags[] = "(Landroid/net/Uri;I)V";

QJniObject androidUri(const QUrl& url)
{
    return QJniObject::callStaticObjectMethod(
        "android/net/Uri", "parse", "(Ljava/lang/String;)Landroid/net/Uri;",
        QJniObject::fromString(url.toString(QUrl::FullyEncoded)).object<jstring>());
}

QJniObject resolver()
{
    const QJniObject context = QNativeInterface::QAndroidApplication::context();
    return context.callObjectMethod("getContentResolver", "()Landroid/content/ContentResolver;");
}

// Appel par le JNIEnv brut : QJniObject::callMethod<void> vide déjà l'exception (Qt 6.12), son échec ne se
// verrait pas.
bool callUriFlags(const QJniObject& target, const char* method, const QJniObject& uri, jint flags)
{
    QJniEnvironment env;
    if (!target.isValid() || !uri.isValid()) {
        return false;
    }
    const jmethodID id = env.findMethod(target.objectClass(), method, kUriFlags);
    if (!id) {
        return false;
    }
    env->CallVoidMethod(target.object(), id, uri.object(), flags);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return true;
}

// Première ligne du fournisseur pour `uri` (curseur à fermer), invalide s'il se tait.
QJniObject firstRow(const QJniObject& uri)
{
    QJniEnvironment env;
    const QJniObject cursor = resolver().callObjectMethod(
        "query",
        "(Landroid/net/Uri;[Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;)"
        "Landroid/database/Cursor;",
        uri.object(), kNone, kNone, kNone, kNone);
    if (env.checkAndClearExceptions() || !cursor.isValid()) {
        return {};
    }
    if (!cursor.callMethod<jboolean>("moveToFirst", "()Z")) {
        cursor.callMethod<void>("close", "()V");
        return {};
    }
    return cursor;
}

// ContentResolver : permissions gardées par le système, sans aucun chemin de fichier.
class AndroidBackend final : public PermissionBackend
{
public:
    bool take(const QUrl& url) override
    {
        const QJniObject uri = androidUri(url);
        const QJniObject content = resolver();
        // Document en lecture seule : la lecture seule est gardée.
        return callUriFlags(content, "takePersistableUriPermission", uri, kRead | kWrite) ||
               callUriFlags(content, "takePersistableUriPermission", uri, kRead);
    }

    bool holds(const QUrl& url) const override
    {
        const QJniObject context = QNativeInterface::QAndroidApplication::context();
        const QJniObject uri = androidUri(url);
        QJniEnvironment env;
        if (!uri.isValid()) {
            return false;
        }
        const jint granted = context.callMethod<jint>("checkCallingOrSelfUriPermission",
                                                      "(Landroid/net/Uri;I)I", uri.object(), kRead);
        return !env.checkAndClearExceptions() && granted == kGranted;
    }

    bool reachable(const QUrl& url) const override
    {
        const QJniObject cursor = firstRow(androidUri(url));
        if (!cursor.isValid()) {
            return false;
        }
        cursor.callMethod<void>("close", "()V");
        return true;
    }

    QString displayName(const QUrl& url) const override
    {
        const QJniObject cursor = firstRow(androidUri(url));
        if (!cursor.isValid()) {
            return {};
        }
        QString name;
        const jint column = cursor.callMethod<jint>(
            "getColumnIndex", "(Ljava/lang/String;)I",
            QJniObject::fromString(QStringLiteral("_display_name")).object<jstring>()); // OpenableColumns
        if (column >= 0) {
            name = cursor.callObjectMethod("getString", "(I)Ljava/lang/String;", column).toString();
        }
        cursor.callMethod<void>("close", "()V");
        return name;
    }

    void release(const QUrl& url) override
    {
        const QJniObject uri = androidUri(url);
        const QJniObject content = resolver();
        if (!callUriFlags(content, "releasePersistableUriPermission", uri, kRead | kWrite)) {
            callUriFlags(content, "releasePersistableUriPermission", uri, kRead); // lecture seule gardée
        }
    }
};
#else
// Bureau : le système ne garde aucune permission ; une URL content:// est tenue, son nom est le repli.
class DesktopBackend final : public PermissionBackend
{
public:
    bool take(const QUrl&) override { return true; }
    bool holds(const QUrl&) const override { return true; }
    bool reachable(const QUrl&) const override { return true; }
    QString displayName(const QUrl&) const override { return {}; }
    void release(const QUrl&) override {}
};
#endif

PermissionBackend& platformBackend()
{
#ifdef Q_OS_ANDROID
    static AndroidBackend backend;
#else
    static DesktopBackend backend;
#endif
    return backend;
}

} // namespace

DocumentPortal::DocumentPortal(QObject* parent) : QObject(parent) {}

PermissionBackend& DocumentPortal::backend() const
{
    return m_backend ? *m_backend : platformBackend();
}

bool DocumentPortal::isContent(const QUrl& url)
{
    return url.scheme() == QLatin1String("content");
}

QString DocumentPortal::fallbackName(const QUrl& url)
{
    return url.isLocalFile() ? QFileInfo(url.toLocalFile()).fileName()
                             : url.path(QUrl::FullyDecoded).section(u'/', -1);
}

QString DocumentPortal::displayName(const QUrl& url) const
{
    if (isContent(url)) {
        const QString name = backend().displayName(url);
        if (!name.isEmpty()) {
            return name;
        }
    }
    return fallbackName(url);
}

bool DocumentPortal::permitted(const QUrl& url) const
{
    return isContent(url) ? backend().holds(url) : true;
}

bool DocumentPortal::reachable(const QUrl& url) const
{
    if (isContent(url)) {
        return backend().reachable(url);
    }
    return url.isLocalFile() && QFileInfo::exists(url.toLocalFile());
}

bool DocumentPortal::keep(const QUrl& url)
{
    return isContent(url) ? backend().take(url) : true;
}

bool DocumentPortal::canOpen(const QUrl& url) const
{
    return permitted(url) && reachable(url);
}

void DocumentPortal::forget(const QUrl& url)
{
    if (isContent(url)) {
        backend().release(url);
    }
}

bool DocumentPortal::admit(const QUrl& url)
{
    if (canOpen(url)) {
        return true;
    }
    emit refused(displayName(url));
    return false;
}

} // namespace actcraft::ui
