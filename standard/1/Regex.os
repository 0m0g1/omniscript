// standard/1/Regex.os
// Tier 4: small backtracking regex (no deps). Supports: literals . * + ? ^ $ [abc] [a-z] [^x] \d \w \s \D \W \S and `|` at top level.
// No groups/captures yet (TODO). Usage: Regex.match("^a+b", "aaab") / Regex.find(pat, text, &start, &len).
module CStr = import "standard/1/c/string.os";
module Str = import "standard/1/String.os";
module L = import "standard/1/List.os";

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
// Loop-free control flow: no `return`, `break` or `continue` inside loop bodies (compiler quirk).
function matchHere(p: char*, t: char*, base: char*) => i64 {
    let res: i64 = -1;
    let p0: int = p[0] as int;
    if (p0 == 0) {
        res = 0;
    } else if (p0 == 36 && (p[1] as int) == 0) {
        if ((t[0] as int) == 0) res = 0;
    } else if (p0 == 94) {
        if (t == base) res = matchHere((p as u64 + 1) as char*, t, base);
    } else {
        let al: u64 = atomLen(p);
        let rest: char* = (p as u64 + al) as char*;
        let q: int = rest[0] as int;
        if (q == 42 || q == 43 || q == 63) {
            let after: char* = (rest as u64 + 1) as char*;
            let min: u64 = 0; if (q == 43) min = 1;
            let max: u64 = 0xFFFFFFFFFFFFFFFF; if (q == 63) max = 1;
            // greedy: count the longest run, then back off one at a time
            let n: u64 = 0;
            while (n < max && atomMatch(p, t[n] as int)) n = n + 1;
            let going: bool = true;
            while (going) {
                if (n >= min) {
                    let r: i64 = matchHere(after, (t as u64 + n) as char*, base);
                    if (r >= 0) { res = r + (n as i64); going = false; }
                }
                if (going) {
                    if (n == 0) going = false; else n = n - 1;
                }
            }
        } else {
            if (atomMatch(p, t[0] as int)) {
                let r2: i64 = matchHere(rest, (t as u64 + 1) as char*, base);
                if (r2 >= 0) res = r2 + 1;
            }
        }
    }
    return res;
}

// Tries each '|' alternative at position t; returns the longest match length or -1.
function matchAlts(pat: char*, t: char*, base: char*) => i64 {
    let start: u64 = 0;
    let i: u64 = 0;
    let best: i64 = -1;
    let going: bool = true;
    while (going) {
        let c: int = pat[i] as int;
        if (c == 92) {
            if ((pat[i + 1] as int) != 0) i = i + 2; else i = i + 1;
        } else {
            if (c == 124 || c == 0) {
                let buf: [256]char;
                let n: u64 = i - start;
                if (n > 255) n = 255;
                let k: u64 = 0;
                while (k < n) { buf[k] = pat[start + k]; k = k + 1; }
                buf[n] = 0 as char;
                let r: i64 = matchHere(&buf as char*, t, base);
                if (r > best) best = r;
                if (c == 0) going = false;
                start = i + 1;
            }
            i = i + 1;
        }
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
    let found: bool = false;
    let going: bool = true;
    while (going) {
        let r: i64 = matchAlts(pat, (text as u64 + i) as char*, text);
        if (r >= 0) {
            start[0] = i;
            len[0] = r as u64;
            found = true;
            going = false;
        } else if ((text[i] as int) == 0) {
            going = false;
        } else {
            i = i + 1;
        }
    }
    return found;
}

function test(pat: char*, text: char*) => bool { let s: u64 = 0; let l: u64 = 0; return find(pat, text, &s, &l); }

struct Match {
    start: u64 = 0;
    len: u64 = 0;
}

// true if the pattern is well formed for this engine: no unclosed [...], no trailing backslash,
// no quantifier (* + ?) with nothing to repeat at the start of an alternative, alternatives < 256 chars.
function validate(pat: char*) => bool {
    let ok: bool = true;
    let i: u64 = 0;
    let altStart: u64 = 0;
    let atStart: bool = true;
    while (ok && (pat[i] as int) != 0) {
        let c: int = pat[i] as int;
        if (c == 92) {
            if ((pat[i + 1] as int) == 0) ok = false;
            else { i = i + 2; atStart = false; }
        } else if (c == 91) {
            let j: u64 = i + 1;
            if ((pat[j] as int) == 94) j = j + 1;
            if ((pat[j] as int) == 93) j = j + 1;
            while ((pat[j] as int) != 0 && (pat[j] as int) != 93) j = j + 1;
            if ((pat[j] as int) == 0) ok = false;
            else { i = j + 1; atStart = false; }
        } else if (c == 124) {
            if (i - altStart > 255) ok = false;
            altStart = i + 1;
            atStart = true;
            i = i + 1;
        } else if (c == 42 || c == 43 || c == 63) {
            if (atStart) ok = false;
            i = i + 1;
        } else {
            if (c != 94) atStart = false;
            i = i + 1;
        }
    }
    if (ok && i - altStart > 255) ok = false;
    return ok;
}

// Like find(), but starts scanning at index `from` (anchors still refer to the start of text).
function findFrom(pat: char*, text: char*, from: u64, start: u64*, len: u64*) => bool {
    let n: u64 = CStr.strlen(text);
    let i: u64 = from;
    let found: bool = false;
    let go: bool = true;
    while (go && i <= n) {
        let r: i64 = matchAlts(pat, (text as u64 + i) as char*, text);
        if (r >= 0) {
            start[0] = i;
            len[0] = r as u64;
            found = true;
            go = false;
        } else {
            i = i + 1;
        }
    }
    return found;
}

// Appends every non-overlapping match (left to right) to out.
function findAll(pat: char*, text: char*, out: L.List<Match>*) => bool {
    let n: u64 = CStr.strlen(text);
    let pos: u64 = 0;
    let ok: bool = true;
    let go: bool = true;
    while (go && ok && pos <= n) {
        let ms: u64 = 0;
        let ml: u64 = 0;
        if (!findFrom(pat, text, pos, &ms, &ml)) {
            go = false;
        } else {
            let m: Match;
            m.start = ms;
            m.len = ml;
            ok = out.push(m);
            if (ml == 0) pos = ms + 1; else pos = ms + ml;
        }
    }
    return ok;
}

function countMatches(pat: char*, text: char*) => u64 {
    let found: L.List<Match>;
    findAll(pat, text, &found);
    let n: u64 = found.len;
    found.free();
    return n;
}

function replaceImpl(pat: char*, text: char*, repl: char*, all: bool, out: Str.String*) => bool {
    let n: u64 = CStr.strlen(text);
    let pos: u64 = 0;
    let copied: u64 = 0;
    let ok: bool = true;
    let go: bool = true;
    while (go && ok && pos <= n) {
        let rs: u64 = 0;
        let rl: u64 = 0;
        if (!findFrom(pat, text, pos, &rs, &rl)) {
            go = false;
        } else {
            ok = out.appendN((text as u64 + copied) as char*, rs - copied);
            if (ok) ok = out.append(repl);
            copied = rs + rl;
            if (rl == 0) pos = rs + 1; else pos = rs + rl;
            if (!all) go = false;
        }
    }
    if (ok) ok = out.appendN((text as u64 + copied) as char*, n - copied);
    return ok;
}

// Appends text with every match replaced by repl.
function replaceAll(pat: char*, text: char*, repl: char*, out: Str.String*) => bool {
    return replaceImpl(pat, text, repl, true, out);
}

// Appends text with only the first match replaced.
function replaceFirst(pat: char*, text: char*, repl: char*, out: Str.String*) => bool {
    return replaceImpl(pat, text, repl, false, out);
}

// Splits text around non-empty matches. Parts are new Strings: free with Text.freeParts(&out).
function split(pat: char*, text: char*, out: L.List<Str.String>*) => bool {
    let n: u64 = CStr.strlen(text);
    let pos: u64 = 0;
    let copied: u64 = 0;
    let ok: bool = true;
    let go: bool = true;
    while (go && ok && pos <= n) {
        let ss: u64 = 0;
        let sl: u64 = 0;
        if (!findFrom(pat, text, pos, &ss, &sl)) {
            go = false;
        } else if (sl == 0) {
            pos = ss + 1;
        } else {
            let part: Str.String;
            ok = part.appendN((text as u64 + copied) as char*, ss - copied);
            if (ok) ok = out.push(part);
            copied = ss + sl;
            pos = copied;
        }
    }
    if (ok) {
        let last: Str.String;
        ok = last.appendN((text as u64 + copied) as char*, n - copied);
        if (ok) ok = out.push(last);
    }
    return ok;
}

// Backslash-escapes the characters this dialect treats as special: \ . * + ? ^ $ [ ] |
function escapeLiteral(s: char*, out: Str.String*) => bool {
    let ok: bool = true;
    let i: u64 = 0;
    while (ok && (s[i] as int) != 0) {
        let c: int = s[i] as int;
        if (c == 92 || c == 46 || c == 42 || c == 43 || c == 63 || c == 94 || c == 36 || c == 91 || c == 93 || c == 124) {
            ok = out.appendChar(92 as char);
        }
        if (ok) ok = out.appendChar(s[i]);
        i = i + 1;
    }
    return ok;
}
