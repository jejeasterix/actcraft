#pragma once
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QString>
#include <QUrl>
#include <QVariantList>

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
    DocumentController* m_document = nullptr;
    PageView* m_view = nullptr;
    QString m_position = QStringLiteral("top");
};

} // namespace actcraft::ui
