#pragma once

#include "actcraft/ui/profilecontroller.h"

#include <QColor>
#include <QImage>
#include <QObject>
#include <QQuickItem>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Palette de couleurs (F-133 à F-135, F-900) : grille étendue 6 × 6, pastilles affichées (8, 16 ou 24, selon
// le profil), couleurs ajoutées (gardées dans le profil, sans doublon) et échantillonnage du copieur de
// couleur : une seule capture de la fenêtre à l'ouverture, lue ensuite au pixel.
class ColorPalette : public QObject
{
    Q_OBJECT
QML_ELEMENT // instancié par ColorSwatches.qml, ExtendedColors.qml et ColorLoupe.qml
Q_PROPERTY(ProfileController* profile READ profile WRITE setProfile NOTIFY changed)
    Q_PROPERTY(QStringList baseColors MEMBER m_base NOTIFY changed) // liée à commands.colors
    Q_PROPERTY(QStringList swatches READ swatches NOTIFY changed)   // pastilles affichées (8, 16 ou 24)
    Q_PROPERTY(QStringList extended READ extended CONSTANT)         // 36 couleurs « #rrggbb »
    Q_PROPERTY(int extraCount READ extraCount NOTIFY changed) public
    : static constexpr int kDefaultSwatches = 16;

    explicit ColorPalette(QObject* parent = nullptr);
    static QStringList extendedGrid(); // 6 teintes × 6 clartés, ligne par ligne
    ProfileController* profile() const noexcept { return m_profile; }
    void setProfile(ProfileController* profile);
    // Couleurs de base puis ajoutées, au nombre du profil ; trop longue, la liste garde les couleurs ajoutées
    // (les plus récentes) en queue et rogne la fin des couleurs de base.
    QStringList swatches() const;
    QStringList extended() const;
    int extraCount() const;
    Q_INVOKABLE void add(const QString& color); // vers le profil, sans doublon (casse ignorée)
    // Copieur : une capture à l'ouverture, puis lecture au pixel.
    void setSnapshot(QImage image);
    Q_INVOKABLE void beginPick();                // demande la capture à la fenêtre (signal snapshotRequested)
    Q_INVOKABLE void grabFrom(QQuickItem* item); // QQuickWindow::grabWindow() de la fenêtre de l'élément
    Q_INVOKABLE QString colorAt(int x, int y) const; // x, y en pixels de la fenêtre ; vide hors image
    int snapshotCount() const noexcept { return m_snapshots; }
signals:
    void changed();
    void snapshotRequested();

private:
    QStringList m_base;
    ProfileController* m_profile = nullptr;
    QImage m_snapshot;
    int m_snapshots = 0;
};

} // namespace actcraft::ui
