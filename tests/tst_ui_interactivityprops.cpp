// L8-T17 : sections de propriétés de l'interactivité (F-577 à F-579, F-582) dans le navigateur de propriétés.
//
// Invariants gardés (ce qui distingue le bon patron du mauvais) :
// - « Mots acceptés » lit et écrit ContainerRule::keywords, la règle que lit core::resolveDrop (H8-12),
// jamais
//   ObjectCommon::keywords du conteneur : keywordsFromLot6 pose deux listes différentes, regarde laquelle
//   bouge et joue le dépôt d'un candidat.
// - Une édition de mode passe par la forme normalisée (core::setTravel, core::normalize) : les booléens du
//   lot 2 priment. Un objet bloqué au lot 2 (movable faux) dont le mode resterait « librement » se lit « non
//   » ; le passer à « verticalement » rend movable vrai. Une écriture brute du mode serait ramenée à « non »
//   par normalize, ou laisserait l'objet bloqué : editIsOneStep et fixedTravelGreysTheTrack le distinguent.
// - Aucune édition de l'interactivité ne redessine le cache : un PageSettingsChanged par objet modifié,
// jamais
//   ObjectChanged (patron de propertyedits.cpp, changeFor ; SetObjectActionsCommand).
// - Une modification sur plusieurs objets est une seule étape : un undo les rétablit tous.
#include <actcraft/actions/objectactions.h>
#include <actcraft/core/containers.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/interactivity.h>
#include <actcraft/core/model.h>
#include <actcraft/core/restrictors.h>
#include <actcraft/core/shapegeometry.h>
#include <actcraft/core/undo.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/interactivityprops.h>
#include <actcraft/ui/propertymodel.h>
#include <actcraft/ui/toolhost.h>

#include <QCoreApplication>
#include <QPointF>
#include <QStringList>
#include <QTemporaryDir>
#include <QtQml/qqmlextensionplugin.h>
#include <QtTest>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

namespace core = actcraft::core;
namespace ui = actcraft::ui;

namespace {

std::vector<core::ObjectId> idsOnPage(const ui::DocumentController& controller)
{
    std::vector<core::ObjectId> ids;
    for (const core::PageObject& object : controller.document().page(0).objects) {
        ids.push_back(object.common.id);
    }
    return ids;
}

// Ajoute `object` à la page 1 par la pile ; rend son identifiant.
core::ObjectId addObject(ui::DocumentController& controller, core::PageObject object)
{
    const std::vector<core::ObjectId> before = idsOnPage(controller);
    controller.execute(std::make_unique<core::AddObjectCommand>(0, std::move(object)));
    for (const core::ObjectId id : idsOnPage(controller)) {
        if (std::find(before.begin(), before.end(), id) == before.end()) {
            return id;
        }
    }
    return 0;
}

core::PageObject shapeAt(double x)
{
    return core::makeShapeObject("rectangle", core::RectF{x, 100, 80, 60}, core::ShapeStyle{});
}

core::PageObject mediaObject()
{
    core::Media media;
    media.bounds = core::RectF{300, 300, 160, 120};
    media.media = "son";
    return core::PageObject{media};
}

core::PageObject strokeObject()
{
    core::Stroke stroke;
    stroke.points = {{10, 10}, {60, 40}, {120, 30}};
    return core::PageObject{stroke};
}

const core::PageObject* objectOf(const ui::DocumentController& controller, core::ObjectId id)
{
    for (const core::PageObject& object : controller.document().page(0).objects) {
        if (object.common.id == id) {
            return &object;
        }
    }
    return nullptr;
}

const core::Restrictions& restrictionsOf(const ui::DocumentController& controller, core::ObjectId id)
{
    return objectOf(controller, id)->common.restrictions;
}

QVariant roleOf(const ui::PropertyModel& model, const QString& key, int role)
{
    const int row = model.rowOf(key);
    return row < 0 ? QVariant() : model.data(model.index(row), role);
}

QVariant valueOf(const ui::PropertyModel& model, const QString& key)
{
    return roleOf(model, key, ui::PropertyModel::ValueRole);
}

QStringList asList(const std::vector<std::string>& values)
{
    QStringList list;
    for (const std::string& value : values) {
        list.append(QString::fromStdString(value));
    }
    return list;
}

// Document, outils, commandes et modèle reliés comme dans Main.qml.
struct Fixture
{
    QTemporaryDir dir;
    ui::DocumentController controller;
    ui::Commands commands;
    ui::ToolHost tools;
    ui::PropertyModel model;
    std::vector<core::Change> seen; // changements observés depuis le dernier clear()

    Fixture()
    {
        controller.newDocument(800, 600);
        commands.setProfileDir(dir.filePath(QStringLiteral("disposition")));
        tools.setSettingsFile(dir.filePath(QStringLiteral("outils.ini")));
        tools.setCommands(&commands);
        tools.setDocumentController(&controller);
        model.setCommands(&commands);
        model.setDocument(&controller);
        model.setTools(&tools);
        QCoreApplication::processEvents();
        controller.addChangeObserver(
            [this](const core::Changes& changes, ui::DocumentController::ChangeOrigin) {
                for (const core::Change& change : changes) {
                    seen.push_back(change);
                }
            });
    }

    void select(const std::vector<core::ObjectId>& ids)
    {
        tools.selectObjects(ids);
        settle();
    }

    void settle()
    {
        // La remise à jour du modèle est différée d'un tour de boucle.
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();
    }

    bool set(const QString& key, const QVariant& value)
    {
        seen.clear();
        const bool applied = model.setValue(model.rowOf(key), value);
        settle();
        return applied;
    }

    // Aucun changement vu ne redessine : seulement des PageSettingsChanged.
    bool noRedraw() const
    {
        return std::all_of(seen.begin(), seen.end(), [](const core::Change& change) {
            return change.kind == core::Change::Kind::PageSettingsChanged;
        });
    }
};

const ui::InteractivityRow* rowNamed(std::string_view key)
{
    for (const ui::InteractivityRow& row : ui::interactivityRows()) {
        if (key == row.key) {
            return &row;
        }
    }
    return nullptr;
}

bool near(core::PointF a, core::PointF b)
{
    return std::abs(a.x - b.x) < 1e-6 && std::abs(a.y - b.y) < 1e-6;
}

QString text(core::PointF p)
{
    return QStringLiteral("(%1, %2)").arg(p.x).arg(p.y);
}

constexpr int kVertical = static_cast<int>(core::Travel::Vertical);
constexpr int kAlongTrack = static_cast<int>(core::Travel::AlongTrack);
constexpr int kFixed = static_cast<int>(core::Travel::Fixed);
constexpr int kPointMark = static_cast<int>(core::Mark::Point);

} // namespace

class TstUiInteractivityprops : public QObject
{
    Q_OBJECT
private slots:
    void fiveSectionsInOrder();
    void multimediaOnlyForMedia();
    void editIsOneStep();
    void fixedTravelGreysTheTrack();
    void keywordsFromLot6();
    void refusedWhileLocked();
    void resizableInRestrictors();
    void pointFollowsMark();
    void actionsShowCatalogLabels();
    void commandRefusals();
};

// Un objet média sélectionné : les cinq sections de l'interactivité suivent celles du lot 6, dans l'ordre.
void TstUiInteractivityprops::fiveSectionsInOrder()
{
    Fixture f;
    const core::ObjectId media = addObject(f.controller, mediaObject());
    QVERIFY(media != 0);
    f.select({media});

    const QStringList sections = ui::sections(f.model);
    QVERIFY2(sections.size() > 5, qPrintable(sections.join(QLatin1Char(','))));
    QCOMPARE(sections.first(), QStringLiteral("identification")); // le lot 6 d'abord
    const QStringList tail = sections.mid(sections.size() - 5);
    const QStringList expected = {QStringLiteral("actions"), QStringLiteral("container"),
                                  QStringLiteral("rotation"), QStringLiteral("restrictors"),
                                  QStringLiteral("multimedia")};
    QCOMPARE(tail, expected);

    // Éditeurs décrits : modes en « choice », objet choisi sur la page en « object », liste d'actions en
    // lecture.
    QCOMPARE(roleOf(f.model, QStringLiteral("travel"), ui::PropertyModel::EditorRole).toString(),
             QStringLiteral("choice"));
    QCOMPARE(roleOf(f.model, QStringLiteral("turning"), ui::PropertyModel::EditorRole).toString(),
             QStringLiteral("choice"));
    QCOMPARE(roleOf(f.model, QStringLiteral("track"), ui::PropertyModel::EditorRole).toString(),
             QStringLiteral("object"));
    QCOMPARE(roleOf(f.model, QStringLiteral("loop"), ui::PropertyModel::EditorRole).toString(),
             QStringLiteral("bool"));
    QCOMPARE(roleOf(f.model, QStringLiteral("actions"), ui::PropertyModel::EditorRole).toString(),
             QStringLiteral("actions"));
    QVERIFY(!f.set(QStringLiteral("actions"), QStringList{QStringLiteral("x")})); // lecture seule

    // Une option du média s'écrit sur le média, sans redessin.
    QVERIFY(f.set(QStringLiteral("loop"), true));
    QVERIFY(std::get<core::Media>(*objectOf(f.controller, media)).loop);
    QVERIFY(!f.seen.empty());
    QVERIFY(f.noRedraw());
}

// Une forme, ou une forme et un média ensemble : pas de section « multimedia » ; les quatre autres restent.
void TstUiInteractivityprops::multimediaOnlyForMedia()
{
    Fixture f;
    const core::ObjectId shape = addObject(f.controller, shapeAt(100));
    const core::ObjectId media = addObject(f.controller, mediaObject());
    f.select({shape});

    QStringList sections = ui::sections(f.model);
    QVERIFY(!sections.contains(QStringLiteral("multimedia")));
    const QStringList expected = {QStringLiteral("actions"), QStringLiteral("container"),
                                  QStringLiteral("rotation"), QStringLiteral("restrictors")};
    QCOMPARE(sections.mid(sections.size() - 4), expected);
    QCOMPARE(f.model.rowOf(QStringLiteral("loop")), -1);

    f.select({shape, media});
    sections = ui::sections(f.model);
    QVERIFY(!sections.contains(QStringLiteral("multimedia")));
    QCOMPARE(sections.mid(sections.size() - 4), expected);
}

// « Peut se déplacer » → « verticalement » sur deux objets, dont un bloqué au lot 2 : une étape, un undo.
void TstUiInteractivityprops::editIsOneStep()
{
    Fixture f;
    const core::ObjectId free = addObject(f.controller, shapeAt(100));
    core::PageObject lockedShape = shapeAt(300);
    lockedShape.common.restrictions.movable = false; // bloqué au lot 2, mode resté « librement »
    const core::ObjectId blocked = addObject(f.controller, lockedShape);
    f.select({free, blocked});

    // Lu par la forme normalisée : « librement » et « non » diffèrent (une lecture brute dirait « librement
    // »).
    QVERIFY(!valueOf(f.model, QStringLiteral("travel")).isValid());

    QVERIFY(f.set(QStringLiteral("travel"), kVertical));
    QCOMPARE(restrictionsOf(f.controller, free).travel, core::Travel::Vertical);
    QCOMPARE(restrictionsOf(f.controller, blocked).travel, core::Travel::Vertical);
    QVERIFY(restrictionsOf(f.controller, free).movable);
    QVERIFY(restrictionsOf(f.controller, blocked).movable); // le booléen suit le mode
    QCOMPARE(valueOf(f.model, QStringLiteral("travel")).toInt(), kVertical);
    QCOMPARE(f.seen.size(), std::size_t{2}); // un changement par objet
    QVERIFY(f.noRedraw());

    // Un seul undo rétablit les deux objets ; les ajouts restent.
    f.seen.clear();
    f.controller.undo();
    f.settle();
    QVERIFY(objectOf(f.controller, free) != nullptr);
    QVERIFY(objectOf(f.controller, blocked) != nullptr);
    QCOMPARE(restrictionsOf(f.controller, free).travel, core::Travel::Free);
    QVERIFY(restrictionsOf(f.controller, free).movable);
    QVERIFY(!restrictionsOf(f.controller, blocked).movable);
    QCOMPARE(f.seen.size(), std::size_t{2}); // un changement rétabli par objet
    QVERIFY(f.noRedraw());
}

// Mode « non » : l'éditeur du chemin est grisé ; « le long d'un chemin » : il ne l'est pas.
void TstUiInteractivityprops::fixedTravelGreysTheTrack()
{
    Fixture f;
    const core::ObjectId track = addObject(f.controller, strokeObject());
    core::PageObject follower = shapeAt(100);
    core::setTravel(follower.common.restrictions, core::Travel::AlongTrack);
    follower.common.restrictions.track = track;
    const core::ObjectId along = addObject(f.controller, follower);
    f.select({along});

    QCOMPARE(valueOf(f.model, QStringLiteral("travel")).toInt(), kAlongTrack);
    QVERIFY(roleOf(f.model, QStringLiteral("track"), ui::kEnabledRole).toBool());
    QCOMPARE(valueOf(f.model, QStringLiteral("track")).toULongLong(), static_cast<qulonglong>(track));

    QVERIFY(f.set(QStringLiteral("travel"), kFixed));
    QCOMPARE(restrictionsOf(f.controller, along).travel, core::Travel::Fixed);
    QVERIFY(!restrictionsOf(f.controller, along).movable); // « non » bloque, comme au lot 2
    QVERIFY(!roleOf(f.model, QStringLiteral("track"), ui::kEnabledRole).toBool());
    QVERIFY(f.noRedraw());

    // Bloqué au lot 2 sur un mode « le long d'un chemin » : le booléen prime, l'objet se lit « non », grisé.
    core::PageObject stale = shapeAt(400);
    stale.common.restrictions.travel = core::Travel::AlongTrack;
    stale.common.restrictions.track = track;
    stale.common.restrictions.movable = false;
    const core::ObjectId blocked = addObject(f.controller, stale);
    f.select({blocked});
    QCOMPARE(valueOf(f.model, QStringLiteral("travel")).toInt(), kFixed);
    QVERIFY(!roleOf(f.model, QStringLiteral("track"), ui::kEnabledRole).toBool());
    // Les lignes du lot 6 ne sont jamais grisées.
    QVERIFY(roleOf(f.model, QStringLiteral("name"), ui::kEnabledRole).toBool());
}

// « Mots acceptés » (H8-12) : la règle du conteneur, ContainerRule::keywords ; les mots-clés du lot 6
// (ObjectCommon::keywords) décrivent l'objet et servent côté candidat. L'éditeur écrit la règle, n'altère pas
// les mots-clés du conteneur, et core::resolveDrop juge contenu un candidat dont les mots-clés portent un mot
// accepté.
void TstUiInteractivityprops::keywordsFromLot6()
{
    Fixture f;
    core::PageObject box = shapeAt(100); // 100, 100, 80 × 60 : centre (140, 130)
    box.common.keywords = {"boîte"};
    box.common.container.accepts = core::Accepts::Keywords;
    box.common.container.keywords = {"chat"};
    const core::ObjectId container = addObject(f.controller, box);
    core::PageObject fruit =
        core::makeShapeObject("rectangle", core::RectF{120, 110, 40, 40}, core::ShapeStyle{}); // même centre
    fruit.common.keywords = {"pomme"};
    const core::ObjectId candidate = addObject(f.controller, fruit);
    f.select({container});

    QCOMPARE(roleOf(f.model, QStringLiteral("contain-words"), ui::PropertyModel::EditorRole).toString(),
             QStringLiteral("keywords"));
    QCOMPARE(valueOf(f.model, QStringLiteral("contain-words")).toStringList(),
             QStringList{QStringLiteral("chat")});
    QVERIFY(roleOf(f.model, QStringLiteral("contain-words"), ui::kEnabledRole).toBool());
    QCOMPARE(core::resolveDrop(f.controller.document().page(0), {candidate}).outcome,
             core::DropOutcome::Free);

    QVERIFY(f.set(QStringLiteral("contain-words"), QStringLiteral("lion pomme")));
    const core::ObjectCommon& common = objectOf(f.controller, container)->common;
    const QStringList written = {QStringLiteral("lion"), QStringLiteral("pomme")};
    QCOMPARE(asList(common.container.keywords), written);
    QCOMPARE(asList(common.keywords), QStringList{QStringLiteral("boîte")}); // mots-clés du conteneur intacts
    QVERIFY(f.noRedraw());
    QCOMPARE(valueOf(f.model, QStringLiteral("contain-words")).toStringList(), written);
    QCOMPARE(valueOf(f.model, QStringLiteral("keywords")).toStringList(),
             QStringList{QStringLiteral("boîte")});
    const core::DropResult drop = core::resolveDrop(f.controller.document().page(0), {candidate});
    QCOMPARE(drop.outcome, core::DropOutcome::Contained);
    QCOMPARE(drop.container, container);

    // Accepte autre chose que des mots : la ligne est grisée.
    QVERIFY(f.set(QStringLiteral("accepts"), static_cast<int>(core::Accepts::Anything)));
    QCOMPARE(objectOf(f.controller, container)->common.container.accepts, core::Accepts::Anything);
    QVERIFY(!roleOf(f.model, QStringLiteral("contain-words"), ui::kEnabledRole).toBool());
}

// Sous le verrou de classe, ou sans commandes : rien n'est écrit, aucune étape.
void TstUiInteractivityprops::refusedWhileLocked()
{
    Fixture f;
    const core::ObjectId shape = addObject(f.controller, shapeAt(100));
    f.select({shape});

    f.commands.setLocked(true);
    QVERIFY(!f.set(QStringLiteral("travel"), kVertical));
    QVERIFY(!f.set(QStringLiteral("bounce-back"), true));
    QCOMPARE(restrictionsOf(f.controller, shape).travel, core::Travel::Free);
    QVERIFY(!objectOf(f.controller, shape)->common.container.bounceBack);
    QVERIFY(f.seen.empty());

    f.commands.setLocked(false);
    f.model.setCommands(nullptr);
    QVERIFY(!f.set(QStringLiteral("travel"), kVertical));
    QCOMPARE(restrictionsOf(f.controller, shape).travel, core::Travel::Free);

    // Témoin : avec les commandes, sans verrou, la même saisie passe.
    f.model.setCommands(&f.commands);
    QVERIFY(f.set(QStringLiteral("travel"), kVertical));
    QCOMPARE(restrictionsOf(f.controller, shape).travel, core::Travel::Vertical);
}

// F-579, E6 : « Taille » dans Limiteurs, en choix « Libre » / « Bloquée », écrit Restrictions::resizable
// (celui que lit core::sizeHandlesShown) ; une étape, sans redessin.
void TstUiInteractivityprops::resizableInRestrictors()
{
    Fixture f;
    const core::ObjectId shape = addObject(f.controller, shapeAt(100));
    f.select({shape});

    const QString key = QStringLiteral("resizable");
    QCOMPARE(roleOf(f.model, key, ui::PropertyModel::SectionRole).toString(), QStringLiteral("restrictors"));
    QCOMPARE(roleOf(f.model, key, ui::PropertyModel::EditorRole).toString(), QStringLiteral("choice"));
    QCOMPARE(roleOf(f.model, key, ui::PropertyModel::ChoicesRole).toStringList(),
             (QStringList{QStringLiteral("Libre"), QStringLiteral("Bloquée")}));
    QVERIFY(roleOf(f.model, key, ui::kEnabledRole).toBool());
    QCOMPARE(valueOf(f.model, key).toInt(), 0);

    QVERIFY(f.set(key, 1));
    QVERIFY(!restrictionsOf(f.controller, shape).resizable);
    QVERIFY(!core::sizeHandlesShown(*objectOf(f.controller, shape)));
    QCOMPARE(f.seen.size(), std::size_t{1});
    QVERIFY(f.noRedraw());
    QCOMPARE(valueOf(f.model, key).toInt(), 1);

    f.seen.clear();
    f.controller.undo();
    f.settle();
    QVERIFY(restrictionsOf(f.controller, shape).resizable);
    QCOMPARE(valueOf(f.model, key).toInt(), 0);
    QVERIFY(!f.set(key, 2)); // choix inconnu
}

// Point choisi : le pivot est en coordonnées de l'objet (restrictors.cpp, transform.map), le repère de pose
// en coordonnées de page (landingPoint : le centre s'y pose). L'éditeur « point » n'est actif que sur « Point
// choisi » ; passer à « Point choisi » pose le centre de l'objet dans la convention du champ. Un point laissé
// à (0, 0) poserait l'objet dans le coin de la page ; un pivot écrit en coordonnées de page serait décalé par
// la transformation de l'objet.
void TstUiInteractivityprops::pointFollowsMark()
{
    Fixture f;
    const core::ObjectId shape = addObject(f.controller, shapeAt(100));
    f.select({shape});
    QVERIFY(f.set(QStringLiteral("angle"), 30)); // repère de l'objet distinct de celui de la page
    const core::PointF center =
        core::boundsOnPage(f.controller.document().page(0), *objectOf(f.controller, shape)).center();

    const QString pivotPoint = QStringLiteral("pivot-point");
    const QString magnetPoint = QStringLiteral("magnet-point");
    QCOMPARE(roleOf(f.model, pivotPoint, ui::PropertyModel::EditorRole).toString(), QStringLiteral("point"));
    QCOMPARE(roleOf(f.model, magnetPoint, ui::PropertyModel::EditorRole).toString(), QStringLiteral("point"));
    QCOMPARE(roleOf(f.model, pivotPoint, ui::PropertyModel::SectionRole).toString(),
             QStringLiteral("rotation"));
    QCOMPARE(roleOf(f.model, magnetPoint, ui::PropertyModel::SectionRole).toString(),
             QStringLiteral("restrictors"));
    // Les libellés disent la convention.
    QVERIFY(roleOf(f.model, pivotPoint, ui::PropertyModel::LabelRole)
                .toString()
                .contains(QStringLiteral("objet")));
    QVERIFY(roleOf(f.model, magnetPoint, ui::PropertyModel::LabelRole)
                .toString()
                .contains(QStringLiteral("page")));
    QVERIFY(!roleOf(f.model, pivotPoint, ui::kEnabledRole).toBool());  // pivot au centre
    QVERIFY(!roleOf(f.model, magnetPoint, ui::kEnabledRole).toBool()); // pas d'aimant

    // Pivot : « Point choisi » sans saisie, le pivot reste au centre de l'objet sur la page.
    QVERIFY(f.set(QStringLiteral("pivot"), kPointMark));
    QVERIFY(roleOf(f.model, pivotPoint, ui::kEnabledRole).toBool());
    {
        const core::PageObject& turned = *objectOf(f.controller, shape);
        const core::PointF onPage = turned.common.transform.map(turned.common.restrictions.pivotPoint);
        QVERIFY2(near(onPage, center), qPrintable(text(onPage) + QStringLiteral(" ≠ ") + text(center)));
        const core::PointF stored = turned.common.restrictions.pivotPoint;
        QCOMPARE(valueOf(f.model, pivotPoint).toPointF(), QPointF(stored.x, stored.y));
    }
    QVERIFY(f.noRedraw());
    QVERIFY(f.set(pivotPoint, QPointF(5, 6))); // saisie : écrite telle quelle, coordonnées de l'objet
    QVERIFY(near(restrictionsOf(f.controller, shape).pivotPoint, {5, 6}));
    QVERIFY(f.set(QStringLiteral("pivot"), static_cast<int>(core::Mark::TopLeft)));
    QVERIFY(!roleOf(f.model, pivotPoint, ui::kEnabledRole).toBool());

    // Repère de pose : « Point choisi » sans saisie, l'objet se pose sur son propre centre, jamais en (0, 0).
    QVERIFY(f.set(QStringLiteral("magnet"), true));
    QVERIFY(!roleOf(f.model, magnetPoint, ui::kEnabledRole).toBool()); // repère : centre
    QVERIFY(f.set(QStringLiteral("magnet-mark"), kPointMark));
    QVERIFY(roleOf(f.model, magnetPoint, ui::kEnabledRole).toBool());
    const core::PointF landed =
        core::landingPoint(f.controller.document().page(0), *objectOf(f.controller, shape), {500, 500});
    QVERIFY2(near(landed, center), qPrintable(text(landed) + QStringLiteral(" ≠ ") + text(center)));
    QCOMPARE(valueOf(f.model, magnetPoint).toPointF(), QPointF(center.x, center.y));
    QVERIFY(f.set(magnetPoint, QPointF(400, 300))); // coordonnées de page
    QVERIFY(
        near(core::landingPoint(f.controller.document().page(0), *objectOf(f.controller, shape), {500, 500}),
             {400, 300}));
    QVERIFY(!f.set(magnetPoint, QStringLiteral("ailleurs"))); // valeur illisible
    QVERIFY(f.noRedraw());
}

// Actions : la liste montre le libellé du catalogue (ObjectActionCatalog), jamais l'identifiant à nous.
void TstUiInteractivityprops::actionsShowCatalogLabels()
{
    namespace actions = actcraft::actions;
    actions::ObjectActionCatalog catalog;
    QString error;
    QVERIFY2(catalog.load(actions::defaultObjectActionsJson(), error), qPrintable(error));
    const QList<const actions::ObjectActionInfo*> pages = catalog.inCategory(actions::ActionCategory::Page);
    QVERIFY(!pages.isEmpty());
    const actions::ObjectActionInfo& info = *pages.first();
    QVERIFY(info.label != info.id); // sinon ce test ne distingue rien

    Fixture f;
    core::PageObject shape = shapeAt(100);
    core::Action known;
    known.catalogId = info.id.toStdString();
    const core::Action unsupported; // lue d'un fichier, hors du catalogue
    shape.common.actions = {known, unsupported};
    const core::ObjectId id = addObject(f.controller, shape);
    f.select({id});

    const QStringList expected = {info.label, QStringLiteral("Action non prise en charge")};
    QCOMPARE(valueOf(f.model, QStringLiteral("actions")).toStringList(), expected);
}

// Refus d'interactivityCommand, chacun avec son témoin accepté : nombre hors bornes (refusé, jamais écrêté),
// objet visé parmi la sélection, chemin qui n'est ni un trait ni une ligne.
void TstUiInteractivityprops::commandRefusals()
{
    Fixture f;
    const core::ObjectId track = addObject(f.controller, strokeObject());
    const core::ObjectId shape = addObject(f.controller, shapeAt(100));
    const core::ObjectId other = addObject(f.controller, shapeAt(300));
    const core::Page& page = f.controller.document().page(0);
    const ui::InteractivityRow* tolerance = rowNamed("tolerance");
    const ui::InteractivityRow* path = rowNamed("track");
    QVERIFY(tolerance != nullptr && path != nullptr);

    QVERIFY(ui::interactivityCommand(page, 0, {shape}, *tolerance, QVariant(1001.0)) == nullptr);
    QVERIFY(ui::interactivityCommand(page, 0, {shape}, *tolerance, QVariant(1000.0)) != nullptr);
    QVERIFY(ui::interactivityCommand(page, 0, {shape}, *path, QVariant(qulonglong{shape})) == nullptr);
    QVERIFY(ui::interactivityCommand(page, 0, {shape, other}, *path, QVariant(qulonglong{other})) == nullptr);
    QVERIFY(ui::interactivityCommand(page, 0, {shape}, *path, QVariant(qulonglong{other})) == nullptr);
    QVERIFY(ui::interactivityCommand(page, 0, {shape}, *path, QVariant(qulonglong{track})) != nullptr);
}

QTEST_MAIN(TstUiInteractivityprops)
#include "tst_ui_interactivityprops.moc"
