// standard/1/Ui/Css/Core/State.os
// All Css-wide mutable state in one place (globals are set by the cascade / sheet parser, never by app code,
// except the two setters cssSetRecord and cssSetMedia).

// ---- diagnostics ----
// ui.recordStyles mirrors into this flag before each cascade (see cssSetRecord).
let gRecordStyles: bool = false;
function cssSetRecord(on: bool) => void { gRecordStyles = on; }

// ---- parser flags (set while parsing declarations) ----
let gSawImportant: bool = false;     // some declaration carried !important: the cascade runs the important pass
let gSheetHasVars: bool = false;     // some declaration was a custom property (--x): the cascade tracks var() scopes
let gImportantPass: bool = false;    // true only inside Cascade/Important.os

// ---- custom properties: one scope per element that declares --x, linked to the nearest ancestor scope ----
struct VarScope { props: Attr*; up: VarScope*; }
struct AttrList { head: Attr*; tail: Attr*; }
let gScope: VarScope* = nullptr;
let gCascadeDepth: int = 0;

// ---- media environment (evaluated when a sheet is parsed) ----
let gMediaW: float = 1280.0;         // viewport width in CSS px
let gMediaH: float = 800.0;
let gMediaDark: bool = true;         // prefers-color-scheme: dark (set false for a light page)
let gMediaDpr: float = 1.0;
// Call before sheetParse; re-parse the author sheets when any of these change.
function cssSetMedia(w: float, h: float, dark: bool) => void { gMediaW = w; gMediaH = h; gMediaDark = dark; }
