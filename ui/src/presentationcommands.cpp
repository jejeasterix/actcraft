#include "presentationcommands.h"

#include "presentationdocument.h"

#include <actcraft/ui/appinfo.h>

#include <QDesktopServices>
#include <QLocale>
#include <QPointF>
#include <QRectF>
#include <QSettings>

namespace actcraft::ui {

namespace {

constexpr double kTextWidth = 320.0;
constexpr double kTextHeight = 40.0;

} // namespace

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
    const QString configured = QSettings().value(QStringLiteral("tools/webHome")).toString().trimmed();
    return configured.isEmpty() ? AppInfo().projectUrl() : configured;
}

void PresentationCommands::setUrlOpener(UrlOpener opener)
{
    m_opener = std::move(opener);
}

bool PresentationCommands::insertDateTime()
{
    if (!m_document) {
        return false;
    }
    DocumentInsertions insertions(*m_document);
    const core::Page* page = insertions.shownPage();
    if (!page) {
        return false;
    }
    const QDateTime now = m_now.isValid() ? m_now : QDateTime::currentDateTime();
    const QString text = QLocale(QLocale::French).toString(now, QStringLiteral("dddd d MMMM yyyy, HH:mm"));
    const QRectF bounds(page->width / 2.0 - kTextWidth / 2.0, page->height / 2.0 - kTextHeight / 2.0,
                        kTextWidth, kTextHeight);
    return insertions.insertText(text, bounds) != 0;
}

bool PresentationCommands::openWebHome()
{
    const QString address = webHome();
    const QUrl url(address, QUrl::StrictMode);
    const QString scheme = url.scheme().toLower();
    const bool web = (scheme == QLatin1String("http") || scheme == QLatin1String("https")) && url.isValid() &&
                     !url.host().isEmpty();
    if (!web) {
        setMessage(
            tr("Adresse refusée : seules les adresses http et https sont ouvertes (%1).").arg(address));
        return false;
    }
    const bool opened = m_opener ? m_opener(url) : QDesktopServices::openUrl(url);
    setMessage(opened ? QString() : tr("Le navigateur du système n’a pas pu ouvrir %1.").arg(address));
    return opened;
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
