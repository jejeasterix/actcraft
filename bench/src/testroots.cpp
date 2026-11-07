#include <actcraft/bench/testroots.h>
#include <actcraft/ui/resourcelibrary.h>

#include <QDir>

namespace actcraft::bench {

void useIsolatedResourceRoots(const QString& outputDir)
{
    const QDir out(outputDir);
    const QString mine = out.filePath(QStringLiteral("mes-ressources"));
    const QString shared = out.filePath(QStringLiteral("ressources-partagees"));
    QDir(mine).removeRecursively();
    QDir(shared).removeRecursively();
    ui::ResourceLibrary::setRootPathsForTests(mine, shared);
}

} // namespace actcraft::bench
