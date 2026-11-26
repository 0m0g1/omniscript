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
        st.gn = 0;
        if (!strEqI(v, "none") && !gridParseTracks(v, ctx, st)) { st.gn = 0; res = AP_UNSUPPORTED; }   // auto-fill / auto-fit, named lines, fit-content
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
