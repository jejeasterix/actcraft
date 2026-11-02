#include <actcraft/ui/fileexchange.h>

namespace actcraft::ui {

// Talon L5-T1 : rempli par L5-T25.
FileExchange::FileExchange(QObject* parent) : QObject(parent) {}

// Talon L5-T1 : rempli par L5-T25.
DocumentController* FileExchange::document() const
{
    return m_document;
}

// Talon L5-T1 : rempli par L5-T25.
void FileExchange::setDocument(DocumentController* document)
{
    m_document = document;
}

// Talon L5-T1 : rempli par L5-T25.
DocumentTabs* FileExchange::tabs() const
{
    return m_tabs;
}

// Talon L5-T1 : rempli par L5-T25.
void FileExchange::setTabs(DocumentTabs* tabs)
{
    m_tabs = tabs;
}

// Talon L5-T1 : rempli par L5-T25.
bool FileExchange::isBusy() const
{
    return false;
}

// Talon L5-T1 : rempli par L5-T30.
bool FileExchange::pdfImportAvailable() const
{
    return false;
}

// Talon L5-T1 : rempli par L5-T30.
bool FileExchange::powerpointAvailable() const
{
    return false;
}

// Talon L5-T1 : rempli par L5-T25.
void FileExchange::exportPdf(const QUrl&) {}

// Talon L5-T1 : rempli par L5-T25.
void FileExchange::exportPageImage(const QUrl&) {}

// Talon L5-T1 : rempli par L5-T30.
void FileExchange::importPdf(const QUrl&) {}

// Talon L5-T1 : rempli par L5-T30.
void FileExchange::importPowerpointImages(const QUrl&) {}

// Talon L5-T1 : rempli par L5-T25.
void FileExchange::printThroughPdf() {}

} // namespace actcraft::ui
