// standard/1/Ui/Css/Properties/Property.os
// Property dispatch (Blink: CSSProperty::ApplyValue). One declaration -> the first property group that knows the name.
//   Properties/Text.os Box.os Border.os Paint.os Display.os Flex.os Grid.os Misc.os
// Returns AP_APPLIED, AP_UNSUPPORTED (known but approximated / not implemented), AP_INVALID (value did not parse)
// or AP_SKIP (declaration belongs to the other pass). Pass 0 handles only font-size so `em` in the same rule uses the
// element's own size; pass 1 everything else.
// Keep cssSupportedProps() in sync when adding a property to a group.
function applyProp(st: Style*, name: char*, v: char*, ctx: CssCtx*, pass: int) => int {
    let isFs: bool = strEq(name, "font-size");
    if (strEq(name, "font")) return applyFontShorthand(st, v, ctx, pass);        // does its own pass handling
    if (pass == 0 && !isFs) return AP_SKIP;
    if (pass == 1 && isFs) return AP_SKIP;
    if (isCustomName(name)) return AP_APPLIED;                                    // stored in the var scope (Values/Var.os)
    if (containsStr(v, "var(")) return AP_UNSUPPORTED;                            // unresolved: applyDeclsRec resolves var() first
    // math functions: whole-value for single-value properties; margin/padding/inset split tokens themselves (side4)
    if (hasMathFn(v) && !inList(name, "margin padding inset margin-inline margin-block padding-inline padding-block border-width")) {
        let cl: Len; lenSet(&cl, LU_AUTO, 0.0);
        if (!startsMathFn(v) || !calcLen(v, ctx, &cl)) return AP_UNSUPPORTED;    // inside another shorthand, or % mixed with px
    }
    let res: int = applyPropText(st, name, v, ctx, isFs);
    if (res == AP_NOMATCH) res = applyPropBox(st, name, v, ctx, isFs);
    if (res == AP_NOMATCH) res = applyPropBorder(st, name, v, ctx, isFs);
    if (res == AP_NOMATCH) res = applyPropPaint(st, name, v, ctx, isFs);
    if (res == AP_NOMATCH) res = applyPropDisplay(st, name, v, ctx, isFs);
    if (res == AP_NOMATCH) res = applyPropFlex(st, name, v, ctx, isFs);
    if (res == AP_NOMATCH) res = applyPropGrid(st, name, v, ctx, isFs);
    if (res == AP_NOMATCH) res = applyPropMisc(st, name, v, ctx, isFs);
    if (res == AP_NOMATCH) res = AP_UNSUPPORTED;      // font-family, background-image, animation, ... (not implemented)
    return res;
}

// Property names the groups understand. Keep in sync with Properties/*.os.
function cssSupportedProps() => char* {
    return "font-size display position color background-color background font-weight font-style text-align text-decoration text-decoration-line line-height white-space width height min-width max-width min-height max-height margin margin-top margin-right margin-bottom margin-left margin-inline margin-block margin-inline-start margin-inline-end margin-block-start margin-block-end padding-inline padding-block padding-inline-start padding-inline-end padding-block-start padding-block-end box-shadow text-shadow text-rendering -webkit-font-smoothing -moz-osx-font-smoothing font-smooth text-size-adjust -webkit-text-size-adjust padding padding-top padding-right padding-bottom padding-left border border-top border-right border-bottom border-left border-width border-color border-style border-radius box-sizing overflow overflow-x overflow-y opacity visibility z-index top right bottom left inset flex-direction flex-wrap flex-flow flex-grow flex-shrink flex-basis flex gap row-gap column-gap grid-gap grid-row-gap grid-column-gap grid-template-columns align-items align-self justify-content place-items place-self cursor list-style list-style-type font border-top-width border-right-width border-bottom-width border-left-width border-top-style border-right-style border-bottom-style border-left-style transition transition-property transition-duration transition-timing-function transition-delay transition-behavior will-change -webkit-tap-highlight-color scroll-behavior touch-action";
}

function cssIsSupported(name: char*) => bool {
    let r: bool = inList(name, cssSupportedProps());
    if (isCustomName(name)) r = true;
    return r;
}
