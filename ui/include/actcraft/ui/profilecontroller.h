#pragma once

#include <actcraft/actions/profilestore.h>
#include <actcraft/ui/commands.h> // types complets exigés par moc pour les propriétés
#include <actcraft/ui/preferences.h>

#include <QHash>
#include <QObject>
#include <QPointF>
#include <QPointer>
#include <QQmlParserStatus>
#include <QTimer>
#include <QVariant>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Profil courant (D6-4) : chargé par actions::ProfileStore, modifié par les setters de l'interface,
// enregistré implicitement kSaveDelayMs après la dernière modification et jamais pendant un contact (le
// relâché relance l'attente). Activer un profil recharge la disposition (Commands::setProfileDir) et
// retient son identifiant dans les réglages du poste. Créable en QML (Main.qml : `appProfile`) ; start()
// n'est appelé que par componentComplete ou, hors QML, explicitement.
class ProfileController : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_ELEMENT
    Q_PROPERTY(actcraft::ui::Commands* commands READ commands WRITE setCommands NOTIFY commandsChanged)
    Q_PROPERTY(actcraft::ui::Preferences* preferences READ preferences WRITE setPreferences NOTIFY
                   preferencesChanged)
    // Dossier des profils de l'utilisateur ; vide : <AppConfigLocation>/profiles. À poser avant le démarrage.
    Q_PROPERTY(QString userRoot READ userRoot WRITE setUserRoot NOTIFY userRootChanged)
    Q_PROPERTY(QString currentId READ currentId NOTIFY profileChanged)
    Q_PROPERTY(QString label READ label NOTIFY profileChanged)
    Q_PROPERTY(QVariantList profiles READ profiles NOTIFY listChanged) // [{id, label, builtin, modified}]
    Q_PROPERTY(int iconPixels READ iconPixels WRITE setIconPixels NOTIFY profileChanged)
    Q_PROPERTY(QString paletteDock READ paletteDock WRITE setPaletteDock NOTIFY profileChanged)
    Q_PROPERTY(QPointF paletteFloat READ paletteFloat WRITE setPaletteFloat NOTIFY profileChanged)
    Q_PROPERTY(bool paletteCollapsed READ paletteCollapsed WRITE setPaletteCollapsed NOTIFY profileChanged)
    Q_PROPERTY(bool shortcutsHidden READ shortcutsHidden WRITE setShortcutsHidden NOTIFY profileChanged)
    Q_PROPERTY(bool palettePinned READ palettePinned WRITE setPalettePinned NOTIFY profileChanged)
    Q_PROPERTY(int swatches READ swatches WRITE setSwatches NOTIFY profileChanged)
    Q_PROPERTY(bool browsersVisible READ browsersVisible WRITE setBrowsersVisible NOTIFY profileChanged)
    Q_PROPERTY(bool browsersPinned READ browsersPinned WRITE setBrowsersPinned NOTIFY profileChanged)
    Q_PROPERTY(QString browsersSide READ browsersSide WRITE setBrowsersSide NOTIFY profileChanged)
    Q_PROPERTY(QString browsersCurrent READ browsersCurrent WRITE setBrowsersCurrent NOTIFY profileChanged)
    Q_PROPERTY(QStringList extraColors READ extraColors NOTIFY profileChanged)
    Q_PROPERTY(QStringList recentTools READ recentTools NOTIFY profileChanged)
    Q_PROPERTY(QStringList pinned READ pinned NOTIFY profileChanged)
    Q_PROPERTY(bool fitPage READ fitPage NOTIFY profileChanged)
    Q_PROPERTY(bool trashVisible READ trashVisible WRITE setTrashVisible NOTIFY profileChanged)
    Q_PROPERTY(bool contactActive READ contactActive WRITE setContactActive NOTIFY contactActiveChanged)
    Q_PROPERTY(int saveCount READ saveCount NOTIFY saved)     // tests et scénarios
    Q_PROPERTY(int startCount READ startCount NOTIFY started) // tests : 1 après componentComplete

public:
    explicit ProfileController(QObject* parent = nullptr); // QML : userRoot, commands, preferences
    ProfileController(actions::ProfileStore store, Preferences* preferences, Commands* commands,
                      QObject* parent = nullptr); // tests : start() à appeler
    ~ProfileController() override;

    static QStringList
    builtinOrder(); // en-classe, preparation, lecture-ecriture, calcul-geometrie, son-image
    static QString defaultProfileId(); // « preparation »
    static QString defaultUserRoot();  // <AppConfigLocation>/profiles

    // Lit le profil courant des préférences ; à défaut, ou s'il ne se lit pas, defaultProfileId().
    void start();
    Q_INVOKABLE bool activate(const QString& id);
    Q_INVOKABLE void saveNow();
    // Écrit <dossier du profil `id`>/layout.json (LayoutEditor::apply) ; profil courant : Commands relit la
    // disposition et profileLayoutChanged est émis. Signale la liste (« modifié » d'un profil fourni). Faux,
    // signal failed émis, si l'écriture est refusée.
    bool writeLayout(const QString& id, const QByteArray& json);
    const actions::Profile& profile() const noexcept;
    actions::Profile& editProfile(); // modifie puis appelle touch()
    void touch();                    // modification : signal, puis enregistrement différé
    actions::ProfileStore& store() noexcept;

    Q_INVOKABLE void addColor(const QString& color);    // F-135, sans doublon
    Q_INVOKABLE void pushRecentTool(const QString& id); // F-140
    Q_INVOKABLE QVariant setting(const QString& section, const QString& key, const QVariant& fallback) const;
    Q_INVOKABLE void setSetting(const QString& section, const QString& key, const QVariant& value);

    Commands* commands() const;
    void setCommands(Commands* commands);
    Preferences* preferences() const;
    void setPreferences(Preferences* preferences);
    QString userRoot() const;
    void setUserRoot(const QString& root);

    QString currentId() const;
    QString label() const;
    QVariantList profiles() const;
    int iconPixels() const;
    void setIconPixels(int pixels);
    QString paletteDock() const;
    void setPaletteDock(const QString& dock);
    QPointF paletteFloat() const;
    void setPaletteFloat(QPointF point);
    bool paletteCollapsed() const;
    void setPaletteCollapsed(bool on);
    bool shortcutsHidden() const;
    void setShortcutsHidden(bool on);
    bool palettePinned() const;
    void setPalettePinned(bool on);
    int swatches() const;
    void setSwatches(int count);
    bool browsersVisible() const;
    void setBrowsersVisible(bool on);
    bool browsersPinned() const;
    void setBrowsersPinned(bool on);
    QString browsersSide() const;
    void setBrowsersSide(const QString& side);
    QString browsersCurrent() const;
    void setBrowsersCurrent(const QString& tab);
    QStringList extraColors() const;
    QStringList recentTools() const;
    QStringList pinned() const;
    bool fitPage() const;
    bool trashVisible() const;
    void setTrashVisible(bool on);
    bool contactActive() const noexcept;
    void setContactActive(bool active); // lié par Main.qml à la garde de contact de la vue (A6, L6-T7)
    int saveCount() const noexcept;
    int startCount() const noexcept;

    static constexpr int kSaveDelayMs = 500;

    void classBegin() override;
    void componentComplete() override;

signals:
    void commandsChanged();
    void preferencesChanged();
    void userRootChanged();
    void profileChanged();
    void listChanged();
    // layout.json du profil courant écrit (writeLayout) ou retiré (« Revenir à l'origine »), même
    // identifiant.
    void profileLayoutChanged();
    void contactActiveChanged();
    void saved();
    void started();
    void failed(const QString& message);

private:
    void scheduleSave();
    void onSaveTimeout();
    void bindCommands();
    // Gestionnaires natifs profile.save-as, delete, reset, import, export (F-882).
    void saveAs(const QString& name);
    void deleteCurrent();
    void resetCurrent();
    void importFrom(const QVariant& source);
    void exportCurrent(const QVariant& target);
    void unbindCommands();
    void applyLayoutDir(const QString& id);
    bool write(const actions::Profile& profile); // faux, signal failed émis, si l'écriture est refusée
    bool hasPendingWrite() const noexcept;

    actions::ProfileStore m_store;
    QString m_userRoot;
    QPointer<Preferences> m_preferences;
    QPointer<Commands> m_commands;
    actions::Profile m_profile;
    // Profils quittés avec une modification non écrite (activation pendant un contact, ou écriture refusée) :
    // écrits par le prochain saveNow(), repris tels quels si on les réactive avant. Jamais le profil courant.
    QHash<QString, actions::Profile> m_departed;
    QTimer m_saveTimer;
    bool m_contactActive = false;
    QString m_pendingActivation; // activation demandée pendant un contact, faite au relâché
    bool m_dirty = false;
    int m_saveCount = 0;
    int m_startCount = 0;
};

} // namespace actcraft::ui
