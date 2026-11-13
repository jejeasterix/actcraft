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
    return QStringLiteral("GNU GPL v3");
}

QString AppInfo::projectUrl() const
{
    return QStringLiteral("https://github.com/jejeasterix/actcraft");
}

namespace {

QString readResource(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
}

} // namespace

QString AppInfo::notices() const
{
    return readResource(QStringLiteral(":/actcraft/THIRD_PARTY_NOTICES.md"));
}

QString AppInfo::copyright() const
{
    return QStringLiteral("Copyright (C) 2026 Jérôme Denis");
}

QString AppInfo::licenseText() const
{
    return readResource(QStringLiteral(":/actcraft/LICENSES/GPL-3.0-only.txt"));
}

} // namespace actcraft::ui
