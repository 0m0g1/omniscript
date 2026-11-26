// standard/1/Ui/Css/Properties/Paint.os
// Property handlers: color, background(-color), box-shadow, opacity, visibility
// applyPropPaint returns AP_NOMATCH when `name` is not one of its properties, so Property.os tries the next group.
// To add a property: put one `else if (strEq(name, "...")) { ... }` branch here and the name in cssSupportedProps().

// light-dark(a, b) -> a copy of the first argument (light), or the second when gMediaDark (prefers-color-scheme: dark). Caller frees it.
// Splits on the first comma outside nested parentheses, so light-dark(rgb(1,2,3), #000) works.
function lightDarkFirst(tok: char*) => char* {
    let n: int = strLen(tok); let i: int = 0;
    while (i < n && (tok[i] as int) != 40) i = i + 1;
    let a: int = i + 1; let depth: int = 0; let end: int = 0 - 1;
    i = a;
    while (i < n && end < 0) {
        let d: int = tok[i] as int;
        if (d == 40) depth = depth + 1;
        else if (d == 41) { if (depth == 0) end = i; else depth = depth - 1; }
        else if (d == 44 && depth == 0) end = i;
        i = i + 1;
    }
    if (end < 0) end = n;
    if (gMediaDark && end < n && (tok[end] as int) == 44) {          // dark scheme: the second argument
        let b: int = end + 1; let z: int = b; let dep: int = 0; let stop: bool = false;
        while (z < n && !stop) {
            let e2: int = tok[z] as int;
            if (e2 == 40) dep = dep + 1;
            else if (e2 == 41) { if (dep == 0) stop = true; else dep = dep - 1; }
            if (!stop) z = z + 1;
        }
        return trimDup(tok, b, z);
    }
    return trimDup(tok, a, end);
}

// (helpers below, then) box-shadow -> st.sh* (one shadow). Only the first shadow of a comma list is kept. Returns AP_APPLIED when Painter draws it exactly
// (inset ring with no offset / no blur, or a hard outer shadow), AP_UNSUPPORTED when it is stored but not drawn (blur, inset offset,
// negative outer spread, extra shadows), AP_INVALID when it did not parse.
// url("x") / url('x') / url(x) -> a copy of x (caller frees). tok starts with "url(".
function cssUrlArg(tok: char*) => char* {
    let n: int = strLen(tok); let a: int = 4;
    // `url()` (empty) fused with a later declaration, e.g. `url()background-image:url(/x.webp)`: the later url() wins, as in CSS
    if (n > 4 && (tok[4] as int) == 41) {
        let k: int = 5;
        while (k + 4 <= n) {
            if ((tok[k] as int) == 117 && (tok[k + 1] as int) == 114 && (tok[k + 2] as int) == 108 && (tok[k + 3] as int) == 40) a = k + 4;
            k = k + 1;
        }
    }
    while (a < n && (tok[a] as int) == 32) a = a + 1;
    if (a < n && ((tok[a] as int) == 34 || (tok[a] as int) == 39)) a = a + 1;
    let e: int = n;
    while (e > a && ((tok[e - 1] as int) == 32 || (tok[e - 1] as int) == 41)) e = e - 1;
    if (e > a && ((tok[e - 1] as int) == 34 || (tok[e - 1] as int) == 39)) e = e - 1;
    return trimDup(tok, a, e);
}

// one background-position token -> 0..1 fraction (keywords and %); lengths are not supported and give `fallback`
function bgPosTok(tok: char*, fallback: float) => float {
    if (strEqI(tok, "left") || strEqI(tok, "top")) return 0.0;
    if (strEqI(tok, "right") || strEqI(tok, "bottom")) return 1.0;
    if (strEqI(tok, "center")) return 0.5;
    let lx: Lex; lx.s = tok; lx.i = 0; lx.ok = false; lx.val = 0.0;
    let o: float = lx.number();
    if (lx.ok && lx.peek() == 37) return o / 100.0;
    return fallback;
}

// ---- linear-gradient ----
function gradSet(st: Style*, i: int, c: u32, p: float) => void {
    if (i == 0) { st.gdC0 = c; st.gdP0 = p; }
    else if (i == 1) { st.gdC1 = c; st.gdP1 = p; }
    else if (i == 2) { st.gdC2 = c; st.gdP2 = p; }
    else { st.gdC3 = c; st.gdP3 = p; }
}
function gradColor(st: Style*, i: int) => u32 {
    if (i == 0) return st.gdC0;
    if (i == 1) return st.gdC1;
    if (i == 2) return st.gdC2;
    return st.gdC3;
}
function gradPos(st: Style*, i: int) => float {
    if (i == 0) return st.gdP0;
    if (i == 1) return st.gdP1;
    if (i == 2) return st.gdP2;
    return st.gdP3;
}

// "linear-gradient(90deg, #333 11%, rgba(0,0,0,.5) 50%, #333 88%)" -> st.gd* (2..4 colour stops, % or auto positions).
// Returns 0 = not understood (nothing stored), 1 = stored and drawn exactly, 2 = stored, drawn approximately
// (diagonal angles use the nearest axis, px stop positions are treated as auto).
function gradParse(tok: char*, ctx: CssCtx*, st: Style*) => int {
    st.gdN = 0;
    if (!startsWith(tok, "linear-gradient(")) return 0;
    let tl: int = strLen(tok);
    if ((tok[tl - 1] as int) != 41) return 0;
    let cols: u32* = CMem.malloc(64) as u32*;
    let pos: float* = CMem.malloc(64) as float*;
    let cr: CRes; cr.ok = false; cr.c = 0;
    let n: int = 0; let ok: bool = true; let approx: bool = false;
    let ang: float = 180.0;
    let end: int = tl - 1; let i: int = 16; let partNo: int = 0;
    while (i <= end && ok) {
        let a: int = i; let depth: int = 0; let scan: bool = true;
        while (i < end && scan) {
            let ch: int = tok[i] as int;
            if (ch == 40) depth = depth + 1;
            else if (ch == 41) depth = depth - 1;
            if (ch == 44 && depth == 0) scan = false; else i = i + 1;
        }
        let part: char* = trimDup(tok, a, i);
        let pl: int = strLen(part);
        let isDir: bool = false;
        if (partNo == 0) {
            if (startsWith(part, "to ")) {
                isDir = true;
                let up: bool = containsStr(part, "top"); let dn: bool = containsStr(part, "bottom");
                let lf: bool = containsStr(part, "left"); let rt: bool = containsStr(part, "right");
                if (up && rt) ang = 45.0; else if (dn && rt) ang = 135.0; else if (dn && lf) ang = 225.0; else if (up && lf) ang = 315.0;
                else if (rt) ang = 90.0; else if (lf) ang = 270.0; else if (dn) ang = 180.0; else if (up) ang = 0.0;
                else ok = false;
            }
            else {
                let c0: int = part[0] as int;
                if ((c0 >= 48 && c0 <= 57) || c0 == 46 || c0 == 45 || c0 == 43) {
                    isDir = true;
                    if (pl > 3 && (part[pl - 3] as int) == 100 && (part[pl - 2] as int) == 101 && (part[pl - 1] as int) == 103) {
                        let num: char* = trimDup(part, 0, pl - 3); ang = parseFloatStr(num, 180.0); strFree(num);
                    }
                    else if (pl > 4 && (part[pl - 4] as int) == 116 && (part[pl - 3] as int) == 117 && (part[pl - 2] as int) == 114 && (part[pl - 1] as int) == 110) {
                        let num2: char* = trimDup(part, 0, pl - 4); ang = parseFloatStr(num2, 0.5) * 360.0; strFree(num2);
                    }
                    else ok = false;
                }
            }
        }
        if (!isDir && ok) {
            if (n >= 4) { ok = false; }
            else {
                let sp: int = 0 - 1; let d2: int = 0; let k: int = 0;
                while (k < pl) {
                    let ch2: int = part[k] as int;
                    if (ch2 == 40) d2 = d2 + 1;
                    else if (ch2 == 41) d2 = d2 - 1;
                    else if ((ch2 == 32 || ch2 == 9) && d2 == 0) sp = k;
                    k = k + 1;
                }
                let colStr: char* = nullptr; let pv: float = 0.0 - 1.0;
                if (sp > 0) {
                    let tail: char* = trimDup(part, sp + 1, pl);
                    let tn: int = strLen(tail);
                    if (tn > 1 && (tail[tn - 1] as int) == 37) {
                        let pn: char* = trimDup(tail, 0, tn - 1); pv = parseFloatStr(pn, 0.0) / 100.0; strFree(pn);
                    }
                    else approx = true;
                    strFree(tail);
                    colStr = trimDup(part, 0, sp);
                }
                else { colStr = trimDup(part, 0, pl); }
                parseColor(colStr, &cr);
                if (cr.ok) { cols[n] = cr.c; pos[n] = pv; n = n + 1; } else ok = false;
                strFree(colStr);
            }
        }
        strFree(part);
        i = i + 1; partNo = partNo + 1;
    }
    let res: int = 0;
    if (ok && n >= 2) {
        if (pos[0] < 0.0) pos[0] = 0.0;
        if (pos[n - 1] < 0.0) pos[n - 1] = 1.0;
        let q: int = 1;
        while (q < n - 1) {
            if (pos[q] < 0.0) {
                let z: int = q + 1;
                while (z < n - 1 && pos[z] < 0.0) z = z + 1;
                pos[q] = pos[q - 1] + (pos[z] - pos[q - 1]) / ((z - q + 1) as float);
            }
            q = q + 1;
        }
        q = 1;
        while (q < n) { if (pos[q] < pos[q - 1]) pos[q] = pos[q - 1]; q = q + 1; }
        while (ang < 0.0) ang = ang + 360.0;
        while (ang >= 360.0) ang = ang - 360.0;
        let rem: float = ang - ((ang / 90.0) as int as float) * 90.0;
        if (rem > 0.5 && rem < 89.5) approx = true;
        st.gdN = n; st.gdAngle = ang;
        q = 0;
        while (q < n) { gradSet(st, q, cols[q], pos[q]); q = q + 1; }
        res = 1;
        if (approx) res = 2;
    }
    CMem.free(cols as void*); CMem.free(pos as void*);
    return res;
}

function applyBoxShadow(st: Style*, v: char*, ctx: CssCtx*) => int {
    let res: int = AP_APPLIED;
    st.shOn = false; st.shInset = false; st.shX = 0.0; st.shY = 0.0; st.shBlur = 0.0; st.shSpread = 0.0; st.shColor = 0;
    if (strEqI(v, "none") || strEqI(v, "initial")) return res;
    // cut at the first top-level comma
    let n: int = strLen(v); let i: int = 0; let depth: int = 0; let end: int = n; let multi: bool = false;
    while (i < n && !multi) {
        let ch: int = v[i] as int;
        if (ch == 40) depth = depth + 1;
        else if (ch == 41) { if (depth > 0) depth = depth - 1; }
        else if (ch == 44 && depth == 0) { end = i; multi = true; }
        i = i + 1;
    }
    let first: char* = trimDup(v, 0, end);
    let lx: Lex; lx.s = first; lx.i = 0; lx.ok = false; lx.val = 0.0;
    let buf: char* = CMem.malloc(96) as char*;
    let l: Len; lenSet(&l, LU_AUTO, 0.0);
    let cr: CRes; cr.ok = false; cr.c = 0;
    let k: int = 0; let bad: bool = false; let hasColor: bool = false; let inset: bool = false;
    let sx: float = 0.0; let sy: float = 0.0; let sb: float = 0.0; let ss: float = 0.0; let col: u32 = st.color;
    while (lx.token(buf, 96)) {
        let c0: int = buf[0] as int;
        if (strEqI(buf, "inset")) { inset = true; }
        else if ((c0 >= 48 && c0 <= 57) || c0 == 46 || c0 == 45 || c0 == 43) {
            if (parseLen(buf, ctx, &l) && l.u == LU_PX) {
                if (k == 0) sx = l.v; else if (k == 1) sy = l.v; else if (k == 2) sb = l.v; else if (k == 3) ss = l.v; else bad = true;
                k = k + 1;
            } else bad = true;
        }
        else if (strEqI(buf, "currentcolor")) { col = st.color; hasColor = true; }
        else { parseColor(buf, &cr); if (cr.ok && !hasColor) { col = cr.c; hasColor = true; } else bad = true; }
    }
    CMem.free(buf as void*); strFree(first);
    if (bad || k < 2) { return AP_INVALID; }
    st.shOn = true; st.shInset = inset; st.shX = sx; st.shY = sy; st.shBlur = sb; st.shSpread = ss; st.shColor = col;
    if (multi) res = AP_UNSUPPORTED;
    if (sb > 0.0 && inset) res = AP_UNSUPPORTED;                      // a blurred outer shadow is drawn (Painter: stacked translucent copies)
    if (inset && (sx != 0.0 || sy != 0.0)) res = AP_UNSUPPORTED;
    if (!inset && ss < 0.0 && sb <= 0.0) res = AP_UNSUPPORTED;
    return res;
}

// text-shadow -> st.ts* (first shadow of a comma list, inherited). Painter draws one hard copy, or a ring of faint copies for a blur.
function applyTextShadow(st: Style*, v: char*, ctx: CssCtx*) => int {
    let res: int = AP_APPLIED;
    st.tsOn = false; st.tsX = 0.0; st.tsY = 0.0; st.tsBlur = 0.0; st.tsColor = 0;
    if (strEqI(v, "none") || strEqI(v, "initial")) return res;
    let n: int = strLen(v); let i: int = 0; let depth: int = 0; let end: int = n; let multi: bool = false;
    while (i < n && !multi) {
        let ch: int = v[i] as int;
        if (ch == 40) depth = depth + 1;
        else if (ch == 41) { if (depth > 0) depth = depth - 1; }
        else if (ch == 44 && depth == 0) { end = i; multi = true; }
        i = i + 1;
    }
    let first: char* = trimDup(v, 0, end);
    let lx: Lex; lx.s = first; lx.i = 0; lx.ok = false; lx.val = 0.0;
    let buf: char* = CMem.malloc(96) as char*;
    let l: Len; lenSet(&l, LU_AUTO, 0.0);
    let cr: CRes; cr.ok = false; cr.c = 0;
    let k: int = 0; let bad: bool = false; let hasColor: bool = false;
    let sx: float = 0.0; let sy: float = 0.0; let sb: float = 0.0; let col: u32 = st.color;
    while (lx.token(buf, 96)) {
        let c0: int = buf[0] as int;
        if ((c0 >= 48 && c0 <= 57) || c0 == 46 || c0 == 45 || c0 == 43) {
            if (parseLen(buf, ctx, &l) && l.u == LU_PX) {
                if (k == 0) sx = l.v; else if (k == 1) sy = l.v; else if (k == 2) sb = l.v; else bad = true;
                k = k + 1;
            } else bad = true;
        }
        else if (strEqI(buf, "currentcolor")) { col = st.color; hasColor = true; }
        else { parseColor(buf, &cr); if (cr.ok && !hasColor) { col = cr.c; hasColor = true; } else bad = true; }
    }
    CMem.free(buf as void*); strFree(first);
    if (bad || k < 2) { return AP_INVALID; }
    st.tsOn = true; st.tsX = sx; st.tsY = sy; st.tsBlur = sb; st.tsColor = col;
    if (multi) res = AP_UNSUPPORTED;
    return res;
}

function applyPropPaint(st: Style*, name: char*, v: char*, ctx: CssCtx*, isFs: bool) => int {
    let res: int = AP_APPLIED;
    let l: Len; lenSet(&l, LU_AUTO, 0.0);
    let cr: CRes; cr.ok = false; cr.c = 0;
    let hit: bool = true;
    if (strEq(name, "color")) {
        if (strEqI(v, "initial")) st.color = DEF_TEXT;
        else { parseColor(v, &cr); if (cr.ok) st.color = cr.c; else res = AP_INVALID; }
    }
    else if (strEq(name, "background-color")) {
        if (strEqI(v, "initial")) st.bg = 0;                                          // initial = transparent
        else if (strEqI(v, "currentcolor")) st.bg = st.color;                         // the colour as cascaded so far
        else { parseColor(v, &cr); if (cr.ok) st.bg = cr.c; else res = AP_INVALID; }
    }
    else if (strEq(name, "background")) {
        // As in CSS the shorthand resets every background sub-property, so the colour goes back to transparent
        // unless a colour token follows (`background: 0 0` is just position 0 0 = transparent).
        // Position / size / repeat / attachment tokens have no effect without an image; only url() and gradients are a gap.
        let lx: Lex; lx.s = v; lx.i = 0; lx.ok = false; lx.val = 0.0;
        let buf: char* = CMem.malloc(512) as char*;
        let sawImage: bool = false; let bad: bool = false;
        st.bg = 0; st.gdN = 0;
        if (st.bgUrl != nullptr) { strFree(st.bgUrl); st.bgUrl = nullptr; }
        st.bgSize = 0; st.bgFx = 0.0; st.bgFy = 0.0; st.bgOver = false;   // the shorthand also resets background-blend-mode
        while (lx.token(buf, 512)) {
            let c0: int = buf[0] as int;
            if (strEqI(buf, "currentcolor")) { st.bg = st.color; }
            else if (strEqI(buf, "initial") || strEqI(buf, "none")) { }
            else if (startsWith(buf, "url(")) { if (st.bgUrl == nullptr) st.bgUrl = cssUrlArg(buf); }
            else if (containsStr(buf, "gradient(")) { if (gradParse(buf, ctx, st) != 1) sawImage = true; }
            else if (startsWith(buf, "light-dark(")) {
                let ld: char* = lightDarkFirst(buf);
                parseColor(ld, &cr);
                if (cr.ok) st.bg = cr.c; else bad = true;
                strFree(ld);
            }
            else {
                parseColor(buf, &cr);
                if (cr.ok) { st.bg = cr.c; }
                else if ((c0 >= 48 && c0 <= 57) || c0 == 46 || c0 == 45 || c0 == 43 || c0 == 47 || c0 == 44) { }   // 0 / 50% / -2px / "/" / ","
                else if (strEqI(buf, "cover")) { st.bgSize = 1; }
            else if (strEqI(buf, "contain")) { st.bgSize = 2; }
            else if (inList(buf, "left right top bottom center repeat no-repeat repeat-x repeat-y space round scroll fixed local border-box padding-box content-box")) { }
                else bad = true;
            }
        }
        CMem.free(buf as void*);
        if (bad) res = AP_INVALID;
        else if (sawImage) res = AP_UNSUPPORTED;                                    // radial / conic / repeating gradients, > 4 stops, diagonal angles
    }
    else if (strEq(name, "background-image")) {
        let lx: Lex; lx.s = v; lx.i = 0; lx.ok = false; lx.val = 0.0;
        let buf: char* = CMem.malloc(512) as char*;
        let sawGrad: bool = false; let gr: int = 1;
        st.gdN = 0;
        if (st.bgUrl != nullptr) { strFree(st.bgUrl); st.bgUrl = nullptr; }
        while (lx.token(buf, 512)) {
            if (startsWith(buf, "url(")) { if (st.bgUrl == nullptr) st.bgUrl = cssUrlArg(buf); }
            else if (containsStr(buf, "gradient(")) { sawGrad = true; gr = gradParse(buf, ctx, st); }
        }
        CMem.free(buf as void*);
        if (sawGrad && gr != 1) res = AP_UNSUPPORTED;                              // only linear-gradient with <= 4 stops on a straight axis is drawn
    }
    else if (strEq(name, "background-size")) {
        st.bgSize = 0;
        if (containsStr(v, "cover")) st.bgSize = 1; else if (containsStr(v, "contain")) st.bgSize = 2;   // lengths / auto = natural size
    }
    else if (strEq(name, "background-position")) {
        let lx: Lex; lx.s = v; lx.i = 0; lx.ok = false; lx.val = 0.0;
        let b0: char* = CMem.malloc(64) as char*; let b1: char* = CMem.malloc(64) as char*;
        b0[0] = 0 as char; b1[0] = 0 as char;
        if (lx.token(b0, 64)) { lx.token(b1, 64); }
        if (strLen(b1) == 0) {                                                       // one value: the other axis is center
            if (strEqI(b0, "top") || strEqI(b0, "bottom")) { st.bgFx = 0.5; st.bgFy = bgPosTok(b0, 0.0); }
            else { st.bgFx = bgPosTok(b0, 0.0); st.bgFy = 0.5; }
        } else if (strEqI(b0, "top") || strEqI(b0, "bottom") || strEqI(b1, "left") || strEqI(b1, "right")) {
            st.bgFy = bgPosTok(b0, 0.0); st.bgFx = bgPosTok(b1, 0.0);                // "top left", "bottom right"...
        } else { st.bgFx = bgPosTok(b0, 0.0); st.bgFy = bgPosTok(b1, 0.0); }
        CMem.free(b0 as void*); CMem.free(b1 as void*);
    }
    else if (strEq(name, "background-blend-mode")) {
        // not a real blend: any mode other than normal puts the background colour over the image (a translucent tint, as `darken` of a dark colour looks)
        st.bgOver = !(strEqI(v, "normal") || strEqI(v, "initial"));
    }
    else if (inList(name, "background-repeat background-attachment background-origin background-clip mix-blend-mode backdrop-filter -webkit-backdrop-filter")) { }   // accepted, no visual effect: they only refine an image (background-image is the reported gap) or are compositing effects not drawn here
    else if (strEq(name, "box-shadow")) { res = applyBoxShadow(st, v, ctx); }                                              // see applyBoxShadow for what is drawn
    else if (strEq(name, "text-shadow")) { res = applyTextShadow(st, v, ctx); }                                         // see applyTextShadow
    else if (strEq(name, "opacity")) {
        let lx: Lex; lx.s = v; lx.i = 0; lx.ok = false; lx.val = 0.0;
        let o: float = lx.number();
        if (lx.ok) { if (lx.peek() == 37) o = o / 100.0; st.opacity = clampF(o, 0.0, 1.0); } else res = AP_INVALID;
    }
    else if (strEq(name, "visibility")) { st.hidden = strEqI(v, "hidden") || strEqI(v, "collapse"); }
    else { hit = false; }
    if (!hit) res = AP_NOMATCH;
    return res;
}
