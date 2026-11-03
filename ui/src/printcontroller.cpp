#include <actcraft/ui/printcontroller.h>

namespace actcraft::ui {

// Talon L5-T1 : rempli par L5-T28.
PrintController::PrintController(QObject* parent) : QObject(parent) {}

// Talon L5-T1 : rempli par L5-T28.
DocumentController* PrintController::document() const
{
    return m_document;
}

// Talon L5-T1 : rempli par L5-T28.
void PrintController::setDocument(DocumentController* document)
{
    m_document = document;
}

// Talon L5-T1 : rempli par L5-T28.
int PrintController::rows() const
{
    return m_rows;
}

// Talon L5-T1 : rempli par L5-T28.
void PrintController::setRows(int value)
{
    m_rows = value;
}

// Talon L5-T1 : rempli par L5-T28.
int PrintController::columns() const
{
    return m_columns;
}

// Talon L5-T1 : rempli par L5-T28.
void PrintController::setColumns(int value)
{
    m_columns = value;
}

// Talon L5-T1 : rempli par L5-T28.
bool PrintController::frames() const
{
    return m_frames;
}

// Talon L5-T1 : rempli par L5-T28.
void PrintController::setFrames(bool value)
{
    m_frames = value;
}

// Talon L5-T1 : rempli par L5-T28.
bool PrintController::pageNumbers() const
{
    return m_pageNumbers;
}

// Talon L5-T1 : rempli par L5-T28.
void PrintController::setPageNumbers(bool value)
{
    m_pageNumbers = value;
}

// Talon L5-T1 : rempli par L5-T28.
QString PrintController::range() const
{
    return m_range;
}

// Talon L5-T1 : rempli par L5-T28.
void PrintController::setRange(const QString& value)
{
    m_range = value;
}

// Talon L5-T1 : rempli par L5-T28.
int PrintController::from() const
{
    return m_from;
}

// Talon L5-T1 : rempli par L5-T28.
void PrintController::setFrom(int value)
{
    m_from = value;
}

// Talon L5-T1 : rempli par L5-T28.
int PrintController::to() const
{
    return m_to;
}

// Talon L5-T1 : rempli par L5-T28.
void PrintController::setTo(int value)
{
    m_to = value;
}

// Talon L5-T1 : rempli par L5-T28.
int PrintController::copies() const
{
    return m_copies;
}

// Talon L5-T1 : rempli par L5-T28.
void PrintController::setCopies(int value)
{
    m_copies = value;
}

// Talon L5-T1 : rempli par L5-T28.
double PrintController::marginMm() const
{
    return m_marginMm;
}

// Talon L5-T1 : rempli par L5-T28.
void PrintController::setMarginMm(double value)
{
    m_marginMm = value;
}

// Talon L5-T1 : rempli par L5-T28.
bool PrintController::gray() const
{
    return m_gray;
}

// Talon L5-T1 : rempli par L5-T28.
void PrintController::setGray(bool value)
{
    m_gray = value;
}

// Talon L5-T1 : rempli par L5-T28.
QString PrintController::notes() const
{
    return m_notes;
}

// Talon L5-T1 : rempli par L5-T28.
void PrintController::setNotes(const QString& value)
{
    m_notes = value;
}

// Talon L5-T1 : rempli par L5-T28.
bool PrintController::noteBreak() const
{
    return m_noteBreak;
}

// Talon L5-T1 : rempli par L5-T28.
void PrintController::setNoteBreak(bool value)
{
    m_noteBreak = value;
}

// Talon L5-T1 : rempli par L5-T28.
bool PrintController::noteSeparator() const
{
    return m_noteSeparator;
}

// Talon L5-T1 : rempli par L5-T28.
void PrintController::setNoteSeparator(bool value)
{
    m_noteSeparator = value;
}

// Talon L5-T1 : rempli par L5-T28.
QString PrintController::printer() const
{
    return m_printer;
}

// Talon L5-T1 : rempli par L5-T28.
void PrintController::setPrinter(const QString& value)
{
    m_printer = value;
}

// Talon L5-T1 : rempli par L5-T28.
QStringList PrintController::printers() const
{
    return {};
}

// Talon L5-T1 : rempli par L5-T28.
int PrintController::sheetCount() const
{
    return 0;
}

// Talon L5-T1 : rempli par L5-T28.
QImage PrintController::previewSheet(int, int)
{
    return {};
}

// Talon L5-T1 : rempli par L5-T28.
bool PrintController::print(const QString&)
{
    return false;
}

// Talon L5-T1 : rempli par L5-T28.
void PrintController::saveSettings() {}

// Talon L5-T1 : rempli par L5-T28.
void PrintController::loadSettings() {}

// Talon L5-T1 : rempli par L5-T28.
QString PrintController::settingsFile() const
{
    return m_settingsFile;
}

// Talon L5-T1 : rempli par L5-T28.
void PrintController::setSettingsFile(const QString& path)
{
    m_settingsFile = path;
}

// Talon L5-T1 : rempli par L5-T28.
bool PrintController::available() const
{
    return false;
}

// Talon L5-T1 : rempli par L5-T28.
int PrintController::previewRenders() const
{
    return m_previewRenders;
}

} // namespace actcraft::ui
