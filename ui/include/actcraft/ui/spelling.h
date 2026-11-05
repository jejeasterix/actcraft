#pragma once

#include <actcraft/core/spellcheck.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/profilecontroller.h>

#include <QObject>
#include <QPointer>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

#include <memory>
#include <optional>

namespace actcraft::ui {

// Vérification de l'orthographe (F-297, L6-T34) : relie core::SpellChecker au document. `spell.check` (F7)
// appelle start() : la vérification part de la première page ; à chaque mot signalé, la page qui le contient
// devient la page affichée et le mot est surligné (highlight, coordonnées de page). Les règles d'ignorance
// viennent de la section `spelling` du profil (aucune clé parallèle). Le remplacement passe par
// DocumentController::execute : une étape d'annulation. La page ne change pas pendant un contact
// (contactActive) : le changement attend le relâché. Le dictionnaire français est une ressource ; Hunspell
// lit des fichiers, la copie vit dans dictionaryFolder (cache de l'application par défaut). Le dictionnaire
// de l'utilisateur est un fichier, un mot par ligne (userDictionaryPath ; par défaut
// `<AppDataLocation>/dictionnaire-utilisateur.txt`, sans réglage). Amendement 12 : la boîte appelle ce
// contrôleur sans passer par le registre ; sous le verrou de classe (ou sans `commands`), « Ajouter au
// dictionnaire », « Remplacer » et « Remplacer tout » ne changent rien.
class Spelling : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(
        actcraft::ui::DocumentController* document READ document WRITE setDocument NOTIFY wiringChanged)
    Q_PROPERTY(actcraft::ui::ProfileController* profile READ profile WRITE setProfile NOTIFY wiringChanged)
    Q_PROPERTY(actcraft::ui::Commands* commands READ commands WRITE setCommands NOTIFY wiringChanged)
    Q_PROPERTY(
        QString userDictionaryPath READ userDictionaryPath WRITE setUserDictionaryPath NOTIFY wiringChanged)
    Q_PROPERTY(QString dictionaryFolder READ dictionaryFolder WRITE setDictionaryFolder NOTIFY wiringChanged)
    Q_PROPERTY(bool contactActive READ contactActive WRITE setContactActive NOTIFY wiringChanged)
    Q_PROPERTY(bool running READ running NOTIFY stateChanged)   // un mot est proposé
    Q_PROPERTY(bool finished READ finished NOTIFY stateChanged) // fin du document atteinte
    Q_PROPERTY(QString message READ message NOTIFY stateChanged)
    Q_PROPERTY(QString word READ word NOTIFY stateChanged)
    Q_PROPERTY(bool repeated READ repeated NOTIFY stateChanged)
    Q_PROPERTY(QStringList suggestions READ suggestions NOTIFY stateChanged)
    Q_PROPERTY(int hitPage READ hitPage NOTIFY stateChanged)        // indice de la page du mot, −1 sans mot
    Q_PROPERTY(QRectF highlight READ highlight NOTIFY stateChanged) // coordonnées de page, vide sans mot

public:
    explicit Spelling(QObject* parent = nullptr);
    ~Spelling() override;

    DocumentController* document() const noexcept { return m_document; }
    void setDocument(DocumentController* document);
    ProfileController* profile() const noexcept { return m_profile; }
    void setProfile(ProfileController* profile);
    Commands* commands() const noexcept { return m_commands; }
    void setCommands(Commands* commands);
    QString userDictionaryPath() const;
    void setUserDictionaryPath(const QString& path);
    QString dictionaryFolder() const;
    void setDictionaryFolder(const QString& folder);
    bool contactActive() const noexcept { return m_contactActive; }
    void setContactActive(bool active); // au relâché : la page du mot en attente est affichée

    bool running() const noexcept { return m_hit.has_value(); }
    bool finished() const noexcept { return m_finished; }
    QString message() const { return m_message; }
    QString word() const;
    bool repeated() const noexcept { return m_hit && m_hit->repeated; }
    QStringList suggestions() const { return m_suggestions; }
    int hitPage() const noexcept { return m_hit ? m_hit->page : -1; }
    QRectF highlight() const noexcept { return m_highlight; }

    Q_INVOKABLE void start();           // `spell.check` : depuis la première page
    Q_INVOKABLE void ignore();          // passe le mot
    Q_INVOKABLE void ignoreAlways();    // passe le mot et ses semblables jusqu'à la prochaine vérification
    Q_INVOKABLE void addToDictionary(); // dictionnaire de l'utilisateur
    Q_INVOKABLE void replace(const QString& replacement);    // une étape d'annulation
    Q_INVOKABLE void replaceAll(const QString& replacement); // chaque occurrence du mot, une étape chacune
    Q_INVOKABLE void close();                                // abandonne la vérification

signals:
    void wiringChanged();
    void stateChanged();

private:
    bool prepare(); // moteur et vérificateur prêts ; faux (avec message) si le dictionnaire manque
    bool
    mayWrite() const; // document et dictionnaire de l'utilisateur : refusés sous verrou ou sans `commands`
    void resetState();
    void advance();
    void publish(std::optional<core::SpellHit> hit);
    void showPendingPage();
    QRectF highlightOf(const core::SpellHit& hit) const;
    bool replaceCurrent(const QString& replacement);

    QPointer<DocumentController> m_document;
    QPointer<ProfileController> m_profile;
    QPointer<Commands> m_commands;
    QString m_userDictionaryPath;
    QString m_dictionaryFolder;
    bool m_contactActive = false;
    bool m_finished = false;
    QString m_message;
    QStringList m_suggestions;
    QRectF m_highlight;
    std::optional<core::SpellHit> m_hit;
    int m_pendingPage = -1;
    core::SpellRules m_rules;
    std::unique_ptr<core::SpellEngine> m_engine;
    std::unique_ptr<core::SpellChecker> m_checker;
};

} // namespace actcraft::ui
