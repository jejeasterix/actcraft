#pragma once

#include <QtGlobal>

// Numéros du format .flipchart lus par ActCraft, écrits à la main d'après notre documentation
// (REA - Activinspire/docs/re/format-flipchart.md § 4 à § 7). Seuls les types et propriétés lus y sont ; les
// noms sont les nôtres. Les numéros de propriétés sont stables entre versions du format (§ 4.2), mais propres
// à chaque classe au-delà de 36 : 38 est la ressource d'une forme et la translucidité d'une annotation.

namespace actcraft::formats::flipchart::ids {

namespace type {
inline constexpr quint16 Flipchart = 1;
inline constexpr quint16 Page = 2;
inline constexpr quint16 Annotation = 3;
inline constexpr quint16 Line = 5;
inline constexpr quint16 Shape = 7;
inline constexpr quint16 Image = 9;
inline constexpr quint16 Text = 11;
inline constexpr quint16 Grid = 13;
inline constexpr quint16 GridLevel = 14;
inline constexpr quint16 Group = 18;
// Actions : numéros du format (§ 7.6), noms à nous.
inline constexpr quint16 ActionTool = 22;
inline constexpr quint16 ActionGoToPage = 23;
inline constexpr quint16 ActionShift = 24;
inline constexpr quint16 ActionProgram = 25;
inline constexpr quint16 ActionOpenFile = 26;
inline constexpr quint16 ActionTargetObject = 28;
inline constexpr quint16 Background = 29;
inline constexpr quint16 ActionTargetText = 50;
inline constexpr quint16 Connector = 52;
inline constexpr quint16 Label = 53;
inline constexpr quint16 Media = 253;
inline constexpr quint16 ActionLaunch = 463;
} // namespace type

inline constexpr bool isAction(quint16 t) noexcept
{
    return t == type::ActionTool || t == type::ActionGoToPage || t == type::ActionShift ||
           t == type::ActionProgram || t == type::ActionOpenFile || t == type::ActionTargetObject ||
           t == type::ActionTargetText || t == type::ActionLaunch;
}

namespace flipchart {
inline constexpr quint16 Name = 2;
inline constexpr quint16 ResourceRegister = 7;
inline constexpr quint16 Title = 12;
inline constexpr quint16 DefaultPageSize = 18;
} // namespace flipchart

namespace page {
inline constexpr quint16 Name = 2;
inline constexpr quint16 Notes = 5;
inline constexpr quint16 Size = 6;
inline constexpr quint16 GridVisible = 17;
} // namespace page

namespace background {
inline constexpr quint16 Kind = 2; // 0 encre, 1 image, 2 ressource
inline constexpr quint16 Ink = 3;
inline constexpr quint16 Image = 4;
} // namespace background

namespace grid {
inline constexpr quint16 ThumbnailScale = 13;
inline constexpr quint16 DefaultScale = 14;
inline constexpr quint16 ScaleStep = 15;
inline constexpr quint16 AllowSnap = 16;
inline constexpr quint16 OnTop = 17;
} // namespace grid

namespace gridlevel {
inline constexpr quint16 LineStyle = 2; // indice + 1 dans GridLine
inline constexpr quint16 Pattern = 3;   // indice + 1 dans GridPattern
inline constexpr quint16 Color = 4;
inline constexpr quint16 Visible = 5;
inline constexpr quint16 Start = 6;
inline constexpr quint16 StepX = 7;
inline constexpr quint16 StepY = 8;
inline constexpr quint16 Angle = 9;
inline constexpr quint16 DrawX = 11;
inline constexpr quint16 DrawY = 12;
inline constexpr quint16 Snap = 13;
} // namespace gridlevel

// Propriétés communes aux objets affichables (§ 7.1).
namespace object {
inline constexpr quint16 Transform = 2;
inline constexpr quint16 Name = 4;
inline constexpr quint16 Visible = 6;
inline constexpr quint16 BoundingRect = 7;
inline constexpr quint16 Layer = 8;
inline constexpr quint16 Depth = 9;
inline constexpr quint16 Ink = 10;
inline constexpr quint16 RotateOrigin = 12;
inline constexpr quint16 CanMove = 13;
inline constexpr quint16 CanSize = 14;
inline constexpr quint16 CanRotate = 15;
inline constexpr quint16 Locked = 18;
inline constexpr quint16 Label = 34; // sauf connecteur
} // namespace object

namespace annotation {
inline constexpr quint16 Translucency = 38;
inline constexpr quint16 Resource = 39;
} // namespace annotation

namespace shape {
inline constexpr quint16 Resource = 38;
inline constexpr quint16 FillMode = 39;
inline constexpr quint16 Open = 42;
inline constexpr quint16 LocalPoints = 49;
} // namespace shape

namespace line {
inline constexpr quint16 Resource = 38;
}

namespace image {
inline constexpr quint16 Resource = 38;
inline constexpr quint16 Transparent = 39;
inline constexpr quint16 TransparentColor = 40;
} // namespace image

namespace text {
inline constexpr quint16 Resource = 38;
}

namespace media {
inline constexpr quint16 Placeholder = 38;
inline constexpr quint16 AutoPlay = 41;
inline constexpr quint16 Loop = 42;
inline constexpr quint16 File = 45;
} // namespace media

namespace connector {
inline constexpr quint16 ObjectA = 34;
inline constexpr quint16 ObjectB = 35;
inline constexpr quint16 PointA = 39;
inline constexpr quint16 PointB = 40;
} // namespace connector

namespace transform {
inline constexpr quint16 Translate = 2;
inline constexpr quint16 Angle = 3;
inline constexpr quint16 Scale = 4;
inline constexpr quint16 Mirrored = 5; // miroir horizontal
inline constexpr quint16 Flipped = 6;  // miroir vertical
} // namespace transform

namespace origin {
inline constexpr quint16 Point = 2;
}

namespace ink {
inline constexpr quint16 Kind = 2; // 0 normal, 1 surligneur, 2 encre révélatrice
inline constexpr quint16 PenWidth = 3;
inline constexpr quint16 PenStyle = 4;
inline constexpr quint16 PenCap = 5;  // 0 plat, 16 carré, 32 rond
inline constexpr quint16 PenJoin = 6; // 0 angle, 64 biseau, 128 rond
inline constexpr quint16 PenColor = 7;
inline constexpr quint16 BrushStyle = 8; // 0 aucun, 1 plein, 2 à 14 motifs, 25 dégradé
inline constexpr quint16 BrushColor = 9;
inline constexpr quint16 BackgroundMode = 10;
inline constexpr quint16 BackgroundColor = 11;
inline constexpr quint16 BrushColor2 = 13;
inline constexpr quint16 Gradient = 14;
} // namespace ink

namespace label {
inline constexpr quint16 Caption = 2;
}

namespace resource {
inline constexpr quint16 Points = 4;       // annotation, ligne
inline constexpr quint16 Polygons = 4;     // forme
inline constexpr quint16 OriginalName = 3; // image, média
inline constexpr quint16 ImageSize = 5;
inline constexpr quint16 TextRuns = 4;
inline constexpr quint16 TextHtml = 5;
} // namespace resource

namespace run {
inline constexpr quint16 Bounds = 2;
inline constexpr quint16 Baseline = 3;
inline constexpr quint16 Color = 5;
inline constexpr quint16 Text = 6;
inline constexpr quint16 Family = 7;
inline constexpr quint16 PointSize = 8;
inline constexpr quint16 Weight = 9; // échelle Qt 4 (50 normal, 75 gras)
inline constexpr quint16 Italic = 10;
inline constexpr quint16 Underline = 11;
inline constexpr quint16 StrikeOut = 12;
} // namespace run

// Actions (§ 7.6) : commande et cible selon le type.
namespace action {
inline constexpr quint16 TargetObjectTarget = 2;
inline constexpr quint16 TargetObjectCommand = 5;
inline constexpr quint16 OpenFileResource = 2;
inline constexpr quint16 OpenFileTarget = 4;
inline constexpr quint16 OpenFileCommand = 10;
inline constexpr quint16 GoToPageCommand = 4;
inline constexpr quint16 ToolCommand = 3;
inline constexpr quint16 ShiftCommand = 3;
inline constexpr quint16 ProgramCommand = 4;
inline constexpr quint16 TargetTextTarget = 3;
inline constexpr quint16 TargetTextCommand = 6;
inline constexpr quint16 LaunchCommand = 4;
} // namespace action

} // namespace actcraft::formats::flipchart::ids
