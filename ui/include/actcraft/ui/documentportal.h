// Talon L11 : rempli par L11-T14.
#pragma once

#include <QObject>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Talon L11 : rempli par L11-T14. Coquille : propriétés posées par la liaison de Main.qml ; la tâche ajoute
// le comportement.
class DocumentPortal : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit DocumentPortal(QObject* parent = nullptr);

signals:

private:
};

} // namespace actcraft::ui
