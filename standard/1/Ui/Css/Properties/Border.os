// standard/1/Ui/Css/Properties/Border.os
// Property handlers: border shorthands, per-side width/style, border-color, border-radius
// applyPropBorder returns AP_NOMATCH when `name` is not one of its properties, so Property.os tries the next group.
// To add a property: put one `else if (strEq(name, "...")) { ... }` branch here and the name in cssSupportedProps().

// border shorthand: width / style / colour tokens in any order. side: 0 all, 1 top, 2 right, 3 bottom, 4 left
function applyBorder(st: Style*, side: int, v: char*, ctx: CssCtx*) => void {
    let lx: Lex; lx.s = v; lx.i = 0; lx.ok = false; lx.val = 0.0;
    let buf: char* = CMem.malloc(96) as char*;
    let w: float = 1.0 * ctx.unit; let none: bool = false; let cr: CRes;
    cr.ok = false; cr.c = 0;
    let gotColor: bool = false; let color: u32 = st.borderColor;
    while (lx.token(buf, 96)) {
        if (strEqI(buf, "none") || strEqI(buf, "hidden")) { none = true; }
        else if (strEqI(buf, "solid") || strEqI(buf, "dashed") || strEqI(buf, "dotted") || strEqI(buf, "double")
              || strEqI(buf, "groove") || strEqI(buf, "ridge") || strEqI(buf, "inset") || strEqI(buf, "outset")) { }   // NEXT: dashed/dotted
        else if (strEqI(buf, "currentcolor")) { color = st.color; gotColor = true; }
        else {
            let l: Len;
            lenSet(&l, LU_PX, 0.0);
            if (parseLen(buf, ctx, &l)) { if (l.u == LU_PX) w = l.v; }
            else { parseColor(buf, &cr); if (cr.ok) { color = cr.c; gotColor = true; } }
        }
    }
    CMem.free(buf as void*);
    if (none) w = 0.0;
    if (gotColor) st.borderColor = color;
    if (side == 0 || side == 1) st.bt = w;
    if (side == 0 || side == 2) st.br = w;
    if (side == 0 || side == 3) st.bb = w;
    if (side == 0 || side == 4) st.bl = w;
}

// border-radius value without var()/calc()/%/"/" -> up to 4 radii in px (CSS order tl tr br bl). Scans number by number, so minified
// values such as "0 12px12px0" (var()s glued together) work. Writes the raw values in order into st.rtl, rtr, rbr, rbl as scratch
// and returns how many were read (the caller then expands them to the four corners).
function parseRadii(v: char*, ctx: CssCtx*, st: Style*) => int {
    let n: int = strLen(v); let i: int = 0; let cnt: int = 0;
    while (i < n && cnt < 4) {
        let ch: int = v[i] as int;
        let isNum: bool = (ch >= 48 && ch <= 57) || ch == 46;
        if (isNum) {
            let a: int = i;
            while (i < n && (((v[i] as int) >= 48 && (v[i] as int) <= 57) || (v[i] as int) == 46)) i = i + 1;
            let e: int = v[i] as int;
            if (e == 101 || e == 69) {                                   // exponent: 1e3px
                let sg: int = v[i + 1] as int;
                let dg: int = v[i + 1] as int;
                if (sg == 43 || sg == 45) dg = v[i + 2] as int;
                if (dg >= 48 && dg <= 57) {
                    i = i + 1; if (sg == 43 || sg == 45) i = i + 1;
                    while (i < n && (v[i] as int) >= 48 && (v[i] as int) <= 57) i = i + 1;
                }
            }
            while (i < n && (((v[i] as int) >= 97 && (v[i] as int) <= 122) || ((v[i] as int) >= 65 && (v[i] as int) <= 90))) i = i + 1;
            let tok: char* = strSub(v, a, i - a);
            let px: float = parseWidthPx(tok, ctx, 0.0);
            if (cnt == 0) st.rtl = px; else if (cnt == 1) st.rtr = px; else if (cnt == 2) st.rbr = px; else st.rbl = px;
            strFree(tok); cnt = cnt + 1;
        } else { i = i + 1; }
    }
    return cnt;
}

function applyPropBorder(st: Style*, name: char*, v: char*, ctx: CssCtx*, isFs: bool) => int {
    let res: int = AP_APPLIED;
    let l: Len; lenSet(&l, LU_AUTO, 0.0);
    let cr: CRes; cr.ok = false; cr.c = 0;
    let hit: bool = true;
    if (strEq(name, "border")) { applyBorder(st, 0, v, ctx); }
    else if (strEq(name, "border-top")) { applyBorder(st, 1, v, ctx); }
    else if (strEq(name, "border-right")) { applyBorder(st, 2, v, ctx); }
    else if (strEq(name, "border-bottom")) { applyBorder(st, 3, v, ctx); }
    else if (strEq(name, "border-left")) { applyBorder(st, 4, v, ctx); }
    else if (strEq(name, "border-width")) {                                      // 1-4 values, top right bottom left
        let w1: Len; let w2: Len; let w3: Len; let w4: Len;
        lenSet(&w1, LU_PX, st.bt); lenSet(&w2, LU_PX, st.br); lenSet(&w3, LU_PX, st.bb); lenSet(&w4, LU_PX, st.bl);
        if (!side4(v, ctx, &w1, &w2, &w3, &w4)) res = AP_INVALID;
        else if (w1.u != LU_PX || w2.u != LU_PX || w3.u != LU_PX || w4.u != LU_PX) res = AP_INVALID;
        else { st.bt = w1.v; st.br = w2.v; st.bb = w3.v; st.bl = w4.v; }
    }
    else if (inList(name, "border-top-width border-right-width border-bottom-width border-left-width")) {
        if (!parseLen(v, ctx, &l) || l.u != LU_PX) res = AP_INVALID;
        else if (strEq(name, "border-top-width")) st.bt = l.v;
        else if (strEq(name, "border-right-width")) st.br = l.v;
        else if (strEq(name, "border-bottom-width")) st.bb = l.v;
        else st.bl = l.v;
    }
    else if (inList(name, "border-top-style border-right-style border-bottom-style border-left-style")) {
        if (strEqI(v, "none") || strEqI(v, "hidden")) {
            if (strEq(name, "border-top-style")) st.bt = 0.0;
            else if (strEq(name, "border-right-style")) st.br = 0.0;
            else if (strEq(name, "border-bottom-style")) st.bb = 0.0;
            else st.bl = 0.0;
        }
    }
    else if (strEq(name, "border-color")) {
        if (strEqI(v, "initial")) st.borderColor = DEF_RULE;
        else if (strEqI(v, "currentcolor")) st.borderColor = st.color;
        else {
            parseColor(v, &cr);
            if (cr.ok) st.borderColor = cr.c;
            else {                                                               // `border-color: a b c d`: one colour only, the first
                let ft: char* = firstToken(v);
                if (strEqI(ft, "currentcolor")) { st.borderColor = st.color; res = AP_UNSUPPORTED; }
                else { parseColor(ft, &cr); if (cr.ok && strLen(ft) < strLen(v)) { st.borderColor = cr.c; res = AP_UNSUPPORTED; } else res = AP_INVALID; }
                strFree(ft);
            }
        }
    }
    else if (strEq(name, "border-style")) { if (strEqI(v, "none") || strEqI(v, "hidden")) { st.bt = 0.0; st.br = 0.0; st.bb = 0.0; st.bl = 0.0; } }
    else if (strEq(name, "border-radius")) {                                     // painted: Cmd.radius -> Pen2D.fillRoundRect / strokeRoundRect
        st.rmix = false;
        if (!containsStr(v, "%") && !containsStr(v, "/") && !containsStr(v, "(")) {
            // 1..4 plain lengths: different corners are painted per corner (background colour); borders / shadows use the largest
            st.rtl = 0.0; st.rtr = 0.0; st.rbr = 0.0; st.rbl = 0.0;
            let cnt: int = parseRadii(v, ctx, st);
            if (cnt >= 1) {
                let a: float = st.rtl; let b2: float = st.rtr; let c2: float = st.rbr; let d: float = st.rbl;
                let tl: float = a; let tr: float = a; let br: float = a; let bl: float = a;
                if (cnt == 2) { tr = b2; bl = b2; }
                else if (cnt == 3) { tr = b2; bl = b2; br = c2; }
                else if (cnt >= 4) { tr = b2; br = c2; bl = d; }
                st.radius = maxF(maxF(tl, tr), maxF(br, bl));
                st.rtl = tl; st.rtr = tr; st.rbr = br; st.rbl = bl;
                if (tl != tr || tl != br || tl != bl) st.rmix = true;
            } else res = AP_INVALID;
        }
        else if (containsStr(v, "%")) { st.radius = 9999.0; if (!strEq(v, "50%") && !strEq(v, "100%")) res = AP_UNSUPPORTED; }   // 50% / 100% = circle / pill (the painter clamps to half the box); other percentages approximated the same way
        else {
            // one radius for all corners: the largest of up to four values (mixed corners are approximated, so they stay UNSUPPORTED)
            let mx: float = 0.0; let k: int = 0; let more: bool = true; let differ: bool = false; let r0: float = 0.0;
            while (k < 4 && more) {
                let tok: char* = nthToken(v, k);
                if (tok == nullptr) { more = false; }
                else {
                    let r: float = parseWidthPx(tok, ctx, 0.0);
                    if (k == 0) r0 = r; else if (r != r0) differ = true;
                    if (r > mx) mx = r;
                    strFree(tok); k = k + 1;
                }
            }
            st.radius = mx;
            if (differ || containsStr(v, "/")) res = AP_UNSUPPORTED;
        }
    }
    else { hit = false; }
    if (!hit) res = AP_NOMATCH;
    return res;
}
