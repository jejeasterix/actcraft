#pragma once

#include "actcraft/ui/webpolicy.h"

#include <QAbstractListModel>
#include <QHash>
#include <QImage>
#include <QPointer>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVector>
#include <QtQml/qqmlregistration.h>

#include <cstdint>
#include <functional>

// Moteur web construit : vrai quand actcraft_ui est lié à Qt WebView (option
// ACTCRAFT_WEBFRAME du CMakeLists.txt racine ; la cible Qt6::WebView pose
// QT_WEBVIEW_LIB chez tous ceux qui la lient).
#ifndef ACTCRAFT_WEBFRAME
#ifdef QT_WEBVIEW_LIB
#define ACTCRAFT_WEBFRAME 1
#else
#define ACTCRAFT_WEBFRAME 0
#endif
#endif

class QNetworkAccessManager;
class QNetworkReply;
class QQuickItem;

namespace actcraft::ui {

class DocumentController;

// Cadres web de la page courante (S27, F-302), vus par WebFrameLayer.qml.
//
// Chaque requête du cadre passe par WebContentPolicy::decide (L8-T29), jamais
// par une règle posée une fois dans le QML de la vue : Qt WebView 6.12 n'offre
// ni filtre de requêtes ni décision de navigation (seuls `loadingChanged` et
// `urlChanged`, après coup, sous WKWebView comme sous WebView2), donc la vue ne
// reçoit jamais d'adresse à charger. Ce modèle lit lui-même la page après
// `decide`, suit les redirections une à une (chacune décidée), puis réécrit le
// HTML avant de le donner à la vue (`content`, chargé par `loadHtml`) : chaque
// sous-ressource est décidée, celles qui ne passent pas sont neutralisées, une
// règle de sécurité du contenu (CSP) n'autorise que les adresses passées, et
// les liens deviennent des renvois internes
// (`#actcraft-link-<n>`) que la vue rend par `followLink`, décidés à leur tour.
class WebFrameItems : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(
        actcraft::ui::DocumentController* document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int blockedCount READ blockedCount NOTIFY countChanged)
    Q_PROPERTY(bool unblockPending READ unblockPending NOTIFY unblockPendingChanged)
    Q_PROPERTY(QString documentKey READ documentKey NOTIFY documentKeyChanged)
    Q_PROPERTY(bool engineBuilt READ engineBuilt CONSTANT)

public:
    enum Role
    {
        ObjectIdRole = Qt::UserRole + 1,
        BoundsRole,  // QRectF, coordonnées de la page
        HtmlRole,    // code saisi, tel qu'enregistré
        AddressRole, // adresse saisie, telle qu'enregistrée
        BlockedRole, // contenu distant d'un document reçu, page non débloquée
        RefusedRole, // dernière requête refusée par la politique
        LoadingRole, // page en cours de lecture par ce modèle
        ContentRole, // HTML réécrit, seul texte donné à la vue
        BaseUrlRole, // adresse de la page montrée (titre) ; `content` n'a que des
                     // adresses absolues
        TitleRole,   // titre du repère
        CanGoBackRole,
        CanGoForwardRole,
        SerialRole // change à chaque contenu neuf : la vue recharge
    };
    Q_ENUM(Role)

    explicit WebFrameItems(QObject* parent = nullptr);
    ~WebFrameItems() override;

    // Hors ligne : DocumentController n'est que déclaré ici (QPointer<T>::data
    // exige le type complet).
    DocumentController* document() const noexcept;
    void setDocument(DocumentController* document);
    int count() const noexcept { return static_cast<int>(m_rows.size()); }
    int blockedCount() const noexcept;
    bool unblockPending() const noexcept { return m_pendingRow >= 0; }
    QString documentKey() const { return m_key; }
    static bool engineBuilt() noexcept { return ACTCRAFT_WEBFRAME != 0; }

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Boîte du cadre (insert.web-frame) : un cadre neuf, au centre de la page, en
    // une étape. Une adresse saisie ici est « saisie par l'utilisateur » pour la
    // politique (seule voie distante d'un document de la session).
    Q_INVOKABLE bool insertFrame(const QString& html, const QString& address);
    // Poignée relâchée : une étape.
    Q_INVOKABLE bool resizeFrame(int row, const QRectF& bounds);
    // webframe.unblock : la confirmation est montrée par la couche ; answer(vrai)
    // débloque la page courante de ce document seulement.
    Q_INVOKABLE void requestUnblock(int row);
    Q_INVOKABLE void answer(bool confirmed);
    // Adresse prise par la vue (urlChanged, chargement commencé) : un renvoi
    // interne est suivi après décision ; toute adresse autre que about:blank (le
    // contenu donné par loadHtml sans base) et ses fragments, même celle de la
    // page affichée, est une navigation du moteur, rendue au modèle (vrai : la
    // vue s'arrête).
    Q_INVOKABLE bool followLink(int row, const QUrl& viewUrl);
    Q_INVOKABLE void goBack(int row);
    Q_INVOKABLE void goForward(int row);
    Q_INVOKABLE void reload(int row);
    // webframe.snapshot : image de la zone du cadre dans la fenêtre (la vue
    // native échappe à grabToImage), posée sur la page en une étape. Faux sans
    // moyen de capture ou si la capture est vide.
    Q_INVOKABLE bool snapshot(int row);
    // Capture par la fenêtre de `pageView` (ui::PageView, pageToScene) : posée
    // par WebFrameLayer.qml.
    Q_INVOKABLE void useWindowCapture(QQuickItem* pageView);
    // Zone de la page → image ; remplacé par les tests.
    using Grabber = std::function<QImage(const QRectF& pageBounds)>;
    void setGrabber(Grabber grabber) { m_grabber = std::move(grabber); }

    const WebContentPolicy& policy() const noexcept { return m_policy; }

    // Réécriture du HTML avant chargement : `decide` répond pour chaque adresse
    // (résolue sur `base`).
    struct Prepared
    {
        QString html;
        QVector<QUrl> links;  // cibles des renvois internes, dans l'ordre
        QStringList allowed;  // adresses passées, autorisées par la règle de
                              // sécurité du contenu
        bool blocked = false; // une adresse au moins bloquée (document reçu, page
                              // non débloquée)
    };
    static Prepared prepareHtml(const QString& html, const QUrl& base,
                                const std::function<WebContentPolicy::Verdict(const QUrl&)>& decide);

signals:
    void documentChanged();
    void countChanged();
    void unblockPendingChanged();
    void documentKeyChanged();

private:
    struct Row
    {
        std::uint64_t id = 0;
        QRectF bounds;
        QString html;
        QString address;
        bool blocked = false;
        bool refused = false;
        bool loading = false;
        QString content;
        QUrl baseUrl;
        QVector<QUrl> links;
        QUrl pendingUrl; // adresse bloquée, lue au déblocage
        QVector<QUrl> history;
        int historyIndex = -1;
        int serial = 0;
    };

    void rebuild();
    void documentReplaced();
    void prepareRow(Row& row);
    void navigate(Row& row, const QUrl& url, bool pushHistory);
    void fetch(Row& row, const QUrl& url, int redirects);
    void fetched(QNetworkReply* reply);
    void showContent(Row& row, const QString& html, const QUrl& base);
    int rowOf(std::uint64_t id) const;
    void rowChanged(int row);
    WebContentPolicy::Verdict decide(const QUrl& url) const;

    struct Pending
    {
        std::uint64_t id = 0;
        int serial = 0;
        int redirects = 0;
    };

    QPointer<DocumentController> m_document;
    QMetaObject::Connection m_connections[5];
    WebContentPolicy m_policy;
    QString m_key;
    std::uint64_t m_generation = 0;
    std::uint64_t m_keys = 0;
    int m_page = -1;
    QVector<Row> m_rows;
    int m_pendingRow = -1;
    Grabber m_grabber;
    QNetworkAccessManager* m_network = nullptr;
    QHash<QNetworkReply*, Pending> m_replies;
};

} // namespace actcraft::ui
