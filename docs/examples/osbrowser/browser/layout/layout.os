// browser/layout/layout.os  -- TIER 3: block flow + inline word wrapping.
// Node doubles as the box for the draft (x,y,w,h). Words become Runs attached to their text node.
// NEXT: real Box tree, margin collapsing, inline-block, floats, flexbox, line baseline, bold/italic metrics,
//       use real font measuring (Font.glyph / pen.textWidth) instead of CHAR_W.
module U = import "browser/util.os";
module D = import "browser/dom/dom.os";

let CHAR_W: float = 0.55;          // average glyph advance as a fraction of font size  (ADAPT to your font)
let LINE_H: float = 1.25;

let curX: float = 0.0;
let curY: float = 0.0;
let lineH: float = 0.0;
let lineLeft: float = 0.0;
let lineW: float = 0.0;

function measure(s: char*, size: float) => float { return (U.slen(s) as float) * size * CHAR_W; }

function maxf(a: float, b: float) => float { if (a > b) return a; return b; }
function minf(a: float, b: float) => float { if (a < b) return a; return b; }

function newLine() => void {
    curY = curY + lineH;
    curX = lineLeft;
    lineH = 0.0;
}

function growBounds(nd: D.Node*, x: float, y: float, w: float, h: float) => void {
    if (!nd.hasBounds) { nd.x = x; nd.y = y; nd.w = w; nd.h = h; nd.hasBounds = true; }
    else {
        let x2: float = maxf(nd.x + nd.w, x + w);
        let y2: float = maxf(nd.y + nd.h, y + h);
        nd.x = minf(nd.x, x); nd.y = minf(nd.y, y);
        nd.w = x2 - nd.x; nd.h = y2 - nd.y;
    }
}

function addRun(tn: D.Node*, text: char*, size: float, color: u32) => void {
    let w: float = measure(text, size);
    let space: float = size * CHAR_W;
    if (curX > lineLeft) {
        if (curX + space + w > lineLeft + lineW) newLine(); else curX = curX + space;
    }
    let r: D.Run* = U.alloc(sizeof(D.Run)) as D.Run*;
    r.x = curX; r.y = curY; r.w = w; r.size = size; r.color = color; r.text = text;
    if (tn.runLast == nullptr) { tn.runFirst = r; tn.runLast = r; } else { tn.runLast.next = r; tn.runLast = r; }
    let lh: float = size * LINE_H;
    if (lh > lineH) lineH = lh;
    curX = curX + w;
    let p: D.Node* = tn.parent;
    while (p != nullptr && p.style.display == 2) { growBounds(p, r.x, r.y, w, lh); p = p.parent; }
}

function layoutText(tn: D.Node*) => void {
    tn.runFirst = nullptr; tn.runLast = nullptr;
    let t: char* = tn.text;
    let n: int = U.slen(t);
    let i: int = 0;
    while (i < n) {
        while (i < n && U.isSpace(t[i] as int)) { i = i + 1; }
        let a: int = i;
        while (i < n && !U.isSpace(t[i] as int)) { i = i + 1; }
        if (i > a) addRun(tn, U.sub(t, a, i), tn.parent.style.fontSize, tn.parent.style.color);
    }
}

function layoutInline(nd: D.Node*) => void {
    if (nd.kind == 2) { layoutText(nd); }
    else if (nd.style.display != 0) {
        nd.hasBounds = false;
        let c: D.Node* = nd.first;
        while (c != nullptr) { layoutChild(nd, c); c = c.next; }
    }
}

function layoutChild(parent: D.Node*, c: D.Node*) => void {
    let saveLeft: float = lineLeft;
    let saveW: float = lineW;
    if (c.kind == 2 || c.style.display == 2) { layoutInline(c); }
    else if (c.style.display == 1) {
        newLine();
        let top: float = curY;
        let used: float = layoutBlock(c, lineLeft, curY, lineW);
        lineLeft = saveLeft; lineW = saveW;
        curY = top + used; curX = lineLeft; lineH = 0.0;
    }
}

// lays out a block at (x,y) in available width; returns outer height consumed (incl. margins)
function layoutBlock(nd: D.Node*, x: float, y: float, availW: float) => float {
    let st: D.Style* = &nd.style;
    nd.x = x + st.margin;
    nd.y = y + st.margin;
    nd.hasBounds = true;
    nd.w = availW - 2.0 * st.margin;
    if (st.width >= 0.0) nd.w = st.width + 2.0 * (st.padding + st.border);
    let inset: float = st.padding + st.border;
    lineLeft = nd.x + inset;
    lineW = nd.w - 2.0 * inset;
    curX = lineLeft; curY = nd.y + inset; lineH = 0.0;
    if (U.streq(nd.tag, "li")) { curX = lineLeft; addRun(nd, "-", st.fontSize, st.color); }
    let innerL: float = lineLeft;
    let innerW: float = lineW;
    let c: D.Node* = nd.first;
    while (c != nullptr) { layoutChild(nd, c); c = c.next; }
    newLine();
    nd.h = curY - (nd.y + inset) + 2.0 * inset;
    if (st.height >= 0.0) nd.h = st.height + 2.0 * inset;
    lineLeft = innerL; lineW = innerW;
    return nd.y + nd.h + st.margin - y;
}

function layoutDocument(root: D.Node*, viewW: float) => float {
    return layoutBlock(root, 0.0, 0.0, viewW);
}
