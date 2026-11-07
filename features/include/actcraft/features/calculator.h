#pragma once

#include <QChar>
#include <QString>

namespace actcraft::features {

// Calculatrice de poche (F-286) : calcul immédiat de gauche à droite, virgule française, 12 chiffres
// significatifs. Touches : "0"…"9", ",", "+", "-", "*", "/", "=", "%", "±", "C" (tout le calcul), "CE"
// (dernière saisie), "M+", "M-", "MR", "MC". Principes P1 à P3 de L7-T0
// (docs/verifications/2026-10-18-lot7-principes.md).
class Calculator
{
public:
    void press(const QString& key);
    QString display() const; // « Erreur » après une division par zéro ou un résultat non fini, jusqu'à « C »
    QString
    expression() const; // calcul en cours, montré au-dessus du résultat (P1) : « 12 + 3 × », « 2 + 3 = »
    QString
    pageText() const; // texte posé sur la page (P3) : « 1,5 + 1 = 2,5 » après « = », sinon l'affichage

private:
    double entry() const;
    void showValue(double value);
    void apply(); // m_accumulator = m_accumulator <m_pending> entry()

    QString m_expression; // jetons séparés par une espace ; opérateurs affichés + − × ÷
    bool m_done = false;  // vrai juste après « = » ou « % »
    QString m_display = QStringLiteral("0");
    double m_accumulator = 0;
    QChar m_pending;        // nul : aucune opération en attente
    bool m_newEntry = true; // vrai : le prochain chiffre remplace l'affichage
    bool m_error = false;
    bool m_recalled = false; // vrai juste après « MR » : l'affichage est une valeur saisie
    double m_memory = 0;
};

} // namespace actcraft::features
