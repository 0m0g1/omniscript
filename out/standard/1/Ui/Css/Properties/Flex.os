// standard/1/Ui/Css/Properties/Flex.os
// Property handlers: flex*, gap, align-*, justify-content, place-*
// applyPropFlex returns AP_NOMATCH when `name` is not one of its properties, so Property.os tries the next group.
// To add a property: put one `else if (strEq(name, "...")) { ... }` branch here and the name in cssSupportedProps().

function applyPropFlex(st: Style*, name: char*, v: char*, ctx: CssCtx*, isFs: bool) => int {
    let res: int = AP_APPLIED;
    let l: Len; lenSet(&l, LU_AUTO, 0.0);
    let cr: CRes; cr.ok = false; cr.c = 0;
    let hit: bool = true;
    if (strEq(name, "flex-direction")) {
        if (strEqI(v, "row")) st.flexDir = FD_ROW;
        else if (strEqI(v, "column")) st.flexDir = FD_COLUMN;
        else if (strEqI(v, "row-reverse")) st.flexDir = FD_ROW_REV;
        else if (strEqI(v, "column-reverse")) st.flexDir = FD_COL_REV;
        else res = AP_INVALID;
    }
    else if (strEq(name, "flex-wrap")) { st.wrap = strEqI(v, "wrap") || strEqI(v, "wrap-reverse"); }
    else if (strEq(name, "flex-flow")) {
        let lx: Lex; lx.s = v; lx.i = 0; lx.ok = false; lx.val = 0.0;
        let buf: char* = CMem.malloc(32) as char*;
        while (lx.token(buf, 32)) {
            if (strEqI(buf, "row")) st.flexDir = FD_ROW;
            else if (strEqI(buf, "column")) st.flexDir = FD_COLUMN;
            else if (strEqI(buf, "row-reverse")) st.flexDir = FD_ROW_REV;
            else if (strEqI(buf, "column-reverse")) st.flexDir = FD_COL_REV;
            else if (strEqI(buf, "wrap")) st.wrap = true;
            else if (strEqI(buf, "nowrap")) st.wrap = false;
        }
        CMem.free(buf as void*);
    }
    else if (strEq(name, "flex-grow")) { st.grow = parseFloatStr(v, 0.0); }
    else if (strEq(name, "flex-shrink")) { st.shrink = parseFloatStr(v, 1.0); }
    else if (strEq(name, "flex-basis")) { if (parseLen(v, ctx, &l)) { st.basis.u = l.u; st.basis.v = l.v; } else res = AP_INVALID; }
    else if (strEq(name, "flex")) {
        if (strEqI(v, "none")) { st.grow = 0.0; st.shrink = 0.0; lenSet(&st.basis, LU_AUTO, 0.0); }
        else if (strEqI(v, "auto")) { st.grow = 1.0; st.shrink = 1.0; lenSet(&st.basis, LU_AUTO, 0.0); }
        else {
            let lx: Lex; lx.s = v; lx.i = 0; lx.ok = false; lx.val = 0.0;
            let buf: char* = CMem.malloc(48) as char*;
            let nums: int = 0; let basisSet: bool = false;
            while (lx.token(buf, 48)) {
                let t: Lex; t.s = buf; t.i = 0; t.ok = false; t.val = 0.0;
                let x: float = t.number();
                if (t.ok && t.atEnd()) {
                    if (nums == 0) st.grow = x; else st.shrink = x;
                    nums = nums + 1;
                } else if (parseLen(buf, ctx, &l)) { st.basis.u = l.u; st.basis.v = l.v; basisSet = true; }
                else res = AP_INVALID;
            }
            CMem.free(buf as void*);
            if (nums == 1) st.shrink = 1.0;
            if (nums > 0 && !basisSet) lenSet(&st.basis, LU_PX, 0.0);            // `flex: 1` means 1 1 0
        }
    }
    else if (strEq(name, "gap") || strEq(name, "row-gap") || strEq(name, "column-gap") || strEq(name, "grid-gap") || strEq(name, "grid-row-gap") || strEq(name, "grid-column-gap")) {
        st.gap = parseWidthPx(v, ctx, st.gap);                                    // one gap for both axes
        if (containsStr(v, " ")) res = AP_UNSUPPORTED;                            // `gap: 6px 18px`: first value only
    }
    else if (strEq(name, "align-items")) {
        let ai: int = alignKeyword(v, 0 - 99);
        if (ai == 0 - 99) res = AP_INVALID; else st.alignItems = ai;
    }
    else if (strEq(name, "align-self")) {
        let asf: int = alignKeyword(v, 0 - 99);
        if (asf == 0 - 99) res = AP_INVALID; else st.alignSelf = asf;
    }
    else if (strEq(name, "justify-content")) {
        if (strEqI(v, "flex-start") || strEqI(v, "start") || strEqI(v, "left") || strEqI(v, "normal")) st.justify = JC_START;
        else if (strEqI(v, "flex-end") || strEqI(v, "end") || strEqI(v, "right")) st.justify = JC_END;
        else if (strEqI(v, "center")) st.justify = JC_CENTER;
        else if (strEqI(v, "space-between")) st.justify = JC_BETWEEN;
        else if (strEqI(v, "space-around")) st.justify = JC_AROUND;
        else if (strEqI(v, "space-evenly")) st.justify = JC_EVENLY;
        else res = AP_INVALID;
    }
    else if (strEq(name, "place-items") || strEq(name, "place-self")) {          // align axis only; the justify axis is not modelled
        let ft: char* = firstToken(v);
        let pa: int = alignKeyword(ft, 0 - 99);
        strFree(ft);
        if (pa == 0 - 99) res = AP_INVALID;
        else { if (strEq(name, "place-items")) st.alignItems = pa; else st.alignSelf = pa; res = AP_UNSUPPORTED; }
    }
    else { hit = false; }
    if (!hit) res = AP_NOMATCH;
    return res;
}
