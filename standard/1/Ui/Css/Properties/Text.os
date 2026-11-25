// standard/1/Ui/Css/Properties/Text.os
// Property handlers: font-size, font-weight/style, text-align, text-decoration, line-height, white-space, rendering hints
// applyPropText returns AP_NOMATCH when `name` is not one of its properties, so Property.os tries the next group.
// To add a property: put one `else if (strEq(name, "...")) { ... }` branch here and the name in cssSupportedProps().

function applyPropText(st: Style*, name: char*, v: char*, ctx: CssCtx*, isFs: bool) => int {
    let res: int = AP_APPLIED;
    let l: Len; lenSet(&l, LU_AUTO, 0.0);
    let cr: CRes; cr.ok = false; cr.c = 0;
    let hit: bool = true;
    if (isFs) {
        if (!ctx.tui) {                                  // terminal: every glyph is one cell
            let fr: FsRes; fr.ok = false; fr.px = 0.0;
            parseFontSize(v, ctx, &fr);
            if (fr.ok) st.fontPx = fr.px; else res = AP_INVALID;
        }
    }
    else if (strEq(name, "font-weight")) {
        let lx: Lex; lx.s = v; lx.i = 0; lx.ok = false; lx.val = 0.0;
        if (strEqI(v, "bold") || strEqI(v, "bolder")) st.bold = true;
        else if (strEqI(v, "normal") || strEqI(v, "lighter")) st.bold = false;
        else { let w: float = lx.number(); if (lx.ok) st.bold = w >= 600.0; else res = AP_INVALID; }
    }
    else if (strEq(name, "font-style")) {
        st.italic = strEqI(v, "italic") || strEqI(v, "oblique");
        if (!st.italic && !strEqI(v, "normal")) res = AP_INVALID;
    }
    else if (strEq(name, "text-align")) {
        if (strEqI(v, "left") || strEqI(v, "start")) st.textAlign = TA_LEFT;
        else if (strEqI(v, "center")) st.textAlign = TA_CENTER;
        else if (strEqI(v, "right") || strEqI(v, "end")) st.textAlign = TA_RIGHT;
        else if (strEqI(v, "justify")) { st.textAlign = TA_JUSTIFY; res = AP_UNSUPPORTED; }   // NEXT: justify (left for now)
        else res = AP_INVALID;
    }
    else if (strEq(name, "text-decoration") || strEq(name, "text-decoration-line")) {
        st.underline = false; st.strike = false;
        let lx: Lex; lx.s = v; lx.i = 0; lx.ok = false; lx.val = 0.0;
        let buf: char* = CMem.malloc(48) as char*;
        while (lx.token(buf, 48)) {
            if (strEqI(buf, "underline")) st.underline = true;
            else if (strEqI(buf, "line-through")) st.strike = true;
        }
        CMem.free(buf as void*);
    }
    else if (strEq(name, "line-height")) {
        if (strEqI(v, "normal") || strEqI(v, "initial")) { st.lineH = 0.0; }
        else {
            let lx: Lex; lx.s = v; lx.i = 0; lx.ok = false; lx.val = 0.0;
            let m: float = lx.number();
            if (lx.ok && lx.atEnd()) { st.lineH = m * st.fontPx; }                   // unitless: multiple of the font size
            else if (parseLen(v, ctx, &l)) {
                if (l.u == LU_PX) st.lineH = l.v;
                else if (l.u == LU_PCT) st.lineH = st.fontPx * l.v / 100.0;
                else res = AP_INVALID;
            }
            else res = AP_INVALID;
        }
    }
    else if (strEq(name, "white-space")) {
        if (strEqI(v, "normal")) st.whiteSpace = WS_NORMAL;
        else if (strEqI(v, "nowrap")) st.whiteSpace = WS_NOWRAP;
        else if (strEqI(v, "pre")) st.whiteSpace = WS_PRE;
        else if (strEqI(v, "pre-wrap")) st.whiteSpace = WS_PREWRAP;
        else if (strEqI(v, "pre-line")) st.whiteSpace = WS_PRELINE;
        else res = AP_INVALID;
    }
    else if (inList(name, "text-rendering -webkit-font-smoothing -moz-osx-font-smoothing font-smooth text-size-adjust -webkit-text-size-adjust")) { }   // rendering hints: no visual effect here
    else if (strEq(name, "font-family")) { }                                     // accepted: every glyph comes from the one built-in atlas font, so the family list cannot change anything
    else { hit = false; }
    if (!hit) res = AP_NOMATCH;
    return res;
}
