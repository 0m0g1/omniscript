// browser/dom/dom.os  -- TIER 1: the document tree + the computed-style/geometry slots layers fill in.
// NEXT: attributes map, classList, querySelector, mutation observers, separate Box tree from Node.
module U = import "browser/util.os";
module CMem = import "standard/1/c/mem.os";

struct Style {
    color: u32 = 0x000000FF;
    bg: u32 = 0;
    bgOn: bool = false;
    borderColor: u32 = 0x000000FF;
    fontSize: float = 16.0;
    display: int = 1;            // 0 none, 1 block, 2 inline
    margin: float = 0.0;         // NEXT: per-side
    padding: float = 0.0;
    border: float = 0.0;
    width: float = 0.0 - 1.0;    // -1 = auto
    height: float = 0.0 - 1.0;
}

struct Run {                     // one laid-out word
    x: float = 0.0; y: float = 0.0; w: float = 0.0; size: float = 16.0;
    color: u32 = 0x000000FF;
    text: char* = nullptr;
    next: Run* = nullptr;
}

struct Node {
    kind: int = 1;               // 1 element, 2 text
    tag: char* = nullptr;
    text: char* = nullptr;
    id: char* = nullptr;
    cls: char* = nullptr;
    attrStyle: char* = nullptr;  // style="" attribute
    inlineStyle: char* = nullptr;// written by JS (el.style.x = ...)
    parent: Node* = nullptr;
    first: Node* = nullptr;
    last: Node* = nullptr;
    next: Node* = nullptr;
    style: Style;
    x: float = 0.0; y: float = 0.0; w: float = 0.0; h: float = 0.0;
    hasBounds: bool = false;
    runFirst: Run* = nullptr;
    runLast: Run* = nullptr;
    onClick: int = 0 - 1;        // JS function index or -1
}

// ADAPT: if the language has no sizeof, replace sizeof(Node) with a literal byte count.
function newNode(kind: int) => Node* {
    let n: Node* = U.alloc(sizeof(Node)) as Node*;
    n.kind = kind;
    n.onClick = 0 - 1;
    n.style.fontSize = 16.0;
    n.style.display = 1;
    n.style.width = 0.0 - 1.0;
    n.style.height = 0.0 - 1.0;
    n.style.color = 0x000000FF;
    n.tag = "";
    n.text = "";
    n.id = "";
    n.cls = "";
    n.attrStyle = "";
    n.inlineStyle = "";
    return n;
}

function newElement(tag: char*) => Node* { let n: Node* = newNode(1); n.tag = tag; return n; }
function newText(t: char*) => Node* { let n: Node* = newNode(2); n.text = t; return n; }

function append(p: Node*, c: Node*) => void {
    c.parent = p;
    if (p.last == nullptr) { p.first = c; p.last = c; }
    else { p.last.next = c; p.last = c; }
}

function clearChildren(p: Node*) => void { p.first = nullptr; p.last = nullptr; }

function getById(n: Node*, id: char*) => Node* {
    let found: Node* = nullptr;
    if (n.kind == 1 && U.streq(n.id, id)) found = n;
    let c: Node* = n.first;
    while (c != nullptr && found == nullptr) {
        found = getById(c, id);
        c = c.next;
    }
    return found;
}

function findTag(n: Node*, tag: char*) => Node* {
    let found: Node* = nullptr;
    if (n.kind == 1 && U.streq(n.tag, tag)) found = n;
    let c: Node* = n.first;
    while (c != nullptr && found == nullptr) {
        found = findTag(c, tag);
        c = c.next;
    }
    return found;
}
