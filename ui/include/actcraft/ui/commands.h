#pragma once

#include <actcraft/actions/commandregistry.h>
#include <actcraft/actions/layout.h>

#include <QHash>
#include <QJSValue>
#include <QObject>
#include <QStringList>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Pont QML du registre de commandes (DA1) : déclarations et disposition lues au démarrage ; menus, palette et
// raccourcis filtrés par plateforme ; déclenchement par nom. Les gestionnaires sont des fonctions JavaScript
// (setHandler), appelées avec l'argument de la commande.
class Commands : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantList menus READ menus NOTIFY layoutChanged)
    Q_PROPERTY(QVariantList palette READ palette NOTIFY layoutChanged)
    Q_PROPERTY(QVariantList shortcuts READ shortcuts NOTIFY layoutChanged)
    Q_PROPERTY(QStringList colors READ colors NOTIFY layoutChanged)
    Q_PROPERTY(QVariantList penWidths READ penWidths NOTIFY layoutChanged)
    Q_PROPERTY(QString platformName READ platformName NOTIFY layoutChanged)
    Q_PROPERTY(QStringList errors READ errors NOTIFY layoutChanged)
    Q_PROPERTY(int revision READ revision NOTIFY stateChanged)

public:
    explicit Commands(QObject* parent = nullptr);

    // Tests : dossier du profil (défaut : <AppConfigLocation>/disposition) et plateforme ; rechargent.
    void setProfileDir(const QString& dir);
    void setPlatform(actions::Platform platform);
    // Relit déclarations et disposition ; garde les gestionnaires posés.
    Q_INVOKABLE void reload();

    // [{id, label, items: [{id, label, shortcut, checkable, separator: false} | {separator: true}]}] ; menus
    // sans commande disponible omis ; séparateurs jamais en tête, en fin ni doublés.
    QVariantList menus() const;
    // [{group, items: [{id, label, icon, checkable}]}] ; « couleurs » et « epaisseur » gardés sans items.
    QVariantList palette() const;
    // [{id, sequence}] des commandes disponibles qui ont un raccourci.
    QVariantList shortcuts() const;
    QStringList colors() const;
    QVariantList penWidths() const;
    QString platformName() const;
    QStringList errors() const;
    int revision() const noexcept;
    const actions::CommandRegistry& registry() const noexcept;

    Q_INVOKABLE bool trigger(const QString& id, const QVariant& argument = QVariant());
    Q_INVOKABLE void setHandler(const QString& id, const QJSValue& handler);
    Q_INVOKABLE QString label(const QString& id) const;
    // « qrc:/qt/qml/ActCraft/Ui/icons/<icône>.svg », ou vide.
    Q_INVOKABLE QString iconUrl(const QString& id) const;
    Q_INVOKABLE QString shortcut(const QString& id) const;
    Q_INVOKABLE bool isAvailable(const QString& id) const;
    Q_INVOKABLE bool isEnabled(const QString& id) const;
    Q_INVOKABLE void setEnabled(const QString& id, bool enabled);
    Q_INVOKABLE bool isChecked(const QString& id) const;
    Q_INVOKABLE void setChecked(const QString& id, bool checked);

signals:
    void layoutChanged();
    void stateChanged();
    void triggered(const QString& id, const QVariant& argument);

private:
    void bindHandler(const QString& id);
    void bumpState();

    actions::CommandRegistry m_registry;
    actions::Layout m_layout;
    actions::Platform m_platform;
    QString m_profileDir;
    QStringList m_errors;
    QHash<QString, QJSValue> m_handlers;
    int m_revision = 0;
};

} // namespace actcraft::ui
