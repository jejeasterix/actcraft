#pragma once

#include <QObject>
#include <QPointF>
#include <QString>
#include <QStringList>
#include <QThread>

#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace actcraft::features {

using InkStrokes = std::vector<std::vector<QPointF>>; // traits en pixels de page

// Reconnaisseur d'écriture (F-295). Créé, utilisé et détruit sur un seul fil (celui de HandwritingThread).
class HandwritingRecognizer
{
public:
    virtual ~HandwritingRecognizer() = default;
    virtual bool available() const = 0;
    virtual QString status() const = 0; // explication si indisponible ; nom du reconnaisseur sinon
    // Meilleur texte puis 10 alternatives au plus, sans doublon ; vide si rien n'est reconnu.
    virtual QStringList recognize(const InkStrokes& strokes) = 0;
};

// Réalisation de la plateforme : handwriting_win.cpp (Windows) ou handwriting_other.cpp (nullptr).
std::unique_ptr<HandwritingRecognizer> makePlatformHandwritingRecognizer();
// Jamais nul : la réalisation de la plateforme, sinon « indisponible » avec son explication.
std::unique_ptr<HandwritingRecognizer> makeHandwritingRecognizer();
// Appartement COM du fil courant : sous Windows, initialisation de COM en appartement unique ; ailleurs,
// rien. Vrai si leaveRecognitionApartment() doit être appelé sur ce même fil.
bool enterRecognitionApartment();
void leaveRecognitionApartment();

struct RecognitionReply
{
    bool available = false;
    QString status;
    QStringList texts; // vide si indisponible ou pour probe()
};

// Fil dédié de la reconnaissance (relecture I7) : créé une fois, gardé ; run() entre dans l'appartement, crée
// le reconnaisseur, sert les demandes, puis le détruit et sort de l'appartement, sur ce seul fil. Aucune
// attente du fil appelant hors du destructeur. Le receveur d'une réponse doit vivre plus longtemps que ce fil
// (le contrôleur de la phase 2 possède le fil et reçoit ses réponses).
class HandwritingThread final : public QThread
{
public:
    using Factory = std::function<std::unique_ptr<HandwritingRecognizer>()>;
    using Callback = std::function<void(const RecognitionReply&)>;

    explicit HandwritingThread(Factory factory = &makeHandwritingRecognizer);
    ~HandwritingThread() override; // quit() puis wait() ; les demandes encore en file sont abandonnées
    HandwritingThread(const HandwritingThread&) = delete;
    HandwritingThread& operator=(const HandwritingThread&) = delete;

    // Non bloquants ; `done` est appelé sur le fil de `receiver` (en file).
    void recognize(InkStrokes strokes, QObject* receiver, Callback done);
    void probe(QObject* receiver, Callback done);

protected:
    void run() override;

private:
    void post(QObject* receiver, Callback done, std::optional<InkStrokes> strokes);

    Factory m_factory;
    QObject* m_context; // vit sur ce fil (moveToThread avant start) ; détruit à la fin de run()
    std::unique_ptr<HandwritingRecognizer> m_recognizer; // touché seulement par ce fil
};

} // namespace actcraft::features
