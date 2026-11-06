#include "actcraft/ui/linkdialogs.h"

namespace actcraft::ui {

LinkDialogs::LinkDialogs(QObject* parent) : QObject(parent) {}
LinkDialogs::~LinkDialogs() = default;
void LinkDialogs::setCommands(Commands*) {}
void LinkDialogs::setDocument(DocumentController*) {}
void LinkDialogs::setToolHost(ToolHost*) {}
void LinkDialogs::setLinks(LinkLauncher*) {}
void LinkDialogs::setArguments(const QString&) {}
bool LinkDialogs::createFileLink(const QUrl&, int, int, quint64)
{
    return false;
}
bool LinkDialogs::createWebsiteLink(const QString&, int, quint64)
{
    return false;
}
bool LinkDialogs::refuse(const QString&)
{
    return false;
}
bool LinkDialogs::locked(const char*) const
{
    return true;
}
bool LinkDialogs::commit(const QString&, const QString&, const QString&, int, quint64, const QByteArray&,
                         const QString&)
{
    return false;
}

} // namespace actcraft::ui
