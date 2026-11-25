// standard/1/Ui/Css/Diagnostics/Notes.os
// StyleNote recording (one note per declaration that targeted a node) for the inspector.
// StyleNote itself is defined in Types.os.

function freeStyleNotes(s0: StyleNote*) => void {
    let s: StyleNote* = s0;
    while (s != nullptr) {
        let nx: StyleNote* = s.next;
        strFree(s.prop); strFree(s.value); strFree(s.selector);
        CMem.free(s as void*);
        s = nx;
    }
}

function noteAdd(n: Node*, prop: char*, value: char*, source: int, status: int, selector: char*) => void {
    let s: StyleNote* = CMem.malloc(sizeof(StyleNote)) as StyleNote*;
    s.prop = strDup(prop); s.value = strDup(value); s.source = source; s.status = status;
    s.selector = nullptr;
    if (selector != nullptr) s.selector = strDup(selector);
    s.next = nullptr;
    if (n.styleNotes == nullptr) { n.styleNotes = s; }
    else { let t: StyleNote* = n.styleNotes; while (t.next != nullptr) t = t.next; t.next = s; }
}

function displayName(d: int) => char* {
    let r: char* = "inline";
    if (d == DP_NONE) r = "none";
    else if (d == DP_BLOCK) r = "block";
    else if (d == DP_INLINE_BLOCK) r = "inline-block";
    else if (d == DP_FLEX) r = "flex";
    else if (d == DP_TABLE) r = "table";
    else if (d == DP_TABLE_GROUP) r = "table-row-group";
    else if (d == DP_TABLE_ROW) r = "table-row";
    else if (d == DP_TABLE_CELL) r = "table-cell";
    else if (d == DP_GRID) r = "grid";
    return r;
}

// source 0: what the tag row set (numeric defaults are listed as "default": no number formatting here)
function noteTagDefaults(n: Node*) => void {
    let row: TagRow* = &n.row;
    noteAdd(n, "display", displayName(row.display), SRC_TAG, SN_APPLIED, n.tag);
    if (row.bold) noteAdd(n, "font-weight", "bold", SRC_TAG, SN_APPLIED, n.tag);
    if (row.italic) noteAdd(n, "font-style", "italic", SRC_TAG, SN_APPLIED, n.tag);
    if (row.underline) noteAdd(n, "text-decoration", "underline", SRC_TAG, SN_APPLIED, n.tag);
    if (row.strike) noteAdd(n, "text-decoration", "line-through", SRC_TAG, SN_APPLIED, n.tag);
    if (row.ws != WS_NORMAL) noteAdd(n, "white-space", "pre", SRC_TAG, SN_APPLIED, n.tag);
    if (row.taSet) noteAdd(n, "text-align", "center", SRC_TAG, SN_APPLIED, n.tag);
    if (row.scale != 1.0) noteAdd(n, "font-size", "default", SRC_TAG, SN_APPLIED, n.tag);
    if (row.mtEm != 0.0 || row.mtPx != 0.0) noteAdd(n, "margin-top", "default", SRC_TAG, SN_APPLIED, n.tag);
    if (row.mbEm != 0.0 || row.mbPx != 0.0) noteAdd(n, "margin-bottom", "default", SRC_TAG, SN_APPLIED, n.tag);
    if (row.mlPx != 0.0) noteAdd(n, "margin-left", "default", SRC_TAG, SN_APPLIED, n.tag);
    if (row.mrPx != 0.0) noteAdd(n, "margin-right", "default", SRC_TAG, SN_APPLIED, n.tag);
    if (row.plPx != 0.0 || row.padPx != 0.0) noteAdd(n, "padding", "default", SRC_TAG, SN_APPLIED, n.tag);
    if (row.bPx > 0.0) noteAdd(n, "border", "default", SRC_TAG, SN_APPLIED, n.tag);
    if (row.color != 0) noteAdd(n, "color", "default", SRC_TAG, SN_APPLIED, n.tag);
    if (row.bg != 0) noteAdd(n, "background-color", "default", SRC_TAG, SN_APPLIED, n.tag);
}

// true when declaring `later` also sets (and so replaces) `earlier`
function propCovers(later: char*, earlier: char*) => bool {
    if (strEq(later, earlier)) return true;
    let r: bool = false;
    if (inList(later, "margin padding border border-top border-right border-bottom border-left flex background overflow text-decoration list-style font transition")) {
        let pre: char* = strCat(later, "-");
        r = startsWith(earlier, pre);
        strFree(pre);
    }
    else if (strEq(later, "inset")) { r = inList(earlier, "top right bottom left"); }
    else if (strEq(later, "border-width")) { r = inList(earlier, "border-top-width border-right-width border-bottom-width border-left-width"); }
    else if (strEq(later, "margin-inline")) { r = inList(earlier, "margin-left margin-right margin-inline-start margin-inline-end"); }
    else if (strEq(later, "margin-block")) { r = inList(earlier, "margin-top margin-bottom margin-block-start margin-block-end"); }
    else if (strEq(later, "padding-inline")) { r = inList(earlier, "padding-left padding-right padding-inline-start padding-inline-end"); }
    else if (strEq(later, "padding-block")) { r = inList(earlier, "padding-top padding-bottom padding-block-start padding-block-end"); }
    else if (strEq(later, "flex-flow")) { r = inList(earlier, "flex-direction flex-wrap"); }
    else if (strEq(later, "gap") || strEq(later, "grid-gap")) { r = inList(earlier, "row-gap column-gap grid-row-gap grid-column-gap"); }
    return r;
}

// An applied note followed by an applied note of the same (or a covering) property was overridden.
function noteResolveOverrides(n: Node*) => void {
    let a: StyleNote* = n.styleNotes;
    while (a != nullptr) {
        if (a.status == SN_APPLIED) {
            let b: StyleNote* = a.next; let hit: bool = false;
            while (b != nullptr && !hit) {
                if (b.status != SN_INVALID && b.status != SN_OVERRIDDEN && propCovers(b.prop, a.prop)) hit = true;
                b = b.next;
            }
            if (hit) a.status = SN_OVERRIDDEN;
        }
        a = a.next;
    }
}
