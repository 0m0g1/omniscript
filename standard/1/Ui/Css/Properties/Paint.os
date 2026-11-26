// standard/1/Ui/Css/Properties/Paint.os
// Property handlers: color, background(-color), box-shadow, opacity, visibility
// applyPropPaint returns AP_NOMATCH when `name` is not one of its properties, so Property.os tries the next group.
// To add a property: put one `else if (strEq(name, "...")) { ... }` branch here and the name in cssSupportedProps().

// light-dark(a, b) -> a copy of the first argument (the light scheme, i.e. color-scheme: normal). Caller frees it.
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
    return trimDup(tok, a, end);
}

// (helpers below, then) box-shadow -> st.sh* (one shadow). Only the first shadow of a comma list is kept. Returns AP_APPLIED when Painter draws it exactly
// (inset ring with no offset / no blur, or a hard outer shadow), AP_UNSUPPORTED when it is stored but not drawn (blur, inset offset,
// negative outer spread, extra shadows), AP_INVALID when it did not parse.
// url("x") / url('x') / url(x) -> a copy of x (caller frees). tok starts with "url(".
function cssUrlArg(tok: char*) => char* {
    let n: int = strLen(tok); let a: int = 4;
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
    if (sb > 0.0) res = AP_UNSUPPORTED;
    if (inset && (sx != 0.0 || sy != 0.0)) res = AP_UNSUPPORTED;
    if (!inset && ss < 0.0) res = AP_UNSUPPORTED;
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
        st.bg = 0;
        if (st.bgUrl != nullptr) { strFree(st.bgUrl); st.bgUrl = nullptr; }
        st.bgSize = 0; st.bgFx = 0.0; st.bgFy = 0.0;
        while (lx.token(buf, 512)) {
            let c0: int = buf[0] as int;
            if (strEqI(buf, "currentcolor")) { st.bg = st.color; }
            else if (strEqI(buf, "initial") || strEqI(buf, "none")) { }
            else if (startsWith(buf, "url(")) { if (st.bgUrl == nullptr) st.bgUrl = cssUrlArg(buf); }
            else if (containsStr(buf, "gradient(")) { sawImage = true; }
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
        else if (sawImage) res = AP_UNSUPPORTED;                                    // gradients are not drawn (url() is)
    }
    else if (strEq(name, "background-image")) {
        let lx: Lex; lx.s = v; lx.i = 0; lx.ok = false; lx.val = 0.0;
        let buf: char* = CMem.malloc(512) as char*;
        let sawGrad: bool = false;
        if (st.bgUrl != nullptr) { strFree(st.bgUrl); st.bgUrl = nullptr; }
        while (lx.token(buf, 512)) {
            if (startsWith(buf, "url(")) { if (st.bgUrl == nullptr) st.bgUrl = cssUrlArg(buf); }
            else if (containsStr(buf, "gradient(")) { sawGrad = true; }
        }
        CMem.free(buf as void*);
        if (sawGrad && st.bgUrl == nullptr) res = AP_UNSUPPORTED;                  // gradients are not drawn
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
    else if (inList(name, "background-repeat background-attachment background-origin background-clip background-blend-mode mix-blend-mode backdrop-filter -webkit-backdrop-filter")) { }   // accepted, no visual effect: they only refine an image (background-image is the reported gap) or are compositing effects not drawn here
    else if (strEq(name, "box-shadow")) { res = applyBoxShadow(st, v, ctx); }                                              // see applyBoxShadow for what is drawn
    else if (strEq(name, "text-shadow")) { if (!strEqI(v, "none") && !strEqI(v, "initial")) res = AP_UNSUPPORTED; }   // text shadows are not painted yet
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
