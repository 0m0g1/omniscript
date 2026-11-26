// standard/1/Ui/Css/Properties/Grid.os
// Property handlers: grid-template-columns, grid-column, grid-auto-rows, grid-auto-flow, user-select
// applyPropGrid returns AP_NOMATCH when `name` is not one of its properties, so Property.os tries the next group.
// To add a property: put one `else if (strEq(name, "...")) { ... }` branch here and the name in cssSupportedProps().

function applyPropGrid(st: Style*, name: char*, v: char*, ctx: CssCtx*, isFs: bool) => int {
    let res: int = AP_APPLIED;
    let l: Len; lenSet(&l, LU_AUTO, 0.0);
    let cr: CRes; cr.ok = false; cr.c = 0;
    let hit: bool = true;
    if (strEq(name, "grid-template-columns")) {
        st.gn = 0; st.gAuto = 0; st.gAutoMin = 0.0;
        if (!strEqI(v, "none") && !gridParseTracks(v, ctx, st)) { st.gn = 0; res = AP_UNSUPPORTED; }   // auto-fill / auto-fit, named lines, fit-content
        // repeat(auto-fill | auto-fit, minmax(<px>, ..) | <px>): keep the minimum so layout can count the columns
        let kind: int = 0;
        if (startsWith(v, "repeat(auto-fill")) kind = 1; else if (startsWith(v, "repeat(auto-fit")) kind = 2;
        if (kind != 0 && st.gn > 0) {
            let n: int = strLen(v); let i: int = 0;
            while (i < n && (v[i] as int) != 44) i = i + 1;
            let inner: char* = trimDup(v, i + 1, n - 1);
            let il: int = strLen(inner);
            let a: int = 0; let z: int = il;
            if (startsWith(inner, "minmax(")) {
                a = 7; z = 7;
                while (z < il && (inner[z] as int) != 44) z = z + 1;
            }
            let minTok: char* = trimDup(inner, a, z);
            let ml: Len; lenSet(&ml, LU_AUTO, 0.0);
            if (parseLen(minTok, ctx, &ml) && ml.u == LU_PX && ml.v > 0.0) { st.gAuto = kind; st.gAutoMin = ml.v; }
            strFree(minTok); strFree(inner);
        }
    }
    else if (strEq(name, "grid-column")) {                                      // item placement: `1 / -1`, `span 2`, `2 / span 2`, `3`
        st.gcs = 0; st.gce = 0; st.gsp = 0;
        if (!gridParseLine(v, st)) { st.gcs = 0; st.gce = 0; st.gsp = 0; res = AP_UNSUPPORTED; }   // named lines
    }
    else if (strEq(name, "grid-auto-rows")) {
        // rows are always as tall as their tallest item (= min-content / auto); other sizes are not implemented
        if (!strEqI(v, "min-content") && !strEqI(v, "auto") && !strEqI(v, "max-content")) res = AP_UNSUPPORTED;
    }
    else if (strEq(name, "grid-auto-flow")) {
        if (!strEqI(v, "row")) res = AP_UNSUPPORTED;                             // column / dense are not implemented
    }
    else if (strEq(name, "user-select")) { }                                  // no text selection in the painter: nothing to apply
    else { hit = false; }
    if (!hit) res = AP_NOMATCH;
    return res;
}
