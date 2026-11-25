// standard/1/Ui/Css/Media/MediaQuery.os
// Media query evaluation (Blink: MediaQueryEvaluator). The environment is the gMedia* globals (Core/State.os).
//   Types      all screen (print / speech / ... are false)           Operators  not only and  ,(or)
//   Features   width height min-/max-width|height  (range syntax: (width >= 600px), (400px <= width <= 700px))
//              prefers-color-scheme prefers-reduced-motion prefers-contrast forced-colors hover pointer
//              any-hover any-pointer orientation display-mode (min|max)-resolution -webkit-(min|max)-device-pixel-ratio
// Unknown features evaluate to false. NEXT: store the condition on the Rule and evaluate at cascade time so a
// resize does not need a re-parse (needs Rule.media in Types.os).

struct MVal { v: float = 0.0; ok: bool = false; }

// "768px" "48em" "1.5" "2dppx" "192dpi" -> px / dppx
function mediaNumber(s: char*, out: MVal*) => void {
    out.ok = false; out.v = 0.0;
    let lx: Lex; lx.s = s; lx.i = 0; lx.ok = false; lx.val = 0.0;
    let x: float = lx.number();
    if (lx.ok) {
        let at: int = lx.i;
        out.ok = true; out.v = x;
        if (eqAtI(s, at, "rem") || eqAtI(s, at, "em")) out.v = x * 16.0;
        else if (eqAtI(s, at, "dpi")) out.v = x / 96.0;
        else if (eqAtI(s, at, "pt")) out.v = x * 1.3333;
        else if (eqAtI(s, at, "dppx") || eqAtI(s, at, "px") || eqAtI(s, at, "x") || (s[at] as int) == 0) out.v = x;
        else out.ok = false;
    }
}

// op: 1 <, 2 <=, 3 >, 4 >=
function mediaCompare(op: int, a: float, b: float) => bool {
    let r: bool = false;
    if (op == 1) r = a < b;
    else if (op == 2) r = a <= b;
    else if (op == 3) r = a > b;
    else if (op == 4) r = a >= b;
    return r;
}

// the text between the parentheses of one media feature. Self recursion only (double range).
function mediaFeature(inner0: char*) => bool {
    let inner: char* = trimDup(inner0, 0, strLen(inner0));
    let n: int = strLen(inner);
    let res: bool = false;
    let colon: int = 0 - 1; let cmp: int = 0 - 1; let k: int = 0;
    while (k < n) {
        let c: int = inner[k] as int;
        if (c == 58 && colon < 0) colon = k;
        if ((c == 60 || c == 62) && cmp < 0) cmp = k;
        k = k + 1;
    }
    let mv: MVal; mv.v = 0.0; mv.ok = false;
    if (colon > 0) {
        let nm0: char* = trimDup(inner, 0, colon); let name: char* = strLowerDup(nm0); strFree(nm0);
        let vl0: char* = trimDup(inner, colon + 1, n); let val: char* = strLowerDup(vl0); strFree(vl0);
        if (strEq(name, "min-width")) { mediaNumber(val, &mv); res = mv.ok && gMediaW >= mv.v; }
        else if (strEq(name, "max-width")) { mediaNumber(val, &mv); res = mv.ok && gMediaW <= mv.v; }
        else if (strEq(name, "width")) { mediaNumber(val, &mv); res = mv.ok && gMediaW == mv.v; }
        else if (strEq(name, "min-height")) { mediaNumber(val, &mv); res = mv.ok && gMediaH >= mv.v; }
        else if (strEq(name, "max-height")) { mediaNumber(val, &mv); res = mv.ok && gMediaH <= mv.v; }
        else if (strEq(name, "height")) { mediaNumber(val, &mv); res = mv.ok && gMediaH == mv.v; }
        else if (strEq(name, "prefers-color-scheme")) {
            if (strEq(val, "dark")) res = gMediaDark;
            else if (strEq(val, "light")) res = !gMediaDark;
        }
        else if (strEq(name, "prefers-reduced-motion") || strEq(name, "prefers-contrast") || strEq(name, "prefers-reduced-transparency")) { res = strEq(val, "no-preference"); }
        else if (strEq(name, "forced-colors")) { res = strEq(val, "none"); }
        else if (strEq(name, "hover") || strEq(name, "any-hover")) { res = strEq(val, "hover"); }
        else if (strEq(name, "pointer") || strEq(name, "any-pointer")) { res = strEq(val, "fine"); }
        else if (strEq(name, "orientation")) {
            if (strEq(val, "landscape")) res = gMediaW >= gMediaH;
            else if (strEq(val, "portrait")) res = gMediaW < gMediaH;
        }
        else if (strEq(name, "display-mode")) { res = strEq(val, "browser"); }
        else if (strEq(name, "min-resolution") || strEq(name, "-webkit-min-device-pixel-ratio") || strEq(name, "min-device-pixel-ratio")) { mediaNumber(val, &mv); res = mv.ok && gMediaDpr >= mv.v; }
        else if (strEq(name, "max-resolution") || strEq(name, "-webkit-max-device-pixel-ratio") || strEq(name, "max-device-pixel-ratio")) { mediaNumber(val, &mv); res = mv.ok && gMediaDpr <= mv.v; }
        strFree(name); strFree(val);
    }
    else if (cmp > 0) {
        let opLen: int = 1;
        if ((inner[cmp + 1] as int) == 61) opLen = 2;
        let op: int = 1;
        if ((inner[cmp] as int) == 60) { if (opLen == 2) op = 2; else op = 1; }
        else { if (opLen == 2) op = 4; else op = 3; }
        // a second operator: "a <= b <= c" is "a <= b" and "b <= c"
        let cmp2: int = 0 - 1; let q: int = cmp + opLen;
        while (q < n) { let c2: int = inner[q] as int; if ((c2 == 60 || c2 == 62) && cmp2 < 0) cmp2 = q; q = q + 1; }
        if (cmp2 > 0) {
            let s1: char* = trimDup(inner, 0, cmp2);
            let s2: char* = trimDup(inner, cmp + opLen, n);
            res = mediaFeature(s1) && mediaFeature(s2);
            strFree(s1); strFree(s2);
        }
        else {
            let l0: char* = trimDup(inner, 0, cmp); let lname: char* = strLowerDup(l0); strFree(l0);
            let r0: char* = trimDup(inner, cmp + opLen, n); let rname: char* = strLowerDup(r0); strFree(r0);
            let featLeft: bool = strEq(lname, "width") || strEq(lname, "height");
            let featRight: bool = strEq(rname, "width") || strEq(rname, "height");
            if (featLeft || featRight) {
                let fv: float = gMediaW;
                let want: char* = rname;
                let opN: int = op;
                if (featLeft) { if (strEq(lname, "height")) fv = gMediaH; }
                else {
                    if (strEq(rname, "height")) fv = gMediaH;
                    want = lname;
                    if (op == 1) opN = 3; else if (op == 2) opN = 4; else if (op == 3) opN = 1; else opN = 2;   // flip: value OP feature -> feature OP' value
                }
                mediaNumber(want, &mv);
                if (mv.ok) res = mediaCompare(opN, fv, mv.v);
            }
            strFree(lname); strFree(rname);
        }
    }
    else { res = inList(inner, "hover any-hover pointer any-pointer color"); }       // boolean context
    strFree(inner);
    return res;
}

// one query (no commas): [not|only] [type] [and (feature)]*
function mediaQueryOne(q: char*) => bool {
    let neg: bool = false; let ok: bool = true; let idx: int = 0; let more: bool = true;
    while (more) {
        let t: char* = nthToken(q, idx);
        if (t == nullptr) { more = false; }
        else {
            if (strEqI(t, "not")) { neg = true; }
            else if (strEqI(t, "only") || strEqI(t, "and") || strEqI(t, "all") || strEqI(t, "screen")) { }
            else if ((t[0] as int) == 40) {
                let inner: char* = trimDup(t, 1, strLen(t) - 1);
                if (!mediaFeature(inner)) ok = false;
                strFree(inner);
            }
            else { ok = false; }                                  // print, speech, unknown types and `or`
            strFree(t);
            idx = idx + 1;
        }
    }
    let r: bool = ok;
    if (neg) r = !ok;
    return r;
}

// "(min-width: 600px) and (max-width: 900px), print" -> true when any comma-separated query matches
function mediaMatches(cond: char*) => bool {
    let n: int = strLen(cond);
    let a: int = 0; let i: int = 0; let depth: int = 0; let any: bool = false; let parts: int = 0;
    while (i <= n) {
        let c: int = 0;
        if (i < n) c = cond[i] as int;
        if (c == 40) depth = depth + 1;
        else if (c == 41) depth = depth - 1;
        if (i == n || (c == 44 && depth == 0)) {
            let one: char* = trimDup(cond, a, i);
            if (strLen(one) > 0) { parts = parts + 1; if (mediaQueryOne(one)) any = true; }
            strFree(one);
            a = i + 1;
        }
        i = i + 1;
    }
    if (parts == 0) any = true;                                   // "@media { }" is "all"
    return any;
}
