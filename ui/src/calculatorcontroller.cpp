#include "calculatorcontroller.h"

#include "presentationdocument.h"

#include <QProcess>
#include <QRectF>
#include <QSettings>

#include <algorithm>
#include <utility>

namespace actcraft::ui {

namespace {

const char* const kModeKey = "tools/calculator/mode";
const char* const kProgramKey = "tools/calculator/program";
constexpr int kLastMode = 2;

bool startProgram(const QString& program)
{
    return QProcess::startDetached(program, {});
}

bool endsWithOperator(const QString& text)
{
    return !text.isEmpty() && QStringLiteral("+−×÷").contains(text.back());
}

// Touches qui saisissent un nombre : la ligne du haut montre alors cette saisie après l'opération.
bool entersANumber(const QString& key)
{
    return key == QLatin1String("MR") ||
           (key.size() == 1 && (key[0].isDigit() || key[0] == QLatin1Char(',')));
}

} // namespace

CalculatorController::CalculatorController(QObject* parent) : QObject(parent), m_launcher(startProgram)
{
    QSettings settings;
    m_mode = std::clamp(settings.value(QString::fromLatin1(kModeKey), 0).toInt(), 0, kLastMode);
    m_programPath = settings.value(QString::fromLatin1(kProgramKey)).toString();
}

QString CalculatorController::display() const
{
    return m_calculator.display();
}

QString CalculatorController::expression() const
{
    const QString shown = m_calculator.expression();
    return m_typing && endsWithOperator(shown) ? shown + QLatin1Char(' ') + m_calculator.display() : shown;
}

QString CalculatorController::pageText() const
{
    return m_calculator.pageText();
}

int CalculatorController::mode() const noexcept
{
    return m_mode;
}

void CalculatorController::setMode(int mode)
{
    mode = std::clamp(mode, 0, kLastMode);
    if (mode == m_mode) {
        return;
    }
    m_mode = mode;
    QSettings settings;
    settings.setValue(QString::fromLatin1(kModeKey), m_mode);
    emit modeChanged();
}

QString CalculatorController::programPath() const
{
    return m_programPath;
}

void CalculatorController::setProgramPath(const QString& path)
{
    if (path == m_programPath) {
        return;
    }
    m_programPath = path;
    QSettings settings;
    settings.setValue(QString::fromLatin1(kProgramKey), m_programPath);
    emit programPathChanged();
}

bool CalculatorController::systemModesAvailable() noexcept
{
#ifdef Q_OS_WIN
    return true;
#else
    return false;
#endif
}

bool CalculatorController::canWrite() const noexcept
{
    return !m_document.isNull();
}

DocumentController* CalculatorController::documentController() const noexcept
{
    return m_document;
}

void CalculatorController::setDocumentController(DocumentController* controller)
{
    if (controller == m_document) {
        return;
    }
    m_document = controller;
    emit documentControllerChanged();
}

void CalculatorController::setLauncher(Launcher launcher)
{
    m_launcher = launcher ? std::move(launcher) : Launcher(startProgram);
}

void CalculatorController::press(const QString& key)
{
    m_calculator.press(key);
    if (key != QStringLiteral("±")) {
        m_typing = entersANumber(key);
    }
    emit calculationChanged();
}

bool CalculatorController::open()
{
    if (!systemModesAvailable() || m_mode == 0) {
        return true;
    }
    const QString program = m_mode == 1 ? QStringLiteral("calc.exe") : m_programPath;
    if (program.isEmpty()) {
        return true;
    }
    return !m_launcher(program);
}

bool CalculatorController::placeOnPage()
{
    if (m_document.isNull()) {
        return false;
    }
    DocumentInsertions insertions(*m_document);
    const core::Page* page = insertions.shownPage();
    if (!page) {
        return false;
    }
    const QRectF bounds(page->width / 2 - 100, page->height / 2 - 20, 200, 40);
    return insertions.insertText(m_calculator.pageText(), bounds) != 0;
}

} // namespace actcraft::ui
