#pragma once

#include <actcraft/input/sample.h>

#include <QString>

namespace actcraft::tools {

// Un outil (DA6) : reçoit les échantillons de son utilisateur, ne voit que son ToolContext.
class Tool : public input::SampleSink
{
public:
    ~Tool() override = default;
    virtual QString id() const = 0;           // « select », « pen », « highlighter », « eraser »
    virtual void activate() {}                // devient l'outil courant
    virtual void deactivate() { cancel(); }   // n'est plus l'outil courant
    virtual bool isBusy() const noexcept = 0; // un geste est en cours
    // Termine le geste en cours proprement (changement de page, Annuler, Échap, changement d'outil) : le
    // stylo verse son trait ; la sélection et la gomme remettent ce qu'elles ont soulevé, sans étape
    // d'annulation.
    virtual void cancel() = 0;
    // Touche (Qt::Key, Qt::KeyboardModifiers) ; vrai si l'outil l'a prise.
    virtual bool onKey(int key, int modifiers)
    {
        static_cast<void>(key);
        static_cast<void>(modifiers);
        return false;
    }
};

} // namespace actcraft::tools
