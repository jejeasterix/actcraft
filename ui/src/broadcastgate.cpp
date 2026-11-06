#include <actcraft/ui/broadcastgate.h>

#include <QKeyEvent>
#include <QQuickTextDocument>

#include <string_view>

namespace actcraft::ui {

namespace {

bool sameFlags(const core::BroadcastRules& a, const core::BroadcastRules& b) noexcept
{
    return a.noCopy == b.noCopy && a.noSave == b.noSave && a.noPrint == b.noPrint && a.noCamera == b.noCamera;
}

std::optional<core::Restriction> restrictionOf(const QString& id)
{
    const QByteArray utf8 = id.toUtf8();
    return core::restrictionFor(std::string_view(utf8.constData(), static_cast<std::size_t>(utf8.size())));
}

} // namespace

BroadcastGate::BroadcastGate(QObject* parent) : QObject(parent) {}

Commands* BroadcastGate::commands() const noexcept
{
    return m_commands.data();
}

void BroadcastGate::setCommands(Commands* commands)
{
    if (m_commands == commands) {
        return;
    }
    if (m_commands) {
        QObject::disconnect(m_commandsConnection);
        m_commands->setRestricted({}); // l'ancien registre n'est plus gardé par cette porte
    }
    m_commands = commands;
    if (m_commands) {
        m_commandsConnection =
            connect(m_commands, &Commands::refusedByRestriction, this, &BroadcastGate::onRefused);
        m_commands->setRestricted(restrictedIds(m_commands->registry(), m_rules));
    }
    emit wiringChanged();
}

DocumentController* BroadcastGate::document() const noexcept
{
    return m_document.data();
}

void BroadcastGate::setDocument(DocumentController* document)
{
    if (m_document == document) {
        return;
    }
    QObject::disconnect(m_documentConnection);
    m_document = document;
    if (m_document) {
        // documentChanged suit aussi chaque édition : apply() ne fait rien tant que les règles sont les
        // mêmes.
        m_documentConnection =
            connect(m_document, &DocumentController::documentChanged, this, &BroadcastGate::apply);
    }
    emit wiringChanged();
    apply();
}

TextEditing* BroadcastGate::textEditing() const noexcept
{
    return m_textEditing.data();
}

void BroadcastGate::setTextEditing(TextEditing* editing)
{
    if (m_textEditing == editing) {
        return;
    }
    QObject::disconnect(m_textConnection);
    m_textEditing = editing;
    if (m_textEditing) {
        m_textConnection =
            connect(m_textEditing, &TextEditing::wiringChanged, this, &BroadcastGate::watchTextField);
    }
    watchTextField();
    emit wiringChanged();
}

// Chaque document du texte a pour parent le TextEdit qui le porte : le filtre ne voit que leurs événements.
void BroadcastGate::watchTextField()
{
    QQuickTextDocument* documents[2] = {m_textEditing ? m_textEditing->textDocument() : nullptr,
                                        m_textEditing ? m_textEditing->columnDocument() : nullptr};
    for (std::size_t i = 0; i < m_textFields.size(); ++i) {
        QObject* field = documents[i] ? documents[i]->parent() : nullptr;
        if (field == m_textFields[i]) {
            continue;
        }
        if (m_textFields[i]) {
            m_textFields[i]->removeEventFilter(this);
        }
        m_textFields[i] = field;
        if (field) {
            field->installEventFilter(this);
        }
    }
}

bool BroadcastGate::eventFilter(QObject* watched, QEvent* event)
{
    const bool watchedField =
        watched != nullptr && (watched == m_textFields[0] || watched == m_textFields[1]);
    if (!watchedField || !m_rules.noCopy ||
        (event->type() != QEvent::ShortcutOverride && event->type() != QEvent::KeyPress)) {
        return QObject::eventFilter(watched, event);
    }
    auto* key = static_cast<QKeyEvent*>(event);
    const bool copy = key->matches(QKeySequence::Copy);
    const bool cut = key->matches(QKeySequence::Cut);
    if (!copy && !cut) {
        return QObject::eventFilter(watched, event);
    }
    if (event->type() == QEvent::ShortcutOverride) {
        // Pris par le champ, comme il le ferait : aucun raccourci de la fenêtre ne part ; l'appui suit,
        // refusé.
        event->accept();
        return true;
    }
    onRefused(copy ? QStringLiteral("edit.copy") : QStringLiteral("edit.cut"));
    return true;
}

bool BroadcastGate::active() const noexcept
{
    return m_rules.noCopy || m_rules.noSave || m_rules.noPrint || m_rules.noCamera;
}

bool BroadcastGate::copyRestricted() const noexcept
{
    return m_rules.noCopy;
}

bool BroadcastGate::saveRestricted() const noexcept
{
    return m_rules.noSave;
}

bool BroadcastGate::printRestricted() const noexcept
{
    return m_rules.noPrint;
}

bool BroadcastGate::cameraRestricted() const noexcept
{
    return m_rules.noCamera;
}

QString BroadcastGate::message() const
{
    return m_message;
}

QSet<QString> BroadcastGate::restrictedIds(const actions::CommandRegistry& registry,
                                           const core::BroadcastRules& rules)
{
    QSet<QString> ids;
    for (const QString& id : registry.ids()) {
        const std::optional<core::Restriction> restriction = restrictionOf(id);
        if (restriction && core::forbids(rules, *restriction)) {
            ids.insert(id);
        }
    }
    return ids;
}

QString BroadcastGate::refusalMessage(core::Restriction restriction)
{
    switch (restriction) {
    case core::Restriction::Copy:
        return tr("Diffusion protégée : ce paperboard interdit la copie de son contenu.");
    case core::Restriction::Save:
        return tr("Diffusion protégée : ce paperboard ne peut pas être enregistré ni exporté.");
    case core::Restriction::Print:
        return tr("Diffusion protégée : ce paperboard ne peut pas être imprimé.");
    case core::Restriction::Camera:
        return tr("Diffusion protégée : la capture de l'écran est interdite avec ce paperboard.");
    }
    return {};
}

bool BroadcastGate::refuse(const QString& id)
{
    const std::optional<core::Restriction> restriction = restrictionOf(id);
    if (!restriction || !core::forbids(m_rules, *restriction)) {
        return false;
    }
    onRefused(id);
    return true;
}

void BroadcastGate::apply()
{
    const core::BroadcastRules rules =
        m_document ? m_document->document().broadcast() : core::BroadcastRules{};
    if (sameFlags(rules, m_rules)) {
        return;
    }
    m_rules = rules;
    if (m_commands) {
        m_commands->setRestricted(restrictedIds(m_commands->registry(), m_rules));
    }
    emit rulesChanged();
}

void BroadcastGate::onRefused(const QString& id)
{
    const std::optional<core::Restriction> restriction = restrictionOf(id);
    if (!restriction) {
        return;
    }
    m_message = refusalMessage(*restriction);
    emit refused(id, m_message);
}

} // namespace actcraft::ui
