#pragma once

#include <QQuickPaintedItem>
#include <QString>
#include <QtQml/qqmlregistration.h>

#include <string>

class QPainter;

namespace actcraft::ui {

// Aperçu d'une équation (F-301) : le dessin de la page (render::paintEquation, en-tête public de render/) en
// mode Editor, qui ajoute les cases vides et le curseur marqués dans le MathML. L'équation dessinée occupe
// tout l'élément : bornes (0, 0, largeur, hauteur), taille et couleur par défaut de core::Equation ; sans
// marque, l'image est donc celle de la page. Sert à l'éditeur et aux boutons des palettes.
class EquationPreview : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString mathml READ mathml WRITE setMathml NOTIFY mathmlChanged)

public:
    explicit EquationPreview(QQuickItem* parent = nullptr);

    QString mathml() const;
    void setMathml(const QString& mathml);

    void paint(QPainter* painter) override;

signals:
    void mathmlChanged();

private:
    QString m_mathml;
    std::string m_utf8;
};

} // namespace actcraft::ui
