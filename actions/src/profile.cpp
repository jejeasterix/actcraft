#include <actcraft/actions/profile.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonValue>

#include <algorithm>
#include <array>
#include <utility>

namespace actcraft::actions {

namespace {

constexpr qsizetype kRecentToolsMax = 8;

// Clés de premier niveau connues de cette version ; tout le reste va dans `unknownKeys`.
const QString kFormat = QStringLiteral("format");
const QString kId = QStringLiteral("id");
const QString kLabel = QStringLiteral("label");
const QString kIconPixels = QStringLiteral("icon-pixels");
const QString kPalette = QStringLiteral("palette");
const QString kBrowsers = QStringLiteral("browsers");
const QString kExtraColors = QStringLiteral("extra-colors");
const QString kRecentTools = QStringLiteral("recent-tools");
const QString kPinned = QStringLiteral("pinned");
const QString kFitPage = QStringLiteral("fit-page");
const QString kTrashVisible = QStringLiteral("trash-visible");
const QString kShortcutObjects = QStringLiteral("shortcut-objects");
const QString kLaunchers = QStringLiteral("launchers");
const QString kSettings = QStringLiteral("settings");

// Identifiants de `BrowserPanel.navigators` (ui/qml/BrowserPanel.qml, amendement 4, A8).
const QStringList kNavigators{QStringLiteral("pages"),      QStringLiteral("ressources"),
                              QStringLiteral("objets"),     QStringLiteral("notes"),
                              QStringLiteral("proprietes"), QStringLiteral("actions")};

constexpr std::array<std::pair<Dock, const char*>, 5> kDocks{{
    {Dock::Left, "left"},
    {Dock::Right, "right"},
    {Dock::Top, "top"},
    {Dock::Bottom, "bottom"},
    {Dock::Floating, "floating"},
}};

// Retire `key` de `object` et la rend (indéfinie si absente).
QJsonValue take(QJsonObject& object, const QString& key)
{
    const auto it = object.constFind(key);
    if (it == object.constEnd())
        return QJsonValue(QJsonValue::Undefined);
    QJsonValue value = it.value();
    object.remove(key);
    return value;
}

QStringList stringsOf(const QJsonValue& value)
{
    QStringList out;
    const QJsonArray array = value.toArray();
    for (const QJsonValue& item : array) {
        if (item.isString())
            out << item.toString();
    }
    return out;
}

QString stringOr(const QJsonValue& value, const QString& fallback)
{
    return value.isString() ? value.toString() : fallback;
}

// « actcraft-profile/1 », « actcraft-profile/1.4 » : version majeure 1 ; « /10 », « /2 » : refusées.
bool majorVersionIsOne(const QString& format)
{
    static const QString prefix = QStringLiteral("actcraft-profile/");
    if (!format.startsWith(prefix))
        return false;
    const QString version = format.mid(prefix.size());
    return version.section(QLatin1Char('.'), 0, 0) == QLatin1String("1");
}

// Ce qui reste de `object` une fois ses clés connues retirées (H6-12 : gardé et réécrit tel quel).
QJsonObject withoutKeys(QJsonObject object, const QStringList& known)
{
    for (const QString& key : known)
        object.remove(key);
    return object;
}

// Écrit `known` par-dessus `unknown` : une clé inconnue n'écrase jamais une clé connue.
QJsonObject mergedOver(QJsonObject unknown, const QJsonObject& known)
{
    for (auto it = known.begin(); it != known.end(); ++it)
        unknown.insert(it.key(), it.value());
    return unknown;
}

const QStringList kPaletteKeys{
    QStringLiteral("dock"),      QStringLiteral("float-x"),          QStringLiteral("float-y"),
    QStringLiteral("collapsed"), QStringLiteral("shortcuts-hidden"), QStringLiteral("pinned"),
    QStringLiteral("swatches")};
const QStringList kBrowserKeys{QStringLiteral("visible"), QStringLiteral("side"), QStringLiteral("pinned"),
                               QStringLiteral("current"), QStringLiteral("width")};

PaletteSettings paletteFrom(const QJsonObject& object)
{
    PaletteSettings palette;
    palette.dock = dockFromName(object.value(QStringLiteral("dock")).toString()).value_or(Dock::Right);
    palette.floatX = object.value(QStringLiteral("float-x")).toDouble(palette.floatX);
    palette.floatY = object.value(QStringLiteral("float-y")).toDouble(palette.floatY);
    palette.collapsed = object.value(QStringLiteral("collapsed")).toBool(palette.collapsed);
    palette.shortcutsHidden =
        object.value(QStringLiteral("shortcuts-hidden")).toBool(palette.shortcutsHidden);
    palette.pinned = object.value(QStringLiteral("pinned")).toBool(palette.pinned);
    const int swatches = object.value(QStringLiteral("swatches")).toInt(palette.swatches);
    palette.swatches = (swatches == 8 || swatches == 16 || swatches == 24) ? swatches : 16;
    palette.unknown = withoutKeys(object, kPaletteKeys);
    return palette;
}

QJsonObject paletteTo(const PaletteSettings& palette)
{
    return mergedOver(palette.unknown, QJsonObject{
                                           {QStringLiteral("dock"), dockName(palette.dock)},
                                           {QStringLiteral("float-x"), palette.floatX},
                                           {QStringLiteral("float-y"), palette.floatY},
                                           {QStringLiteral("collapsed"), palette.collapsed},
                                           {QStringLiteral("shortcuts-hidden"), palette.shortcutsHidden},
                                           {QStringLiteral("pinned"), palette.pinned},
                                           {QStringLiteral("swatches"), palette.swatches},
                                       });
}

BrowserSettings browsersFrom(const QJsonObject& object)
{
    BrowserSettings browsers;
    browsers.visible = object.value(QStringLiteral("visible")).toBool(browsers.visible);
    browsers.side = stringOr(object.value(QStringLiteral("side")), browsers.side);
    browsers.pinned = object.value(QStringLiteral("pinned")).toBool(browsers.pinned);
    const QString current = object.value(QStringLiteral("current")).toString();
    browsers.current = kNavigators.contains(current) ? current : QStringLiteral("pages");
    browsers.width = object.value(QStringLiteral("width")).toInt(browsers.width);
    browsers.unknown = withoutKeys(object, kBrowserKeys);
    return browsers;
}

QJsonObject browsersTo(const BrowserSettings& browsers)
{
    return mergedOver(browsers.unknown, QJsonObject{
                                            {QStringLiteral("visible"), browsers.visible},
                                            {QStringLiteral("side"), browsers.side},
                                            {QStringLiteral("pinned"), browsers.pinned},
                                            {QStringLiteral("current"), browsers.current},
                                            {QStringLiteral("width"), browsers.width},
                                        });
}

} // namespace

QString dockName(Dock dock)
{
    for (const auto& [value, name] : kDocks) {
        if (value == dock)
            return QString::fromLatin1(name);
    }
    return QStringLiteral("right");
}

std::optional<Dock> dockFromName(const QString& name)
{
    for (const auto& [value, text] : kDocks) {
        if (name == QLatin1String(text))
            return value;
    }
    return std::nullopt;
}

int clampIconPixels(int pixels) noexcept
{
    return std::clamp(pixels, kIconPixelsMin, kIconPixelsMax);
}

QByteArray profileToJson(const Profile& profile)
{
    // Clés inconnues d'abord, clés connues ensuite : une clé inconnue n'écrase jamais une clé connue.
    QJsonObject object = profile.unknownKeys;
    object.insert(kFormat, QString::fromLatin1(kProfileFormat));
    object.insert(kId, profile.id);
    object.insert(kLabel, profile.label);
    object.insert(kIconPixels, profile.iconPixels);
    object.insert(kPalette, paletteTo(profile.palette));
    object.insert(kBrowsers, browsersTo(profile.browsers));
    object.insert(kExtraColors, QJsonArray::fromStringList(profile.extraColors));
    object.insert(kRecentTools, QJsonArray::fromStringList(profile.recentTools));
    object.insert(kPinned, QJsonArray::fromStringList(profile.pinned));
    object.insert(kFitPage, profile.fitPage);
    object.insert(kTrashVisible, profile.trashVisible);
    object.insert(kShortcutObjects, profile.shortcutObjects);
    object.insert(kLaunchers, profile.launchers);
    object.insert(kSettings, profile.settings);
    return QJsonDocument(object).toJson(QJsonDocument::Indented);
}

bool profileFromJson(const QByteArray& json, Profile& profile, QString& error)
{
    QJsonParseError parse{};
    const QJsonDocument document = QJsonDocument::fromJson(json, &parse);
    if (parse.error != QJsonParseError::NoError) {
        error = QStringLiteral("profil illisible : %1").arg(parse.errorString());
        return false;
    }
    if (!document.isObject()) {
        error = QStringLiteral("profil illisible : un objet JSON est attendu");
        return false;
    }
    // Chaque clé connue est retirée de cette copie ; ce qui reste est gardé tel quel.
    QJsonObject rest = document.object();
    const QJsonValue format = take(rest, kFormat);
    if (!format.isString()) {
        error = QStringLiteral("profil sans « format »");
        return false;
    }
    if (!majorVersionIsOne(format.toString())) {
        error = QStringLiteral("format de profil non pris en charge : %1").arg(format.toString());
        return false;
    }

    Profile out;
    out.id = stringOr(take(rest, kId), QString());
    out.label = stringOr(take(rest, kLabel), QString());
    out.iconPixels = clampIconPixels(take(rest, kIconPixels).toInt(out.iconPixels));
    out.palette = paletteFrom(take(rest, kPalette).toObject());
    out.browsers = browsersFrom(take(rest, kBrowsers).toObject());
    out.extraColors = stringsOf(take(rest, kExtraColors));
    out.recentTools = stringsOf(take(rest, kRecentTools)).mid(0, kRecentToolsMax);
    out.pinned = stringsOf(take(rest, kPinned));
    out.fitPage = take(rest, kFitPage).toBool(out.fitPage);
    out.trashVisible = take(rest, kTrashVisible).toBool(out.trashVisible);
    out.shortcutObjects = take(rest, kShortcutObjects).toObject();
    out.launchers = take(rest, kLaunchers).toObject();
    out.settings = take(rest, kSettings).toObject();
    out.unknownKeys = rest;
    profile = std::move(out);
    error.clear();
    return true;
}

} // namespace actcraft::actions
