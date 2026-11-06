#include <actcraft/actions/objectactions.h>

namespace actcraft::actions {

bool ObjectActionCatalog::load(const QByteArray&, QString& error)
{
    error = QStringLiteral("non implémenté");
    return false;
}
const ObjectActionInfo* ObjectActionCatalog::find(const QString&) const { return nullptr; }
QList<const ObjectActionInfo*> ObjectActionCatalog::inCategory(ActionCategory) const { return {}; }
QList<const ObjectActionInfo*> ObjectActionCatalog::dragDropList() const { return {}; }
QByteArray defaultObjectActionsJson() { return {}; }

} // namespace actcraft::actions
