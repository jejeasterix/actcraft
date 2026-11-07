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

QJniObject androidUri(const QUrl& url)
{
    return QJniObject::callStaticObjectMethod(
        "android/net/Uri", "parse", "(Ljava/lang/String;)Landroid/net/Uri;",
        QJniObject::fromString(url.toString(QUrl::FullyEncoded)).object<jstring>());
}

QJniObject resolver(const QJniObject& context)
{
    return context.callObjectMethod("getContentResolver", "()Landroid/content/ContentResolver;");
}

// ContentResolver : permissions gardées par le système, sans aucun chemin de fichier.
class AndroidBackend final : public PermissionBackend
{
public:
    bool take(const QUrl& url) override
    {
        const QJniObject context = QNativeInterface::QAndroidApplication::context();
        const QJniObject uri = androidUri(url);
        const QJniObject content = resolver(context);
        QJniEnvironment env;
        if (!uri.isValid() || !content.isValid()) {
            return false;
        }
        content.callMethod<void>("takePersistableUriPermission", "(Landroid/net/Uri;I)V", uri.object(),
                                 kRead | kWrite);
        if (!env.checkAndClearExceptions()) {
            return true;
        }
        // Document en lecture seule : la lecture seule est gardée.
        content.callMethod<void>("takePersistableUriPermission", "(Landroid/net/Uri;I)V", uri.object(),
                                 kRead);
        return !env.checkAndClearExceptions();
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
        if (env.checkAndClearExceptions() || granted != kGranted) {
            return false;
        }
        // Permission encore là mais document retiré : le fournisseur ne rend plus de ligne.
        const QJniObject cursor = resolver(context).callObjectMethod(
            "query",
            "(Landroid/net/Uri;[Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;)"
            "Landroid/database/Cursor;",
            uri.object(), kNone, kNone, kNone, kNone);
        if (env.checkAndClearExceptions() || !cursor.isValid()) {
            return false;
        }
        const bool found = cursor.callMethod<jboolean>("moveToFirst", "()Z");
        cursor.callMethod<void>("close", "()V");
        return !env.checkAndClearExceptions() && found;
    }

    void release(const QUrl& url) override
    {
        const QJniObject context = QNativeInterface::QAndroidApplication::context();
        const QJniObject uri = androidUri(url);
        const QJniObject content = resolver(context);
        QJniEnvironment env;
        if (uri.isValid() && content.isValid()) {
            content.callMethod<void>("releasePersistableUriPermission", "(Landroid/net/Uri;I)V", uri.object(),
                                     kRead | kWrite);
        }
        env.checkAndClearExceptions(); // permission déjà rendue : rien à faire
    }
};
#else
// Bureau : le système ne garde aucune permission ; une URL content:// est tenue.
class DesktopBackend final : public PermissionBackend
{
public:
    bool take(const QUrl&) override { return true; }
    bool holds(const QUrl&) const override { return true; }
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

QString DocumentPortal::displayName(const QUrl& url)
{
    return url.isLocalFile() ? QFileInfo(url.toLocalFile()).fileName()
                             : url.path(QUrl::FullyDecoded).section(u'/', -1);
}

bool DocumentPortal::keep(const QUrl& url)
{
    return isContent(url) ? backend().take(url) : true;
}

bool DocumentPortal::canOpen(const QUrl& url) const
{
    if (isContent(url)) {
        return backend().holds(url);
    }
    return url.isLocalFile() && QFileInfo::exists(url.toLocalFile());
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
