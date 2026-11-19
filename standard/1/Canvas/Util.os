function copyBytes(dst: u8*, src: u8*, n: u64) => void {
    let i: u64 = 0;
    while (i < n) { dst[i] = src[i]; i = i + 1; }
}

function clampi(v: int, lo: int, hi: int) => int {
    let r: int = v;
    if (r < lo) r = lo;
    if (r > hi) r = hi;
    return r;
}

function imin(a: int, b: int) => int { if (a < b) return a; return b; }
function imax(a: int, b: int) => int { if (a > b) return a; return b; }

// Round to the nearest whole pixel. Text and widget geometry go through this: the font atlas is sampled with
// NEAREST, so a glyph at a half-pixel position doubles some columns and drops others (that is the "smear").
function snap(v: float) => float {
    let r: float = 0.0;
    if (v >= 0.0) { let n: int = (v + 0.5) as int; r = n as float; }
    else { let m: int = ((0.0 - v) + 0.5) as int; r = 0.0 - (m as float); }
    return r;
}

function isWordCh(c: int) => bool {
    return (c >= 48 && c <= 57) || (c >= 65 && c <= 90) || (c >= 97 && c <= 122) || c == 95;
}

function catStr(a: char*, b: char*) => char* {
    let la: int = 0; while (a[la] as int != 0) { la = la + 1; }
    let lb: int = 0; while (b[lb] as int != 0) { lb = lb + 1; }
    let r = CMem.malloc((la + lb + 1) as u64) as char*;
    let i: int = 0; while (i < la) { r[i] = a[i]; i = i + 1; }
    i = 0; while (i < lb) { r[la + i] = b[i]; i = i + 1; }
    r[la + lb] = 0 as char;
    return r;
}

function slen(s: char*) => int {
    let n: int = 0;
    while (s[n] as int != 0) { n = n + 1; }
    return n;
}
