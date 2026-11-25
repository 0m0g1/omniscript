// standard/1/Ui/Css/Properties/Box.os
// Property handlers: width/height (+min/max), margin*, padding*, logical margin/padding, box-sizing
// applyPropBox returns AP_NOMATCH when `name` is not one of its properties, so Property.os tries the next group.
// To add a property: put one `else if (strEq(name, "...")) { ... }` branch here and the name in cssSupportedProps().

function applyPropBox(st: Style*, name: char*, v: char*, ctx: CssCtx*, isFs: bool) => int {
    let res: int = AP_APPLIED;
    let l: Len; lenSet(&l, LU_AUTO, 0.0);
    let cr: CRes; cr.ok = false; cr.c = 0;
    let hit: bool = true;
    if (strEq(name, "width")) { if (parseLen(v, ctx, &l) && l.u != LU_NONE) { st.width.u = l.u; st.width.v = l.v; } else res = AP_INVALID; }
    else if (strEq(name, "height")) { if (parseLen(v, ctx, &l) && l.u != LU_NONE) { st.height.u = l.u; st.height.v = l.v; } else res = AP_INVALID; }
    else if (strEq(name, "min-width")) { if (parseLen(v, ctx, &l)) { st.minW.u = l.u; st.minW.v = l.v; } else res = AP_INVALID; }
    else if (strEq(name, "max-width")) { if (parseLen(v, ctx, &l)) { st.maxW.u = l.u; st.maxW.v = l.v; } else res = AP_INVALID; }
    else if (strEq(name, "min-height")) { if (parseLen(v, ctx, &l)) { st.minH.u = l.u; st.minH.v = l.v; } else res = AP_INVALID; }
    else if (strEq(name, "max-height")) { if (parseLen(v, ctx, &l)) { st.maxH.u = l.u; st.maxH.v = l.v; } else res = AP_INVALID; }
    else if (strEq(name, "margin")) { if (!side4(v, ctx, &st.mt, &st.mr, &st.mb, &st.ml)) res = AP_INVALID; }
    else if (strEq(name, "margin-top")) { if (parseLen(v, ctx, &l)) { st.mt.u = l.u; st.mt.v = l.v; } else res = AP_INVALID; }
    else if (strEq(name, "margin-right")) { if (parseLen(v, ctx, &l)) { st.mr.u = l.u; st.mr.v = l.v; } else res = AP_INVALID; }
    else if (strEq(name, "margin-bottom")) { if (parseLen(v, ctx, &l)) { st.mb.u = l.u; st.mb.v = l.v; } else res = AP_INVALID; }
    else if (strEq(name, "margin-left")) { if (parseLen(v, ctx, &l)) { st.ml.u = l.u; st.ml.v = l.v; } else res = AP_INVALID; }
    else if (strEq(name, "padding")) { if (!side4(v, ctx, &st.pt, &st.pr, &st.pb, &st.pl)) res = AP_INVALID; }
    else if (strEq(name, "padding-top")) { if (parseLen(v, ctx, &l)) { st.pt.u = l.u; st.pt.v = l.v; } else res = AP_INVALID; }
    else if (strEq(name, "padding-right")) { if (parseLen(v, ctx, &l)) { st.pr.u = l.u; st.pr.v = l.v; } else res = AP_INVALID; }
    else if (strEq(name, "padding-bottom")) { if (parseLen(v, ctx, &l)) { st.pb.u = l.u; st.pb.v = l.v; } else res = AP_INVALID; }
    else if (strEq(name, "padding-left")) { if (parseLen(v, ctx, &l)) { st.pl.u = l.u; st.pl.v = l.v; } else res = AP_INVALID; }
    else if (inList(name, "margin-inline margin-block padding-inline padding-block")) {      // LTR, horizontal writing mode
        let a1: Len; let a2: Len; let a3: Len; let a4: Len;
        lenSet(&a1, LU_PX, 0.0); lenSet(&a2, LU_PX, 0.0); lenSet(&a3, LU_PX, 0.0); lenSet(&a4, LU_PX, 0.0);
        if (!side4(v, ctx, &a1, &a2, &a3, &a4)) res = AP_INVALID;
        else if (strEq(name, "margin-inline")) { copyLen(&st.ml, &a1); copyLen(&st.mr, &a2); }
        else if (strEq(name, "margin-block")) { copyLen(&st.mt, &a1); copyLen(&st.mb, &a2); }
        else if (strEq(name, "padding-inline")) { copyLen(&st.pl, &a1); copyLen(&st.pr, &a2); }
        else { copyLen(&st.pt, &a1); copyLen(&st.pb, &a2); }
    }
    else if (inList(name, "margin-inline-start margin-inline-end margin-block-start margin-block-end padding-inline-start padding-inline-end padding-block-start padding-block-end")) {
        if (!parseLen(v, ctx, &l)) res = AP_INVALID;
        else if (strEq(name, "margin-inline-start")) copyLen(&st.ml, &l);
        else if (strEq(name, "margin-inline-end")) copyLen(&st.mr, &l);
        else if (strEq(name, "margin-block-start")) copyLen(&st.mt, &l);
        else if (strEq(name, "margin-block-end")) copyLen(&st.mb, &l);
        else if (strEq(name, "padding-inline-start")) copyLen(&st.pl, &l);
        else if (strEq(name, "padding-inline-end")) copyLen(&st.pr, &l);
        else if (strEq(name, "padding-block-start")) copyLen(&st.pt, &l);
        else copyLen(&st.pb, &l);
    }
    else if (strEq(name, "box-sizing")) { st.boxSizing = 0; if (strEqI(v, "border-box")) st.boxSizing = 1; else if (!strEqI(v, "content-box")) res = AP_INVALID; }
    else { hit = false; }
    if (!hit) res = AP_NOMATCH;
    return res;
}
