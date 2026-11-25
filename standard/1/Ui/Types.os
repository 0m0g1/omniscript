// standard/1/Ui/Types.os
// Plain data structs. Heap objects are malloc + memset(0) (struct field defaults do not run), so every struct
// that is heap-allocated has an explicit reset()/init step. Node and Event live in Tree.os (they need Node).

// Layout units. GUI: cellW 8, cellH 16, lineH 18, unit 1.0. TUI: cellW = cellH = lineH = 1, unit 0.125 (8px per cell).
struct Metrics {
    cellW: float = 8.0; cellH: float = 16.0; lineH: float = 18.0;
    unit: float = 1.0;                 // layout units per CSS px
}

// A CSS length after parsing. v is in layout units (px, em, rem, vw, vh already converted) except LU_PCT (percent).
struct Len { v: float = 0.0; u: int = 0; }

function lenSet(l: Len*, u: int, v: float) => void { l.u = u; l.v = v; }

// Computed style. Colours 0xRRGGBBAA; alpha 0 background = none.
struct Style {
    display: int = 0; position: int = 0;
    color: u32 = 0; bg: u32 = 0; borderColor: u32 = 0;
    fontPx: float = 16.0;
    bold: bool = false; italic: bool = false; underline: bool = false; strike: bool = false;
    textAlign: int = 0; lineH: float = 0.0;      // lineH 0 = normal (fontPx * Metrics.lineH / cellH)
    whiteSpace: int = 0; hidden: bool = false;   // hidden = visibility:hidden
    width: Len; height: Len; minW: Len; maxW: Len; minH: Len; maxH: Len;
    mt: Len; mr: Len; mb: Len; ml: Len;
    pt: Len; pr: Len; pb: Len; pl: Len;
    bt: float = 0.0; br: float = 0.0; bb: float = 0.0; bl: float = 0.0;
    radius: float = 0.0;               // CM_FILL / CM_BORDER corner radius (px); <= 0 is a plain rectangle
    boxSizing: int = 0; overflow: int = 0;
    aspect: float = 0.0;               // aspect-ratio as width / height; 0 = auto (Block.placeBlock derives an auto height from the width)
    shOn: bool = false; shInset: bool = false;   // one box-shadow: lengths below are layout units; Painter only draws blur 0
    shX: float = 0.0; shY: float = 0.0; shBlur: float = 0.0; shSpread: float = 0.0; shColor: u32 = 0;
    opacity: float = 1.0; zIndex: int = 0;
    top: Len; right: Len; bottom: Len; left: Len;
    flexDir: int = 0; wrap: bool = false; grow: float = 0.0; shrink: float = 1.0; basis: Len;
    gap: float = 0.0; alignItems: int = 0; alignSelf: int = 9; justify: int = 0;
    gn: int = 0;                                  // grid-template-columns: track count (max 12); track i is g<i>: LU_PX / LU_PCT fixed, LU_AUTO = flexible (v = fr weight)
    g0: Len; g1: Len; g2: Len; g3: Len; g4: Len; g5: Len; g6: Len; g7: Len; g8: Len; g9: Len; g10: Len; g11: Len;
    cursor: int = 0; listStyle: int = 0;

    reset() => void {
        this.display = DP_INLINE; this.position = PS_STATIC;
        this.color = DEF_TEXT; this.bg = 0; this.borderColor = DEF_RULE;
        this.fontPx = 16.0;
        this.bold = false; this.italic = false; this.underline = false; this.strike = false;
        this.textAlign = TA_LEFT; this.lineH = 0.0; this.whiteSpace = WS_NORMAL; this.hidden = false;
        lenSet(&this.width, LU_AUTO, 0.0); lenSet(&this.height, LU_AUTO, 0.0);
        lenSet(&this.minW, LU_AUTO, 0.0); lenSet(&this.minH, LU_AUTO, 0.0);
        lenSet(&this.maxW, LU_NONE, 0.0); lenSet(&this.maxH, LU_NONE, 0.0);
        lenSet(&this.mt, LU_PX, 0.0); lenSet(&this.mr, LU_PX, 0.0); lenSet(&this.mb, LU_PX, 0.0); lenSet(&this.ml, LU_PX, 0.0);
        lenSet(&this.pt, LU_PX, 0.0); lenSet(&this.pr, LU_PX, 0.0); lenSet(&this.pb, LU_PX, 0.0); lenSet(&this.pl, LU_PX, 0.0);
        this.bt = 0.0; this.br = 0.0; this.bb = 0.0; this.bl = 0.0;
        this.radius = 0.0; this.boxSizing = 0; this.overflow = OV_VISIBLE;
        this.aspect = 0.0;
        this.shOn = false; this.shInset = false; this.shX = 0.0; this.shY = 0.0; this.shBlur = 0.0; this.shSpread = 0.0; this.shColor = 0;
        this.opacity = 1.0; this.zIndex = 0;
        lenSet(&this.top, LU_AUTO, 0.0); lenSet(&this.right, LU_AUTO, 0.0);
        lenSet(&this.bottom, LU_AUTO, 0.0); lenSet(&this.left, LU_AUTO, 0.0);
        this.flexDir = FD_ROW; this.wrap = false; this.grow = 0.0; this.shrink = 1.0; lenSet(&this.basis, LU_AUTO, 0.0);
        this.gn = 0;
        lenSet(&this.g0, LU_AUTO, 1.0); lenSet(&this.g1, LU_AUTO, 1.0); lenSet(&this.g2, LU_AUTO, 1.0); lenSet(&this.g3, LU_AUTO, 1.0);
        lenSet(&this.g4, LU_AUTO, 1.0); lenSet(&this.g5, LU_AUTO, 1.0); lenSet(&this.g6, LU_AUTO, 1.0); lenSet(&this.g7, LU_AUTO, 1.0);
        lenSet(&this.g8, LU_AUTO, 1.0); lenSet(&this.g9, LU_AUTO, 1.0); lenSet(&this.g10, LU_AUTO, 1.0); lenSet(&this.g11, LU_AUTO, 1.0);
        this.gap = 0.0; this.alignItems = AL_STRETCH; this.alignSelf = AL_AUTO; this.justify = JC_START;
        this.cursor = CU_DEFAULT; this.listStyle = LS_DISC;
    }

    // CSS inherited properties
    inheritFrom(p: Style*) => void {
        this.color = p.color; this.fontPx = p.fontPx;
        this.bold = p.bold; this.italic = p.italic; this.underline = p.underline; this.strike = p.strike;
        this.textAlign = p.textAlign; this.lineH = p.lineH; this.whiteSpace = p.whiteSpace;
        this.hidden = p.hidden; this.cursor = p.cursor; this.listStyle = p.listStyle;
    }
}

// Resolved box in layout units, absolute document coordinates.
struct Box {
    x: float = 0.0; y: float = 0.0; w: float = 0.0; h: float = 0.0;          // border box
    cx: float = 0.0; cy: float = 0.0; cw: float = 0.0; ch: float = 0.0;      // content box (ch < 0 while height is auto)
    ml: float = 0.0; mr: float = 0.0; mt: float = 0.0; mb: float = 0.0;
    pl: float = 0.0; pr: float = 0.0; pt: float = 0.0; pb: float = 0.0;
    bl: float = 0.0; br: float = 0.0; bt: float = 0.0; bb: float = 0.0;
    autoL: bool = false; autoR: bool = false;
    contentH: float = 0.0;             // scrollable content height
}

// One placed piece of a text run (a slice of node.ctext), or an atomic inline box (atom != nullptr).
// `next` chains a text node's frags; `lnext` chains one inline run during layout.
struct Frag {
    x: float = 0.0; y: float = 0.0; w: float = 0.0; h: float = 0.0;
    from: int = 0; len: int = 0; line: int = 0;
    atom: void* = nullptr;             // Node* for atomic inline items
    next: Frag* = nullptr; lnext: Frag* = nullptr;
}

struct Attr { name: char* = nullptr; value: char* = nullptr; next: Attr* = nullptr; }

// Per-node event handler slot. fn = function pointer, user = anything the app wants back. See Events.os (invoke).
struct Handler { fn: void* = nullptr; user: void* = nullptr; }

// One row of the tag table (Tags.os). Margins: *Em scale with the element font size, *Px with Metrics.unit.
struct TagRow {
    kind: int = 0; display: int = 0; spec: int = 0; ctl: int = 0;
    scale: float = 1.0;                // font-size multiplier of the parent font size
    mtEm: float = 0.0; mbEm: float = 0.0;
    mtPx: float = 0.0; mbPx: float = 0.0; mlPx: float = 0.0; mrPx: float = 0.0;
    plPx: float = 0.0; padPx: float = 0.0; bPx: float = 0.0;
    bold: bool = false; italic: bool = false; underline: bool = false; strike: bool = false; mono: bool = false;
    ws: int = 0; ta: int = 0; taSet: bool = false;
    color: u32 = 0; bg: u32 = 0;
    known: bool = true;
}

// Context for CSS length conversion.
struct CssCtx {
    unit: float = 1.0;                 // layout units per CSS px
    curPx: float = 16.0;               // font size used for em (own font size, or the parent's for font-size itself)
    rootPx: float = 16.0;
    vw: float = 0.0; vh: float = 0.0;
    tui: bool = false;                 // font-size is ignored in a terminal (every glyph is one cell)
    parent: Style* = nullptr;          // parent element's computed style, for `inherit` (nullptr at the root)
}

// Compound selector. Stored right-to-left: `left` is the compound to the left, `comb` the combinator between.
struct Compound {
    tag: char* = nullptr; id: char* = nullptr;
    cls: char* = nullptr;              // required classes, space separated
    attr: char* = nullptr; attrVal: char* = nullptr; attrOp: int = 0;
    pseudo: int = 0; never: bool = false;
    comb: int = 4; left: Compound* = nullptr;
}

struct Rule {
    sel: Compound* = nullptr; decls: Attr* = nullptr;
    spec: int = 0; seq: int = 0;
    next: Rule* = nullptr;
    selText: char* = nullptr;          // selector as written (inspector); may be nullptr
}

// One declaration that targeted a node and how it was handled (node.styleNotes; constants SN_* / SRC_* are in Css.os).
struct StyleNote {
    prop: char* = nullptr; value: char* = nullptr; source: int = 0; status: int = 0;
    selector: char* = nullptr; next: StyleNote* = nullptr;
}

// Rules sorted by (specificity, source order): iterating head -> tail applies them in cascade order.
struct Sheet {
    head: Rule* = nullptr; seq: int = 0;
    usesPseudo: bool = false;          // some rule has :hover/:focus/:active/:checked, so state changes must restyle
}

// Display-list command. Which fields matter depends on `op`.
struct Cmd {
    op: int = 0;
    x: float = 0.0; y: float = 0.0; w: float = 0.0; h: float = 0.0;
    color: u32 = 0;
    text: char* = nullptr; tfrom: int = 0; tlen: int = 0;     // CM_TEXT slice; CM_MARK: tfrom = list style, tlen = index; CM_IMAGE: tfrom = image id
    size: float = 16.0; flags: int = 0;
    bt: float = 0.0; br: float = 0.0; bb: float = 0.0; bl: float = 0.0;
    radius: float = 0.0;               // CM_FILL / CM_BORDER corner radius (px); <= 0 is a plain rectangle
}
