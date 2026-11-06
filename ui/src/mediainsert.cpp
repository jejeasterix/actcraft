#include <actcraft/ui/mediainsert.h>

namespace actcraft::ui {

MediaInsert::MediaInsert(QObject* parent) : QObject(parent) {}
Commands* MediaInsert::commands() const
{
    return m_commands;
}
void MediaInsert::setCommands(Commands* commands)
{
    m_commands = commands;
}
DocumentController* MediaInsert::document() const
{
    return m_document;
}
void MediaInsert::setDocument(DocumentController* document)
{
    m_document = document;
}
bool MediaInsert::isBusy() const
{
    return m_pending > 0;
}
void MediaInsert::setFrameProbe(FrameProbe probe)
{
    m_probe = std::move(probe);
}
void MediaInsert::insertFile(const QUrl&) {}
void MediaInsert::insertResource(const QString&) {}
bool MediaInsert::allowed()
{
    return true;
}

} // namespace actcraft::ui
