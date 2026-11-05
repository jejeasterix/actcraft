#pragma once

#include <QLocale>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

class QCoreApplication;

namespace actcraft::ui {

class Preferences;

// Langue de l'interface (D6-11) : choix parmi les traductions livrées (les `.qm` des ressources), réglage
// `workstation/language` de Preferences (vide : la langue du système), appliqué au démarrage seulement. Un
// changement annonce le redémarrage et ne recharge rien.
class Language : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(Preferences* preferences READ preferences WRITE setPreferences NOTIFY preferencesChanged)
    Q_PROPERTY(bool restartPending READ restartPending NOTIFY restartPendingChanged)

public:
    explicit Language(QObject* parent = nullptr);

    // Codes des traductions livrées (`actcraft_<code>.qm`), triés.
    static QStringList available();
    // Réglage vide : langue du système ; sans traduction pour elle, ou réglage inconnu : le français.
    static QString resolve(const QString& setting, const QStringList& available, const QLocale& system);
    static QString effective(const Preferences& preferences);
    // Installe le traducteur de la langue choisie sur `app` ; faux si aucun `.qm` n'a pu être chargé.
    static bool install(QCoreApplication& app, Preferences& preferences);

    Preferences* preferences() const;
    void setPreferences(Preferences* preferences);
    bool restartPending() const;

    // Écrit le réglage (vide : langue du système).
    Q_INVOKABLE void change(const QString& code);

signals:
    void preferencesChanged();
    void restartPendingChanged();
    void restartNeeded();

private:
    void onPreferencesChanged();

    QPointer<Preferences> m_preferences;
    QString m_atStart; // réglage au moment où la page a été montrée : celui que l'application a appliqué
    QString m_last;
    bool m_pending = false;
};

} // namespace actcraft::ui
