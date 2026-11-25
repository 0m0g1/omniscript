// standard/1/Ui/Css/Media/Supports.os
// @supports evaluation. A `(property: value)` test is true when the engine implements the property (cssIsSupported,
// Properties/Property.os), so progressive-enhancement blocks only apply what this engine can actually do.
// selector(...), font-tech(...) and other functions are false.

function supportsDecl(inner: char*) => bool {
    let n: int = strLen(inner); let colon: int = 0 - 1; let k: int = 0;
    while (k < n) { if ((inner[k] as int) == 58 && colon < 0) colon = k; k = k + 1; }
    let r: bool = false;
    if (colon > 0) {
        let nm0: char* = trimDup(inner, 0, colon); let name: char* = strLowerDup(nm0); strFree(nm0);
        r = cssIsSupported(name);
        strFree(name);
    }
    return r;
}

// "(display: grid) and (not (aspect-ratio: 1))" -> bool. Self recursion only.
function supportsMatches(cond: char*) => bool {
    let acc: bool = true; let have: bool = false; let neg: bool = false; let mode: int = 1;
    let idx: int = 0; let more: bool = true;
    while (more) {
        let t: char* = nthToken(cond, idx);
        if (t == nullptr) { more = false; }
        else {
            if (strEqI(t, "not")) { neg = true; }
            else if (strEqI(t, "and")) { mode = 1; }
            else if (strEqI(t, "or")) { mode = 2; }
            else {
                let val: bool = false;
                if ((t[0] as int) == 40) {
                    let inner: char* = trimDup(t, 1, strLen(t) - 1);
                    if ((inner[0] as int) == 40 || eqAtI(inner, 0, "not ")) val = supportsMatches(inner);
                    else val = supportsDecl(inner);
                    strFree(inner);
                }
                if (neg) val = !val;
                neg = false;
                if (!have) { acc = val; have = true; }
                else if (mode == 2) { acc = acc || val; }
                else { acc = acc && val; }
            }
            strFree(t);
            idx = idx + 1;
        }
    }
    return acc;
}
