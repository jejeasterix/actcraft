#pragma once

#include <actcraft/features/dice.h>
#include <actcraft/ui/documentcontroller.h>

#include <QObject>
#include <QPointer>
#include <QRandomGenerator>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <vector>

namespace actcraft::ui {

// Dés (F-285) : l'état de features::Dice offert à QML. Le nombre de dés (1 au premier lancement) et la
// vitesse (3 à 11 s, 7 par défaut) sont mémorisés dans les réglages (tools/dice/count, tools/dice/speed),
// réécrits à chaque changement. Le lancer tire ses valeurs finales tout de suite ; pendant la durée choisie,
// les faces affichées sont tirées à part (animation à 100 ms), puis remplacées par les valeurs finales. Le
// hasard est injectable : la propriété `seed` fixe celui des valeurs finales. Le total s'écrit sur la page
// affichée par DocumentInsertions::insertText, en une étape d'annulation.
class DiceController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)
    Q_PROPERTY(int speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(int minCount READ minCount CONSTANT)
    Q_PROPERTY(int maxCount READ maxCount CONSTANT)
    Q_PROPERTY(int minSpeed READ minSpeed CONSTANT)
    Q_PROPERTY(int maxSpeed READ maxSpeed CONSTANT)
    // Faces affichées (entiers de 1 à 6) : celles de l'animation pendant le lancer, les valeurs finales
    // après.
    Q_PROPERTY(QVariantList faces READ faces NOTIFY facesChanged)
    // Somme du dernier lancer terminé ; 0 pendant un lancer et avant le premier.
    Q_PROPERTY(int total READ total NOTIFY facesChanged)
    Q_PROPERTY(bool rolling READ rolling NOTIFY rollingChanged)
    Q_PROPERTY(bool canWrite READ canWrite NOTIFY canWriteChanged)
    Q_PROPERTY(uint seed READ seed WRITE setSeed NOTIFY seedChanged)
    Q_PROPERTY(actcraft::ui::DocumentController* documentController READ documentController WRITE
                   setDocumentController NOTIFY documentControllerChanged)

public:
    explicit DiceController(QObject* parent = nullptr);

    int count() const noexcept;
    void setCount(int count);
    int speed() const noexcept;
    void setSpeed(int seconds);
    static int minCount() noexcept;
    static int maxCount() noexcept;
    static int minSpeed() noexcept;
    static int maxSpeed() noexcept;
    QVariantList faces() const;
    int total() const noexcept;
    bool rolling() const noexcept;
    bool canWrite() const noexcept;
    quint32 seed() const noexcept;
    void setSeed(quint32 seed);
    DocumentController* documentController() const noexcept;
    void setDocumentController(DocumentController* controller);

    // Lance les dés (sans effet pendant un lancer).
    Q_INVOKABLE void roll();
    // Termine l'animation tout de suite : les valeurs finales s'affichent.
    Q_INVOKABLE void skip();
    // Écrit le total du dernier lancer terminé près du centre de la page affichée ; faux sans lancer terminé
    // ou sans document.
    Q_INVOKABLE bool writeTotal();

signals:
    void countChanged();
    void speedChanged();
    void facesChanged();
    void rollingChanged();
    void canWriteChanged();
    void seedChanged();
    void documentControllerChanged();

private:
    void tick();
    void finish();
    void shuffle();
    void refreshCanWrite();

    quint32 m_seed;
    features::Dice m_dice;
    int m_count = features::Dice::kDefaultCount;
    int m_speed = features::Dice::kDefaultSeconds;
    std::vector<int> m_final;
    std::vector<int> m_shown;
    QRandomGenerator m_noise; // faces de l'animation, jamais celles du résultat
    QTimer m_ticker;
    int m_remainingMs = 0;
    bool m_rolling = false;
    bool m_canWrite = false;
    QPointer<DocumentController> m_document;
};

} // namespace actcraft::ui
