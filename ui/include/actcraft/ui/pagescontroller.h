#pragma once

#include <actcraft/core/model.h>
#include <actcraft/ui/documentcontroller.h>

#include <QList>
#include <QObject>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

#include <vector>

namespace actcraft::ui {

// Pages du document vues par le navigateur de pages et le menu de page (F-600, F-601, F-771, F-801 à F-803).
class PagesController : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Pages)
    Q_PROPERTY(actcraft::ui::DocumentController* documentController READ documentController WRITE
                   setDocumentController NOTIFY wiringChanged)
    Q_PROPERTY(QList<int> selection READ selection NOTIFY selectionChanged) // indices triés, jamais vide
    Q_PROPERTY(bool canPaste READ canPaste NOTIFY clipboardChanged)

public:
    explicit PagesController(QObject* parent = nullptr);
    DocumentController* documentController() const noexcept;
    void setDocumentController(DocumentController* controller);
    QList<int> selection() const;
    bool canPaste() const noexcept;

    // F-801 : sans modificateur, sélection = {index} et la page s'affiche ; Maj : plage depuis l'ancre ; Ctrl
    // : bascule (la dernière page sélectionnée ne se retire pas). `modifiers` : Qt::KeyboardModifiers.
    Q_INVOKABLE void click(int index, int modifiers);
    Q_INVOKABLE void goTo(int index);           // F-800 : sans effet de changement de page
    Q_INVOKABLE void moveSelection(int before); // F-802 : pages gardées sélectionnées
    Q_INVOKABLE void insertBlank(bool after);   // F-600, par rapport à la page affichée
    Q_INVOKABLE void copySelection();           // F-601 : presse-papiers interne
    Q_INVOKABLE void cutSelection();
    Q_INVOKABLE void paste();              // F-601, F-803 : avant la page active
    Q_INVOKABLE void duplicateSelection(); // F-600 : après la dernière page sélectionnée
    Q_INVOKABLE void deleteSelection();

signals:
    void wiringChanged();
    void selectionChanged();
    void clipboardChanged();

private:
    QPointer<DocumentController> m_controller;
    QList<int> m_selection;
    int m_anchor = 0;
    std::vector<core::Page> m_clipboard;
};

} // namespace actcraft::ui
