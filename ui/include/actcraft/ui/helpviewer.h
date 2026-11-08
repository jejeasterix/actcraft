#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Modèle du lecteur d'aide hors ligne (D6-10) : pages embarquées (qrc:/actcraft/help/), liens internes
// suivis, liens externes remis au QML (qui les ouvre par le navigateur du système). Aucun chargement réseau.
class HelpViewer : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(HelpViewerModel) // QML_ELEMENT heurterait HelpViewer.qml dans ActCraft.Ui
    Q_PROPERTY(QUrl source READ source NOTIFY sourceChanged FINAL)
    Q_PROPERTY(QString html READ html NOTIFY htmlChanged FINAL)

public:
    explicit HelpViewer(QObject* parent = nullptr);

    QUrl source() const { return m_source; }
    QString html() const { return m_html; }

    // Page de l'outil (« pen » ou « tool.pen ») si elle existe, sinon l'index.
    Q_INVOKABLE void openFor(QString toolId);
    // L11-C11 (K) : page (nom sans extension ; vide : aucune) que la prochaine ouverture montre à la place de
    // celle de l'outil, puis oubliée. Partagée par toutes les instances : la deuxième palette n'atteint pas
    // le lecteur de Main.qml, elle pose la demande puis déclenche help.contents.
    Q_INVOKABLE void requestNextPage(const QString& name);
    // Lien interne (qrc ou relatif à la page courante) : change de page ; sinon : signal externalLink.
    Q_INVOKABLE void follow(QUrl link);

signals:
    void sourceChanged();
    void htmlChanged();
    void externalLink(const QUrl& url);

private:
    void show(const QUrl& url);

    QUrl m_source;
    QString m_html;
};

} // namespace actcraft::ui
