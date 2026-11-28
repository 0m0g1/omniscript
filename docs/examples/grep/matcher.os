/// grep matcher: decides whether a line matches, and where.
/// Two engines behind one small interface (init / matches / findFrom):
///   literal  - naive substring search (default, -F)
///   regex    - standard/1/Regex.os (-E)
/// Several patterns (-e a -e b, -f file) are kept in one string, one pattern per '\n'-separated segment.
/// Literal mode tries every segment; regex mode hands the engine "(p1)|(p2)|...".
/// -x (whole line) and -w (whole word) are applied on top of either engine.
/// Case-insensitive (-i) works by lower-casing the patterns once and a scratch copy of every line (ASCII only).
/// NEXT STEPS: Boyer-Moore-Horspool for litFindAt; a precompiled regex once Regex.os offers one.

module Str = import "standard/1/String.os";
module L = import "standard/1/List.os";
module RX = import "standard/1/Regex.os";

struct Matcher {
    pat: Str.String;                 // all patterns, '\n' separated (lower-cased when icase)
    rx: Str.String;                  // regex mode: what the regex engine gets
    lowered: Str.String;             // scratch: lower-cased copy of the current line
    starts: L.List<u64>;             // offset of every pattern inside pat
    lens: L.List<u64>;               // length of every pattern
    regex: bool = false;
    icase: bool = false;
    word: bool = false;              // -w
    whole: bool = false;             // -x
    noPats: bool = false;            // no pattern at all (-f with an empty file): nothing matches
    matchAll: bool = false;          // some pattern is empty: every line matches
}

// Lower-cases the patterns but leaves the character after a backslash alone, so \D / \W / \S keep their meaning.
function lowerPattern(m: Matcher*) => void {
    let out: Str.String;
    let i: u64 = 0;
    while (i < m.pat.len) {
        let c: int = m.pat.data[i] as int;
        if (c == 92) {                                    // backslash: copy it and the next byte verbatim
            out.appendChar(m.pat.data[i]);
            i = i + 1;
            if (i < m.pat.len) { out.appendChar(m.pat.data[i]); i = i + 1; }
        } else {
            if (c >= 65 && c <= 90) c = c + 32;
            out.appendChar(c as char);
            i = i + 1;
        }
    }
    m.pat.set(out.cstr());
    out.free();
}

// patterns: '\n'-separated list, `count` of them. false = the regex is invalid (caller prints the error, exits 2).
function init(m: Matcher*, patterns: char*, count: u64, regex: bool, icase: bool, word: bool, whole: bool) => bool {
    m.regex = regex;
    m.icase = icase;
    m.word = word;
    m.whole = whole;
    if (count == 0) { m.noPats = true; return true; }
    m.pat.set(patterns);
    if (icase) lowerPattern(m);

    // split into segments
    let i: u64 = 0;
    let segStart: u64 = 0;
    while (i <= m.pat.len) {
        let c: int = 0;
        if (i < m.pat.len) c = m.pat.data[i] as int;
        if (i == m.pat.len || c == 10) {
            let n: u64 = i - segStart;
            m.starts.push(segStart);
            m.lens.push(n);
            if (n == 0) m.matchAll = true;
            segStart = i + 1;
        }
        i = i + 1;
    }

    if (regex) {
        let nseg: u64 = m.lens.len;
        if (whole) { m.rx.appendChar(94 as char); m.rx.appendChar(40 as char); }          // ^(
        let k: u64 = 0;
        while (k < nseg) {
            if (k > 0) m.rx.appendChar(124 as char);                                       // |
            if (nseg > 1) m.rx.appendChar(40 as char);                                     // (
            let off: u64 = m.starts.at(k);
            let ln: u64 = m.lens.at(k);
            let j: u64 = 0;
            while (j < ln) { m.rx.appendChar(m.pat.data[off + j]); j = j + 1; }
            if (nseg > 1) m.rx.appendChar(41 as char);                                     // )
            k = k + 1;
        }
        if (whole) { m.rx.appendChar(41 as char); m.rx.appendChar(36 as char); }          // )$
        if (!RX.validate(m.rx.cstr())) return false;
    }
    return true;
}

function destroy(m: Matcher*) => void {
    m.pat.free();
    m.rx.free();
    m.lowered.free();
    m.starts.free();
    m.lens.free();
}

// The text to match against: the line itself, or its lower-cased copy.
function textFor(m: Matcher*, line: Str.String*) => char* {
    if (!m.icase) return line.cstr();
    m.lowered.set(line.cstr());
    m.lowered.toLower();
    return m.lowered.cstr();
}

function lenFor(m: Matcher*, line: Str.String*) => u64 {
    if (!m.icase) return line.len;
    return m.lowered.len;
}

function isWordChar(c: int) => bool {
    if (c >= 48 && c <= 57) return true;
    if (c >= 65 && c <= 90) return true;
    if (c >= 97 && c <= 122) return true;
    return c == 95;
}

// Naive substring search from byte `from`; the needle is nee[off .. off+nlen). First-byte check before the inner loop.
function litFindAt(hay: char*, hlen: u64, nee: char*, off: u64, nlen: u64, from: u64, start: u64*) => bool {
    if (nlen == 0) return false;
    if (hlen < nlen) return false;
    let first: int = nee[off] as int;
    let last: u64 = hlen - nlen;
    let i: u64 = from;
    while (i <= last) {
        if ((hay[i] as int) == first) {
            let j: u64 = 1;
            while (j < nlen && (hay[i + j] as int) == (nee[off + j] as int)) { j = j + 1; }
            if (j == nlen) { start[0] = i; return true; }
        }
        i = i + 1;
    }
    return false;
}

// Do text[0..n) and nee[off..off+n) hold the same bytes?
function sameAt(text: char*, nee: char*, off: u64, n: u64) => bool {
    let i: u64 = 0;
    while (i < n && (text[i] as int) == (nee[off + i] as int)) { i = i + 1; }
    return i == n;
}

// Earliest match at or after `from` over all patterns (the longest one wins a tie). No -w / -x handling here.
function rawFind(m: Matcher*, text: char*, tlen: u64, from: u64, outStart: u64*, outLen: u64*) => bool {
    if (m.regex) return RX.findFrom(m.rx.cstr(), text, from, outStart, outLen);
    let found: bool = false;
    let bs: u64 = 0;
    let bl: u64 = 0;
    let k: u64 = 0;
    while (k < m.lens.len) {
        let nl: u64 = m.lens.at(k);
        let st: u64 = 0;
        if (litFindAt(text, tlen, m.pat.cstr(), m.starts.at(k), nl, from, &st)) {
            if (!found || st < bs || (st == bs && nl > bl)) { bs = st; bl = nl; found = true; }
        }
        k = k + 1;
    }
    if (found) { outStart[0] = bs; outLen[0] = bl; }
    return found;
}

// First match at or after `from` that is not glued to other word characters (-w).
function wordFind(m: Matcher*, text: char*, tlen: u64, from: u64, outStart: u64*, outLen: u64*) => bool {
    let pos: u64 = from;
    let st: u64 = 0;
    let ln: u64 = 0;
    let result: bool = false;
    let go: bool = true;
    while (go && pos <= tlen) {
        if (!rawFind(m, text, tlen, pos, &st, &ln)) {
            go = false;
        } else {
            let okL: bool = true;
            let okR: bool = true;
            if (st > 0 && isWordChar(text[st - 1] as int)) okL = false;
            if (st + ln < tlen && isWordChar(text[st + ln] as int)) okR = false;
            if (ln > 0 && okL && okR) {
                outStart[0] = st;
                outLen[0] = ln;
                result = true;
                go = false;
            } else {
                pos = st + 1;
            }
        }
    }
    return result;
}

function matchText(m: Matcher*, text: char*, tlen: u64) => bool {
    let st: u64 = 0;
    let ln: u64 = 0;
    let result: bool = false;
    if (m.whole) {                                        // -x: the whole line must match
        if (m.regex) {
            result = RX.find(m.rx.cstr(), text, &st, &ln);
        } else {
            let k: u64 = 0;
            while (!result && k < m.lens.len) {
                if (m.lens.at(k) == tlen && sameAt(text, m.pat.cstr(), m.starts.at(k), tlen)) result = true;
                k = k + 1;
            }
        }
    } else if (m.matchAll) {                              // an empty pattern matches every line (like real grep)
        result = true;
    } else if (m.word) {
        result = wordFind(m, text, tlen, 0, &st, &ln);
    } else if (m.regex) {
        result = RX.find(m.rx.cstr(), text, &st, &ln);
    } else {
        result = rawFind(m, text, tlen, 0, &st, &ln);
    }
    return result;
}

// Does this line contain a match?
function matches(m: Matcher*, line: Str.String*) => bool {
    if (m.noPats) return false;
    let text: char* = textFor(m, line);
    let tlen: u64 = lenFor(m, line);
    return matchText(m, text, tlen);
}

// Next match at or after byte `from`: start/len of the match. Used for --color and -o.
// ASSUMPTION: Regex.findFrom reports `start` as an absolute offset into `text`.
function findFrom(m: Matcher*, line: Str.String*, from: u64, outStart: u64*, outLen: u64*) => bool {
    if (m.noPats) return false;
    let text: char* = textFor(m, line);
    let tlen: u64 = lenFor(m, line);
    if (m.whole) {                                        // the match is the whole line
        if (from == 0 && tlen > 0 && matchText(m, text, tlen)) {
            outStart[0] = 0;
            outLen[0] = tlen;
            return true;
        }
        return false;
    }
    if (m.word) return wordFind(m, text, tlen, from, outStart, outLen);
    return rawFind(m, text, tlen, from, outStart, outLen);
}
