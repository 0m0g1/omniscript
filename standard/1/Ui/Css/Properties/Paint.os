// standard/1/Ui/Css/Properties/Paint.os
// Property handlers: color, background(-color), box-shadow, opacity, visibility
// applyPropPaint returns AP_NOMATCH when `name` is not one of its properties, so Property.os tries the next group.
// To add a property: put one `else if (strEq(name, "...")) { ... }` branch here and the name in cssSupportedProps().

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
        let lx: Lex; lx.s = v; lx.i = 0; lx.ok = false; lx.val = 0.0;
        let buf: char* = CMem.malloc(96) as char*;
        let gotBg: bool = false;
        if (strEqI(v, "initial") || strEqI(v, "none")) { st.bg = 0; gotBg = true; }
        while (lx.token(buf, 96)) {
            if (strEqI(buf, "currentcolor")) { st.bg = st.color; gotBg = true; }
            else { parseColor(buf, &cr); if (cr.ok) { st.bg = cr.c; gotBg = true; } }
        }      // NEXT: url(), gradients
        CMem.free(buf as void*);
        if (!gotBg) res = AP_UNSUPPORTED;                                           // image / gradient only
    }
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
