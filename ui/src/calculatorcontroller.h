#pragma once

#include <actcraft/features/calculator.h>
#include <actcraft/ui/documentcontroller.h>

#include <QObject>
#include <QPointer>
#include <QString>
#include <QtQml/qqmlregistration.h>

#include <functional>

namespace actcraft::ui {

// Calculatrice (F-286) : l'état de features::Calculator offert à QML. Deux lignes : `expression` (le calcul
// en cours, avec la saisie courante après l'opération en attente : « 12 + 3 ») au-dessus de `display` (le
// résultat ou la saisie). `press` reçoit les touches de features::Calculator ; « C » efface tout le calcul,
// « CE » la dernière saisie. Le réglage « calculatrice » (tools/calculator/mode, tools/calculator/program)
// vaut 0 ActCraft (défaut), 1 calculatrice du système, 2 programme choisi ; les deux derniers n'existent que
// sous Windows, ailleurs `open()` retombe sur ActCraft. Le programme est lancé par un lanceur injectable (les
// tests n'en lancent jamais). `placeOnPage` écrit `pageText()` sur la page affichée par DocumentInsertions,
// en une étape d'annulation ; `placeAt` fait de même à l'endroit où le résultat est lâché.
class CalculatorController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString display READ display NOTIFY calculationChanged)
    Q_PROPERTY(QString expression READ expression NOTIFY calculationChanged)
    Q_PROPERTY(QString pageText READ pageText NOTIFY calculationChanged)
    Q_PROPERTY(int mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(QString programPath READ programPath WRITE setProgramPath NOTIFY programPathChanged)
    Q_PROPERTY(bool systemModesAvailable READ systemModesAvailable CONSTANT)
    Q_PROPERTY(bool canWrite READ canWrite NOTIFY documentControllerChanged)
    Q_PROPERTY(actcraft::ui::DocumentController* documentController READ documentController WRITE
                   setDocumentController NOTIFY documentControllerChanged)

public:
    // Lance un programme détaché ; vrai s'il est parti.
    using Launcher = std::function<bool(const QString& program)>;

    explicit CalculatorController(QObject* parent = nullptr);

    QString display() const;
    QString expression() const;
    QString pageText() const;
    int mode() const noexcept;
    void setMode(int mode);
    QString programPath() const;
    void setProgramPath(const QString& path);
    static bool systemModesAvailable() noexcept;
    bool canWrite() const noexcept;
    DocumentController* documentController() const noexcept;
    void setDocumentController(DocumentController* controller);
    // Remplace le lanceur (vide : retour au lanceur du système).
    void setLauncher(Launcher launcher);

    Q_INVOKABLE void press(const QString& key);
    // Applique le réglage : vrai si le panneau d'ActCraft doit se montrer, faux si un programme a été lancé
    // (ou a échoué à partir : le panneau se montre alors, pour ne pas laisser l'enseignant sans
    // calculatrice).
    Q_INVOKABLE bool open();
    // Écrit le calcul près du centre de la page affichée ; faux sans document.
    Q_INVOKABLE bool placeOnPage();
    // Écrit le calcul centré sur un point de la page affichée (le résultat glissé hors du panneau) ; une étape
    // d'annulation ; faux sans document.
    Q_INVOKABLE bool placeAt(double pageX, double pageY);

signals:
    void calculationChanged();
    void modeChanged();
    void programPathChanged();
    void documentControllerChanged();

private:
    features::Calculator m_calculator;
    Launcher m_launcher;
    int m_mode = 0;
    QString m_programPath;
    bool m_typing = false; // la dernière touche a saisi un nombre
    QPointer<DocumentController> m_document;
};

} // namespace actcraft::ui
