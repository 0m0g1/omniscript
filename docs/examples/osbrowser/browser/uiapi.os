// browser/uiapi.os  -- the ONLY place the browser touches Ui internals that the usage guide does not document.
// If the first compile complains about Node fields, fix them here (one line each) and nothing else changes.
module UI = import "standard/1/Ui.os";

// ADAPT: the node's tag name field (char*, lowercase, e.g. "div").
function tagOf(n: UI.Node*) => char* {
    let t: char* = n.tag;
    if (t == nullptr) t = "";
    return t;
}

// ADAPT: text of a text node / control caption (guide: "the node's `text` field").
function textOf(n: UI.Node*) => char* {
    let t: char* = n.text;
    if (t == nullptr) t = "";
    return t;
}

// attribute value or "" (getAttr returns nullptr when missing)
function attr(n: UI.Node*, name: char*) => char* {
    let v: char* = n.getAttr(name);
    if (v == nullptr) v = "";
    return v;
}
