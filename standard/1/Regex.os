// standard/1/Regex.os
// Tier 4: small backtracking regex (no deps). Supports: literals . * + ? ^ $ [abc] [a-z] [^x] \d \w \s \D \W \S and `|` at top level.
// No groups/captures yet (TODO). Usage: Regex.match("^a+b", "aaab") / Regex.find(pat, text, &start, &len).
module CStr = import "standard/1/c/string.os";

function isWord(c: int) => bool { return (c >= 48 && c <= 57) || (c >= 65 && c <= 90) || (c >= 97 && c <= 122) || c == 95; }
function isDig(c: int) => bool { return c >= 48 && c <= 57; }
function isSp(c: int) => bool { return c == 32 || (c >= 9 && c <= 13); }

// Length in pattern chars of one atom starting at p.
function atomLen(p: char*) => u64 {
    if ((p[0] as int) == 92) return 2;
    if ((p[0] as int) == 91) {
        let i: u64 = 1;
        if ((p[i] as int) == 94) i = i + 1;
        if ((p[i] as int) == 93) i = i + 1;
        while ((p[i] as int) != 0 && (p[i] as int) != 93) i = i + 1;
        if ((p[i] as int) == 93) i = i + 1;
        return i;
    }
    return 1;
}

// Does atom at p match character c?
function atomMatch(p: char*, c: int) => bool {
    if (c == 0) return false;
    let a: int = p[0] as int;
    if (a == 46) return true;
    if (a == 92) {
        let e: int = p[1] as int;
        if (e == 100) return isDig(c);
        if (e == 68) return !isDig(c);
        if (e == 119) return isWord(c);
        if (e == 87) return !isWord(c);
        if (e == 115) return isSp(c);
        if (e == 83) return !isSp(c);
        if (e == 110) return c == 10;
        if (e == 116) return c == 9;
        return c == e;
    }
    if (a == 91) {
        let i: u64 = 1;
        let neg: bool = false;
        if ((p[i] as int) == 94) { neg = true; i = i + 1; }
        let hit: bool = false;
        let first: bool = true;
        while ((p[i] as int) != 0 && ((p[i] as int) != 93 || first)) {
            first = false;
            let lo: int = p[i] as int;
            if ((p[i + 1] as int) == 45 && (p[i + 2] as int) != 93 && (p[i + 2] as int) != 0) {
                let hi: int = p[i + 2] as int;
                if (c >= lo && c <= hi) hit = true;
                i = i + 3;
            } else {
                if (c == lo) hit = true;
                i = i + 1;
            }
        }
        return hit != neg;
    }
    return a == c;
}

// Match pattern p (one alternative, no '|') at text t. Returns matched length or -1.
function matchHere(p: char*, t: char*, base: char*) => i64 {
    if ((p[0] as int) == 0) return 0;
    if ((p[0] as int) == 36 && (p[1] as int) == 0) { if ((t[0] as int) == 0) return 0; return -1; }
    if ((p[0] as int) == 94) { if (t == base) return matchHere((p as u64 + 1) as char*, t, base); return -1; }
    let al: u64 = atomLen(p);
    let rest: char* = (p as u64 + al) as char*;
    let q: int = rest[0] as int;
    if (q == 42 || q == 43 || q == 63) {
        let after: char* = (rest as u64 + 1) as char*;
        let min: u64 = 0; if (q == 43) min = 1;
        let max: u64 = 0xFFFFFFFFFFFFFFFF; if (q == 63) max = 1;
        // greedy: count max run then back off
        let n: u64 = 0;
        while (n < max && atomMatch(p, t[n] as int)) n = n + 1;
        while (true) {
            if (n >= min) {
                let r: i64 = matchHere(after, (t as u64 + n) as char*, base);
                if (r >= 0) return r + (n as i64);
            }
            if (n == 0) break;
            n = n - 1;
        }
        return -1;
    }
    if (atomMatch(p, t[0] as int)) {
        let r: i64 = matchHere(rest, (t as u64 + 1) as char*, base);
        if (r >= 0) return r + 1;
    }
    return -1;
}

// Tries each '|' alternative at position t.
function matchAlts(pat: char*, t: char*, base: char*) => i64 {
    let start: u64 = 0;
    let i: u64 = 0;
    let best: i64 = -1;
    while (true) {
        let c: int = pat[i] as int;
        if (c == 92) { if ((pat[i + 1] as int) != 0) i = i + 2; else i = i + 1; continue; }
        if (c == 124 || c == 0) {
            // copy alternative
            let buf: [256]char;
            let n: u64 = i - start;
            if (n > 255) n = 255;
            let k: u64 = 0;
            while (k < n) { buf[k] = pat[start + k]; k = k + 1; }
            buf[n] = 0 as char;
            let r: i64 = matchHere(&buf as char*, t, base);
            if (r > best) best = r;
            if (c == 0) break;
            start = i + 1;
        }
        i = i + 1;
    }
    return best;
}

// true if the whole text matches (anchored both ends)
function matchAll(pat: char*, text: char*) => bool {
    let r: i64 = matchAlts(pat, text, text);
    return r >= 0 && (r as u64) == CStr.strlen(text);
}

// finds first match: sets start index and length
function find(pat: char*, text: char*, start: u64*, len: u64*) => bool {
    let i: u64 = 0;
    while (true) {
        let r: i64 = matchAlts(pat, (text as u64 + i) as char*, text);
        if (r >= 0) { *start = i; *len = r as u64; return true; }
        if ((text[i] as int) == 0) return false;
        i = i + 1;
    }
    return false;
}

function test(pat: char*, text: char*) => bool { let s: u64 = 0; let l: u64 = 0; return find(pat, text, &s, &l); }
