// standard/1/Ui/Css/Core/Strings.os
// Small string helpers used by every Css module.

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

function trimDup(s: char*, from: int, to: int) => char* {
    let a: int = from; let b: int = to;
    while (a < b && isSpaceC(s[a] as int)) a = a + 1;
    while (b > a && isSpaceC(s[b - 1] as int)) b = b - 1;
    return strSub(s, a, b - a);
}

// first top-level token of a value ("red blue" -> "red", "rgb(1, 2, 3) blue" -> "rgb(1, 2, 3)")
function firstToken(v: char*) => char* {
    let n: int = strLen(v);
    let i: int = 0; let depth: int = 0; let scan: bool = true;
    while (i < n && scan) {
        let d: int = v[i] as int;
        if (d == 40) depth = depth + 1;
        else if (d == 41) depth = depth - 1;
        if (depth == 0 && (d == 32 || d == 9 || d == 10 || d == 13)) scan = false;
        else i = i + 1;
    }
    return trimDup(v, 0, i);
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

// Token `idx` (0-based) of a whitespace separated list, ignoring whitespace inside parentheses.
// "0 auto clamp(1rem, 4vw, 3rem)" -> idx 2 = "clamp(1rem, 4vw, 3rem)". nullptr when there is no such token.
function nthToken(v: char*, idx: int) => char* {
    let n: int = strLen(v);
    let i: int = 0; let count: int = 0; let res: char* = nullptr; let done: bool = false;
    while (i < n && !done) {
        let c0: int = v[i] as int;
        if (isSpaceC(c0)) { i = i + 1; }
        else {
            let a: int = i; let depth: int = 0; let scan: bool = true;
            while (i < n && scan) {
                let d: int = v[i] as int;
                if (d == 40) depth = depth + 1;
                else if (d == 41) depth = depth - 1;
                if (depth <= 0 && isSpaceC(d)) scan = false; else i = i + 1;
            }
            if (count == idx) { res = trimDup(v, a, i); done = true; }
            count = count + 1;
        }
    }
    return res;
}
