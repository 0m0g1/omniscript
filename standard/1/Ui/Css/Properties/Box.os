// standard/1/Ui/Css/Properties/Box.os
// Property handlers: width/height (+min/max), aspect-ratio, margin*, padding*, logical margin/padding, box-sizing
// applyPropBox returns AP_NOMATCH when `name` is not one of its properties, so Property.os tries the next group.
// To add a property: put one `else if (strEq(name, "...")) { ... }` branch here and the name in cssSupportedProps().

// aspect-ratio value -> width / height. 0.0 = auto, -1.0 = invalid. Accepts "auto", "16/9", "16 / 9", "1.5", "auto 16/9", "16/9 auto".
function parseRatio(v: char*) => float {
    let res: float = -1.0;
    if (strEqI(v, "auto")) { res = 0.0; }
    else {
        let len: int = strLen(v);
        let a: char* = CMem.malloc((len + 1) as u64) as char*;
        let d: char* = CMem.malloc((len + 1) as u64) as char*;
        let na: int = 0; let nd: int = 0; let inDen: bool = false; let bad: bool = false; let i: int = 0;
        while (i < len) {
            let ch: int = v[i] as int;
            if ((ch >= 48 && ch <= 57) || ch == 46) {
                if (inDen) { d[nd] = ch as char; nd = nd + 1; } else { a[na] = ch as char; na = na + 1; }
            }
            else if (ch == 47) { if (inDen) bad = true; inDen = true; }
            else if (ch == 32 || ch == 9 || (ch >= 65 && ch <= 90) || (ch >= 97 && ch <= 122)) { }   // spaces and the `auto` keyword
            else { bad = true; }
            i = i + 1;
        }
        a[na] = 0 as char; d[nd] = 0 as char;
        if (!bad && na > 0 && (!inDen || nd > 0)) {
            let num: float = parseFloatStr(a, 0.0);
            let den: float = 1.0;
            if (inDen) den = parseFloatStr(d, 0.0);
            if (num > 0.0 && den > 0.0) res = num / den;
        }
        CMem.free(a as void*); CMem.free(d as void*);
    }
    return res;
}

function applyPropBox(st: Style*, name: char*, v: char*, ctx: CssCtx*, isFs: bool) => int {
    let res: int = AP_APPLIED;
    let l: Len; lenSet(&l, LU_AUTO, 0.0);
    let cr: CRes; cr.ok = false; cr.c = 0;
    let hit: bool = true;
    if (strEq(name, "width")) {
        st.wkw = 0;
        if (strEqI(v, "min-content")) { st.width.u = LU_AUTO; st.width.v = 0.0; st.wkw = 1; }          // sized in Block.placeBlock (minPref)
        else if (strEqI(v, "max-content")) { st.width.u = LU_AUTO; st.width.v = 0.0; st.wkw = 2; }
        else if (strEqI(v, "fit-content")) { st.width.u = LU_AUTO; st.width.v = 0.0; st.wkw = 3; }
        else if (parseLen(v, ctx, &l) && l.u != LU_NONE) { st.width.u = l.u; st.width.v = l.v; }
        else res = AP_INVALID;
    }
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
    else if (strEq(name, "aspect-ratio")) { let r: float = parseRatio(v); if (r < 0.0) res = AP_INVALID; else st.aspect = r; }
    else if (strEq(name, "box-sizing")) { st.boxSizing = 0; if (strEqI(v, "border-box")) st.boxSizing = 1; else if (!strEqI(v, "content-box")) res = AP_INVALID; }
    else { hit = false; }
    if (!hit) res = AP_NOMATCH;
    return res;
}
