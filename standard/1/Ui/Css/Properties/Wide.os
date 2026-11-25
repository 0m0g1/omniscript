// standard/1/Ui/Css/Properties/Wide.os
// CSS-wide keywords (Blink: CSSValue::IsCSSWideKeyword, StyleCascade::ResolveKeyword): inherit, initial, unset, revert, revert-layer.
// Called from applyProp before any property group, so every property supports them without touching its handler.
//   inherit  -> the parent's computed value (ctx.parent, set by cascadeNode)
//   initial  -> the value of a freshly reset Style
//   unset    -> inherit for inherited properties, initial for the rest
//   revert / revert-layer -> approximated as unset (the UA sheet here is the tag row, which cascadeNode already applied first)
// A property that copyProp does not know returns AP_UNSUPPORTED (it is shown in the inspector, nothing is silently dropped).

function isCssWide(v: char*) => bool {
    if (strEq(v, "inherit") || strEq(v, "initial") || strEq(v, "unset")) return true;
    if (strEq(v, "revert") || strEq(v, "revert-layer")) return true;
    return false;
}

// properties that inherit by default (`unset` = inherit for these)
function cssIsInheritedProp(name: char*) => bool {
    return inList(name, "color font-size font-weight font-style text-align line-height white-space visibility cursor list-style list-style-type");
}

// copy the Style field(s) that `name` controls from s to d. false = name not mapped.
function copyCssProp(d: Style*, s: Style*, name: char*) => bool {
    let ok: bool = true;
    if (strEq(name, "color")) { d.color = s.color; }
    else if (inList(name, "background-color background")) { d.bg = s.bg; }
    else if (strEq(name, "border-color")) { d.borderColor = s.borderColor; }
    else if (strEq(name, "border-radius")) { d.radius = s.radius; }
    else if (strEq(name, "opacity")) { d.opacity = s.opacity; }
    else if (strEq(name, "z-index")) { d.zIndex = s.zIndex; }
    else if (strEq(name, "display")) { d.display = s.display; }
    else if (strEq(name, "position")) { d.position = s.position; }
    else if (strEq(name, "visibility")) { d.hidden = s.hidden; }
    else if (strEq(name, "box-sizing")) { d.boxSizing = s.boxSizing; }
    else if (inList(name, "overflow overflow-x overflow-y")) { d.overflow = s.overflow; }
    else if (strEq(name, "cursor")) { d.cursor = s.cursor; }
    else if (inList(name, "list-style list-style-type")) { d.listStyle = s.listStyle; }
    else if (strEq(name, "font-size")) { d.fontPx = s.fontPx; }
    else if (strEq(name, "font-weight")) { d.bold = s.bold; }
    else if (strEq(name, "font-style")) { d.italic = s.italic; }
    else if (strEq(name, "text-align")) { d.textAlign = s.textAlign; }
    else if (strEq(name, "line-height")) { d.lineH = s.lineH; }
    else if (strEq(name, "white-space")) { d.whiteSpace = s.whiteSpace; }
    else if (inList(name, "text-decoration text-decoration-line")) { d.underline = s.underline; d.strike = s.strike; }
    else if (strEq(name, "width")) { d.width = s.width; }
    else if (strEq(name, "height")) { d.height = s.height; }
    else if (strEq(name, "min-width")) { d.minW = s.minW; }
    else if (strEq(name, "max-width")) { d.maxW = s.maxW; }
    else if (strEq(name, "min-height")) { d.minH = s.minH; }
    else if (strEq(name, "max-height")) { d.maxH = s.maxH; }
    else if (strEq(name, "margin")) { d.mt = s.mt; d.mr = s.mr; d.mb = s.mb; d.ml = s.ml; }
    else if (strEq(name, "margin-top")) { d.mt = s.mt; }
    else if (strEq(name, "margin-right")) { d.mr = s.mr; }
    else if (strEq(name, "margin-bottom")) { d.mb = s.mb; }
    else if (strEq(name, "margin-left")) { d.ml = s.ml; }
    else if (strEq(name, "margin-inline")) { d.ml = s.ml; d.mr = s.mr; }
    else if (strEq(name, "margin-block")) { d.mt = s.mt; d.mb = s.mb; }
    else if (strEq(name, "padding")) { d.pt = s.pt; d.pr = s.pr; d.pb = s.pb; d.pl = s.pl; }
    else if (strEq(name, "padding-top")) { d.pt = s.pt; }
    else if (strEq(name, "padding-right")) { d.pr = s.pr; }
    else if (strEq(name, "padding-bottom")) { d.pb = s.pb; }
    else if (strEq(name, "padding-left")) { d.pl = s.pl; }
    else if (strEq(name, "padding-inline")) { d.pl = s.pl; d.pr = s.pr; }
    else if (strEq(name, "padding-block")) { d.pt = s.pt; d.pb = s.pb; }
    else if (inList(name, "border border-width")) { d.bt = s.bt; d.br = s.br; d.bb = s.bb; d.bl = s.bl; d.borderColor = s.borderColor; }
    else if (inList(name, "border-top border-top-width")) { d.bt = s.bt; }
    else if (inList(name, "border-right border-right-width")) { d.br = s.br; }
    else if (inList(name, "border-bottom border-bottom-width")) { d.bb = s.bb; }
    else if (inList(name, "border-left border-left-width")) { d.bl = s.bl; }
    else if (inList(name, "gap row-gap column-gap grid-gap grid-row-gap grid-column-gap")) { d.gap = s.gap; }
    else if (strEq(name, "flex-direction")) { d.flexDir = s.flexDir; }
    else if (strEq(name, "flex-wrap")) { d.wrap = s.wrap; }
    else if (strEq(name, "flex-flow")) { d.flexDir = s.flexDir; d.wrap = s.wrap; }
    else if (strEq(name, "flex-grow")) { d.grow = s.grow; }
    else if (strEq(name, "flex-shrink")) { d.shrink = s.shrink; }
    else if (strEq(name, "flex-basis")) { d.basis = s.basis; }
    else if (strEq(name, "flex")) { d.grow = s.grow; d.shrink = s.shrink; d.basis = s.basis; }
    else if (strEq(name, "align-items")) { d.alignItems = s.alignItems; }
    else if (strEq(name, "align-self")) { d.alignSelf = s.alignSelf; }
    else if (strEq(name, "justify-content")) { d.justify = s.justify; }
    else if (strEq(name, "place-items")) { d.alignItems = s.alignItems; }
    else if (strEq(name, "place-self")) { d.alignSelf = s.alignSelf; }
    else if (strEq(name, "top")) { d.top = s.top; }
    else if (strEq(name, "right")) { d.right = s.right; }
    else if (strEq(name, "bottom")) { d.bottom = s.bottom; }
    else if (strEq(name, "left")) { d.left = s.left; }
    else if (strEq(name, "inset")) { d.top = s.top; d.right = s.right; d.bottom = s.bottom; d.left = s.left; }
    else { ok = false; }
    return ok;
}

// Applies `inherit | initial | unset | revert | revert-layer` for one property. Returns AP_APPLIED or AP_UNSUPPORTED.
function applyCssWide(st: Style*, name: char*, v: char*, ctx: CssCtx*) => int {
    let initial: bool = strEq(v, "initial");
    if (!strEq(v, "inherit") && !initial) initial = !cssIsInheritedProp(name);      // unset / revert / revert-layer
    let dflt: Style; dflt.reset();
    let src: Style* = &dflt;
    if (!initial && ctx.parent != nullptr) src = ctx.parent;
    let ok: bool = copyCssProp(st, src, name);
    if (ok && initial && strEq(name, "font-size")) st.fontPx = ctx.rootPx;
    if (!ok) return AP_UNSUPPORTED;
    return AP_APPLIED;
}
