#include "buildinfo.h"

#include <actcraft/bench/machineinfo.h>

#include <QSettings>
#include <QSysInfo>
#include <QtGlobal>

#ifdef Q_OS_MACOS
#include <string>
#include <sys/sysctl.h>
#endif

namespace actcraft::bench {

namespace {
#ifdef Q_OS_MACOS
QString sysctlString(const char* name)
{
    size_t size = 0;
    if (sysctlbyname(name, nullptr, &size, nullptr, 0) != 0 || size == 0) {
        return {};
    }
    std::string buffer(size, '\0');
    if (sysctlbyname(name, buffer.data(), &size, nullptr, 0) != 0) {
        return {};
    }
    return QString::fromUtf8(buffer.c_str()).trimmed();
}
#endif
} // namespace

MachineInfo readMachineInfo()
{
    MachineInfo info;
#if defined(Q_OS_WIN)
    const QSettings bios(QStringLiteral("HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\BIOS"),
                         QSettings::NativeFormat);
    info.model = (bios.value(QStringLiteral("SystemManufacturer")).toString() + QLatin1Char(' ') +
                  bios.value(QStringLiteral("SystemProductName")).toString())
                     .trimmed();
    const QSettings cpu(
        QStringLiteral("HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
        QSettings::NativeFormat);
    info.cpu = cpu.value(QStringLiteral("ProcessorNameString")).toString().trimmed();
#elif defined(Q_OS_MACOS)
    info.model = sysctlString("hw.model");
    info.cpu = sysctlString("machdep.cpu.brand_string");
#endif
    info.os = QSysInfo::prettyProductName();
    info.kernel = QSysInfo::kernelVersion();
    info.qtVersion = QString::fromLatin1(qVersion());
    info.commit = QStringLiteral(ACTCRAFT_GIT_COMMIT);
    info.commitDirty = ACTCRAFT_GIT_DIRTY != 0;
    info.buildType = QStringLiteral(ACTCRAFT_BUILD_TYPE);
    info.compilerFlags = QStringLiteral(ACTCRAFT_CXX_FLAGS);
    return info;
}

QString sanitizeForFileName(const QString& text)
{
    QString out;
    bool dash = false;
    for (const QChar c : text) {
        if ((c >= QLatin1Char('a') && c <= QLatin1Char('z')) ||
            (c >= QLatin1Char('A') && c <= QLatin1Char('Z')) ||
            (c >= QLatin1Char('0') && c <= QLatin1Char('9'))) {
            if (dash && !out.isEmpty()) {
                out += QLatin1Char('-');
            }
            out += c;
            dash = false;
        } else {
            dash = true;
        }
    }
    return out.isEmpty() ? QStringLiteral("inconnu") : out;
}

} // namespace actcraft::bench
