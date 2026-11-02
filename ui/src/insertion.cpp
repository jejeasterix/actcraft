#include <actcraft/ui/insertion.h>

namespace actcraft::ui {

// Talon L5-T1 : rempli par L5-T15.
Insertion::Insertion(QObject* parent) : QObject(parent) {}

// Talon L5-T1 : rempli par L5-T15.
DocumentController* Insertion::document() const
{
    return m_document;
}

// Talon L5-T1 : rempli par L5-T15.
void Insertion::setDocument(DocumentController* document)
{
    m_document = document;
}

// Talon L5-T1 : rempli par L5-T15.
bool Insertion::isBusy() const
{
    return false;
}

// Talon L5-T1 : rempli par L5-T15.
bool Insertion::lastDecodeOffMainThread() const
{
    return false;
}

// Talon L5-T1 : rempli par L5-T15.
void Insertion::insertImage(const QUrl&) {}

// Talon L5-T1 : rempli par L5-T15.
void Insertion::insertMedia(const QUrl&) {}

// Talon L5-T1 : rempli par L5-T15.
void Insertion::insertImageData(const QImage&) {}

// Talon L5-T1 : rempli par L5-T15.
bool Insertion::acceptDrop(const QVariantList&, const QString&, QPointF)
{
    return false;
}

} // namespace actcraft::ui
