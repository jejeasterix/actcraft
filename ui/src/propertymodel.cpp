// L6-T12 : modèle du navigateur de propriétés (F-570 à F-576, F-581, F-583, F-840) ; L8-T17 : sections de
// l'interactivité (interactivityprops.h).
#include <actcraft/core/arrange.h>
#include <actcraft/core/objectedits.h>
#include <actcraft/core/objectflags.h>
#include <actcraft/core/propertyedits.h>
#include <actcraft/tools/userstate.h>
#include <actcraft/ui/interactivityprops.h>
#include <actcraft/ui/propertymodel.h>

#include <QColor>
#include <QRegularExpression>
#include <QStringList>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <variant>

namespace actcraft::ui {

namespace {

enum class Editor : std::uint8_t
{
    Text,
    Number,
    Color,
    Slider,
    Choice,
    Bool,
    Keywords,
};

// Liste de choix d'une propriété « choice » : l'indice est la valeur sous-jacente de l'énumération.
enum class Choices : std::uint8_t
{
    None,
    Layer,
    Pen,
    Fill,
};

// Une ligne possible : propriété de la page (`page`) ou d'objet (`object`).
struct Descriptor
{
    const char* section;
    const char* key;
    const char* label;
    Editor editor;
    Choices choices;
    bool page;
    core::PageProperty pageProperty;
    core::ObjectProperty objectProperty;
    double min;
    double max;
};

constexpr double kNoBound = std::numeric_limits<double>::quiet_NaN();

constexpr Descriptor pageRow(const char* section, const char* key, const char* label, Editor editor,
                             core::PageProperty property, double min = kNoBound, double max = kNoBound)
{
    return {section, key, label, editor, Choices::None, true, property, core::ObjectProperty::Name, min, max};
}

constexpr Descriptor objectRow(const char* section, const char* key, const char* label, Editor editor,
                               core::ObjectProperty property, Choices choices = Choices::None,
                               double min = kNoBound, double max = kNoBound)
{
    return {section, key, label, editor, choices, false, core::PageProperty::Name, property, min, max};
}

// F-583 : Identification, Page.
const std::array kPageRows = {
    pageRow("identification", "name", "Nom", Editor::Text, core::PageProperty::Name),
    pageRow("identification", "description", "Description", Editor::Text, core::PageProperty::Description),
    pageRow("identification", "keywords", "Mots-clés", Editor::Keywords, core::PageProperty::Keywords),
    pageRow("page", "width", "Largeur", Editor::Number, core::PageProperty::Width, 1,
            DocumentController::kMaxPageSide),
    pageRow("page", "height", "Hauteur", Editor::Number, core::PageProperty::Height, 1,
            DocumentController::kMaxPageSide),
};

// Lignes d'objet, dans l'ordre d'affichage. Une ligne n'apparaît que si la propriété s'applique à chaque
// objet sélectionné (core::readProperty) : la même règle que SetObjectPropertiesCommand, qui ignore les
// objets auxquels elle ne s'applique pas. La légende a son style (core::LabelStyle, composé) : aucun éditeur
// typé ne le porte ici.
const std::array kObjectRows = {
    objectRow("identification", "name", "Nom", Editor::Text, core::ObjectProperty::Name),
    objectRow("identification", "keywords", "Mots-clés", Editor::Keywords, core::ObjectProperty::Keywords),
    objectRow("appearance", "layer", "Couche", Editor::Choice, core::ObjectProperty::Layer, Choices::Layer),
    objectRow("appearance", "translucency", "Translucidité", Editor::Slider,
              core::ObjectProperty::Translucency, Choices::None, 0, 255),
    objectRow("appearance", "visible", "Visible", Editor::Bool, core::ObjectProperty::Visible),
    objectRow("appearance", "locked", "Verrouillé", Editor::Bool, core::ObjectProperty::Locked),
    objectRow("outline", "outline-style", "Style", Editor::Choice, core::ObjectProperty::OutlineStyle,
              Choices::Pen),
    objectRow("outline", "outline-color", "Couleur", Editor::Color, core::ObjectProperty::OutlineColor),
    objectRow("outline", "outline-width", "Épaisseur", Editor::Number, core::ObjectProperty::OutlineWidth,
              Choices::None, 0, 200),
    objectRow("fill", "fill-style", "Style", Editor::Choice, core::ObjectProperty::FillStyle, Choices::Fill),
    objectRow("fill", "fill-color", "Couleur", Editor::Color, core::ObjectProperty::FillColor),
    objectRow("position", "left", "Gauche", Editor::Number, core::ObjectProperty::Left, Choices::None,
              -DocumentController::kMaxPageSide, DocumentController::kMaxPageSide),
    objectRow("position", "top", "Haut", Editor::Number, core::ObjectProperty::Top, Choices::None,
              -DocumentController::kMaxPageSide, DocumentController::kMaxPageSide),
    objectRow("position", "width", "Largeur", Editor::Number, core::ObjectProperty::Width, Choices::None, 1,
              DocumentController::kMaxPageSide),
    objectRow("position", "height", "Hauteur", Editor::Number, core::ObjectProperty::Height, Choices::None, 1,
              DocumentController::kMaxPageSide),
    objectRow("position", "angle", "Angle", Editor::Number, core::ObjectProperty::Angle, Choices::None, -360,
              360),
    objectRow("label", "label", "Texte", Editor::Text, core::ObjectProperty::Label),
    objectRow("misc", "mirrored", "Miroir", Editor::Bool, core::ObjectProperty::Mirrored),
};

QString editorKey(Editor editor)
{
    switch (editor) {
    case Editor::Text:
        return QStringLiteral("text");
    case Editor::Number:
        return QStringLiteral("number");
    case Editor::Color:
        return QStringLiteral("color");
    case Editor::Slider:
        return QStringLiteral("slider");
    case Editor::Choice:
        return QStringLiteral("choice");
    case Editor::Bool:
        return QStringLiteral("bool");
    case Editor::Keywords:
        return QStringLiteral("keywords");
    }
    return {};
}

QString sectionLabel(const QString& section)
{
    if (section == QLatin1String("identification")) {
        return PropertyModel::tr("Identification");
    }
    if (section == QLatin1String("appearance")) {
        return PropertyModel::tr("Apparence");
    }
    if (section == QLatin1String("outline")) {
        return PropertyModel::tr("Contour");
    }
    if (section == QLatin1String("fill")) {
        return PropertyModel::tr("Remplissage");
    }
    if (section == QLatin1String("position")) {
        return PropertyModel::tr("Position");
    }
    if (section == QLatin1String("label")) {
        return PropertyModel::tr("Étiquette");
    }
    if (section == QLatin1String("misc")) {
        return PropertyModel::tr("Divers");
    }
    return PropertyModel::tr("Page");
}

// Indice = valeur sous-jacente de core::Layer, core::PenStyle, core::FillKind.
QStringList choiceLabels(Choices choices)
{
    switch (choices) {
    case Choices::Layer:
        return {PropertyModel::tr("Arrière-plan"), PropertyModel::tr("Couche inférieure"),
                PropertyModel::tr("Couche intermédiaire"), PropertyModel::tr("Couche du dessus")};
    case Choices::Pen:
        return {PropertyModel::tr("Aucun"),       PropertyModel::tr("Continu"),
                PropertyModel::tr("Tirets"),      PropertyModel::tr("Points"),
                PropertyModel::tr("Tiret-point"), PropertyModel::tr("Tiret-point-point")};
    case Choices::Fill:
        return {PropertyModel::tr("Aucun"), PropertyModel::tr("Uni"), PropertyModel::tr("Motif"),
                PropertyModel::tr("Dégradé")};
    case Choices::None:
        break;
    }
    return {};
}

QString kindLabel(core::ObjectKind kind)
{
    switch (kind) {
    case core::ObjectKind::Shape:
        return PropertyModel::tr("Forme");
    case core::ObjectKind::Image:
        return PropertyModel::tr("Image");
    case core::ObjectKind::Stroke:
        return PropertyModel::tr("Trait");
    case core::ObjectKind::Line:
        return PropertyModel::tr("Ligne");
    case core::ObjectKind::Text:
        return PropertyModel::tr("Texte");
    case core::ObjectKind::Media:
        return PropertyModel::tr("Média");
    case core::ObjectKind::Group:
        return PropertyModel::tr("Groupe");
    case core::ObjectKind::Connector:
        return PropertyModel::tr("Connecteur");
    case core::ObjectKind::Equation:
        return PropertyModel::tr("Équation");
    case core::ObjectKind::WebFrame: // lot 8 : comme un objet inconnu
    case core::ObjectKind::Unknown:
        break;
    }
    return PropertyModel::tr("Objet");
}

QStringList toStringList(const std::vector<std::string>& values)
{
    QStringList list;
    list.reserve(static_cast<qsizetype>(values.size()));
    for (const std::string& value : values) {
        list.append(QString::fromStdString(value));
    }
    return list;
}

// Valeur du cœur → QVariant du rôle ValueRole.
QVariant toVariant(const core::PropertyValue& value)
{
    struct Visitor
    {
        QVariant operator()(const std::string& v) const { return QString::fromStdString(v); }
        QVariant operator()(const std::vector<std::string>& v) const { return toStringList(v); }
        QVariant operator()(double v) const { return v; }
        QVariant operator()(bool v) const { return v; }
        QVariant operator()(std::uint32_t v) const { return QColor::fromRgba(v); }
        QVariant operator()(core::Layer v) const { return static_cast<int>(v); }
        QVariant operator()(core::PenStyle v) const { return static_cast<int>(v); }
        QVariant operator()(core::FillKind v) const { return static_cast<int>(v); }
        QVariant operator()(const core::LabelStyle&) const { return {}; }
    };
    return std::visit(Visitor{}, value);
}

std::vector<std::string> keywordsOf(const QVariant& value)
{
    QStringList list;
    if (value.metaType().id() == QMetaType::QString) {
        // Éditeur de mots-clés de L6-T13 : séparés par des espaces ; une virgule sépare aussi.
        static const QRegularExpression separators(QStringLiteral("[\\s,]+"));
        list = value.toString().split(separators, Qt::SkipEmptyParts);
    } else {
        list = value.toStringList();
    }
    std::vector<std::string> keywords;
    for (const QString& keyword : std::as_const(list)) {
        const QString trimmed = keyword.trimmed();
        if (!trimmed.isEmpty()) {
            keywords.push_back(trimmed.toStdString());
        }
    }
    return keywords;
}

// QVariant reçu de l'éditeur → valeur du cœur ; std::nullopt si illisible.
std::optional<core::PropertyValue> fromVariant(const Descriptor& descriptor, const QVariant& value)
{
    if (!value.isValid()) {
        return std::nullopt;
    }
    switch (descriptor.editor) {
    case Editor::Text:
        return core::PropertyValue{value.toString().toStdString()};
    case Editor::Keywords:
        return core::PropertyValue{keywordsOf(value)};
    case Editor::Number:
    case Editor::Slider: {
        bool ok = false;
        const double number = value.toDouble(&ok);
        if (!ok || !std::isfinite(number)) {
            return std::nullopt;
        }
        // Bornes publiées (MinRole, MaxRole) : hors d'elles, refus, jamais écrêtage.
        if ((!std::isnan(descriptor.min) && number < descriptor.min) ||
            (!std::isnan(descriptor.max) && number > descriptor.max)) {
            return std::nullopt;
        }
        return core::PropertyValue{number};
    }
    case Editor::Bool:
        if (!value.canConvert<bool>()) {
            return std::nullopt;
        }
        return core::PropertyValue{value.toBool()};
    case Editor::Color: {
        const QColor color =
            value.metaType().id() == QMetaType::QColor ? value.value<QColor>() : QColor(value.toString());
        if (!color.isValid()) {
            return std::nullopt;
        }
        return core::PropertyValue{static_cast<std::uint32_t>(color.rgba())};
    }
    case Editor::Choice: {
        bool ok = false;
        const int index = value.toInt(&ok);
        if (!ok || index < 0 || index >= choiceLabels(descriptor.choices).size()) {
            return std::nullopt;
        }
        switch (descriptor.choices) {
        case Choices::Layer:
            return core::PropertyValue{static_cast<core::Layer>(index)};
        case Choices::Pen:
            return core::PropertyValue{static_cast<core::PenStyle>(index)};
        case Choices::Fill:
            return core::PropertyValue{static_cast<core::FillKind>(index)};
        case Choices::None:
            break;
        }
        return std::nullopt;
    }
    }
    return std::nullopt;
}

// Couche, visibilité, verrou et translucidité s'écrivent par les commandes du lot 5 (isDelegated) ; le
// reste par SetObjectPropertiesCommand.
std::unique_ptr<core::UndoCommand> objectCommand(std::size_t page, std::vector<core::ObjectId> ids,
                                                 core::ObjectProperty property, core::PropertyValue value)
{
    switch (property) {
    case core::ObjectProperty::Layer:
        return std::make_unique<core::SetLayerCommand>(page, std::move(ids), std::get<core::Layer>(value));
    case core::ObjectProperty::Visible:
        return std::make_unique<core::SetObjectFlagCommand>(page, std::move(ids), core::ObjectFlag::Hidden,
                                                            !std::get<bool>(value));
    case core::ObjectProperty::Locked:
        return std::make_unique<core::SetObjectFlagCommand>(page, std::move(ids), core::ObjectFlag::Locked,
                                                            std::get<bool>(value));
    case core::ObjectProperty::Translucency: {
        const double t = std::clamp(std::round(std::get<double>(value)), 0.0, 255.0);
        return std::make_unique<core::SetTranslucencyCommand>(page, std::move(ids),
                                                              static_cast<std::uint8_t>(t));
    }
    default:
        return std::make_unique<core::SetObjectPropertiesCommand>(static_cast<int>(page), std::move(ids),
                                                                  property, std::move(value));
    }
}

} // namespace

// Ligne du lot 6 (descriptor) ou de l'interactivité (interactivity), jamais les deux.
struct PropertyModel::Row
{
    const Descriptor* descriptor = nullptr;
    QVariant value; // invalide si les objets diffèrent
    const InteractivityRow* interactivity = nullptr;
    bool enabled = true; // kEnabledRole ; toujours vrai pour une ligne du lot 6
};

PropertyModel::PropertyModel(QObject* parent) : QAbstractListModel(parent)
{
    connect(this, &PropertyModel::contactActiveChanged, this, [this] {
        if (!m_contactActive && m_pending) {
            schedule(); // la remise à jour reportée pendant le contact
        }
    });
}

PropertyModel::~PropertyModel()
{
    detachDocument();
}

int PropertyModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

QVariant PropertyModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }
    const Row& row = m_rows[static_cast<std::size_t>(index.row())];
    if (const InteractivityRow* i = row.interactivity) {
        switch (role) {
        case SectionRole:
            return QString::fromLatin1(i->section);
        case SectionLabelRole:
            return interactivitySectionLabel(QString::fromLatin1(i->section));
        case KeyRole:
            return QString::fromLatin1(i->key);
        case Qt::DisplayRole:
        case LabelRole:
            return tr(i->label);
        case EditorRole:
            return interactivityEditorKey(i->editor);
        case ValueRole:
            return row.value;
        case ChoicesRole:
            return interactivityChoices(i->field);
        case MinRole:
            return std::isnan(i->min) ? QVariant() : QVariant(i->min);
        case MaxRole:
            return std::isnan(i->max) ? QVariant() : QVariant(i->max);
        case kEnabledRole:
            return row.enabled;
        default:
            return {};
        }
    }
    const Descriptor& d = *row.descriptor;
    switch (role) {
    case SectionRole:
        return QString::fromLatin1(d.section);
    case SectionLabelRole:
        return sectionLabel(QString::fromLatin1(d.section));
    case KeyRole:
        return QString::fromLatin1(d.key);
    case Qt::DisplayRole:
    case LabelRole:
        return tr(d.label);
    case EditorRole:
        return editorKey(d.editor);
    case ValueRole:
        return row.value;
    case ChoicesRole:
        return choiceLabels(d.choices);
    case MinRole:
        return std::isnan(d.min) ? QVariant() : QVariant(d.min);
    case MaxRole:
        return std::isnan(d.max) ? QVariant() : QVariant(d.max);
    case kEnabledRole:
        return true;
    default:
        return {};
    }
}

QHash<int, QByteArray> PropertyModel::roleNames() const
{
    return {
        {SectionRole, "section"}, {SectionLabelRole, "sectionLabel"},
        {KeyRole, "key"},         {LabelRole, "label"},
        {EditorRole, "editor"},   {ValueRole, "value"},
        {ChoicesRole, "choices"}, {MinRole, "minimum"},
        {MaxRole, "maximum"},     {kEnabledRole, "enabled"},
    };
}

bool PropertyModel::setValue(int row, const QVariant& value)
{
    if (!editable() || row < 0 || row >= rowCount()) {
        return false;
    }
    const Row& shown = m_rows[static_cast<std::size_t>(row)];
    if (const InteractivityRow* i = shown.interactivity) {
        // L8-T17 : une ligne grisée refuse ; une étape pour tous les objets montrés, close aussitôt.
        if (!shown.enabled || m_ids.empty() || !value.isValid()) {
            return false;
        }
        const auto page = static_cast<std::size_t>(m_page);
        // « Mots acceptés » : la règle du conteneur (H8-12), saisie lue comme la ligne des mots-clés du
        // lot 6.
        std::unique_ptr<core::UndoCommand> command =
            i->field == InteractivityField::ContainWords
                ? std::make_unique<SetInteractivityCommand>(page, m_ids, InteractivityField::ContainWords,
                                                            InteractivityValue{keywordsOf(value)})
                : interactivityCommand(m_document->document().page(page), page, m_ids, *i, value);
        if (!command) {
            return false;
        }
        m_document->closeMerge();
        m_sliding = false;
        const int appliedBefore = m_applied;
        m_document->execute(std::move(command));
        m_document->closeMerge();
        return m_applied != appliedBefore;
    }
    const Descriptor& d = *shown.descriptor;
    std::optional<core::PropertyValue> converted = fromVariant(d, value);
    if (!converted) {
        return false;
    }
    if (!d.page && m_ids.empty()) {
        return false;
    }
    // Une modification ponctuelle reste une étape à elle (L6-T11 : la commande fusionne avec sa voisine de
    // même page, mêmes objets, même propriété) : elle clôt l'étape restée ouverte avant elle, puis la
    // sienne. Seule la réglette glissée fusionne, jusqu'à endEdit() ; son premier pas clôt l'étape restée
    // ouverte avant le glissé.
    const bool point = d.editor != Editor::Slider;
    if (point) {
        m_document->closeMerge();
        m_sliding = false;
    } else if (!m_sliding) {
        m_document->closeMerge();
        m_sliding = true;
    }
    const auto page = static_cast<std::size_t>(m_page);
    const int appliedBefore = m_applied;
    if (d.page) {
        m_document->execute(
            std::make_unique<core::SetPagePropertiesCommand>(m_page, d.pageProperty, std::move(*converted)));
    } else {
        m_document->execute(objectCommand(page, m_ids, d.objectProperty, std::move(*converted)));
    }
    if (point) {
        m_document->closeMerge();
    }
    // Le cœur peut refuser ce que la ligne publie (un trait n'a pas d'épaisseur nulle) : sans changement
    // observé ni drapeau modifié, rien n'a été appliqué.
    return m_applied != appliedBefore;
}

int PropertyModel::rowOf(const QString& key) const
{
    for (std::size_t i = 0; i < m_rows.size(); ++i) {
        const char* rowKey =
            m_rows[i].interactivity != nullptr ? m_rows[i].interactivity->key : m_rows[i].descriptor->key;
        if (key == QLatin1String(rowKey)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

DocumentController* PropertyModel::document() const noexcept
{
    return m_document.data();
}

void PropertyModel::setDocument(DocumentController* document)
{
    if (m_document == document) {
        return;
    }
    detachDocument();
    m_document = document;
    attachDocument();
    emit sourcesChanged();
    updateEditable();
    requestRefresh();
}

ToolHost* PropertyModel::tools() const noexcept
{
    return m_tools.data();
}

void PropertyModel::setTools(ToolHost* tools)
{
    if (m_tools == tools) {
        return;
    }
    disconnect(m_selectionConnection);
    m_tools = tools;
    if (m_tools) {
        // A5 : la sélection se lit, le modèle ne sélectionne rien.
        m_selectionConnection =
            connect(m_tools.data(), &ToolHost::selectionChanged, this, &PropertyModel::requestRefresh);
    }
    emit sourcesChanged();
    requestRefresh();
}

Commands* PropertyModel::commands() const noexcept
{
    return m_commands.data();
}

void PropertyModel::setCommands(Commands* commands)
{
    if (m_commands == commands) {
        return;
    }
    disconnect(m_commandsConnection);
    m_commands = commands;
    if (m_commands) {
        m_commandsConnection =
            connect(m_commands.data(), &Commands::stateChanged, this, &PropertyModel::updateEditable);
    }
    emit sourcesChanged();
    updateEditable();
}

QString PropertyModel::title() const
{
    return m_title;
}

bool PropertyModel::contactActive() const noexcept
{
    return m_contactActive;
}

void PropertyModel::setContactActive(bool active)
{
    if (m_contactActive == active) {
        return;
    }
    m_contactActive = active;
    emit contactActiveChanged();
}

bool PropertyModel::editable() const
{
    return m_document != nullptr && m_commands != nullptr && !m_commands->isLocked();
}

int PropertyModel::refreshCount() const noexcept
{
    return m_refreshes;
}

void PropertyModel::requestRefresh()
{
    m_pending = true;
    if (!m_contactActive) {
        schedule();
    }
}

void PropertyModel::schedule()
{
    if (m_scheduled) {
        return;
    }
    m_scheduled = true;
    QMetaObject::invokeMethod(this, &PropertyModel::flush, Qt::QueuedConnection);
}

void PropertyModel::flush()
{
    m_scheduled = false;
    if (!m_pending || m_contactActive) {
        return; // pendant un contact : repris à sa fin (contactActiveChanged)
    }
    m_pending = false;
    std::vector<Row> rows = rebuild();
    const bool sameRows =
        rows.size() == m_rows.size() &&
        std::equal(rows.begin(), rows.end(), m_rows.begin(), [](const Row& a, const Row& b) {
            return a.descriptor == b.descriptor && a.interactivity == b.interactivity;
        });
    if (sameRows) {
        // Mêmes lignes : seules les valeurs (et le grisé de l'interactivité) changent, les délégués restent
        // (réglette tenue, champ en saisie). Une ligne du lot 6 ne change que sur ValueRole.
        std::vector<std::pair<int, QList<int>>> changed;
        for (std::size_t i = 0; i < rows.size(); ++i) {
            QList<int> roles;
            if (rows[i].value != m_rows[i].value) {
                m_rows[i].value = std::move(rows[i].value);
                roles.append(ValueRole);
            }
            if (rows[i].enabled != m_rows[i].enabled) {
                m_rows[i].enabled = rows[i].enabled;
                roles.append(kEnabledRole);
            }
            if (!roles.isEmpty()) {
                changed.emplace_back(static_cast<int>(i), std::move(roles));
            }
        }
        for (const auto& [row, roles] : changed) {
            emit dataChanged(index(row), index(row), roles);
        }
    } else {
        beginResetModel();
        m_rows = std::move(rows);
        endResetModel();
    }
    ++m_refreshes;
    emit refreshed();
}

std::vector<PropertyModel::Row> PropertyModel::rebuild()
{
    std::vector<Row> rows;
    m_ids.clear();
    m_title.clear();
    if (!m_document || m_document->pageCount() <= 0) {
        return rows;
    }
    m_page = m_document->currentIndex();
    const core::Page& page = m_document->document().page(static_cast<std::size_t>(m_page));
    std::vector<const core::PageObject*> objects;
    if (m_tools) {
        const std::vector<core::ObjectId>& selection = m_tools->user().selection();
        const std::unordered_set<core::ObjectId> selected(selection.begin(), selection.end());
        for (const core::PageObject& object : page.objects) {
            if (selected.count(object.common.id) != 0) {
                objects.push_back(&object);
                m_ids.push_back(object.common.id);
            }
        }
    }
    if (objects.empty()) {
        m_title = tr("Page");
        for (const Descriptor& d : kPageRows) {
            rows.push_back({&d, toVariant(core::readPageProperty(page, d.pageProperty)), nullptr, true});
        }
        return rows;
    }
    m_title = objects.size() == 1 ? kindLabel(core::kindOf(*objects.front()))
                                  : tr("%1 objets").arg(static_cast<qlonglong>(objects.size()));
    for (const Descriptor& d : kObjectRows) {
        std::optional<core::PropertyValue> first;
        bool common = true;
        bool applies = true;
        for (const core::PageObject* object : objects) {
            std::optional<core::PropertyValue> value = core::readProperty(*object, d.objectProperty);
            if (!value) {
                applies = false; // ne s'applique pas à tous : pas une propriété commune
                break;
            }
            if (!first) {
                first = std::move(value);
            } else if (!(*value == *first)) {
                common = false;
            }
        }
        if (applies) {
            rows.push_back({&d, common ? toVariant(*first) : QVariant(), nullptr, true});
        }
    }
    // L8-T17 : les cinq sections de l'interactivité, après celles du lot 6. Valeur commune ou invalide, comme
    // ci-dessus ; grisée si elle l'est pour un des objets.
    for (const InteractivityRow& r : interactivityRows()) {
        const bool applies =
            std::all_of(objects.begin(), objects.end(),
                        [&r](const core::PageObject* object) { return appliesTo(r, *object); });
        if (!applies) {
            continue;
        }
        QVariant first;
        bool common = true;
        bool enabled = true;
        for (std::size_t i = 0; i < objects.size(); ++i) {
            QVariant value = readInteractivity(*objects[i], r.field);
            if (i == 0) {
                first = std::move(value);
            } else if (value != first) {
                common = false;
            }
            enabled = enabled && isEnabled(*objects[i], r.field);
        }
        rows.push_back({nullptr, common ? first : QVariant(), &r, enabled});
    }
    return rows;
}

void PropertyModel::attachDocument()
{
    if (!m_document) {
        return;
    }
    DocumentController* controller = m_document.data();
    m_observer =
        controller->addChangeObserver([this](const core::Changes&, DocumentController::ChangeOrigin) {
            ++m_applied;
            requestRefresh();
        });
    m_documentConnections = {
        connect(controller, &DocumentController::documentChanged, this, &PropertyModel::requestRefresh),
        connect(controller, &DocumentController::pagesChanged, this, &PropertyModel::requestRefresh),
        // Verrou, masqué, copie au glissement, d'où qu'ils viennent : verrouiller ne rend aucun Change, ce
        // signal seul le dit (L6-C2). Ni Commands::triggered, ni remise à jour faite à la main après
        // setValue.
        connect(controller, &DocumentController::objectFlagsChanged, this,
                &PropertyModel::onObjectFlagsChanged),
    };
}

void PropertyModel::detachDocument()
{
    for (const QMetaObject::Connection& connection : std::as_const(m_documentConnections)) {
        disconnect(connection);
    }
    m_documentConnections.clear();
    m_sliding = false;
    if (m_document && m_observer > 0) {
        m_document->removeChangeObserver(m_observer);
    }
    m_observer = 0;
}

void PropertyModel::endEdit()
{
    m_sliding = false;
    if (m_document) {
        m_document->closeMerge();
    }
}

void PropertyModel::updateEditable()
{
    const bool now = editable();
    if (now != m_editable) {
        m_editable = now;
        emit editableChanged();
    }
}

void PropertyModel::onObjectFlagsChanged(int page, const QList<quint64>& ids)
{
    ++m_applied;
    // Filtre : seuls comptent les objets montrés, c'est-à-dire sélectionnés sur la page affichée.
    if (!m_document || !m_tools || page != m_document->currentIndex()) {
        return;
    }
    const std::vector<core::ObjectId>& selection = m_tools->user().selection();
    const bool shown = std::any_of(ids.begin(), ids.end(), [&selection](quint64 id) {
        return std::find(selection.begin(), selection.end(), static_cast<core::ObjectId>(id)) !=
               selection.end();
    });
    if (shown) {
        requestRefresh();
    }
}

} // namespace actcraft::ui
