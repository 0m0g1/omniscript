// standard/1/Ui/Css/Properties/Display.os
// Property handlers: display, position, offsets, z-index, overflow, cursor, list-style
// applyPropDisplay returns AP_NOMATCH when `name` is not one of its properties, so Property.os tries the next group.
// To add a property: put one `else if (strEq(name, "...")) { ... }` branch here and the name in cssSupportedProps().

function applyPropDisplay(st: Style*, name: char*, v: char*, ctx: CssCtx*, isFs: bool) => int {
    let res: int = AP_APPLIED;
    let l: Len; lenSet(&l, LU_AUTO, 0.0);
    let cr: CRes; cr.ok = false; cr.c = 0;
    let hit: bool = true;
    if (strEq(name, "display")) {
        if (!setDisplay(st, v)) res = AP_INVALID;
        else if (strEqI(v, "contents")) res = AP_UNSUPPORTED;                       // treated as block
    }
    else if (strEq(name, "position")) {
        if (strEqI(v, "static")) st.position = PS_STATIC;
        else if (strEqI(v, "relative")) st.position = PS_RELATIVE;
        else if (strEqI(v, "sticky")) { st.position = PS_RELATIVE; res = AP_UNSUPPORTED; }   // NEXT: sticky
        else if (strEqI(v, "absolute")) st.position = PS_ABSOLUTE;
        else if (strEqI(v, "fixed")) st.position = PS_FIXED;
        else res = AP_INVALID;
    }
    else if (strEq(name, "overflow") || strEq(name, "overflow-x") || strEq(name, "overflow-y")) {
        let ov: int = overflowKeyword(v, 0 - 99);
        if (ov == 0 - 99) res = AP_INVALID; else st.overflow = ov;                   // x and y share one value
    }
    else if (strEq(name, "z-index")) { if (!strEqI(v, "auto")) st.zIndex = parseFloatStr(v, 0.0) as int; else st.zIndex = 0; }
    else if (strEq(name, "top")) { if (parseLen(v, ctx, &l)) { st.top.u = l.u; st.top.v = l.v; } else res = AP_INVALID; }
    else if (strEq(name, "right")) { if (parseLen(v, ctx, &l)) { st.right.u = l.u; st.right.v = l.v; } else res = AP_INVALID; }
    else if (strEq(name, "bottom")) { if (parseLen(v, ctx, &l)) { st.bottom.u = l.u; st.bottom.v = l.v; } else res = AP_INVALID; }
    else if (strEq(name, "left")) { if (parseLen(v, ctx, &l)) { st.left.u = l.u; st.left.v = l.v; } else res = AP_INVALID; }
    else if (strEq(name, "inset")) { if (!side4(v, ctx, &st.top, &st.right, &st.bottom, &st.left)) res = AP_INVALID; }
    else if (strEq(name, "cursor")) {
        if (strEqI(v, "pointer")) st.cursor = CU_POINTER;
        else if (strEqI(v, "text")) st.cursor = CU_TEXT;
        else st.cursor = CU_DEFAULT;
    }
    else if (strEq(name, "list-style") || strEq(name, "list-style-type")) {
        if (strEqI(v, "none")) st.listStyle = LS_NONE;
        else if (strEqI(v, "disc")) st.listStyle = LS_DISC;
        else if (strEqI(v, "circle")) st.listStyle = LS_CIRCLE;
        else if (strEqI(v, "square")) st.listStyle = LS_SQUARE;
        else if (strEqI(v, "decimal")) st.listStyle = LS_DECIMAL;
        else res = AP_UNSUPPORTED;                                                // lower-alpha, upper-roman, ...
    }
    else { hit = false; }
    if (!hit) res = AP_NOMATCH;
    return res;
}
