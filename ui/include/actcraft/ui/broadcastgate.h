#pragma once

#include <actcraft/actions/commandregistry.h>
#include <actcraft/core/broadcast.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/textediting.h>

#include <QObject>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QtQml/qqmlregistration.h>

#include <array>

namespace actcraft::ui {

// Diffusion protégée II (F-680, S18, L8-T32) : à chaque document ouvert, la porte calcule les identifiants
// que ses règles refusent (core::restrictionFor, core::forbids) et les pose dans le registre, qui les combine
// comme le verrou de classe (D6-7) : grisés, refusés par trigger, sans toucher l'état propre. Un essai refusé
// affiche un message écrit par nous. Un document sans règles rouvre tout.
class BroadcastGate : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(actcraft::ui::Commands* commands READ commands WRITE setCommands NOTIFY wiringChanged)
    Q_PROPERTY(
        actcraft::ui::DocumentController* document READ document WRITE setDocument NOTIFY wiringChanged)
    // Champs de texte de la page (TextEditor.qml) : Copier et Couper du clavier, hors commande, y sont
    // refusés quand la copie est interdite (filtre posé sur les seuls éléments qui portent les documents du
    // texte).
    Q_PROPERTY(
        actcraft::ui::TextEditing* textEditing READ textEditing WRITE setTextEditing NOTIFY wiringChanged)
    Q_PROPERTY(bool active READ active NOTIFY rulesChanged)
    Q_PROPERTY(bool copyRestricted READ copyRestricted NOTIFY rulesChanged)
    Q_PROPERTY(bool saveRestricted READ saveRestricted NOTIFY rulesChanged)
    Q_PROPERTY(bool printRestricted READ printRestricted NOTIFY rulesChanged)
    Q_PROPERTY(bool cameraRestricted READ cameraRestricted NOTIFY rulesChanged)
    Q_PROPERTY(QString message READ message NOTIFY refused) // dernier refus

public:
    explicit BroadcastGate(QObject* parent = nullptr);

    Commands* commands() const noexcept;
    void setCommands(Commands* commands);
    DocumentController* document() const noexcept;
    void setDocument(DocumentController* document);
    TextEditing* textEditing() const noexcept;
    void setTextEditing(TextEditing* editing);

    bool active() const noexcept;
    bool copyRestricted() const noexcept;
    bool saveRestricted() const noexcept;
    bool printRestricted() const noexcept;
    bool cameraRestricted() const noexcept;
    QString message() const;

    // Identifiants déclarés que `rules` refusent, par catégorie seulement (H8-8 : sans lire `restrictable`).
    static QSet<QString> restrictedIds(const actions::CommandRegistry& registry,
                                       const core::BroadcastRules& rules);
    static QString refusalMessage(core::Restriction restriction);
    // Refus d'un geste hors registre (« Enregistrer les modifications ? ») : message de sa catégorie ; faux
    // si les règles ne refusent pas `id`.
    Q_INVOKABLE bool refuse(const QString& id);

signals:
    void wiringChanged();
    void rulesChanged();
    void refused(const QString& id, const QString& message);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void apply();
    void watchTextField();
    void onRefused(const QString& id);

    QPointer<Commands> m_commands;
    QPointer<DocumentController> m_document;
    core::BroadcastRules m_rules;
    QString m_message;
    QMetaObject::Connection m_documentConnection;
    QMetaObject::Connection m_commandsConnection;
    QPointer<TextEditing> m_textEditing;
    std::array<QPointer<QObject>, 2> m_textFields; // champ horizontal, colonne verticale
    QMetaObject::Connection m_textConnection;
};

} // namespace actcraft::ui
