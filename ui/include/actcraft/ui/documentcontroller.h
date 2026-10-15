#pragma once

#include <actcraft/core/document.h>
#include <actcraft/render/fixedpageitem.h>

#include <QObject>
#include <QPointer>
#include <QString>
#include <QtQml/qqmlregistration.h>

#include <cstddef>

namespace actcraft::ui {

// Document ouvert et page affichée (squelette du lot 1, DA2). Une seule source de vérité à la fois : la page
// affichée vit dans l'élément « page fixe » (le stylo y verse ses traits) ; le document garde les autres
// pages. La page affichée est recopiée dans le document avant tout changement de page et à toute lecture
// (document()). Aucune copie suivie trait par trait : un écrivain qui ne passe pas par appendStroke serait
// perdu. Le lot 2 remplace ce relais par le vrai modèle et sa pile d'annulation.
//
// Conditions d'emploi, que ce relais ne vérifie pas :
// - même élément : `fixedPage` doit être l'élément que le stylo remplit, celui de `PageView.fixedPage`.
//   Les deux liaisons sont indépendantes ; liées à deux éléments différents, les traits iraient dans une
//   page que le document ne lit jamais.
// - durée de vie : la page affichée n'existe que dans l'élément. S'il est détruit avant le contrôleur sans
//   avoir été détaché (setFixedPage(nullptr)), les traits faits depuis le dernier changement de page sont
//   perdus, sans message.
class DocumentController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(
        actcraft::render::FixedPageItem* fixedPage READ fixedPage WRITE setFixedPage NOTIFY fixedPageChanged)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY pagesChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY pagesChanged)
    Q_PROPERTY(QString pageLabel READ pageLabel NOTIFY pagesChanged)

public:
    explicit DocumentController(QObject* parent = nullptr);

    render::FixedPageItem* fixedPage() const noexcept;
    // L'élément reçoit aussitôt la page courante ; l'ancien rend la sienne au document.
    void setFixedPage(render::FixedPageItem* page);
    int pageCount() const noexcept;
    int currentIndex() const noexcept;
    QString pageLabel() const; // « n / N » (F-002)
    // Document à jour de la page affichée. Chaque appel copie la page affichée dans le document : à appeler
    // sur un événement (changement de page, enregistrement), pas par image.
    const core::Document& document();

    // F-125 : sur la dernière page, ajoute une page vierge et l'affiche.
    Q_INVOKABLE void nextPage();
    Q_INVOKABLE void previousPage();
    // Banc et tests : la page 1 devient la page générée de 300 objets (core::generatePage). Ni le nombre de
    // pages ni la page affichée ne changent : pas de pagesChanged.
    Q_INVOKABLE void loadGeneratedFirstPage();

signals:
    void fixedPageChanged();
    // Juste avant un déplacement (page ajoutée comprise), avant la recopie de la page affichée dans le
    // document : la vue y termine le trait en cours, qui reste ainsi sur la page où il a été commencé.
    void pageAboutToChange();
    // Une fois par déplacement (page ajoutée comprise), quand l'élément montre déjà la nouvelle page.
    void pagesChanged();

private:
    // Recopie la page affichée (celle de l'élément) dans le document.
    void syncFromView();
    // Affiche la page `index` ; exige un document déjà à jour de la page quittée.
    void show(std::size_t index);

    core::Document m_document;
    std::size_t m_current = 0;
    QPointer<render::FixedPageItem> m_fixed;
};

} // namespace actcraft::ui
