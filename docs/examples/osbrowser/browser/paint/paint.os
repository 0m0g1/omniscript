// browser/paint/paint.os  -- TIER 4: Node tree -> backend-neutral display list.
// NEXT: z-order/stacking contexts, clipping/overflow, border radius, images, text decoration, opacity.
module U = import "browser/util.os";
module D = import "browser/dom/dom.os";

let CMD_RECT: int = 0;
let CMD_TEXT: int = 1;

struct Cmd {
    kind: int = 0;
    x: float = 0.0; y: float = 0.0; w: float = 0.0; h: float = 0.0;
    color: u32 = 0;
    text: char* = nullptr;
    size: float = 16.0;
    next: Cmd* = nullptr;
}

let head: Cmd* = nullptr;
let tail: Cmd* = nullptr;

function push(c: Cmd*) => void {
    if (tail == nullptr) { head = c; tail = c; } else { tail.next = c; tail = c; }
}

function rect(x: float, y: float, w: float, h: float, color: u32) => void {
    let c: Cmd* = U.alloc(sizeof(Cmd)) as Cmd*;
    c.kind = 0; c.x = x; c.y = y; c.w = w; c.h = h; c.color = color;
    push(c);
}

function text(x: float, y: float, s: char*, size: float, color: u32) => void {
    let c: Cmd* = U.alloc(sizeof(Cmd)) as Cmd*;
    c.kind = 1; c.x = x; c.y = y; c.text = s; c.size = size; c.color = color;
    push(c);
}

function paintNode(nd: D.Node*) => void {
    if (nd.kind == 1 && nd.style.display == 0) { }
    else {
        if (nd.kind == 1 && nd.style.display == 1) {
            let st: D.Style* = &nd.style;
            if (st.bgOn) rect(nd.x, nd.y, nd.w, nd.h, st.bg);
            if (st.border > 0.0) {
                let b: float = st.border;
                rect(nd.x, nd.y, nd.w, b, st.borderColor);
                rect(nd.x, nd.y + nd.h - b, nd.w, b, st.borderColor);
                rect(nd.x, nd.y, b, nd.h, st.borderColor);
                rect(nd.x + nd.w - b, nd.y, b, nd.h, st.borderColor);
            }
        } else if (nd.kind == 1 && nd.style.bgOn && nd.hasBounds) {
            rect(nd.x, nd.y, nd.w, nd.h, nd.style.bg);           // inline background
        }
        let r: D.Run* = nd.runFirst;
        while (r != nullptr) { text(r.x, r.y, r.text, r.size, r.color); r = r.next; }
        let c: D.Node* = nd.first;
        while (c != nullptr) { paintNode(c); c = c.next; }
    }
}

// returns head of the list; viewport background first
function buildDisplayList(root: D.Node*, viewW: float, pageH: float) => Cmd* {
    head = nullptr; tail = nullptr;
    let bodyN: D.Node* = D.findTag(root, "body");
    let bg: u32 = 0xFFFFFFFF;
    if (bodyN != nullptr && bodyN.style.bgOn) bg = bodyN.style.bg;
    rect(0.0, 0.0, viewW, pageH, bg);
    paintNode(root);
    return head;
}

// hit test: deepest node under (x,y) in page coords; used for events
function hitTest(nd: D.Node*, x: float, y: float) => D.Node* {
    let found: D.Node* = nullptr;
    if (nd.kind == 1 && nd.style.display != 0) {
        let c: D.Node* = nd.first;
        while (c != nullptr) {
            let h: D.Node* = hitTest(c, x, y);
            if (h != nullptr) found = h;
            c = c.next;
        }
        if (found == nullptr && nd.hasBounds && x >= nd.x && x <= nd.x + nd.w && y >= nd.y && y <= nd.y + nd.h) found = nd;
    }
    return found;
}
