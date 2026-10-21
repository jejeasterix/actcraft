#include "presentationcommands.h"

#include <QSettings>

namespace actcraft::ui {

PresentationCommands::PresentationCommands(QObject* parent) : QObject(parent) {}

DocumentController* PresentationCommands::documentController() const noexcept
{
    return m_document;
}

void PresentationCommands::setDocumentController(DocumentController* controller)
{
    if (controller == m_document) {
        return;
    }
    m_document = controller;
    emit documentControllerChanged();
}

QDateTime PresentationCommands::nowOverride() const
{
    return m_now;
}

void PresentationCommands::setNowOverride(const QDateTime& now)
{
    if (now == m_now) {
        return;
    }
    m_now = now;
    emit nowOverrideChanged();
}

QString PresentationCommands::message() const
{
    return m_message;
}

QString PresentationCommands::webHome() const
{
    return {};
}

void PresentationCommands::setUrlOpener(UrlOpener opener)
{
    m_opener = std::move(opener);
}

bool PresentationCommands::insertDateTime()
{
    return false;
}

bool PresentationCommands::openWebHome()
{
    return false;
}

void PresentationCommands::setMessage(const QString& message)
{
    if (message == m_message) {
        return;
    }
    m_message = message;
    emit messageChanged();
}

} // namespace actcraft::ui
