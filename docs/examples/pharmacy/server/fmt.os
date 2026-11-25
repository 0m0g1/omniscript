// pharmacy/server/fmt.os -- money (integer cents) and number parsing.
module Str = import "standard/1/String.os";
module CFG = import "pharmacy/server/config.os";

function pad2(n: i64, out: Str.String*) => void {
    if (n < 10) out.appendChar(48 as char);
    out.appendInt(n);
}

function money(c: i64, out: Str.String*) => void {
    out.append(CFG.CURRENCY);
    out.appendInt(c / 100);
    out.appendChar(46 as char);
    pad2(c % 100, out);
}

// "12", "12.5", "12.50" -> cents. Rejects letters, a second dot, more than 2 decimals, empty text.
function parseMoney(s: char*, out: i64*) => bool {
    let i: u64 = 0;
    let ok: bool = true;
    let whole: i64 = 0;
    let frac: i64 = 0;
    let nd: int = 0;
    let seen: bool = false;
    let digits: int = 0;
    while (ok && (s[i] as int) != 0) {
        let c: int = s[i] as int;
        if (c >= 48 && c <= 57) {
            digits = digits + 1;
            if (!seen) { whole = whole * 10 + ((c - 48) as i64); }
            else {
                if (nd < 2) { frac = frac * 10 + ((c - 48) as i64); nd = nd + 1; }
                else { ok = false; }
            }
            if (whole > 100000000) ok = false;
        } else if (c == 46 && !seen) { seen = true; }
        else { ok = false; }
        i = i + 1;
    }
    if (digits == 0) ok = false;
    if (nd == 1) frac = frac * 10;
    out[0] = whole * 100 + frac;
    return ok;
}

// whole number 0..999999999 (digits only)
function parseInt(s: char*, out: i64*) => bool {
    let i: u64 = 0;
    let ok: bool = (s[0] as int) != 0;
    let v: i64 = 0;
    while (ok && (s[i] as int) != 0) {
        let c: int = s[i] as int;
        if (c >= 48 && c <= 57) { v = v * 10 + ((c - 48) as i64); }
        else { ok = false; }
        if (v > 999999999) ok = false;
        i = i + 1;
    }
    out[0] = v;
    return ok;
}

// optional sign, then digits (adjustments can be negative)
function parseSigned(s: char*, out: i64*) => bool {
    let neg: bool = false;
    let ok: bool = true;
    let v: i64 = 0;
    if ((s[0] as int) == 45) {
        neg = true;
        ok = parseInt(&s[1], &v);
    } else {
        ok = parseInt(s, &v);
    }
    if (neg) v = 0 - v;
    out[0] = v;
    return ok;
}
