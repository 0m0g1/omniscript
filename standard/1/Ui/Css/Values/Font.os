// standard/1/Ui/Css/Values/Font.os
// font-size values and the `font` shorthand.

// font-size value (keyword, length or %) -> px. Shared by `font-size` and the `font` shorthand.
struct FsRes { ok: bool = false; px: float = 0.0; }

function parseFontSize(v: char*, ctx: CssCtx*, out: FsRes*) => void {
    let px: float = 0.0; let have: bool = false;
    let l: Len; lenSet(&l, LU_AUTO, 0.0);
    if (strEqI(v, "xx-small")) { px = 9.0 * ctx.unit; have = true; }
    else if (strEqI(v, "x-small")) { px = 10.0 * ctx.unit; have = true; }
    else if (strEqI(v, "small")) { px = 13.0 * ctx.unit; have = true; }
    else if (strEqI(v, "medium")) { px = 16.0 * ctx.unit; have = true; }
    else if (strEqI(v, "large")) { px = 18.0 * ctx.unit; have = true; }
    else if (strEqI(v, "x-large")) { px = 24.0 * ctx.unit; have = true; }
    else if (strEqI(v, "xx-large")) { px = 32.0 * ctx.unit; have = true; }
    else if (strEqI(v, "smaller")) { px = ctx.curPx / 1.2; have = true; }
    else if (strEqI(v, "larger")) { px = ctx.curPx * 1.2; have = true; }
    else if (parseLen(v, ctx, &l)) {
        if (l.u == LU_PX) { px = l.v; have = true; }
        else if (l.u == LU_PCT) { px = ctx.curPx * l.v / 100.0; have = true; }
    }
    out.ok = have && px > 0.0;
    out.px = px;
}

// font: [style] [weight] size[/line-height] family...
// Pass 0 applies the size (and returns the one diagnostic for the declaration); pass 1 applies style / weight / line-height
// and returns AP_SKIP so the inspector lists the declaration once. The family is accepted but not rendered (as font-family).
// As in CSS, the shorthand resets style, weight and line-height to their initial values.
function applyFontShorthand(st: Style*, v: char*, ctx: CssCtx*, pass: int) => int {
    if (containsStr(v, "var(") || containsStr(v, "calc(")) { if (pass == 0) return AP_UNSUPPORTED; return AP_SKIP; }
    let n: int = strLen(v);
    let i: int = 0;
    let italicF: bool = false; let boldF: bool = false;
    let found: bool = false; let bad: bool = false;
    let fr: FsRes; fr.ok = false; fr.px = 0.0;
    let lhTok: char* = nullptr;
    while (i < n && !found && !bad) {
        let c0: int = v[i] as int;
        if (c0 == 32 || c0 == 9 || c0 == 10 || c0 == 13) { i = i + 1; }
        else {
            let a: int = i; let depth: int = 0; let scan: bool = true;
            while (i < n && scan) {
                let d: int = v[i] as int;
                if (d == 40) depth = depth + 1;
                else if (d == 41) depth = depth - 1;
                if (depth == 0 && (d == 32 || d == 9 || d == 10 || d == 13)) scan = false;
                else i = i + 1;
            }
            let tok: char* = trimDup(v, a, i);
            let tl: int = strLen(tok);
            let slash: int = 0 - 1; let k: int = 0;
            while (k < tl) { if ((tok[k] as int) == 47 && slash < 0) slash = k; k = k + 1; }
            if (slash >= 0) {                                                     // size/line-height
                let sz: char* = trimDup(tok, 0, slash);
                parseFontSize(sz, ctx, &fr);
                strFree(sz);
                if (fr.ok) { found = true; lhTok = trimDup(tok, slash + 1, tl); } else bad = true;
            }
            else if (strEqI(tok, "italic") || strEqI(tok, "oblique")) { italicF = true; }
            else if (strEqI(tok, "bold") || strEqI(tok, "bolder")) { boldF = true; }
            else if (strEqI(tok, "normal") || strEqI(tok, "lighter") || strEqI(tok, "small-caps")) { }
            else {
                let lx: Lex; lx.s = tok; lx.i = 0; lx.ok = false; lx.val = 0.0;
                let w: float = lx.number();
                if (lx.ok && lx.atEnd()) { if (w >= 600.0) boldF = true; }        // bare number = weight
                else { parseFontSize(tok, ctx, &fr); if (fr.ok) found = true; else bad = true; }
            }
            strFree(tok);
        }
    }
    let res: int = AP_INVALID;
    if (found) res = AP_APPLIED;
    if (pass == 0) {
        if (found && !ctx.tui) st.fontPx = fr.px;
        strFree(lhTok);
        return res;
    }
    if (found) {
        st.italic = italicF; st.bold = boldF; st.lineH = 0.0;
        if (lhTok != nullptr && !strEqI(lhTok, "normal")) {
            let lx2: Lex; lx2.s = lhTok; lx2.i = 0; lx2.ok = false; lx2.val = 0.0;
            let m: float = lx2.number();
            let ll: Len; lenSet(&ll, LU_AUTO, 0.0);
            if (lx2.ok && lx2.atEnd()) st.lineH = m * st.fontPx;
            else if (parseLen(lhTok, ctx, &ll)) {
                if (ll.u == LU_PX) st.lineH = ll.v;
                else if (ll.u == LU_PCT) st.lineH = st.fontPx * ll.v / 100.0;
            }
        }
    }
    strFree(lhTok);
    return AP_SKIP;
}
