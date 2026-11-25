// standard/1/Ui/Css/Properties/Grid.os
// Property handlers: grid-template-columns
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
    else { hit = false; }
    if (!hit) res = AP_NOMATCH;
    return res;
}
