// browser/js/scanner.os  -- source text -> token array (V8's "parsing/scanner").
module U = import "util.os";

let T_EOF: int = 0;
let T_NUM: int = 1;
let T_STR: int = 2;
let T_ID: int = 3;
let T_PUNCT: int = 4;
let T_TPL: int = 5;      // template literal, s = raw text between the backticks (the parser splits it)

struct Tok {
    t: int = 0;
    s: char* = nullptr;
    n: double = 0.0;
    line: int = 0;
    file: char* = nullptr;
    off: int = 0;                // character offset in the source buffer
}

// browser.os puts `/*@@name*/` before each script; the scanner turns it into the file name + a line counter restart.
let curLine: int = 1;
let curFile: char* = "script";
let curOff: int = 0;

let toks: Tok* = nullptr;
let ntok: int = 0;

function count() => int { return ntok; }

function add(t: int, s: char*, n: double) => void {
    toks[ntok].t = t; toks[ntok].s = s; toks[ntok].n = n;
    toks[ntok].line = curLine; toks[ntok].file = curFile; toks[ntok].off = curOff;
    ntok = ntok + 1;
}

function idStart(c: int) => bool { return U.isAlpha(c) || c == 95 || c == 36; }
function idPart(c: int) => bool { return U.isAlnum(c) || c == 95 || c == 36; }

function hexVal(c: int) => int {
    if (c >= 48 && c <= 57) return c - 48;
    if (c >= 97 && c <= 102) return c - 87;
    if (c >= 65 && c <= 70) return c - 55;
    return 0 - 1;
}

// processes \n \t \r \\ \' \" \` \$ \uXXXX (code points < 128 only) in src[a..b)
function unescape(src: char*, a: int, b: int) => char* {
    let out: char* = U.newStr(b - a + 2);
    let p: int = 0;
    let i: int = a;
    while (i < b) {
        let c: int = src[i] as int;
        if (c == 92 && i + 1 < b) {
            i = i + 1;
            let e: int = src[i] as int;
            if (e == 110) { out[p] = 10 as char; p = p + 1; }
            else if (e == 116) { out[p] = 9 as char; p = p + 1; }
            else if (e == 114) { out[p] = 13 as char; p = p + 1; }
            else if (e == 117 && i + 4 < b) {
                let v: int = hexVal(src[i + 1] as int) * 4096 + hexVal(src[i + 2] as int) * 256 + hexVal(src[i + 3] as int) * 16 + hexVal(src[i + 4] as int);
                if (v > 0 && v < 128) { out[p] = v as char; p = p + 1; } else { out[p] = 63 as char; p = p + 1; }
                i = i + 4;
            }
            else { out[p] = e as char; p = p + 1; }
        }
        else { out[p] = c as char; p = p + 1; }
        i = i + 1;
    }
    out[p] = 0 as char;
    return out;
}

// number at src[i..]: digits, optional fraction (".4" allowed), optional exponent. Returns the end index.
function scanNumber(src: char*, n: int, start: int) => int {
    let i: int = start;
    while (i < n && U.isDigit(src[i] as int)) i = i + 1;
    if (i < n && (src[i] as int) == 46) { i = i + 1; while (i < n && U.isDigit(src[i] as int)) i = i + 1; }
    let mant: char* = U.sub(src, start, i);
    if ((mant[0] as int) == 46) mant = U.cat("0", mant);
    let v: double = U.toNum(mant);
    if (i < n && ((src[i] as int) == 101 || (src[i] as int) == 69)) {
        let j: int = i + 1;
        let neg: bool = false;
        if (j < n && ((src[j] as int) == 43 || (src[j] as int) == 45)) { neg = (src[j] as int) == 45; j = j + 1; }
        if (j < n && U.isDigit(src[j] as int)) {
            let ex: int = 0;
            while (j < n && U.isDigit(src[j] as int)) { ex = ex * 10 + ((src[j] as int) - 48); j = j + 1; }
            while (ex > 0) { if (neg) v = v / 10.0; else v = v * 10.0; ex = ex - 1; }
            i = j;
        }
    }
    add(T_NUM, "", v);
    return i;
}

function punct(src: char*, n: int, i: int) => int {   // adds one punctuator token, returns its length
    let c1: int = src[i] as int;
    let c2: int = 0;
    let c3: int = 0;
    if (i + 1 < n) c2 = src[i + 1] as int;
    if (i + 2 < n) c3 = src[i + 2] as int;
    if (c1 == 61 && c2 == 61 && c3 == 61) { add(T_PUNCT, "==", 0.0); return 3; }
    if (c1 == 33 && c2 == 61 && c3 == 61) { add(T_PUNCT, "!=", 0.0); return 3; }
    if (c1 == 46 && c2 == 46 && c3 == 46) { add(T_PUNCT, "...", 0.0); return 3; }
    if (c2 != 0) {
        let two: char* = U.sub(src, i, i + 2);
        if (U.streq(two, "=>") || U.streq(two, "==") || U.streq(two, "!=") || U.streq(two, "<=") || U.streq(two, ">=")
            || U.streq(two, "&&") || U.streq(two, "||") || U.streq(two, "??") || U.streq(two, "++") || U.streq(two, "--")
            || U.streq(two, "+=") || U.streq(two, "-=") || U.streq(two, "*=") || U.streq(two, "/=")) {
            add(T_PUNCT, two, 0.0); return 2;
        }
        if (U.streq(two, "?.") && !U.isDigit(c3)) { add(T_PUNCT, two, 0.0); return 2; }
    }
    add(T_PUNCT, U.sub(src, i, i + 1), 0.0);
    return 1;
}

function scan(src: char*) => Tok* {
    let n: int = U.slen(src);
    toks = U.alloc(sizeof(Tok) * (n + 2)) as Tok*;
    ntok = 0;
    curLine = 1; curFile = "script";
    let i: int = 0;
    while (i < n) {
        curOff = i;
        let c: int = src[i] as int;
        let c2: int = 0;
        if (i + 1 < n) c2 = src[i + 1] as int;
        if (U.isSpace(c)) { if (c == 10) curLine = curLine + 1; i = i + 1; }
        else if (c == 47 && c2 == 47) { while (i < n && (src[i] as int) != 10) i = i + 1; }
        else if (c == 47 && c2 == 42) {
            let c3: int = 0;
            if (i + 3 < n && (src[i + 2] as int) == 64 && (src[i + 3] as int) == 64) c3 = 1;   // /*@@name*/
            let a: int = i + 4;
            i = i + 2;
            while (i + 1 < n && !((src[i] as int) == 42 && (src[i + 1] as int) == 47)) { if ((src[i] as int) == 10) curLine = curLine + 1; i = i + 1; }
            if (c3 == 1) { curFile = U.sub(src, a, i); curLine = 0; }
            i = i + 2;
        }
        else if (U.isDigit(c) || (c == 46 && U.isDigit(c2))) { i = scanNumber(src, n, i); }
        else if (idStart(c)) {
            let a: int = i;
            while (i < n && idPart(src[i] as int)) i = i + 1;
            add(T_ID, U.sub(src, a, i), 0.0);
        }
        else if (c == 34 || c == 39) {
            let a: int = i + 1;
            i = a;
            while (i < n && (src[i] as int) != c) { if ((src[i] as int) == 92) i = i + 1; i = i + 1; }
            add(T_STR, unescape(src, a, i), 0.0);
            i = i + 1;
        }
        else if (c == 96) {                                   // template: find the closing backtick, skipping ${ ... } and escapes
            let a: int = i + 1;
            i = a;
            let depth: int = 0;
            let go: bool = true;
            while (i < n && go) {
                let d: int = src[i] as int;
                if (d == 10) curLine = curLine + 1;
                if (d == 92) { i = i + 2; }
                else if (d == 36 && i + 1 < n && (src[i + 1] as int) == 123) { depth = depth + 1; i = i + 2; }
                else if (d == 125 && depth > 0) { depth = depth - 1; i = i + 1; }
                else if (d == 96 && depth == 0) { go = false; }
                else { i = i + 1; }
            }
            add(T_TPL, U.sub(src, a, i), 0.0);
            i = i + 1;
        }
        else { i = i + punct(src, n, i); }
    }
    add(T_EOF, "", 0.0);
    return toks;
}
