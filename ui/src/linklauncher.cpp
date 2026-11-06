#include "actcraft/ui/linklauncher.h"

// Talon du test rouge (L8-T10) : compile, ne valide rien et n'ouvre rien.
namespace actcraft::ui {

LinkLauncher::LinkLauncher(QObject* parent)
    : QObject(parent)
{
}

void LinkLauncher::setOpener(Opener opener)
{
    m_opener = std::move(opener);
}

void LinkLauncher::request(const QString& target, int kind)
{
    Q_UNUSED(target);
    Q_UNUSED(kind);
}

void LinkLauncher::requestStored(const QByteArray& bytes, const QString& originalName, const QString& documentKey)
{
    Q_UNUSED(bytes);
    Q_UNUSED(originalName);
    Q_UNUSED(documentKey);
}

void LinkLauncher::answer(int token, bool accepted)
{
    Q_UNUSED(token);
    Q_UNUSED(accepted);
}

QString LinkLauncher::refusalFor(const QUrl& url, int kind)
{
    Q_UNUSED(url);
    Q_UNUSED(kind);
    return {};
}

void LinkLauncher::ask(const QUrl& url, int kind)
{
    Q_UNUSED(url);
    Q_UNUSED(kind);
}

} // namespace actcraft::ui
