#include <actcraft/ui/helpviewer.h>

namespace actcraft::ui {

HelpViewer::HelpViewer(QObject* parent)
    : QObject(parent)
{
}

void HelpViewer::openFor(QString)
{
}

void HelpViewer::follow(QUrl)
{
}

void HelpViewer::show(const QUrl&)
{
}

} // namespace actcraft::ui
