// standard/1/Ui/Css/Values/Color.os
// #rgb #rgba #rrggbb #rrggbbaa, rgb()/rgba()/hsl()/hsla(), transparent, named colours. Colours are 0xRRGGBBAA.

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
