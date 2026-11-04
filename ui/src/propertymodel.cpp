// L6-T12 : talon du test rouge, rempli à l'étape suivante.
#include <actcraft/ui/propertymodel.h>

namespace actcraft::ui {

struct PropertyModel::Row
{
};

PropertyModel::PropertyModel(QObject* parent) : QAbstractListModel(parent) {}

PropertyModel::~PropertyModel() = default;

int PropertyModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

QVariant PropertyModel::data(const QModelIndex&, int) const
{
    return {};
}

QHash<int, QByteArray> PropertyModel::roleNames() const
{
    return {};
}

bool PropertyModel::setValue(int, const QVariant&)
{
    return false;
}

int PropertyModel::rowOf(const QString&) const
{
    return -1;
}

DocumentController* PropertyModel::document() const noexcept
{
    return m_document.data();
}

void PropertyModel::setDocument(DocumentController* document)
{
    m_document = document;
}

ToolHost* PropertyModel::tools() const noexcept
{
    return m_tools.data();
}

void PropertyModel::setTools(ToolHost* tools)
{
    m_tools = tools;
}

Commands* PropertyModel::commands() const noexcept
{
    return m_commands.data();
}

void PropertyModel::setCommands(Commands* commands)
{
    m_commands = commands;
}

QString PropertyModel::title() const
{
    return m_title;
}

bool PropertyModel::contactActive() const noexcept
{
    return m_contactActive;
}

void PropertyModel::setContactActive(bool active)
{
    m_contactActive = active;
}

bool PropertyModel::editable() const
{
    return false;
}

int PropertyModel::refreshCount() const noexcept
{
    return m_refreshes;
}

void PropertyModel::requestRefresh() {}
void PropertyModel::schedule() {}
void PropertyModel::flush() {}
void PropertyModel::rebuild() {}
void PropertyModel::attachDocument() {}
void PropertyModel::detachDocument() {}
void PropertyModel::updateEditable() {}
void PropertyModel::onObjectFlagsChanged(int, const QList<quint64>&) {}

} // namespace actcraft::ui
