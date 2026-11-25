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
        let buf: char* = CMem.malloc(96) as char*;
        let sawImage: bool = false; let bad: bool = false;
        st.bg = 0;
        while (lx.token(buf, 96)) {
            let c0: int = buf[0] as int;
            if (strEqI(buf, "currentcolor")) { st.bg = st.color; }
            else if (strEqI(buf, "initial") || strEqI(buf, "none")) { }
            else if (startsWith(buf, "url(") || containsStr(buf, "gradient(")) { sawImage = true; }
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
                else if (inList(buf, "left right top bottom center repeat no-repeat repeat-x repeat-y space round cover contain scroll fixed local border-box padding-box content-box")) { }
                else bad = true;
            }
        }
        CMem.free(buf as void*);
        if (bad) res = AP_INVALID;
        else if (sawImage) res = AP_UNSUPPORTED;                                    // NEXT: url(), gradients
    }
    else if (inList(name, "background-size background-position background-repeat background-attachment background-origin background-clip background-blend-mode mix-blend-mode backdrop-filter -webkit-backdrop-filter")) { }   // accepted, no visual effect: they only refine an image (background-image is the reported gap) or are compositing effects not drawn here
    else if (strEq(name, "box-shadow") || strEq(name, "text-shadow")) { if (!strEqI(v, "none") && !strEqI(v, "initial")) res = AP_UNSUPPORTED; }   // shadows are not painted yet
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
