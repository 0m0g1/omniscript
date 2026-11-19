// standard/1/Ui.os
// Tier 7: HTML-grade UI library. One retained tree + style + layout + events + controls.
//   Canvas (draw + input)  ->  Ui (tree, style, layout, events, controls)  ->  Browser and apps
//
//   Node      one element or text run. Tree links, attributes, computed Style, layout Box, control state.
//   Tags      the HTML tag table (~115 elements): tag name -> kind, default display, default style.
//   Css       CSS text parser, selectors, cascade, property application.
//   Layout    block + inline flow, flex, table, absolute/relative/fixed (one engine, `Layout`).
//   Paint     Node tree -> display list (rects, borders, text, clips, images). The display list is the seam.
//   Events    hit test, hover, focus, click/key/input/change; a poll queue (ui.poll) and per-node flags.
//   Controls  button/input/checkbox/radio/range wrap Pen2D widgets; progress/meter/details/dialog are native.
//   Backend   Gui (display list -> Pen2D) and Tui (display list -> character grid + ANSI text).
//
// Ui never touches GL. It only talks to Canvas (Pen2D + Win). Canvas never imports Ui.
// Units: layout runs in floats. Metrics.unit = layout units per CSS px (GUI 1.0, TUI 0.125 = 8px per cell).
// Colours are 0xRRGGBBAA. Not compiled/run yet. No `return` inside loop bodies (compiler quirk).
module C = import "standard/1/Canvas.os";
module CMem = import "standard/1/c/mem.os";

// `include` pastes each file in place (order matters). Files marked (fragment) are pasted INSIDE a struct body.
include "standard/1/Ui/Constants.os";
include "standard/1/Ui/Util.os";
include "standard/1/Ui/Types.os";
include "standard/1/Ui/Tags.os";
include "standard/1/Ui/Tree.os";                  // struct Node; pastes Api.os (fragment) into its body
include "standard/1/Ui/Css.os";
include "standard/1/Ui/Measure.os";
include "standard/1/Ui/Controls/Control.os";
include "standard/1/Ui/Layout/Block.os";          // struct Layout; pastes Flex.os, Table.os, Abs.os (fragments) into its body
include "standard/1/Ui/Paint.os";
include "standard/1/Ui/Events.os";
include "standard/1/Ui/Backend/Gui.os";
include "standard/1/Ui/Backend/Tui.os";
include "standard/1/Ui/Root.os";
