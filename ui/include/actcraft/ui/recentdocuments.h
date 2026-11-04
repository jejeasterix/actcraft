#pragma once

#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

class DocumentController;
class Preferences;

// Documents récents du poste (F-700, D6-1) : `workstation/recent-documents`, dix au plus, le dernier ouvert ou
// enregistré en tête. Un fichier local absent est caché (non oublié) ; une URL content:// est gardée, son
// existence ne se teste pas. `preferences` nul : liste en mémoire seulement.
class RecentDocuments : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(actcraft::ui::Preferences* preferences READ preferences WRITE setPreferences NOTIFY preferencesChanged)
    Q_PROPERTY(QVariantList entries READ entries NOTIFY changed) // [{url, name, when}]
    Q_PROPERTY(int count READ count NOTIFY changed)

public:
    static constexpr int kMax = 10;

    explicit RecentDocuments(QObject* parent = nullptr);

    Preferences* preferences() const { return m_preferences; }
    void setPreferences(Preferences* preferences);
    QVariantList entries() const;
    int count() const;

    // Met `url` en tête (une URL déjà présente remonte) ; une URL vide est ignorée.
    Q_INVOKABLE void note(const QUrl& url);
    Q_INVOKABLE void clear();
    // Nom, pages, objets, ressources et taille du fichier, lus sur le modèle en mémoire (jamais en relisant le
    // fichier) ; `bytes` vaut 0 pour une URL qui n'est pas un fichier local.
    Q_INVOKABLE QVariantMap summary(actcraft::ui::DocumentController* document, const QUrl& url) const;

signals:
    void changed();
    void preferencesChanged();

private:
    void reload();
    void persist();

    QPointer<Preferences> m_preferences;
    QStringList m_urls; // les plus récentes d'abord, au plus kMax
};

} // namespace actcraft::ui
