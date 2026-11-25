// standard/1/Ui/Css/Properties/Misc.os
// Property handlers: properties that are accepted and have no effect here (transitions, will-change, ...)
// applyPropMisc returns AP_NOMATCH when `name` is not one of its properties, so Property.os tries the next group.
// To add a property: put one `else if (strEq(name, "...")) { ... }` branch here and the name in cssSupportedProps().

function applyPropMisc(st: Style*, name: char*, v: char*, ctx: CssCtx*, isFs: bool) => int {
    let res: int = AP_APPLIED;
    let l: Len; lenSet(&l, LU_AUTO, 0.0);
    let cr: CRes; cr.ok = false; cr.c = 0;
    let hit: bool = true;
    if (inList(name, "transition transition-property transition-duration transition-timing-function transition-delay transition-behavior will-change -webkit-tap-highlight-color scroll-behavior touch-action")) { }
        // no animation / touch / scroll-smoothing engine: the end state of a transition is the style that is applied, so nothing is lost
    else { hit = false; }
    if (!hit) res = AP_NOMATCH;
    return res;
}
