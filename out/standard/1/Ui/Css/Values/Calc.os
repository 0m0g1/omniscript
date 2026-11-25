// standard/1/Ui/Css/Values/Calc.os
// calc(), min(), max(), clamp(): one length or one percentage (no px/% mixing except as noted in calcMathPick).

// ---- calc() ----
// calc(...) with + - * / and nested parentheses. The result is one length (px) or one percentage; mixing the two
// (calc(100% - 20px)) cannot be stored in a single Len, so it fails and the declaration stays UNSUPPORTED.
// CalcV.k: 0 = plain number, 1 = length (layout units), 2 = percentage.
struct CalcV { v: float = 0.0; k: int = 0; bad: bool = false; }

function calcSkipWs(lx: Lex*) => void {
    let s: char* = lx.s;
    let c: int = s[lx.i] as int;
    while (c == 32 || c == 9 || c == 10 || c == 13) { lx.i = lx.i + 1; c = s[lx.i] as int; }
}

// precedence climbing: + - bind at 1, * / at 2. Only recurses into itself.
function calcExpr(lx: Lex*, minPrec: int, ctx: CssCtx*, o: CalcV*) => void {
    o.v = 0.0; o.k = 0; o.bad = false;
    calcSkipWs(lx);
    let s: char* = lx.s;
    let c: int = s[lx.i] as int;
    if (c == 40 || eqAtI(s, lx.i, "calc(")) {
        if (c == 40) lx.i = lx.i + 1; else lx.i = lx.i + 5;
        calcExpr(lx, 0, ctx, o);
        calcSkipWs(lx);
        if ((s[lx.i] as int) == 41) lx.i = lx.i + 1; else o.bad = true;
    }
    else if (eqAtI(s, lx.i, "min(") || eqAtI(s, lx.i, "max(") || eqAtI(s, lx.i, "clamp(")) {
        // min() / max() / clamp(): up to three arguments, each a full expression (self recursion only)
        let which: int = 3;
        if (eqAtI(s, lx.i, "min(")) { which = 1; lx.i = lx.i + 4; }
        else if (eqAtI(s, lx.i, "max(")) { which = 2; lx.i = lx.i + 4; }
        else { lx.i = lx.i + 6; }
        let ma: CalcV; ma.v = 0.0; ma.k = 0; ma.bad = false;
        let mb: CalcV; mb.v = 0.0; mb.k = 0; mb.bad = false;
        let mc: CalcV; mc.v = 0.0; mc.k = 0; mc.bad = false;
        let cnt: int = 0; let go: bool = true;
        while (go) {
            let t: CalcV; t.v = 0.0; t.k = 0; t.bad = false;
            calcExpr(lx, 0, ctx, &t);
            if (t.bad) { o.bad = true; go = false; }
            else {
                if (cnt == 0) { ma.v = t.v; ma.k = t.k; }
                else if (cnt == 1) { mb.v = t.v; mb.k = t.k; }
                else if (cnt == 2) { mc.v = t.v; mc.k = t.k; }
                cnt = cnt + 1;
                calcSkipWs(lx);
                if ((s[lx.i] as int) == 44) lx.i = lx.i + 1; else go = false;
            }
        }
        if (!o.bad) { if ((s[lx.i] as int) == 41) lx.i = lx.i + 1; else o.bad = true; }
        if (!o.bad) calcMathPick(which, cnt, &ma, &mb, &mc, o);
    }
    else {
        let sign: float = 1.0;
        if (c == 45) { sign = 0.0 - 1.0; lx.i = lx.i + 1; }
        else if (c == 43) { lx.i = lx.i + 1; }
        lx.ok = false;
        let n: float = lx.number();
        if (!lx.ok) o.bad = true;
        else {
            o.v = sign * n;
            let at: int = lx.i;
            if (eqAtI(s, at, "px")) { o.v = o.v * ctx.unit; o.k = 1; lx.i = at + 2; }
            else if (eqAtI(s, at, "rem")) { o.v = o.v * ctx.rootPx; o.k = 1; lx.i = at + 3; }
            else if (eqAtI(s, at, "em")) { o.v = o.v * ctx.curPx; o.k = 1; lx.i = at + 2; }
            else if (eqAtI(s, at, "dvw") || eqAtI(s, at, "svw") || eqAtI(s, at, "lvw")) { o.v = o.v * ctx.vw / 100.0; o.k = 1; lx.i = at + 3; }
            else if (eqAtI(s, at, "dvh") || eqAtI(s, at, "svh") || eqAtI(s, at, "lvh")) { o.v = o.v * ctx.vh / 100.0; o.k = 1; lx.i = at + 3; }
            else if (eqAtI(s, at, "vmin")) { let mn: float = ctx.vw; if (ctx.vh < mn) mn = ctx.vh; o.v = o.v * mn / 100.0; o.k = 1; lx.i = at + 4; }
            else if (eqAtI(s, at, "vmax")) { let mx: float = ctx.vw; if (ctx.vh > mx) mx = ctx.vh; o.v = o.v * mx / 100.0; o.k = 1; lx.i = at + 4; }
            else if (eqAtI(s, at, "ch") || eqAtI(s, at, "ex")) { o.v = o.v * ctx.curPx * 0.5; o.k = 1; lx.i = at + 2; }
            else if (eqAtI(s, at, "vw")) { o.v = o.v * ctx.vw / 100.0; o.k = 1; lx.i = at + 2; }
            else if (eqAtI(s, at, "vh")) { o.v = o.v * ctx.vh / 100.0; o.k = 1; lx.i = at + 2; }
            else if (eqAtI(s, at, "pt")) { o.v = o.v * 1.3333 * ctx.unit; o.k = 1; lx.i = at + 2; }
            else if (eqAtI(s, at, "in")) { o.v = o.v * 96.0 * ctx.unit; o.k = 1; lx.i = at + 2; }
            else if (eqAtI(s, at, "cm")) { o.v = o.v * 37.795 * ctx.unit; o.k = 1; lx.i = at + 2; }
            else if (eqAtI(s, at, "mm")) { o.v = o.v * 3.7795 * ctx.unit; o.k = 1; lx.i = at + 2; }
            else if (eqAtI(s, at, "%")) { o.k = 2; lx.i = at + 1; }
        }
    }
    let more: bool = !o.bad;
    while (more) {
        calcSkipWs(lx);
        let op: int = s[lx.i] as int;
        let prec: int = 0;
        if (op == 43 || op == 45) prec = 1;
        else if (op == 42 || op == 47) prec = 2;
        if (prec == 0 || prec < minPrec) more = false;
        else {
            lx.i = lx.i + 1;
            let r: CalcV; r.v = 0.0; r.k = 0; r.bad = false;
            calcExpr(lx, prec + 1, ctx, &r);
            if (r.bad) { o.bad = true; more = false; }
            else if (op == 43 || op == 45) {
                if (o.k != r.k) { o.bad = true; more = false; }
                else if (op == 43) o.v = o.v + r.v;
                else o.v = o.v - r.v;
            }
            else if (op == 42) {
                if (o.k == 0) { o.k = r.k; o.v = o.v * r.v; }
                else if (r.k == 0) o.v = o.v * r.v;
                else { o.bad = true; more = false; }
            }
            else {
                if (r.k != 0 || r.v == 0.0) { o.bad = true; more = false; }
                else o.v = o.v / r.v;
            }
        }
    }
}

// tok must be a whole calc(...) expression. false when it does not parse or is not a single length / percentage.
function calcLen(tok: char*, ctx: CssCtx*, out: Len*) => bool {
    let lx: Lex; lx.s = tok; lx.i = 0; lx.ok = false; lx.val = 0.0;
    let r: CalcV; r.v = 0.0; r.k = 0; r.bad = false;
    calcExpr(&lx, 0, ctx, &r);
    calcSkipWs(&lx);
    let good: bool = !r.bad && (tok[lx.i] as int) == 0 && r.k != 0;
    if (good) {
        if (r.k == 1) lenSet(out, LU_PX, r.v); else lenSet(out, LU_PCT, r.v);
    }
    return good;
}

// ---- math functions: min() max() clamp() ----
// Operand kinds as in CalcV (0 number, 1 length, 2 percentage). A percentage resolves against a container that is
// unknown at cascade time, so when px operands exist the percentages are ignored: min(100%, 1200px) -> 1200px.
function calcMathPick(which: int, cnt: int, a: CalcV*, b: CalcV*, c: CalcV*, o: CalcV*) => void {
    o.bad = false;
    if (cnt < 1 || cnt > 3) { o.bad = true; }
    else if (which == 3) {
        if (cnt != 3) { o.bad = true; }
        else if (a.k == b.k && b.k == c.k) {                 // clamp(lo, val, hi) = max(lo, min(val, hi))
            let m: float = b.v;
            if (c.v < m) m = c.v;
            if (a.v > m) m = a.v;
            o.v = m; o.k = a.k;
        }
        else if (a.k == c.k) { o.v = c.v; o.k = c.k; }       // preferred value is relative: take the upper bound
        else { o.bad = true; }
    }
    else {
        let kind: int = a.k;
        if (a.k == 1 || (cnt > 1 && b.k == 1) || (cnt > 2 && c.k == 1)) kind = 1;
        let r: float = 0.0; let have: bool = false;
        if (a.k == kind) { r = a.v; have = true; }
        if (cnt > 1 && b.k == kind) {
            if (!have) r = b.v;
            else if (which == 1 && b.v < r) r = b.v;
            else if (which == 2 && b.v > r) r = b.v;
            have = true;
        }
        if (cnt > 2 && c.k == kind) {
            if (!have) r = c.v;
            else if (which == 1 && c.v < r) r = c.v;
            else if (which == 2 && c.v > r) r = c.v;
            have = true;
        }
        o.v = r; o.k = kind;
    }
}

// calc( / min( / max( / clamp( at index i, not the tail of another name (so "minmax(" does not count)
function mathFnAt(v: char*, i: int) => bool {
    let ok: bool = false;
    if (eqAtI(v, i, "calc(") || eqAtI(v, i, "min(") || eqAtI(v, i, "max(") || eqAtI(v, i, "clamp(")) {
        ok = true;
        if (i > 0) { let p: int = v[i - 1] as int; if (isNameC(p)) ok = false; }
    }
    return ok;
}
function startsMathFn(v: char*) => bool { return mathFnAt(v, 0); }
function hasMathFn(v: char*) => bool {
    let n: int = strLen(v); let i: int = 0; let found: bool = false;
    while (i < n && !found) { if (mathFnAt(v, i)) found = true; i = i + 1; }
    return found;
}
