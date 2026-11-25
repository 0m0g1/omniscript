// browser/uiapi.os -- the ONLY place the browser touches Ui Node internals (verified against Tree.os / Types.os).
// Every other file (html, js, web bindings, inspector) goes through these accessors.
module UI = import "standard/1/Ui.os";

function tagOf(n: UI.Node*) => char* { let t: char* = n.tag; if (t == nullptr) t = ""; return t; }
function textOf(n: UI.Node*) => char* { let t: char* = n.text; if (t == nullptr) t = ""; return t; }
function attr(n: UI.Node*, name: char*) => char* { let v: char* = n.getAttr(name); if (v == nullptr) v = ""; return v; }

function isText(n: UI.Node*) => bool { return n.kind == UI.NK_TEXT; }
function parentOf(n: UI.Node*) => UI.Node* { return n.parent; }
function firstOf(n: UI.Node*) => UI.Node* { return n.first; }
function lastOf(n: UI.Node*) => UI.Node* { return n.last; }
function nextOf(n: UI.Node*) => UI.Node* { return n.next; }
function prevOf(n: UI.Node*) => UI.Node* { return n.prev; }
function idOf(n: UI.Node*) => char* { let t: char* = n.id; if (t == nullptr) t = ""; return t; }
function classOf(n: UI.Node*) => char* { let t: char* = n.cls; if (t == nullptr) t = ""; return t; }

// geometry in layout units (document coordinates, border box); valid after ui.frame()
function boxX(n: UI.Node*) => float { return n.box.x; }
function boxY(n: UI.Node*) => float { return n.box.y; }
function boxW(n: UI.Node*) => float { return n.box.w; }
function boxH(n: UI.Node*) => float { return n.box.h; }

// state
function checkedOf(n: UI.Node*) => bool { return n.checked; }
function disabledOf(n: UI.Node*) => bool { return n.disabled; }
function focusOf(n: UI.Node*) => bool { return n.focus; }
function valueOf(n: UI.Node*) => char* { let t: char* = n.text; if (t == nullptr) t = ""; return t; }
function selIndexOf(n: UI.Node*) => int { return n.selIndex; }

// identity: JS wrapper object hangs off a node. Tree.os has no userData field yet (P0 Ui change), so
// the web layer keeps a table keyed by node pointer until Node.userData exists (see js/web/dom.os).
function isSame(a: UI.Node*, b: UI.Node*) => bool { return a == b; }

// ---- style diagnostics (Css.os StyleNote list, filled while ui.recordStyles is true) ----
// ADAPT: StyleNote field names / status numbers. Spec used: prop, value, selector, source (0 tag default, 1 stylesheet, 2 inline),
// status (0 applied, 1 unsupported, 2 invalid, 3 overridden), next.
let SN_APPLIED: int = 0;
let SN_UNSUPPORTED: int = 1;
let SN_INVALID: int = 2;
let SN_OVERRIDDEN: int = 3;

function notesOf(n: UI.Node*) => UI.StyleNote* { return n.styleNotes; }
function noteNext(s: UI.StyleNote*) => UI.StyleNote* { return s.next; }
function noteProp(s: UI.StyleNote*) => char* { let t: char* = s.prop; if (t == nullptr) t = ""; return t; }
function noteValue(s: UI.StyleNote*) => char* { let t: char* = s.value; if (t == nullptr) t = ""; return t; }
function noteSel(s: UI.StyleNote*) => char* { let t: char* = s.selector; if (t == nullptr) t = ""; return t; }
function noteSource(s: UI.StyleNote*) => int { return s.source; }
function noteStatus(s: UI.StyleNote*) => int { return s.status; }
