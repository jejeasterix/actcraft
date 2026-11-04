#pragma once
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QString>
#include <QUrl>
#include <QVariantList>

#include <memory>
#include <vector>

namespace actcraft::ui {

// Onglets des paperboards ouverts (F-010, F-011) ; rempli par L5-T20.
class DocumentTabs : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(DocumentController* document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    // La vue unique, montrée par oneViewForAllTabs ; aucune demande de redessin avant un garage :
    // resumeSession rattache la page, et restart vide déjà la couche d'attente.
    Q_PROPERTY(PageView* view READ view WRITE setView NOTIFY viewChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QString position READ position WRITE setPosition NOTIFY positionChanged) // top, bottom, hidden
public:
    enum Role
    {
        TitleRole = Qt::UserRole + 1,
        ModifiedRole,
        CurrentRole,
        UrlRole,
    };
    Q_ENUM(Role)

    explicit DocumentTabs(QObject* parent = nullptr);
    ~DocumentTabs() override; // L5-T20 : les sessions garées sont détruites dans documenttabs.cpp
    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    DocumentController* document() const;
    void setDocument(DocumentController* document);
    PageView* view() const;
    void setView(PageView* view);
    int currentIndex() const;
    QString position() const;
    void setPosition(const QString& position);

    Q_INVOKABLE void newTab();
    Q_INVOKABLE void openInNewTab(const QUrl& url);
    Q_INVOKABLE void activate(int index);
    Q_INVOKABLE bool requestClose(int index); // faux : saveRequested(int) émis
    Q_INVOKABLE void closeWithoutSaving(int index);
    Q_INVOKABLE void copyPagesToTab(const QVariantList& pageIndexes,
                                    int tabIndex); // F-011 : avant la première page
    // L5-C11 : un onglet garé ou affiché est-il modifié ? (Quitter les parcourt tous ; ajout seul.)
    Q_INVOKABLE bool isModifiedAt(int index) const;
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();

signals:
    void documentChanged();
    void countChanged();
    void viewChanged();
    void currentIndexChanged();
    void positionChanged();
    void saveRequested(int index);

private:
    // L5-T20 : un onglet ; `parked` est nul pour l'onglet affiché (son document est au contrôleur).
    struct Tab;
    bool canSwitch() const;
    void switchTo(int index, std::unique_ptr<DocumentController::ParkedSession> leaving);
    void emitRow(int row);
    DocumentController* m_document = nullptr;
    PageView* m_view = nullptr;
    QString m_position = QStringLiteral("top");
    std::vector<Tab> m_tabs;
    int m_current = 0;
    QMetaObject::Connection m_titleConnection;
    QMetaObject::Connection m_modifiedConnection;
    QMetaObject::Connection m_urlConnection;
};

} // namespace actcraft::ui
