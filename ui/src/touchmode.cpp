#include <actcraft/input/touchmodes.h>
#include <actcraft/ui/touchmode.h>

#include <string>

namespace actcraft::ui {

namespace {

constexpr char kCommand[] = "touch.mode";
constexpr char kSetting[] = "workstation/touch-mode";

bool knownId(const QString& id, input::TouchMode& out)
{
    return input::touchModeFromId(id.toStdString(), out);
}

} // namespace

TouchModeController::TouchModeController(QObject* parent) : QObject(parent), m_mode(QStringLiteral("full")) {}

TouchModeController::~TouchModeController()
{
    if (m_commands) {
        m_commands->setNativeHandler(QString::fromLatin1(kCommand), {});
    }
}

Commands* TouchModeController::commands() const noexcept
{
    return m_commands;
}

void TouchModeController::setCommands(Commands* commands)
{
    if (m_commands == commands) {
        return;
    }
    if (m_commands) {
        m_commands->setNativeHandler(QString::fromLatin1(kCommand), {});
    }
    m_commands = commands;
    if (m_commands) {
        m_commands->setNativeHandler(QString::fromLatin1(kCommand),
                                     [this](const QVariant& argument) { setMode(argument.toString()); });
    }
    emit commandsChanged();
}

Preferences* TouchModeController::preferences() const noexcept
{
    return m_preferences;
}

void TouchModeController::setPreferences(Preferences* preferences)
{
    if (m_preferences == preferences) {
        return;
    }
    m_preferences = preferences;
    if (m_preferences) {
        // Réglage du poste lu au démarrage ; absent ou inconnu : gestion complète.
        const QString stored = m_preferences->value(QString::fromLatin1(kSetting)).toString();
        input::TouchMode parsed{};
        const QString id = knownId(stored, parsed) ? stored : QStringLiteral("full");
        if (id != m_mode) {
            m_mode = id;
            applyPolicy();
            emit modeChanged();
        }
    }
    emit preferencesChanged();
}

PageView* TouchModeController::view() const noexcept
{
    return m_view;
}

void TouchModeController::setView(PageView* view)
{
    if (m_view == view) {
        return;
    }
    if (m_view) {
        disconnect(m_view, &PageView::contactActiveChanged, this, nullptr);
    }
    m_view = view;
    if (m_view) {
        connect(m_view, &PageView::contactActiveChanged, this, &TouchModeController::applyPending);
        applyPolicy();
    }
    emit viewChanged();
}

QString TouchModeController::mode() const
{
    return m_mode;
}

bool TouchModeController::setMode(const QString& id)
{
    input::TouchMode parsed{};
    if (!knownId(id, parsed)) {
        return false;
    }
    if (m_view && m_view->contactActive()) {
        m_pending = id; // la vue garde sa politique jusqu'au relâché
        return true;
    }
    apply(id);
    return true;
}

void TouchModeController::apply(const QString& wanted)
{
    const QString id = wanted; // copie : wanted peut être m_pending
    m_pending.clear();
    if (m_preferences) {
        m_preferences->setValue(QString::fromLatin1(kSetting), id);
    }
    if (id == m_mode) {
        return;
    }
    m_mode = id;
    applyPolicy();
    emit modeChanged();
}

void TouchModeController::applyPolicy()
{
    if (!m_view) {
        return;
    }
    input::TouchMode parsed{};
    if (!knownId(m_mode, parsed)) {
        parsed = input::TouchMode::Full;
    }
    m_view->setRoutingPolicy(input::makeTouchPolicy(parsed));
}

void TouchModeController::applyPending()
{
    if (!m_pending.isEmpty() && m_view && !m_view->contactActive()) {
        apply(m_pending);
    }
}

} // namespace actcraft::ui
