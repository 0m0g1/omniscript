// standard/1/Ui/Css/Values/Length.os
// Lengths, border widths, 1-4 value shorthands.

function copyLen(dst: Len*, src: Len*) => void { dst.u = src.u; dst.v = src.v; }

// ---- lengths ----
// px/em/rem/%/vw/vh/pt/in/cm/mm/auto/none/bare 0 -> Len in layout units (percent stays percent)
function parseLen(tok: char*, ctx: CssCtx*, out: Len*) => bool {
    let ok: bool = false;
    if (startsMathFn(tok)) ok = calcLen(tok, ctx, out);
    else if (strEqI(tok, "auto")) { lenSet(out, LU_AUTO, 0.0); ok = true; }
    else if (strEqI(tok, "none")) { lenSet(out, LU_NONE, 0.0); ok = true; }
    else {
        let lx: Lex; lx.s = tok; lx.i = 0; lx.ok = false; lx.val = 0.0;
        let v: float = lx.number();
        if (lx.ok) {
            let at: int = lx.i;
            if (tok[at] as int == 0 || eqAtI(tok, at, "px")) { lenSet(out, LU_PX, v * ctx.unit); ok = true; }
            else if (eqAtI(tok, at, "%")) { lenSet(out, LU_PCT, v); ok = true; }
            else if (eqAtI(tok, at, "rem")) { lenSet(out, LU_PX, v * ctx.rootPx); ok = true; }
            else if (eqAtI(tok, at, "em")) { lenSet(out, LU_PX, v * ctx.curPx); ok = true; }
            else if (eqAtI(tok, at, "dvw") || eqAtI(tok, at, "svw") || eqAtI(tok, at, "lvw")) { lenSet(out, LU_PX, v * ctx.vw / 100.0); ok = true; }
            else if (eqAtI(tok, at, "dvh") || eqAtI(tok, at, "svh") || eqAtI(tok, at, "lvh")) { lenSet(out, LU_PX, v * ctx.vh / 100.0); ok = true; }
            else if (eqAtI(tok, at, "vmin")) { let mn: float = ctx.vw; if (ctx.vh < mn) mn = ctx.vh; lenSet(out, LU_PX, v * mn / 100.0); ok = true; }
            else if (eqAtI(tok, at, "vmax")) { let mx: float = ctx.vw; if (ctx.vh > mx) mx = ctx.vh; lenSet(out, LU_PX, v * mx / 100.0); ok = true; }
            else if (eqAtI(tok, at, "ch") || eqAtI(tok, at, "ex")) { lenSet(out, LU_PX, v * ctx.curPx * 0.5); ok = true; }
            else if (eqAtI(tok, at, "vw")) { lenSet(out, LU_PX, v * ctx.vw / 100.0); ok = true; }
            else if (eqAtI(tok, at, "vh")) { lenSet(out, LU_PX, v * ctx.vh / 100.0); ok = true; }
            else if (eqAtI(tok, at, "pt")) { lenSet(out, LU_PX, v * 1.3333 * ctx.unit); ok = true; }
            else if (eqAtI(tok, at, "in")) { lenSet(out, LU_PX, v * 96.0 * ctx.unit); ok = true; }
            else if (eqAtI(tok, at, "cm")) { lenSet(out, LU_PX, v * 37.795 * ctx.unit); ok = true; }
            else if (eqAtI(tok, at, "mm")) { lenSet(out, LU_PX, v * 3.7795 * ctx.unit); ok = true; }
        }
        else if (strEqI(tok, "thin")) { lenSet(out, LU_PX, 1.0 * ctx.unit); ok = true; }
        else if (strEqI(tok, "medium")) { lenSet(out, LU_PX, 3.0 * ctx.unit); ok = true; }
        else if (strEqI(tok, "thick")) { lenSet(out, LU_PX, 5.0 * ctx.unit); ok = true; }
    }
    return ok;
}

// Border width in layout units (px only). Keeps `def` when the token is not a length.
function parseWidthPx(tok: char*, ctx: CssCtx*, def: float) => float {
    let l: Len;
    lenSet(&l, LU_PX, def);
    if (parseLen(tok, ctx, &l)) { if (l.u != LU_PX) lenSet(&l, LU_PX, def); }
    return l.v;
}

// 1-4 values -> top right bottom left (CSS shorthand rules). Tokens are split outside parentheses (nthToken),
// so calc()/min()/max()/clamp() with spaces and commas inside are one value.
function side4(v: char*, ctx: CssCtx*, t: Len*, r: Len*, b: Len*, l: Len*) => bool {
    let a: Len; let bb: Len; let c: Len; let d: Len;
    lenSet(&a, LU_PX, 0.0); lenSet(&bb, LU_PX, 0.0); lenSet(&c, LU_PX, 0.0); lenSet(&d, LU_PX, 0.0);
    let n: int = 0; let good: bool = true; let more: bool = true;
    while (n < 4 && more) {
        let tok: char* = nthToken(v, n);
        if (tok == nullptr) { more = false; }
        else {
            let ok: bool = false;
            if (n == 0) ok = parseLen(tok, ctx, &a);
            else if (n == 1) ok = parseLen(tok, ctx, &bb);
            else if (n == 2) ok = parseLen(tok, ctx, &c);
            else ok = parseLen(tok, ctx, &d);
            if (!ok) good = false;
            strFree(tok);
            n = n + 1;
        }
    }
    if (good && n > 0) {
        if (n == 1) { bb = a; c = a; d = a; }
        else if (n == 2) { c = a; d = bb; }
        else if (n == 3) { d = bb; }
        t.u = a.u; t.v = a.v;
        r.u = bb.u; r.v = bb.v;
        b.u = c.u; b.v = c.v;
        l.u = d.u; l.v = d.v;
    }
    return good && n > 0;
}
