// standard/1/Ui/Css.os
// CSS text parser, selector matching, cascade, property application.
//   Properties: see applyProp (display/position/box model/flex/text/colour/overflow/opacity/z-index/...).
//   Selectors:  tag, .class, #id, *, [attr] [attr=v] [attr~=v] [attr^=v] [attr$=v] [attr*=v], descendant, >, +, ~,
//               :hover :focus :active :checked :disabled :enabled :first-child :last-child :root, comma lists.
//   NEXT: @media, var(), calc(), :not(), :nth-child(), !important (parsed and ignored), pre-parsed decl cache.
// Cascade order: inherited props -> tag defaults -> sheet rules by (specificity, source order) -> inline style -> API fields.
// font-size is applied in a first pass so `em` in the same rule uses the element's own size.

struct CRes { ok: bool = false; c: u32 = 0; }

function hexVal(c: int) => int {
    let r: int = 0 - 1;
    if (c >= 48 && c <= 57) r = c - 48;
    else if (c >= 97 && c <= 102) r = c - 97 + 10;
    else if (c >= 65 && c <= 70) r = c - 65 + 10;
    return r;
}

function rgbaPack(r: int, g: int, b: int, a: int) => u32 {
    let rr: u32 = clampF(r as float, 0.0, 255.0) as u32;
    let gg: u32 = clampF(g as float, 0.0, 255.0) as u32;
    let bb: u32 = clampF(b as float, 0.0, 255.0) as u32;
    let aa: u32 = clampF(a as float, 0.0, 255.0) as u32;
    return (rr << 24) | (gg << 16) | (bb << 8) | aa;
}

// "name rrggbb name rrggbb ..." scan; sets res on a hit
function scanNamed(name: char*, list: char*, res: CRes*) => void {
    let i: int = 0; let more: bool = true;
    while (more) {
        while (list[i] as int == 32) i = i + 1;
        if (list[i] as int == 0) { more = false; }
        else {
            let j: int = 0; let same: bool = true;
            while (list[i + j] as int != 32 && list[i + j] as int != 0) {
                if (same) { if (name[j] as int != list[i + j] as int) same = false; }
                j = j + 1;
            }
            let nameEnds: bool = same && name[j] as int == 0;
            i = i + j;
            while (list[i] as int == 32) i = i + 1;
            let h: int = 0; let k: int = 0;
            while (k < 6) { h = h * 16 + hexVal(list[i + k] as int); k = k + 1; }
            if (nameEnds) { res.ok = true; res.c = ((h as u32) << 8) | 0xFF; more = false; }
            i = i + 6;
        }
    }
}

function namedColor(name: char*, res: CRes*) => void {
    res.ok = false; res.c = 0;
    scanNamed(name, "aliceblue f0f8ff antiquewhite faebd7 aqua 00ffff aquamarine 7fffd4 azure f0ffff beige f5f5dc bisque ffe4c4 black 000000 blanchedalmond ffebcd blue 0000ff blueviolet 8a2be2 brown a52a2a burlywood deb887 cadetblue 5f9ea0 chartreuse 7fff00 chocolate d2691e coral ff7f50 cornflowerblue 6495ed cornsilk fff8dc crimson dc143c cyan 00ffff", res);
    if (!res.ok) scanNamed(name, "darkblue 00008b darkcyan 008b8b darkgoldenrod b8860b darkgray a9a9a9 darkgreen 006400 darkgrey a9a9a9 darkkhaki bdb76b darkmagenta 8b008b darkolivegreen 556b2f darkorange ff8c00 darkorchid 9932cc darkred 8b0000 darksalmon e9967a darkseagreen 8fbc8f darkslateblue 483d8b darkslategray 2f4f4f darkslategrey 2f4f4f darkturquoise 00ced1 darkviolet 9400d3 deeppink ff1493 deepskyblue 00bfff dimgray 696969 dimgrey 696969 dodgerblue 1e90ff", res);
    if (!res.ok) scanNamed(name, "firebrick b22222 floralwhite fffaf0 forestgreen 228b22 fuchsia ff00ff gainsboro dcdcdc ghostwhite f8f8ff gold ffd700 goldenrod daa520 gray 808080 green 008000 greenyellow adff2f grey 808080 honeydew f0fff0 hotpink ff69b4 indianred cd5c5c indigo 4b0082 ivory fffff0 khaki f0e68c lavender e6e6fa lavenderblush fff0f5 lawngreen 7cfc00 lemonchiffon fffacd", res);
    if (!res.ok) scanNamed(name, "lightblue add8e6 lightcoral f08080 lightcyan e0ffff lightgoldenrodyellow fafad2 lightgray d3d3d3 lightgreen 90ee90 lightgrey d3d3d3 lightpink ffb6c1 lightsalmon ffa07a lightseagreen 20b2aa lightskyblue 87cefa lightslategray 778899 lightslategrey 778899 lightsteelblue b0c4de lightyellow ffffe0 lime 00ff00 limegreen 32cd32 linen faf0e6 magenta ff00ff maroon 800000", res);
    if (!res.ok) scanNamed(name, "mediumaquamarine 66cdaa mediumblue 0000cd mediumorchid ba55d3 mediumpurple 9370db mediumseagreen 3cb371 mediumslateblue 7b68ee mediumspringgreen 00fa9a mediumturquoise 48d1cc mediumvioletred c71585 midnightblue 191970 mintcream f5fffa mistyrose ffe4e1 moccasin ffe4b5 navajowhite ffdead navy 000080 oldlace fdf5e6 olive 808000 olivedrab 6b8e23 orange ffa500 orangered ff4500 orchid da70d6", res);
    if (!res.ok) scanNamed(name, "palegoldenrod eee8aa palegreen 98fb98 paleturquoise afeeee palevioletred db7093 papayawhip ffefd5 peachpuff ffdab9 peru cd853f pink ffc0cb plum dda0dd powderblue b0e0e6 purple 800080 rebeccapurple 663399 red ff0000 rosybrown bc8f8f royalblue 4169e1 saddlebrown 8b4513 salmon fa8072 sandybrown f4a460 seagreen 2e8b57 seashell fff5ee sienna a0522d silver c0c0c0", res);
    if (!res.ok) scanNamed(name, "skyblue 87ceeb slateblue 6a5acd slategray 708090 slategrey 708090 snow fffafa springgreen 00ff7f steelblue 4682b4 tan d2b48c teal 008080 thistle d8bfd8 tomato ff6347 turquoise 40e0d0 violet ee82ee wheat f5deb3 white ffffff whitesmoke f5f5f5 yellow ffff00 yellowgreen 9acd32", res);
}

function hue2rgb(p: float, q: float, t0: float) => float {
    let t: float = t0;
    if (t < 0.0) t = t + 1.0;
    if (t > 1.0) t = t - 1.0;
    let r: float = p;
    if (t < 1.0 / 6.0) r = p + (q - p) * 6.0 * t;
    else if (t < 0.5) r = q;
    else if (t < 2.0 / 3.0) r = p + (q - p) * (2.0 / 3.0 - t) * 6.0;
    return r;
}

// up to four numbers read from "rgb(...)" style argument lists
struct NumList {
    v0: float = 0.0; v1: float = 0.0; v2: float = 0.0; v3: float = 1.0;
    p0: bool = false; p1: bool = false; p2: bool = false; p3: bool = false;
    n: int = 0;
    put(v: float, pct: bool) => void {
        if (this.n == 0) { this.v0 = v; this.p0 = pct; }
        else if (this.n == 1) { this.v1 = v; this.p1 = pct; }
        else if (this.n == 2) { this.v2 = v; this.p2 = pct; }
        else if (this.n == 3) { this.v3 = v; this.p3 = pct; }
        this.n = this.n + 1;
    }
}

// #rgb #rgba #rrggbb #rrggbbaa, rgb()/rgba()/hsl()/hsla(), transparent, named colours
function parseColor(v: char*, res: CRes*) => void {
    res.ok = false; res.c = 0;
    let n: int = strLen(v);
    if (n > 0 && v[0] as int == 35) {
        let h: int = 0; let k: int = 1; let good: bool = true;
        while (k < n) { let d: int = hexVal(v[k] as int); if (d < 0) good = false; else h = h * 16 + d; k = k + 1; }
        let digits: int = n - 1;
        if (good && digits == 6) { res.ok = true; res.c = ((h as u32) << 8) | 0xFF; }
        else if (good && digits == 8) { res.ok = true; res.c = h as u32; }
        else if (good && (digits == 3 || digits == 4)) {
            let r: int = hexVal(v[1] as int); let g: int = hexVal(v[2] as int); let b: int = hexVal(v[3] as int);
            let a: int = 15;
            if (digits == 4) a = hexVal(v[4] as int);
            res.ok = true; res.c = rgbaPack(r * 17, g * 17, b * 17, a * 17);
        }
    }
    else if (eqAtI(v, 0, "rgb") || eqAtI(v, 0, "hsl")) {
        let isHsl: bool = lowerC(v[0] as int) == 104;
        let lx: Lex; lx.s = v; lx.i = 0; lx.ok = false; lx.val = 0.0;
        while (lx.peek() != 0 && lx.peek() != 40) lx.i = lx.i + 1;
        if (lx.peek() == 40) {
            lx.i = lx.i + 1;
            let nl: NumList;
            nl.n = 0; nl.v3 = 1.0;
            let more: bool = true;
            while (more) {
                while (isSpaceC(lx.peek()) || lx.peek() == 44 || lx.peek() == 47) lx.i = lx.i + 1;
                if (lx.peek() == 0 || lx.peek() == 41) { more = false; }
                else {
                    let x: float = lx.number();
                    if (!lx.ok) { more = false; }
                    else {
                        let pct: bool = false;
                        if (lx.peek() == 37) { pct = true; lx.i = lx.i + 1; }
                        else if (isNameC(lx.peek())) { while (isNameC(lx.peek())) lx.i = lx.i + 1; }   // deg / turn: treated as plain numbers
                        nl.put(x, pct);
                    }
                }
            }
            if (nl.n >= 3) {
                let alpha: float = 1.0;
                if (nl.n >= 4) { alpha = nl.v3; if (nl.p3) alpha = nl.v3 / 100.0; }
                let a255: int = (alpha * 255.0 + 0.5) as int;
                if (!isHsl) {
                    let r: float = nl.v0; let g: float = nl.v1; let b: float = nl.v2;
                    if (nl.p0) r = r * 2.55;
                    if (nl.p1) g = g * 2.55;
                    if (nl.p2) b = b * 2.55;
                    res.ok = true; res.c = rgbaPack((r + 0.5) as int, (g + 0.5) as int, (b + 0.5) as int, a255);
                } else {
                    let hh: float = nl.v0 / 360.0;
                    while (hh >= 1.0) hh = hh - 1.0;
                    while (hh < 0.0) hh = hh + 1.0;
                    let s: float = clampF(nl.v1 / 100.0, 0.0, 1.0);
                    let l: float = clampF(nl.v2 / 100.0, 0.0, 1.0);
                    let q: float = l + s - l * s;
                    if (l < 0.5) q = l * (1.0 + s);
                    let p: float = 2.0 * l - q;
                    let r2: float = hue2rgb(p, q, hh + 1.0 / 3.0);
                    let g2: float = hue2rgb(p, q, hh);
                    let b2: float = hue2rgb(p, q, hh - 1.0 / 3.0);
                    res.ok = true;
                    res.c = rgbaPack((r2 * 255.0 + 0.5) as int, (g2 * 255.0 + 0.5) as int, (b2 * 255.0 + 0.5) as int, a255);
                }
            }
        }
    }
    else if (strEqI(v, "transparent")) { res.ok = true; res.c = 0; }
    else {
        let low: char* = strLowerDup(v);
        namedColor(low, res);
        strFree(low);
    }
}

// ---- lengths ----
// px/em/rem/%/vw/vh/pt/in/cm/mm/auto/none/bare 0 -> Len in layout units (percent stays percent)
function parseLen(tok: char*, ctx: CssCtx*, out: Len*) => bool {
    let ok: bool = false;
    if (strEqI(tok, "auto")) { lenSet(out, LU_AUTO, 0.0); ok = true; }
    else if (strEqI(tok, "none")) { lenSet(out, LU_NONE, 0.0); ok = true; }
    else {
        let lx: Lex; lx.s = tok; lx.i = 0; lx.ok = false; lx.val = 0.0;
        let v: float = lx.number();
        if (lx.ok) {
            let at: int = lx.i;
            if (tok[at] as int == 0 || eqAtI(tok, at, "px")) { lenSet(out, LU_PX, v * ctx.unit); ok = true; }
            else if (eqAtI(tok, at, "%")) { lenSet(out, LU_PCT, v); ok = true; }
            else if (eqAtI(tok, at, "rem")) { lenSet(out, LU_PX, v * ctx.rootPx); ok = true; }
            else if (eqAtI(tok, at, "em")) { lenSet(out, LU_PX, v * ctx.curPx); ok = true; }
            else if (eqAtI(tok, at, "vw")) { lenSet(out, LU_PX, v * ctx.vw / 100.0); ok = true; }
            else if (eqAtI(tok, at, "vh")) { lenSet(out, LU_PX, v * ctx.vh / 100.0); ok = true; }
            else if (eqAtI(tok, at, "pt")) { lenSet(out, LU_PX, v * 1.3333 * ctx.unit); ok = true; }
            else if (eqAtI(tok, at, "in")) { lenSet(out, LU_PX, v * 96.0 * ctx.unit); ok = true; }
            else if (eqAtI(tok, at, "cm")) { lenSet(out, LU_PX, v * 37.795 * ctx.unit); ok = true; }
            else if (eqAtI(tok, at, "mm")) { lenSet(out, LU_PX, v * 3.7795 * ctx.unit); ok = true; }
        }
        else if (strEqI(tok, "thin")) { lenSet(out, LU_PX, 1.0 * ctx.unit); ok = true; }
        else if (strEqI(tok, "medium")) { lenSet(out, LU_PX, 3.0 * ctx.unit); ok = true; }
        else if (strEqI(tok, "thick")) { lenSet(out, LU_PX, 5.0 * ctx.unit); ok = true; }
    }
    return ok;
}

// Border width in layout units (px only). Keeps `def` when the token is not a length.
function parseWidthPx(tok: char*, ctx: CssCtx*, def: float) => float {
    let l: Len;
    lenSet(&l, LU_PX, def);
    if (parseLen(tok, ctx, &l)) { if (l.u != LU_PX) lenSet(&l, LU_PX, def); }
    return l.v;
}

// 1-4 values -> top right bottom left (CSS shorthand rules)
function side4(v: char*, ctx: CssCtx*, t: Len*, r: Len*, b: Len*, l: Len*) => void {
    let lx: Lex; lx.s = v; lx.i = 0; lx.ok = false; lx.val = 0.0;
    let buf: char* = CMem.malloc(64) as char*;
    let a: Len; let bb: Len; let c: Len; let d: Len;
    lenSet(&a, LU_PX, 0.0); lenSet(&bb, LU_PX, 0.0); lenSet(&c, LU_PX, 0.0); lenSet(&d, LU_PX, 0.0);
    let n: int = 0; let good: bool = true;
    while (n < 4 && lx.token(buf, 64)) {
        let ok: bool = false;
        if (n == 0) ok = parseLen(buf, ctx, &a);
        else if (n == 1) ok = parseLen(buf, ctx, &bb);
        else if (n == 2) ok = parseLen(buf, ctx, &c);
        else ok = parseLen(buf, ctx, &d);
        if (!ok) good = false;
        n = n + 1;
    }
    CMem.free(buf as void*);
    if (good && n > 0) {
        if (n == 1) { bb = a; c = a; d = a; }
        else if (n == 2) { c = a; d = bb; }
        else if (n == 3) { d = bb; }
        t.u = a.u; t.v = a.v;
        r.u = bb.u; r.v = bb.v;
        b.u = c.u; b.v = c.v;
        l.u = d.u; l.v = d.v;
    }
}

// border shorthand: width / style / colour tokens in any order. side: 0 all, 1 top, 2 right, 3 bottom, 4 left
function applyBorder(st: Style*, side: int, v: char*, ctx: CssCtx*) => void {
    let lx: Lex; lx.s = v; lx.i = 0; lx.ok = false; lx.val = 0.0;
    let buf: char* = CMem.malloc(96) as char*;
    let w: float = 1.0 * ctx.unit; let none: bool = false; let cr: CRes;
    cr.ok = false; cr.c = 0;
    let gotColor: bool = false; let color: u32 = st.borderColor;
    while (lx.token(buf, 96)) {
        if (strEqI(buf, "none") || strEqI(buf, "hidden")) { none = true; }
        else if (strEqI(buf, "solid") || strEqI(buf, "dashed") || strEqI(buf, "dotted") || strEqI(buf, "double")
              || strEqI(buf, "groove") || strEqI(buf, "ridge") || strEqI(buf, "inset") || strEqI(buf, "outset")) { }   // NEXT: dashed/dotted
        else {
            let l: Len;
            lenSet(&l, LU_PX, 0.0);
            if (parseLen(buf, ctx, &l)) { if (l.u == LU_PX) w = l.v; }
            else { parseColor(buf, &cr); if (cr.ok) { color = cr.c; gotColor = true; } }
        }
    }
    CMem.free(buf as void*);
    if (none) w = 0.0;
    if (gotColor) st.borderColor = color;
    if (side == 0 || side == 1) st.bt = w;
    if (side == 0 || side == 2) st.br = w;
    if (side == 0 || side == 3) st.bb = w;
    if (side == 0 || side == 4) st.bl = w;
}

function setDisplay(st: Style*, v: char*) => void {
    if (strEqI(v, "none")) st.display = DP_NONE;
    else if (strEqI(v, "block") || strEqI(v, "list-item") || strEqI(v, "contents")) st.display = DP_BLOCK;   // NEXT: contents
    else if (strEqI(v, "inline")) st.display = DP_INLINE;
    else if (strEqI(v, "inline-block")) st.display = DP_INLINE_BLOCK;
    else if (strEqI(v, "flex") || strEqI(v, "inline-flex")) st.display = DP_FLEX;
    else if (strEqI(v, "table") || strEqI(v, "inline-table")) st.display = DP_TABLE;
    else if (strEqI(v, "table-row")) st.display = DP_TABLE_ROW;
    else if (strEqI(v, "table-cell")) st.display = DP_TABLE_CELL;
    else if (strEqI(v, "table-row-group") || strEqI(v, "table-header-group") || strEqI(v, "table-footer-group")) st.display = DP_TABLE_GROUP;
    else if (strEqI(v, "grid") || strEqI(v, "inline-grid")) st.display = DP_GRID;
}

function alignKeyword(v: char*, def: int) => int {
    let r: int = def;
    if (strEqI(v, "stretch") || strEqI(v, "normal")) r = AL_STRETCH;
    else if (strEqI(v, "flex-start") || strEqI(v, "start") || strEqI(v, "baseline") || strEqI(v, "self-start")) r = AL_START;
    else if (strEqI(v, "flex-end") || strEqI(v, "end") || strEqI(v, "self-end")) r = AL_END;
    else if (strEqI(v, "center")) r = AL_CENTER;
    else if (strEqI(v, "auto")) r = AL_AUTO;
    return r;
}

function overflowKeyword(v: char*, def: int) => int {
    let r: int = def;
    if (strEqI(v, "visible")) r = OV_VISIBLE;
    else if (strEqI(v, "hidden") || strEqI(v, "clip")) r = OV_HIDDEN;
    else if (strEqI(v, "scroll")) r = OV_SCROLL;
    else if (strEqI(v, "auto")) r = OV_AUTO;
    return r;
}

// Applies one declaration. pass 0 handles only font-size, pass 1 everything else.
function applyProp(st: Style*, name: char*, v: char*, ctx: CssCtx*, pass: int) => void {
    let isFs: bool = strEq(name, "font-size");
    if (pass == 0 && !isFs) return;
    if (pass == 1 && isFs) return;
    let l: Len; lenSet(&l, LU_AUTO, 0.0);
    let cr: CRes; cr.ok = false; cr.c = 0;

    if (isFs) {
        if (!ctx.tui) {                                  // terminal: every glyph is one cell
            let px: float = 0.0; let have: bool = false;
            if (strEqI(v, "xx-small")) { px = 9.0 * ctx.unit; have = true; }
            else if (strEqI(v, "x-small")) { px = 10.0 * ctx.unit; have = true; }
            else if (strEqI(v, "small")) { px = 13.0 * ctx.unit; have = true; }
            else if (strEqI(v, "medium")) { px = 16.0 * ctx.unit; have = true; }
            else if (strEqI(v, "large")) { px = 18.0 * ctx.unit; have = true; }
            else if (strEqI(v, "x-large")) { px = 24.0 * ctx.unit; have = true; }
            else if (strEqI(v, "xx-large")) { px = 32.0 * ctx.unit; have = true; }
            else if (strEqI(v, "smaller")) { px = ctx.curPx / 1.2; have = true; }
            else if (strEqI(v, "larger")) { px = ctx.curPx * 1.2; have = true; }
            else if (parseLen(v, ctx, &l)) {
                if (l.u == LU_PX) { px = l.v; have = true; }
                else if (l.u == LU_PCT) { px = ctx.curPx * l.v / 100.0; have = true; }
            }
            if (have && px > 0.0) st.fontPx = px;
        }
    }
    else if (strEq(name, "display")) { setDisplay(st, v); }
    else if (strEq(name, "position")) {
        if (strEqI(v, "static")) st.position = PS_STATIC;
        else if (strEqI(v, "relative") || strEqI(v, "sticky")) st.position = PS_RELATIVE;   // NEXT: sticky
        else if (strEqI(v, "absolute")) st.position = PS_ABSOLUTE;
        else if (strEqI(v, "fixed")) st.position = PS_FIXED;
    }
    else if (strEq(name, "color")) { parseColor(v, &cr); if (cr.ok) st.color = cr.c; }
    else if (strEq(name, "background-color")) { parseColor(v, &cr); if (cr.ok) st.bg = cr.c; }
    else if (strEq(name, "background")) {
        let lx: Lex; lx.s = v; lx.i = 0; lx.ok = false; lx.val = 0.0;
        let buf: char* = CMem.malloc(96) as char*;
        while (lx.token(buf, 96)) { parseColor(buf, &cr); if (cr.ok) st.bg = cr.c; }      // NEXT: url(), gradients
        CMem.free(buf as void*);
    }
    else if (strEq(name, "font-weight")) {
        let lx: Lex; lx.s = v; lx.i = 0; lx.ok = false; lx.val = 0.0;
        if (strEqI(v, "bold") || strEqI(v, "bolder")) st.bold = true;
        else if (strEqI(v, "normal") || strEqI(v, "lighter")) st.bold = false;
        else { let w: float = lx.number(); if (lx.ok) st.bold = w >= 600.0; }
    }
    else if (strEq(name, "font-style")) { st.italic = strEqI(v, "italic") || strEqI(v, "oblique"); }
    else if (strEq(name, "text-align")) {
        if (strEqI(v, "left") || strEqI(v, "start")) st.textAlign = TA_LEFT;
        else if (strEqI(v, "center")) st.textAlign = TA_CENTER;
        else if (strEqI(v, "right") || strEqI(v, "end")) st.textAlign = TA_RIGHT;
        else if (strEqI(v, "justify")) st.textAlign = TA_JUSTIFY;                    // NEXT: justify (left for now)
    }
    else if (strEq(name, "text-decoration") || strEq(name, "text-decoration-line")) {
        st.underline = false; st.strike = false;
        let lx: Lex; lx.s = v; lx.i = 0; lx.ok = false; lx.val = 0.0;
        let buf: char* = CMem.malloc(48) as char*;
        while (lx.token(buf, 48)) {
            if (strEqI(buf, "underline")) st.underline = true;
            else if (strEqI(buf, "line-through")) st.strike = true;
        }
        CMem.free(buf as void*);
    }
    else if (strEq(name, "line-height")) {
        if (strEqI(v, "normal")) { st.lineH = 0.0; }
        else {
            let lx: Lex; lx.s = v; lx.i = 0; lx.ok = false; lx.val = 0.0;
            let m: float = lx.number();
            if (lx.ok && lx.atEnd()) { st.lineH = m * st.fontPx; }                   // unitless: multiple of the font size
            else if (parseLen(v, ctx, &l)) {
                if (l.u == LU_PX) st.lineH = l.v;
                else if (l.u == LU_PCT) st.lineH = st.fontPx * l.v / 100.0;
            }
        }
    }
    else if (strEq(name, "white-space")) {
        if (strEqI(v, "normal")) st.whiteSpace = WS_NORMAL;
        else if (strEqI(v, "nowrap")) st.whiteSpace = WS_NOWRAP;
        else if (strEqI(v, "pre")) st.whiteSpace = WS_PRE;
        else if (strEqI(v, "pre-wrap")) st.whiteSpace = WS_PREWRAP;
        else if (strEqI(v, "pre-line")) st.whiteSpace = WS_PRELINE;
    }
    else if (strEq(name, "width")) { if (parseLen(v, ctx, &l)) { if (l.u != LU_NONE) { st.width.u = l.u; st.width.v = l.v; } } }
    else if (strEq(name, "height")) { if (parseLen(v, ctx, &l)) { if (l.u != LU_NONE) { st.height.u = l.u; st.height.v = l.v; } } }
    else if (strEq(name, "min-width")) { if (parseLen(v, ctx, &l)) { st.minW.u = l.u; st.minW.v = l.v; } }
    else if (strEq(name, "max-width")) { if (parseLen(v, ctx, &l)) { st.maxW.u = l.u; st.maxW.v = l.v; } }
    else if (strEq(name, "min-height")) { if (parseLen(v, ctx, &l)) { st.minH.u = l.u; st.minH.v = l.v; } }
    else if (strEq(name, "max-height")) { if (parseLen(v, ctx, &l)) { st.maxH.u = l.u; st.maxH.v = l.v; } }
    else if (strEq(name, "margin")) { side4(v, ctx, &st.mt, &st.mr, &st.mb, &st.ml); }
    else if (strEq(name, "margin-top")) { if (parseLen(v, ctx, &l)) { st.mt.u = l.u; st.mt.v = l.v; } }
    else if (strEq(name, "margin-right")) { if (parseLen(v, ctx, &l)) { st.mr.u = l.u; st.mr.v = l.v; } }
    else if (strEq(name, "margin-bottom")) { if (parseLen(v, ctx, &l)) { st.mb.u = l.u; st.mb.v = l.v; } }
    else if (strEq(name, "margin-left")) { if (parseLen(v, ctx, &l)) { st.ml.u = l.u; st.ml.v = l.v; } }
    else if (strEq(name, "padding")) { side4(v, ctx, &st.pt, &st.pr, &st.pb, &st.pl); }
    else if (strEq(name, "padding-top")) { if (parseLen(v, ctx, &l)) { st.pt.u = l.u; st.pt.v = l.v; } }
    else if (strEq(name, "padding-right")) { if (parseLen(v, ctx, &l)) { st.pr.u = l.u; st.pr.v = l.v; } }
    else if (strEq(name, "padding-bottom")) { if (parseLen(v, ctx, &l)) { st.pb.u = l.u; st.pb.v = l.v; } }
    else if (strEq(name, "padding-left")) { if (parseLen(v, ctx, &l)) { st.pl.u = l.u; st.pl.v = l.v; } }
    else if (strEq(name, "border")) { applyBorder(st, 0, v, ctx); }
    else if (strEq(name, "border-top")) { applyBorder(st, 1, v, ctx); }
    else if (strEq(name, "border-right")) { applyBorder(st, 2, v, ctx); }
    else if (strEq(name, "border-bottom")) { applyBorder(st, 3, v, ctx); }
    else if (strEq(name, "border-left")) { applyBorder(st, 4, v, ctx); }
    else if (strEq(name, "border-width")) {
        let w: float = parseWidthPx(v, ctx, st.bt);
        st.bt = w; st.br = w; st.bb = w; st.bl = w;                                  // NEXT: per-side values
    }
    else if (strEq(name, "border-color")) { parseColor(v, &cr); if (cr.ok) st.borderColor = cr.c; }
    else if (strEq(name, "border-style")) { if (strEqI(v, "none") || strEqI(v, "hidden")) { st.bt = 0.0; st.br = 0.0; st.bb = 0.0; st.bl = 0.0; } }
    else if (strEq(name, "border-radius")) { st.radius = parseWidthPx(v, ctx, st.radius); }     // NEXT: painted (Pen2D has no rounded rects)
    else if (strEq(name, "box-sizing")) { st.boxSizing = 0; if (strEqI(v, "border-box")) st.boxSizing = 1; }
    else if (strEq(name, "overflow")) { st.overflow = overflowKeyword(v, st.overflow); }
    else if (strEq(name, "overflow-x") || strEq(name, "overflow-y")) { st.overflow = overflowKeyword(v, st.overflow); }
    else if (strEq(name, "opacity")) {
        let lx: Lex; lx.s = v; lx.i = 0; lx.ok = false; lx.val = 0.0;
        let o: float = lx.number();
        if (lx.ok) { if (lx.peek() == 37) o = o / 100.0; st.opacity = clampF(o, 0.0, 1.0); }
    }
    else if (strEq(name, "visibility")) { st.hidden = strEqI(v, "hidden") || strEqI(v, "collapse"); }
    else if (strEq(name, "z-index")) { if (!strEqI(v, "auto")) st.zIndex = parseFloatStr(v, 0.0) as int; else st.zIndex = 0; }
    else if (strEq(name, "top")) { if (parseLen(v, ctx, &l)) { st.top.u = l.u; st.top.v = l.v; } }
    else if (strEq(name, "right")) { if (parseLen(v, ctx, &l)) { st.right.u = l.u; st.right.v = l.v; } }
    else if (strEq(name, "bottom")) { if (parseLen(v, ctx, &l)) { st.bottom.u = l.u; st.bottom.v = l.v; } }
    else if (strEq(name, "left")) { if (parseLen(v, ctx, &l)) { st.left.u = l.u; st.left.v = l.v; } }
    else if (strEq(name, "inset")) { side4(v, ctx, &st.top, &st.right, &st.bottom, &st.left); }
    else if (strEq(name, "flex-direction")) {
        if (strEqI(v, "row")) st.flexDir = FD_ROW;
        else if (strEqI(v, "column")) st.flexDir = FD_COLUMN;
        else if (strEqI(v, "row-reverse")) st.flexDir = FD_ROW_REV;
        else if (strEqI(v, "column-reverse")) st.flexDir = FD_COL_REV;
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
    else if (strEq(name, "flex-basis")) { if (parseLen(v, ctx, &l)) { st.basis.u = l.u; st.basis.v = l.v; } }
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
            }
            CMem.free(buf as void*);
            if (nums == 1) st.shrink = 1.0;
            if (nums > 0 && !basisSet) lenSet(&st.basis, LU_PX, 0.0);            // `flex: 1` means 1 1 0
        }
    }
    else if (strEq(name, "gap") || strEq(name, "row-gap") || strEq(name, "column-gap")) {
        st.gap = parseWidthPx(v, ctx, st.gap);                                    // one gap for both axes
    }
    else if (strEq(name, "align-items")) { st.alignItems = alignKeyword(v, st.alignItems); }
    else if (strEq(name, "align-self")) { st.alignSelf = alignKeyword(v, st.alignSelf); }
    else if (strEq(name, "justify-content")) {
        if (strEqI(v, "flex-start") || strEqI(v, "start") || strEqI(v, "left") || strEqI(v, "normal")) st.justify = JC_START;
        else if (strEqI(v, "flex-end") || strEqI(v, "end") || strEqI(v, "right")) st.justify = JC_END;
        else if (strEqI(v, "center")) st.justify = JC_CENTER;
        else if (strEqI(v, "space-between")) st.justify = JC_BETWEEN;
        else if (strEqI(v, "space-around")) st.justify = JC_AROUND;
        else if (strEqI(v, "space-evenly")) st.justify = JC_EVENLY;
    }
    else if (strEq(name, "cursor")) {
        if (strEqI(v, "pointer")) st.cursor = CU_POINTER;
        else if (strEqI(v, "text")) st.cursor = CU_TEXT;
        else st.cursor = CU_DEFAULT;
    }
    else if (strEq(name, "list-style") || strEq(name, "list-style-type")) {
        if (strEqI(v, "none")) st.listStyle = LS_NONE;
        else if (strEqI(v, "disc")) st.listStyle = LS_DISC;
        else if (strEqI(v, "circle")) st.listStyle = LS_CIRCLE;
        else if (strEqI(v, "square")) st.listStyle = LS_SQUARE;
        else if (strEqI(v, "decimal")) st.listStyle = LS_DECIMAL;
    }
    // ignored for now: font-family, background-image, transition, animation, grid-*, var(), calc()
}

// ---- declaration lists ----
function freeAttrs(a0: Attr*) => void {
    let a: Attr* = a0;
    while (a != nullptr) { let nx: Attr* = a.next; strFree(a.name); strFree(a.value); CMem.free(a as void*); a = nx; }
}

function trimDup(s: char*, from: int, to: int) => char* {
    let a: int = from; let b: int = to;
    while (a < b && isSpaceC(s[a] as int)) a = a + 1;
    while (b > a && isSpaceC(s[b - 1] as int)) b = b - 1;
    return strSub(s, a, b - a);
}

// "a: b; c: d" -> list of (lowercase name, value). `!important` is stripped and ignored.
function parseDecls(text: char*) => Attr* {
    let head: Attr* = nullptr; let tail: Attr* = nullptr;
    let n: int = strLen(text);
    let i: int = 0;
    while (i < n) {
        let start: int = i; let depth: int = 0; let colon: int = 0 - 1; let quote: int = 0;
        let more: bool = true;
        while (i < n && more) {
            let c: int = text[i] as int;
            if (quote != 0) { if (c == quote) quote = 0; }
            else if (c == 34 || c == 39) { quote = c; }
            else if (c == 40) { depth = depth + 1; }
            else if (c == 41) { if (depth > 0) depth = depth - 1; }
            else if (c == 58 && depth == 0 && colon < 0) { colon = i; }
            else if (c == 59 && depth == 0) { more = false; }
            if (more) i = i + 1;
        }
        if (colon > start) {
            let name: char* = trimDup(text, start, colon);
            let value: char* = trimDup(text, colon + 1, i);
            let vl: int = strLen(value);
            let bang: int = 0 - 1; let k: int = 0;
            while (k < vl) { if (value[k] as int == 33 && bang < 0) bang = k; k = k + 1; }
            if (bang >= 0) { let v2: char* = trimDup(value, 0, bang); strFree(value); value = v2; }
            let ln: char* = strLowerDup(name);
            strFree(name);
            let a: Attr* = CMem.malloc(sizeof(Attr)) as Attr*;
            a.name = ln; a.value = value; a.next = nullptr;
            if (tail != nullptr) tail.next = a; else head = a;
            tail = a;
        }
        i = i + 1;
    }
    return head;
}

function applyDecls(st: Style*, decls: Attr*, ctx: CssCtx*, pass: int) => void {
    let a: Attr* = decls;
    while (a != nullptr) { applyProp(st, a.name, a.value, ctx, pass); a = a.next; }
}

// parse + apply a style="..." string (font-size first, then the rest)
function applyDeclText(st: Style*, text: char*, ctx: CssCtx*, parentPx: float) => void {
    let d: Attr* = parseDecls(text);
    ctx.curPx = parentPx; applyDecls(st, d, ctx, 0);
    ctx.curPx = st.fontPx; applyDecls(st, d, ctx, 1);
    freeAttrs(d);
}

// ---- selectors ----
function readIdent(lx: Lex*) => char* {
    let start: int = lx.i;
    while (isNameC(lx.peek())) lx.i = lx.i + 1;
    return strSub(lx.s, start, lx.i - start);
}

function skipBalanced(lx: Lex*) => void {      // at '(' : skip to the matching ')'
    let depth: int = 0; let more: bool = true;
    while (more) {
        let c: int = lx.peek();
        if (c == 0) { more = false; }
        else {
            if (c == 40) depth = depth + 1;
            if (c == 41) { depth = depth - 1; if (depth == 0) more = false; }
            lx.i = lx.i + 1;
        }
    }
}

// one compound selector; nullptr when nothing could be read
function parseCompound(lx: Lex*, sheet: Sheet*) => Compound* {
    let c: Compound* = CMem.malloc(sizeof(Compound)) as Compound*;
    CMem.memset(c as void*, 0, sizeof(Compound));
    c.comb = COMB_NONE;
    let read: int = 0; let more: bool = true;
    while (more) {
        let ch: int = lx.peek();
        if (ch == 42) { lx.i = lx.i + 1; read = read + 1; }
        else if (ch == 35) { lx.i = lx.i + 1; c.id = readIdent(lx); read = read + 1; }
        else if (ch == 46) {
            lx.i = lx.i + 1;
            let nm: char* = readIdent(lx);
            if (c.cls == nullptr) { c.cls = nm; }
            else { let joined: char* = strCat(c.cls, " "); let j2: char* = strCat(joined, nm); strFree(joined); strFree(nm); strFree(c.cls); c.cls = j2; }
            read = read + 1;
        }
        else if (ch == 91) {
            lx.i = lx.i + 1; lx.skipWs();
            c.attr = strLowerDup("");
            strFree(c.attr);
            let an: char* = readIdent(lx);
            c.attr = strLowerDup(an); strFree(an);
            lx.skipWs();
            c.attrOp = AOP_EXISTS;
            let o: int = lx.peek();
            if (o == 61) { c.attrOp = AOP_EQ; lx.i = lx.i + 1; }
            else if (o == 126) { c.attrOp = AOP_WORD; lx.i = lx.i + 2; }
            else if (o == 94) { c.attrOp = AOP_PREFIX; lx.i = lx.i + 2; }
            else if (o == 36) { c.attrOp = AOP_SUFFIX; lx.i = lx.i + 2; }
            else if (o == 42) { c.attrOp = AOP_SUB; lx.i = lx.i + 2; }
            if (c.attrOp != AOP_EXISTS) {
                lx.skipWs();
                let q: int = lx.peek();
                if (q == 34 || q == 39) {
                    lx.i = lx.i + 1;
                    let vs: int = lx.i;
                    while (lx.peek() != q && lx.peek() != 0) lx.i = lx.i + 1;
                    c.attrVal = strSub(lx.s, vs, lx.i - vs);
                    if (lx.peek() == q) lx.i = lx.i + 1;
                } else {
                    let vs2: int = lx.i;
                    while (lx.peek() != 93 && lx.peek() != 0 && !isSpaceC(lx.peek())) lx.i = lx.i + 1;
                    c.attrVal = strSub(lx.s, vs2, lx.i - vs2);
                }
            }
            lx.skipWs();
            if (lx.peek() == 93) lx.i = lx.i + 1;
            read = read + 1;
        }
        else if (ch == 58) {
            lx.i = lx.i + 1;
            if (lx.peek() == 58) { lx.i = lx.i + 1; c.never = true; }                 // ::before etc.  NEXT
            let pn: char* = readIdent(lx);
            if (lx.peek() == 40) { skipBalanced(lx); c.never = true; }                 // :not() :nth-child()  NEXT
            else if (strEqI(pn, "hover")) { c.pseudo = c.pseudo | PSE_HOVER; sheet.usesPseudo = true; }
            else if (strEqI(pn, "focus")) { c.pseudo = c.pseudo | PSE_FOCUS; sheet.usesPseudo = true; }
            else if (strEqI(pn, "active")) { c.pseudo = c.pseudo | PSE_ACTIVE; sheet.usesPseudo = true; }
            else if (strEqI(pn, "checked")) { c.pseudo = c.pseudo | PSE_CHECKED; sheet.usesPseudo = true; }
            else if (strEqI(pn, "disabled")) { c.pseudo = c.pseudo | PSE_DISABLED; }
            else if (strEqI(pn, "enabled")) { c.pseudo = c.pseudo | PSE_ENABLED; }
            else if (strEqI(pn, "first-child")) { c.pseudo = c.pseudo | PSE_FIRST; }
            else if (strEqI(pn, "last-child")) { c.pseudo = c.pseudo | PSE_LAST; }
            else if (strEqI(pn, "root")) { c.pseudo = c.pseudo | PSE_ROOT; }
            else { c.never = true; }
            strFree(pn);
            read = read + 1;
        }
        else if (isNameC(ch)) {
            let tn: char* = readIdent(lx);
            c.tag = strLowerDup(tn); strFree(tn);
            read = read + 1;
        }
        else { more = false; }
    }
    if (read == 0) { CMem.free(c as void*); return nullptr; }
    return c;
}

// right-most compound of the parsed selector (nullptr if it does not parse)
function parseSelector(text: char*, sheet: Sheet*) => Compound* {
    let lx: Lex; lx.s = text; lx.i = 0; lx.ok = false; lx.val = 0.0;
    let last: Compound* = nullptr; let bad: bool = false; let more: bool = true;
    while (more) {
        let sawSpace: bool = false;
        while (isSpaceC(lx.peek())) { lx.i = lx.i + 1; sawSpace = true; }
        let ch: int = lx.peek();
        if (ch == 0) { more = false; }
        else {
            let comb: int = COMB_DESC;
            if (ch == 62) { comb = COMB_CHILD; lx.i = lx.i + 1; lx.skipWs(); }
            else if (ch == 43) { comb = COMB_ADJ; lx.i = lx.i + 1; lx.skipWs(); }
            else if (ch == 126) { comb = COMB_SIB; lx.i = lx.i + 1; lx.skipWs(); }
            let c: Compound* = parseCompound(&lx, sheet);
            if (c == nullptr) { bad = true; more = false; }
            else {
                c.left = last;
                if (last == nullptr) c.comb = COMB_NONE; else c.comb = comb;
                last = c;
            }
        }
    }
    if (bad) return nullptr;
    return last;
}

function specOf(c0: Compound*) => int {
    let s: int = 0; let c: Compound* = c0;
    while (c != nullptr) {
        if (c.id != nullptr) s = s + 10000;
        if (c.cls != nullptr) s = s + 100 * (1 + (strLen(c.cls) - strLen(c.cls)));
        if (c.attr != nullptr) s = s + 100;
        if (c.pseudo != 0) s = s + 100;
        if (c.tag != nullptr) s = s + 1;
        c = c.left;
    }
    return s;
}

// ---- matching ----
function containsStr(h: char*, needle: char*) => bool {
    let found: bool = false;
    let hl: int = strLen(h); let nl: int = strLen(needle);
    let i: int = 0;
    while (i + nl <= hl && !found) {
        if (eqAtI(h, i, needle)) found = true;
        i = i + 1;
    }
    return found;
}

function matchCompound(n: Node*, c: Compound*) => bool {
    if (c.never) return false;
    if (n.kind == NK_TEXT) return false;
    let ok: bool = true;
    if (c.tag != nullptr) { if (!strEq(c.tag, n.tag)) ok = false; }
    if (ok && c.id != nullptr) { if (n.id == nullptr || !strEq(n.id, c.id)) ok = false; }
    if (ok && c.cls != nullptr) { if (n.cls == nullptr || !hasAllTokens(n.cls, c.cls)) ok = false; }
    if (ok && c.attr != nullptr) {
        let av: char* = n.getAttr(c.attr);
        if (av == nullptr) { ok = false; }
        else if (c.attrOp == AOP_EQ) { if (!strEq(av, c.attrVal)) ok = false; }
        else if (c.attrOp == AOP_WORD) { if (!hasToken(av, c.attrVal, 0, strLen(c.attrVal))) ok = false; }
        else if (c.attrOp == AOP_PREFIX) { if (!startsWith(av, c.attrVal)) ok = false; }
        else if (c.attrOp == AOP_SUFFIX) {
            let al: int = strLen(av); let vl: int = strLen(c.attrVal);
            if (vl > al || !eqAtI(av, al - vl, c.attrVal)) ok = false;
        }
        else if (c.attrOp == AOP_SUB) { if (!containsStr(av, c.attrVal)) ok = false; }
    }
    if (ok && c.pseudo != 0) {
        if ((c.pseudo & PSE_HOVER) != 0 && !n.hover) ok = false;
        if ((c.pseudo & PSE_FOCUS) != 0 && !n.focus) ok = false;
        if ((c.pseudo & PSE_ACTIVE) != 0 && !n.active) ok = false;
        if ((c.pseudo & PSE_CHECKED) != 0 && !n.checked) ok = false;
        if ((c.pseudo & PSE_DISABLED) != 0 && !n.disabled) ok = false;
        if ((c.pseudo & PSE_ENABLED) != 0 && n.disabled) ok = false;
        if ((c.pseudo & PSE_FIRST) != 0 && n.prevEl() != nullptr) ok = false;
        if ((c.pseudo & PSE_LAST) != 0 && n.nextEl() != nullptr) ok = false;
        if ((c.pseudo & PSE_ROOT) != 0 && n.parent != nullptr) ok = false;
    }
    return ok;
}

function matchChain(n: Node*, c: Compound*) => bool {
    if (!matchCompound(n, c)) return false;
    if (c.left == nullptr) return true;
    let ok: bool = false;
    if (c.comb == COMB_CHILD) { if (n.parent != nullptr) ok = matchChain(n.parent, c.left); }
    else if (c.comb == COMB_DESC) {
        let p: Node* = n.parent;
        while (p != nullptr && !ok) { ok = matchChain(p, c.left); p = p.parent; }
    }
    else if (c.comb == COMB_ADJ) { let s: Node* = n.prevEl(); if (s != nullptr) ok = matchChain(s, c.left); }
    else if (c.comb == COMB_SIB) {
        let s2: Node* = n.prevEl();
        while (s2 != nullptr && !ok) { ok = matchChain(s2, c.left); s2 = s2.prevEl(); }
    }
    return ok;
}

// ---- sheet ----
function freeCompound(c0: Compound*) => void {
    let c: Compound* = c0;
    while (c != nullptr) {
        let nx: Compound* = c.left;
        strFree(c.tag); strFree(c.id); strFree(c.cls); strFree(c.attr); strFree(c.attrVal);
        CMem.free(c as void*);
        c = nx;
    }
}

function sheetAdd(sheet: Sheet*, sel: Compound*, decls: Attr*) => void {
    let r: Rule* = CMem.malloc(sizeof(Rule)) as Rule*;
    r.sel = sel; r.decls = decls; r.spec = specOf(sel); r.seq = sheet.seq; r.next = nullptr;
    sheet.seq = sheet.seq + 1;
    if (sheet.head == nullptr || sheet.head.spec > r.spec) { r.next = sheet.head; sheet.head = r; }
    else {
        let p: Rule* = sheet.head;
        while (p.next != nullptr && p.next.spec <= r.spec) p = p.next;     // stable: equal specificity keeps source order
        r.next = p.next; p.next = r;
    }
}

function sheetClear(sheet: Sheet*) => void {
    let r: Rule* = sheet.head;
    while (r != nullptr) { let nx: Rule* = r.next; freeCompound(r.sel); freeAttrs(r.decls); CMem.free(r as void*); r = nx; }
    sheet.head = nullptr; sheet.seq = 0; sheet.usesPseudo = false;
}

function skipCssWs(lx: Lex*) => void {
    let more: bool = true;
    while (more) {
        while (isSpaceC(lx.peek())) lx.i = lx.i + 1;
        if (lx.peek() == 47 && lx.s[lx.i + 1] as int == 42) {
            lx.i = lx.i + 2;
            while (lx.peek() != 0 && !(lx.peek() == 42 && lx.s[lx.i + 1] as int == 47)) lx.i = lx.i + 1;
            if (lx.peek() != 0) lx.i = lx.i + 2;
        } else { more = false; }
    }
}

// Adds every rule in `css` to the sheet. Selector lists become one rule per selector.
function sheetParse(sheet: Sheet*, css: char*) => void {
    let lx: Lex; lx.s = css; lx.i = 0; lx.ok = false; lx.val = 0.0;
    let more: bool = true;
    while (more) {
        skipCssWs(&lx);
        if (lx.peek() == 0) { more = false; }
        else if (lx.peek() == 64) {
            // at-rule: skip it. NEXT: @media (viewport conditions), @import, @font-face
            let depth: int = 0; let stop: bool = false;
            while (!stop && lx.peek() != 0) {
                let c: int = lx.peek();
                if (c == 123) depth = depth + 1;
                if (c == 125) { depth = depth - 1; if (depth <= 0) stop = true; }
                if (c == 59 && depth == 0) stop = true;
                lx.i = lx.i + 1;
            }
        } else {
            let ss: int = lx.i;
            while (lx.peek() != 123 && lx.peek() != 0) lx.i = lx.i + 1;
            let selText: char* = strSub(css, ss, lx.i - ss);
            if (lx.peek() == 123) lx.i = lx.i + 1;
            let bs: int = lx.i; let depth2: int = 1;
            while (lx.peek() != 0 && depth2 > 0) {
                if (lx.peek() == 123) depth2 = depth2 + 1;
                if (lx.peek() == 125) depth2 = depth2 - 1;
                if (depth2 > 0) lx.i = lx.i + 1;
            }
            let body: char* = strSub(css, bs, lx.i - bs);
            if (lx.peek() == 125) lx.i = lx.i + 1;
            // split the selector list on commas
            let sl: int = strLen(selText); let a: int = 0; let k: int = 0;
            while (k <= sl) {
                if (k == sl || selText[k] as int == 44) {
                    let one: char* = strSub(selText, a, k - a);
                    let sel: Compound* = parseSelector(one, sheet);
                    strFree(one);
                    if (sel != nullptr) sheetAdd(sheet, sel, parseDecls(body));
                    a = k + 1;
                }
                k = k + 1;
            }
            strFree(selText); strFree(body);
        }
    }
}

// ---- cascade ----
// Tag defaults (Tags.os row) applied on top of inherited values.
function applyTagDefaults(st: Style*, row: TagRow*, parentPx: float, ctx: CssCtx*) => void {
    st.display = row.display;
    let px: float = parentPx;
    if (!ctx.tui) px = parentPx * row.scale;
    st.fontPx = px;
    if (row.bold) st.bold = true;
    if (row.italic) st.italic = true;
    if (row.underline) st.underline = true;
    if (row.strike) st.strike = true;
    if (row.ws != WS_NORMAL) st.whiteSpace = row.ws;
    if (row.taSet) st.textAlign = row.ta;
    if (row.color != 0) st.color = row.color;
    if (row.bg != 0) st.bg = row.bg;
    let u: float = ctx.unit;
    lenSet(&st.mt, LU_PX, row.mtEm * px + row.mtPx * u);
    lenSet(&st.mb, LU_PX, row.mbEm * px + row.mbPx * u);
    lenSet(&st.ml, LU_PX, row.mlPx * u);
    lenSet(&st.mr, LU_PX, row.mrPx * u);
    lenSet(&st.pl, LU_PX, (row.plPx + row.padPx) * u);
    lenSet(&st.pt, LU_PX, row.padPx * u);
    lenSet(&st.pr, LU_PX, row.padPx * u);
    lenSet(&st.pb, LU_PX, row.padPx * u);
    if (row.bPx > 0.0) {
        st.bt = row.bPx * u; st.br = row.bPx * u; st.bb = row.bPx * u; st.bl = row.bPx * u;
        st.borderColor = DEF_RULE;
    }
}

function cascadeNode(n: Node*, parent: Style*, sheet: Sheet*, ctx: CssCtx*, force: bool) => void {
    let need: bool = force || n.dirtyStyle;
    if (need) {
        n.st.reset();
        n.st.inheritFrom(parent);
        if (n.kind == NK_TEXT) {
            n.st.display = DP_INLINE;
        } else {
            applyTagDefaults(&n.st, &n.row, parent.fontPx, ctx);
            let r: Rule* = sheet.head;
            while (r != nullptr) {
                if (matchChain(n, r.sel)) {
                    ctx.curPx = parent.fontPx; applyDecls(&n.st, r.decls, ctx, 0);
                    ctx.curPx = n.st.fontPx;   applyDecls(&n.st, r.decls, ctx, 1);
                }
                r = r.next;
            }
            if (n.inlineCss != nullptr) applyDeclText(&n.st, n.inlineCss, ctx, parent.fontPx);
            ctx.curPx = n.st.fontPx;
            // API fields (assigned in app code) win over CSS
            let u: float = ctx.unit;
            if (n.gap >= 0.0) n.st.gap = n.gap * u;
            if (n.padding >= 0.0) {
                lenSet(&n.st.pt, LU_PX, n.padding * u); lenSet(&n.st.pr, LU_PX, n.padding * u);
                lenSet(&n.st.pb, LU_PX, n.padding * u); lenSet(&n.st.pl, LU_PX, n.padding * u);
            }
            if (n.grow >= 0.0) n.st.grow = n.grow;
            if (n.width >= 0.0) lenSet(&n.st.width, LU_PX, n.width * u);
            if (n.height >= 0.0) lenSet(&n.st.height, LU_PX, n.height * u);
            // element rules that CSS cannot override
            if (n.kind == NK_HIDDEN) n.st.display = DP_NONE;
            if (n.hasAttr("hidden")) n.st.display = DP_NONE;
            if (n.row.spec == SP_DIALOG) {
                if (!n.open) n.st.display = DP_NONE;
                else if (n.st.position == PS_STATIC) n.st.position = PS_FIXED;      // open dialog = centred overlay (Abs.os)
            }
            if (n.row.spec == SP_INPUT && n.itype == IT_HIDDEN) n.st.display = DP_NONE;
        }
        n.dirtyStyle = false;
    }
    if (need || n.subStyle) {
        let c: Node* = n.first;
        while (c != nullptr) { cascadeNode(c, &n.st, sheet, ctx, force || need); c = c.next; }
    }
    n.subStyle = false;
}
