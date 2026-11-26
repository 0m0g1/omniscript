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
    else if (inList(name, "text-decoration-color text-decoration-thickness text-underline-offset color-scheme")) { }
        // decoration is drawn in the text colour at the default thickness; the dark/light UA scheme is fixed. Cosmetic only, nothing structural is lost
    else if (strEq(name, "transform")) {
        // only the identity forms are exact; real transforms (translate / rotate / scale) are not applied
        if (!inList(v, "none translateY(0px) translateY(0) translateX(0px) translateX(0) translateZ(0) translateZ(0px) translate(0) translate(0px) translate(0,0) translate3d(0,0,0)")) res = AP_UNSUPPORTED;
    }
    else if (strEq(name, "fill")) {
        // svg icons are painted in the text colour, so a solid fill colour is applied as the element's text colour
        // (currentColor keeps it; none / gradients / url(#id) stay unsupported)
        if (inList(v, "currentcolor currentColor")) { }
        else {
            parseColor(v, &cr);
            if (cr.ok) st.color = cr.c;
            else res = AP_UNSUPPORTED;
        }
    }
    else if (strEq(name, "filter")) {
        // only invert(100%) is drawn: the browser inverts the colours of <img> bitmaps inside the element when they load
        // (a trailing saturate(0%) / brightness(>=100%) changes nothing on the black-to-white icons this is used for)
        st.fInv = false;
        if (strEqI(v, "none") || strEqI(v, "initial")) { }
        else if (containsStr(v, "invert(100%)") || strEq(v, "invert(1)")) st.fInv = true;
        else res = AP_UNSUPPORTED;
    }
    else if (strEq(name, "outline")) {
        // `outline: none` / `0` is the common reset and is exactly what we render (no outline). A real outline is not painted yet.
        if (!(strEq(v, "none") || strEq(v, "0") || strEq(v, "0px"))) res = AP_UNSUPPORTED;
    }
    else if (strEq(name, "object-fit")) {
        // <img> content inside its box: drawn by Paint (NK_REPLACED). fill = stretch (the initial value).
        if (strEqI(v, "fill") || strEqI(v, "initial")) st.objFit = 0;
        else if (strEqI(v, "cover")) st.objFit = 1;
        else if (strEqI(v, "contain")) st.objFit = 2;
        else if (strEqI(v, "none")) st.objFit = 3;
        else if (strEqI(v, "scale-down")) st.objFit = 4;
        else res = AP_INVALID;
    }
    else if (strEq(name, "object-position")) {
        // keywords and percentages only (same grammar as background-position); lengths fall back to center
        let lx: Lex; lx.s = v; lx.i = 0; lx.ok = false; lx.val = 0.0;
        let b0: char* = CMem.malloc(64) as char*; let b1: char* = CMem.malloc(64) as char*;
        b0[0] = 0 as char; b1[0] = 0 as char;
        if (lx.token(b0, 64)) { lx.token(b1, 64); }
        if (strLen(b1) == 0) {
            if (strEqI(b0, "top") || strEqI(b0, "bottom")) { st.objFx = 0.5; st.objFy = bgPosTok(b0, 0.5); }
            else { st.objFx = bgPosTok(b0, 0.5); st.objFy = 0.5; }
        } else if (strEqI(b0, "top") || strEqI(b0, "bottom") || strEqI(b1, "left") || strEqI(b1, "right")) {
            st.objFy = bgPosTok(b0, 0.5); st.objFx = bgPosTok(b1, 0.5);
        } else { st.objFx = bgPosTok(b0, 0.5); st.objFy = bgPosTok(b1, 0.5); }
        CMem.free(b0 as void*); CMem.free(b1 as void*);
    }
    else if (strEq(name, "background-image")) {
        // `none` is the initial value; gradients / url() need Paint support
        if (!strEq(v, "none")) res = AP_UNSUPPORTED;
    }
    else { hit = false; }
    if (!hit) res = AP_NOMATCH;
    return res;
}
