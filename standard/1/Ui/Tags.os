// standard/1/Ui/Tags.os
// The tag table: name -> kind, default display, default style. Every HTML tag exists on day one; unknown tags
// become a plain inline box, so pages never crash on them. Rows are written as groups (space separated names),
// one group per line of the table in the guide. `// NEXT:` marks behaviour that is only a default row for now.

function tagSet(r: TagRow*, kind: int, disp: int, spec: int, ctl: int) => void {
    r.kind = kind; r.display = disp; r.spec = spec; r.ctl = ctl;
    r.scale = 1.0; r.mtEm = 0.0; r.mbEm = 0.0; r.mtPx = 0.0; r.mbPx = 0.0; r.mlPx = 0.0; r.mrPx = 0.0;
    r.plPx = 0.0; r.padPx = 0.0; r.bPx = 0.0;
    r.bold = false; r.italic = false; r.underline = false; r.strike = false; r.mono = false;
    r.ws = WS_NORMAL; r.ta = TA_LEFT; r.taSet = false; r.color = 0; r.bg = 0; r.known = true;
}

function tagBlock(r: TagRow*, spec: int) => void { tagSet(r, NK_BOX, DP_BLOCK, spec, CK_NONE); }
function tagInline(r: TagRow*, spec: int) => void { tagSet(r, NK_BOX, DP_INLINE, spec, CK_NONE); }
function tagHeading(r: TagRow*, scale: float, margin: float) => void {
    tagBlock(r, SP_NONE); r.scale = scale; r.mtEm = margin; r.mbEm = margin; r.bold = true;
}

// Fills r for `name` (lowercase). Returns false for an unknown tag (r then describes a generic inline box).
function tagLookup(name: char*, r: TagRow*) => bool {
    tagInline(r, SP_NONE);
    r.known = false;

    // ---- root / metadata ----
    if (inList(name, "html")) { tagBlock(r, SP_HTML); }
    else if (inList(name, "head title base link meta style script noscript template slot")) {
        // link/style/script are consumed by the browser. NEXT: slot (display: contents)
        tagSet(r, NK_HIDDEN, DP_NONE, SP_NONE, CK_NONE);
    }
    // ---- sections ----
    else if (inList(name, "body")) { tagBlock(r, SP_BODY); r.mtPx = 8.0; r.mbPx = 8.0; r.mlPx = 8.0; r.mrPx = 8.0; }
    else if (inList(name, "article section nav aside header footer main hgroup search")) { tagBlock(r, SP_NONE); }
    else if (inList(name, "address")) { tagBlock(r, SP_NONE); r.italic = true; }
    // ---- headings ----
    else if (inList(name, "h1")) { tagHeading(r, 2.0, 0.67); }
    else if (inList(name, "h2")) { tagHeading(r, 1.5, 0.83); }
    else if (inList(name, "h3")) { tagHeading(r, 1.17, 1.0); }
    else if (inList(name, "h4")) { tagHeading(r, 1.0, 1.33); }
    else if (inList(name, "h5")) { tagHeading(r, 0.83, 1.67); }
    else if (inList(name, "h6")) { tagHeading(r, 0.67, 2.33); }
    // ---- grouping ----
    else if (inList(name, "div figcaption")) { tagBlock(r, SP_NONE); }
    else if (inList(name, "p")) { tagBlock(r, SP_NONE); r.mtEm = 1.0; r.mbEm = 1.0; }
    else if (inList(name, "pre")) { tagBlock(r, SP_PRE); r.mtEm = 1.0; r.mbEm = 1.0; r.mono = true; r.ws = WS_PRE; }
    else if (inList(name, "blockquote figure")) { tagBlock(r, SP_NONE); r.mtEm = 1.0; r.mbEm = 1.0; r.mlPx = 40.0; r.mrPx = 40.0; }
    else if (inList(name, "hr")) { tagBlock(r, SP_HR); r.mtEm = 0.5; r.mbEm = 0.5; r.bPx = 1.0; }
    // ---- lists ----
    else if (inList(name, "ul menu")) { tagBlock(r, SP_UL); r.mtEm = 1.0; r.mbEm = 1.0; r.plPx = 40.0; }
    else if (inList(name, "ol")) { tagBlock(r, SP_OL); r.mtEm = 1.0; r.mbEm = 1.0; r.plPx = 40.0; }
    else if (inList(name, "li")) { tagBlock(r, SP_LI); }
    else if (inList(name, "dl")) { tagBlock(r, SP_NONE); r.mtEm = 1.0; r.mbEm = 1.0; }
    else if (inList(name, "dt")) { tagBlock(r, SP_NONE); r.bold = true; }
    else if (inList(name, "dd")) { tagBlock(r, SP_NONE); r.mlPx = 40.0; }
    // ---- breaks ----
    else if (inList(name, "br")) { tagInline(r, SP_BR); }
    else if (inList(name, "wbr")) { tagInline(r, SP_WBR); }
    // ---- text semantics (inline) ----
    else if (inList(name, "span bdi bdo ruby rt rp time data abbr dfn")) {
        tagInline(r, SP_NONE);
        if (inList(name, "dfn")) r.italic = true;
        if (inList(name, "rt")) r.scale = 0.5;       // NEXT: ruby positioning
    }
    else if (inList(name, "a")) { tagInline(r, SP_A); r.color = DEF_LINK; r.underline = true; }
    else if (inList(name, "strong b")) { tagInline(r, SP_NONE); r.bold = true; }
    else if (inList(name, "em i cite var")) { tagInline(r, SP_NONE); r.italic = true; }
    else if (inList(name, "u ins")) { tagInline(r, SP_NONE); r.underline = true; }
    else if (inList(name, "s del")) { tagInline(r, SP_NONE); r.strike = true; }
    else if (inList(name, "small sub sup")) { tagInline(r, SP_NONE); r.scale = 0.83; }   // NEXT: sub/sup baseline shift
    else if (inList(name, "mark")) { tagInline(r, SP_NONE); r.bg = DEF_MARK_BG; r.color = DEF_MARK_FG; }
    else if (inList(name, "code kbd samp")) { tagInline(r, SP_NONE); r.mono = true; }
    else if (inList(name, "q")) { tagInline(r, SP_NONE); }                                // NEXT: quote marks
    // ---- tables ----
    else if (inList(name, "table")) { tagSet(r, NK_BOX, DP_TABLE, SP_TABLE, CK_NONE); }
    else if (inList(name, "caption")) { tagBlock(r, SP_NONE); r.ta = TA_CENTER; r.taSet = true; }
    else if (inList(name, "colgroup col")) { tagSet(r, NK_HIDDEN, DP_NONE, SP_NONE, CK_NONE); }  // NEXT: column widths
    else if (inList(name, "thead tbody tfoot")) { tagSet(r, NK_BOX, DP_TABLE_GROUP, SP_NONE, CK_NONE); }
    else if (inList(name, "tr")) { tagSet(r, NK_BOX, DP_TABLE_ROW, SP_NONE, CK_NONE); }
    else if (inList(name, "td")) { tagSet(r, NK_BOX, DP_TABLE_CELL, SP_NONE, CK_NONE); r.padPx = 1.0; }
    else if (inList(name, "th")) {
        tagSet(r, NK_BOX, DP_TABLE_CELL, SP_NONE, CK_NONE); r.padPx = 1.0; r.bold = true; r.ta = TA_CENTER; r.taSet = true;
    }
    // ---- forms: controls ----
    else if (inList(name, "input")) { tagSet(r, NK_CONTROL, DP_INLINE_BLOCK, SP_INPUT, CK_TEXT); }
    else if (inList(name, "textarea")) { tagSet(r, NK_CONTROL, DP_INLINE_BLOCK, SP_TEXTAREA, CK_TEXTAREA); }  // NEXT: multi-line
    else if (inList(name, "button")) { tagSet(r, NK_CONTROL, DP_INLINE_BLOCK, SP_BUTTON, CK_BUTTON); }
    else if (inList(name, "select")) { tagSet(r, NK_CONTROL, DP_INLINE_BLOCK, SP_SELECT, CK_SELECT); }       // NEXT: popup list
    else if (inList(name, "progress")) { tagSet(r, NK_CONTROL, DP_INLINE_BLOCK, SP_PROGRESS, CK_PROGRESS); }
    else if (inList(name, "meter")) { tagSet(r, NK_CONTROL, DP_INLINE_BLOCK, SP_METER, CK_METER); }
    else if (inList(name, "option optgroup datalist")) { tagSet(r, NK_HIDDEN, DP_NONE, SP_OPTION, CK_NONE); }
    // ---- forms: structure ----
    else if (inList(name, "form legend")) { tagBlock(r, SP_FORM); }
    else if (inList(name, "label")) { tagInline(r, SP_LABEL); }
    else if (inList(name, "output")) { tagInline(r, SP_NONE); }
    else if (inList(name, "fieldset")) { tagBlock(r, SP_FIELDSET); r.bPx = 1.0; r.padPx = 6.0; r.mlPx = 2.0; r.mrPx = 2.0; }
    // ---- interactive ----
    else if (inList(name, "details")) { tagBlock(r, SP_DETAILS); }
    else if (inList(name, "summary")) { tagBlock(r, SP_SUMMARY); r.bold = true; r.plPx = 16.0; }
    else if (inList(name, "dialog")) { tagBlock(r, SP_DIALOG); r.padPx = 12.0; r.bPx = 1.0; r.bg = 0x2A2A33FF; }
    // ---- embedded ----
    else if (inList(name, "img picture")) { tagSet(r, NK_REPLACED, DP_INLINE_BLOCK, SP_IMG, CK_NONE); }
    else if (inList(name, "canvas")) { tagSet(r, NK_REPLACED, DP_INLINE_BLOCK, SP_CANVAS, CK_NONE); }    // NEXT: a Pen2D target
    else if (inList(name, "svg math iframe embed object video audio map")) {
        tagSet(r, NK_REPLACED, DP_INLINE_BLOCK, SP_NONE, CK_NONE);                                       // placeholder box
    }
    else if (inList(name, "source track area")) { tagSet(r, NK_HIDDEN, DP_NONE, SP_NONE, CK_NONE); }
    else { r.known = false; }
    return r.known;
}

// <input type=...> -> IT_*
function inputTypeOf(t: char*) => int {
    let r: int = IT_TEXT;
    if (t == nullptr) return IT_TEXT;
    if (strEqI(t, "password")) r = IT_PASSWORD;
    else if (strEqI(t, "search")) r = IT_SEARCH;
    else if (strEqI(t, "email")) r = IT_EMAIL;
    else if (strEqI(t, "number")) r = IT_NUMBER;
    else if (strEqI(t, "url")) r = IT_URL;
    else if (strEqI(t, "tel")) r = IT_TEL;
    else if (strEqI(t, "checkbox")) r = IT_CHECKBOX;
    else if (strEqI(t, "radio")) r = IT_RADIO;
    else if (strEqI(t, "range")) r = IT_RANGE;
    else if (strEqI(t, "button")) r = IT_BUTTON;
    else if (strEqI(t, "submit")) r = IT_SUBMIT;
    else if (strEqI(t, "reset")) r = IT_RESET;
    else if (strEqI(t, "hidden")) r = IT_HIDDEN;
    else if (strEqI(t, "file") || strEqI(t, "color") || strEqI(t, "date") || strEqI(t, "time")
          || strEqI(t, "datetime-local") || strEqI(t, "month") || strEqI(t, "week")) r = IT_STUB;   // NEXT: native pickers
    return r;
}

// IT_* -> CK_*
function ctlOfInput(it: int) => int {
    let k: int = CK_TEXT;
    if (it == IT_CHECKBOX) k = CK_CHECK;
    else if (it == IT_RADIO) k = CK_RADIO;
    else if (it == IT_RANGE) k = CK_RANGE;
    else if (it == IT_BUTTON || it == IT_SUBMIT || it == IT_RESET) k = CK_BUTTON;
    else if (it == IT_HIDDEN) k = CK_NONE;
    return k;
}
