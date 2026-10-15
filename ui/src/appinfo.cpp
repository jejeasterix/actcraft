#include <actcraft/ui/appinfo.h>

#include <QFile>

#ifndef ACTCRAFT_VERSION
#error "ACTCRAFT_VERSION doit être défini par ui/CMakeLists.txt"
#endif

// Ressource des notices (qt_add_resources « ui_notices ») : Q_INIT_RESOURCE hors de tout espace de noms.
static void initNoticesResource()
{
    Q_INIT_RESOURCE(ui_notices);
}

namespace actcraft::ui {

AppInfo::AppInfo(QObject* parent) : QObject(parent)
{
    static const bool initialised = [] {
        initNoticesResource();
        return true;
    }();
    Q_UNUSED(initialised)
}

QString AppInfo::version() const
{
    return QStringLiteral(ACTCRAFT_VERSION);
}

QString AppInfo::licenseName() const
{
    return QStringLiteral("MIT");
}

QString AppInfo::projectUrl() const
{
    return QStringLiteral("https://github.com/jejeasterix/actcraft");
}

QString AppInfo::notices() const
{
    QFile file(QStringLiteral(":/actcraft/THIRD_PARTY_NOTICES.md"));
    return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
}

} // namespace actcraft::ui
