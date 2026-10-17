#pragma once

// Document de test qui remplit chaque champ du modèle d'une valeur hors défaut (DA2) : allers-retours du
// format natif (L2-T5, L2-T6) et des fichiers (L2-T17). Écrit par nous ; les « images » sont de faux octets
// (le format ne les décode pas). tst_formats_nativejson vérifie que chaque champ est rempli
// (richDocumentFillsEveryField) et que deux champs frères de même type diffèrent dans une instance au moins
// (richDocumentSeparatesSiblings) : sinon, les échanger dans l'écrivain ou le lecteur garderait l'empreinte.

#include <actcraft/core/document.h>

#include <QByteArray>

#include <string>
#include <utility>
#include <vector>

namespace richdoc {

inline const QByteArray& pngBytes()
{
    static const QByteArray bytes("\x89PNG\r\n\x1a\nfaux", 12);
    return bytes;
}

inline const QByteArray& mp3Bytes()
{
    static const QByteArray bytes("ID3\x04\0\0\0\0\0\0son", 13);
    return bytes;
}

inline actcraft::core::Document richDocument()
{
    using namespace actcraft::core;
    const std::string png = sha256Hex(pngBytes());
    const std::string mp3 = sha256Hex(mp3Bytes());
    std::vector<Page> pages(3);

    Page& p1 = pages[0];
    p1.name = "Page un";
    p1.notes = "Notes : « é è à »";

    Shape rect;
    rect.kind = ShapeKind::Rectangle;
    rect.bounds = {10, 20, 100, 50};
    rect.fill = 0xFF336699u;
    rect.outline = 0xFF112233u;
    rect.outlineWidth = 2.5;
    rect.opacity = 0.75;
    rect.windingFill = true; // `open` reste faux ici, vrai sur le chemin (o4)
    PageObject o1 = rect;
    o1.common.id = 1;
    o1.common.name = "Rectangle";
    o1.common.layer = Layer::Bottom;
    o1.common.visible = false;
    o1.common.locked = true;
    o1.common.translucency = 40;
    o1.common.restrictions.movable = false;
    o1.common.restrictions.resizable = false; // rotatable reste vrai : o2 porte l'autre combinaison
    o1.common.transform = compose(compose(scaling(2, 3), rotation(30)), translation(5.5, -2.25));
    o1.common.label = "Légende";
    Action action;
    action.kind = ActionKind::OnObject;
    action.sourceType = 28;
    action.command = 184;
    action.target = 6;
    action.resource = mp3;
    o1.common.actions.push_back(action);
    p1.objects.push_back(o1);

    ImageObject generated;
    generated.bounds = {200, 10, 64, 48};
    generated.sourceWidth = 128;
    generated.sourceHeight = 96;
    generated.pattern = ImagePattern::Rings;
    generated.colorA = 0xFF00FF00u;
    generated.colorB = 0xFF0000FFu;
    generated.seed = 7;
    PageObject o2 = generated;
    o2.common.id = 2;
    o2.common.restrictions.resizable = false;
    o2.common.restrictions.rotatable = false;
    p1.objects.push_back(o2);

    ImageObject picture;
    picture.bounds = {0, 0, 2, 2};
    picture.sourceWidth = 2;
    picture.sourceHeight = 2;
    picture.resource = png;
    picture.pattern = ImagePattern::Rings;
    picture.colorA = 0xFF00FF00u;
    picture.colorB = 0xFF0000FFu;
    picture.seed = 3;
    picture.hasKeyColor = true;
    picture.keyColor = 0xFFFFFFFFu;
    PageObject o3 = picture;
    o3.common.id = 3;
    o3.common.transform = scaling(50, 25);
    p1.objects.push_back(o3);

    Shape path;
    path.kind = ShapeKind::Path;
    path.bounds = {-3, -3, 86, 66};
    path.paths = {{{0, 0}, {80, 0}, {80, 60}}, {{20, 20}, {60, 20}, {40, 50}}};
    path.open = true;
    path.polygon = {{1, 1}, {9, 1}, {5, 8}};
    path.fill = 0xFF010101u;
    path.outline = 0xFF020202u;
    path.outlineWidth = 4;
    path.opacity = 0.5;
    path.pen.style = PenStyle::Dash;
    path.pen.color = 0xFF010203u;
    path.pen.width = 6;
    path.pen.cap = PenCap::Flat;
    path.pen.join = PenJoin::Bevel;
    path.brush.kind = FillKind::Gradient;
    path.brush.color = 0xFF00AA00u;
    path.brush.color2 = 0x80FFFFFFu;
    path.brush.gradient = GradientKind::Radial;
    path.brush.pattern = 5;
    path.brush.opaqueBackground = true;
    path.brush.backgroundColor = 0xFF444444u;
    PageObject o4 = path;
    o4.common.id = 4;
    p1.objects.push_back(o4);

    Line line;
    line.points = {{0, 0}, {50, 25}, {100, 0}};
    line.pen.style = PenStyle::DashDotDot;
    line.pen.width = 3;
    PageObject o5 = line;
    o5.common.id = 5;
    p1.objects.push_back(o5);

    Text text;
    text.bounds = {0, 0, 180, 40};
    text.html = "<p>Bonjour <b>tout</b> le monde</p>";
    TextSpan first;
    first.text = "Bonjour";
    first.fontFamily = "Arial";
    first.pointSize = 24;
    first.weight = 700;
    first.italic = true;
    first.color = 0xFFFF0000u;
    first.underline = true;
    first.strikeOut = false;
    first.baseline = {0, 30};
    first.bounds = {0, 0, 90, 36};
    TextSpan second = first;
    second.text = "tout";
    second.weight = 400;
    second.italic = false;
    second.strikeOut = true;
    text.runs = {first, second};
    PageObject o6 = text;
    o6.common.id = 6;
    p1.objects.push_back(o6);

    Media media;
    media.bounds = {0, 0, 64, 48};
    media.placeholder = png;
    media.media = mp3;
    media.fileName = "chanson.mp3";
    media.playOnOpen = true;
    media.loop = false; // vrai sur le média de la page 3
    PageObject o7 = media;
    o7.common.id = 7;
    p1.objects.push_back(o7);

    Group group;
    group.members = {4, 5};
    PageObject o8 = group;
    o8.common.id = 8;
    p1.objects.push_back(o8);

    Connector connector;
    connector.firstEnd = {1, 2};
    connector.secondEnd = {300, 400};
    connector.objectA = 4;
    connector.objectB = 6;
    connector.pen.color = 0xFF990000u;
    PageObject o9 = connector;
    o9.common.id = 9;
    o9.common.layer = Layer::Top;
    p1.objects.push_back(o9);

    Stroke stroke;
    stroke.points = {{10, 10, 0.25f}, {20, 15, 0.5f}, {30, 10, 0.75f}};
    stroke.baseWidth = 6;
    stroke.color = 0x80FFFF00u;
    stroke.kind = StrokeKind::Highlighter;
    stroke.cap = PenCap::Round;
    PageObject o10 = stroke;
    o10.common.id = 10;
    o10.common.layer = Layer::Top;
    p1.objects.push_back(o10);

    Page& p2 = pages[1];
    p2.name = "Page deux";
    p2.notes = "Notes de la page deux";
    p2.width = 800;
    p2.height = 600;
    p2.background.fill.kind = FillKind::Gradient;
    p2.background.fill.color = 0xFFC8DCFFu;
    p2.background.fill.color2 = 0xFFFFFFFFu;
    p2.background.fill.gradient = GradientKind::DiagonalUp;
    p2.background.image = png;
    p2.grid.visible = true;
    p2.grid.snapEnabled = false; // vrai sur la grille de la page 3
    p2.grid.drawnAbove = true;
    p2.grid.previewZoom = 0.5;
    p2.grid.baseZoom = 2;
    p2.grid.zoomIncrement = 0.25;
    GridLevel coarse;
    coarse.spacingX = 50;
    coarse.spacingY = 25;
    coarse.color = 0xFF0000FFu;
    coarse.snap = false;
    GridLevel fine;
    fine.visible = false;
    fine.start = {3, 4};
    fine.pattern = GridPattern::Angle;
    fine.angle = 45;
    fine.line = GridLine::Dotted;
    fine.linesAlongY = false;
    fine.spacingX = 64;
    fine.spacingY = 48;
    fine.color = 0xFF00FF00u;
    GridLevel third;
    third.linesAlongX = false;
    third.linesAlongY = false;
    third.spacingX = 16;
    third.spacingY = 12;
    third.color = 0xFF808080u;
    p2.grid.levels = {coarse, fine, third};

    Page& p3 = pages[2];
    p3.grid.snapEnabled = true;
    p3.grid.drawnAbove = true;
    Media song;
    song.bounds = {500, 300, 32, 24};
    song.media = mp3;
    song.fileName = "boucle.mp3";
    song.loop = true;
    PageObject o11 = song;
    o11.common.id = 11;
    p3.objects.push_back(o11);

    Document document(std::move(pages));
    document.setTitle("Document riche");
    document.addResource(pngBytes(), "png");
    document.addResource(mp3Bytes(), "MP3");
    document.reserveIdsBelow(20);
    return document;
}

} // namespace richdoc
