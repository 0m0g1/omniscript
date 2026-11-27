/// grep matcher: decides whether a line matches, and where.
/// Two engines behind one small interface (init / matches / findFrom):
///   literal  - naive substring search (default, -F)
///   regex    - standard/1/Regex.os (-E)
/// Case-insensitive (-i) works by lower-casing the pattern once and a scratch copy of every line (ASCII only).
/// NEXT STEPS: swap litFind for Boyer-Moore-Horspool or a memchr-style first-byte scan; add -w / -x;
/// add a precompiled regex once Regex.os offers one.

module Str = import "standard/1/String.os";
module RX = import "standard/1/Regex.os";

struct Matcher {
    pat: Str.String;                 // pattern (already lower-cased when icase)
    lowered: Str.String;             // scratch: lower-cased copy of the current line
    regex: bool = false;
    icase: bool = false;
    plen: u64 = 0;
}

// Lower-cases the pattern but leaves the character after a backslash alone, so \D / \W / \S keep their meaning.
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

// false = the regex is invalid (caller prints the error and exits 2).
function init(m: Matcher*, pattern: char*, regex: bool, icase: bool) => bool {
    m.regex = regex;
    m.icase = icase;
    m.pat.set(pattern);
    if (icase) lowerPattern(m);
    m.plen = m.pat.len;
    if (regex) {
        if (!RX.validate(m.pat.cstr())) return false;
    }
    return true;
}

function destroy(m: Matcher*) => void {
    m.pat.free();
    m.lowered.free();
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

// Naive substring search from byte `from`. First-byte check before the inner loop keeps it reasonably quick.
function litFind(hay: char*, hlen: u64, nee: char*, nlen: u64, from: u64, start: u64*) => bool {
    if (nlen == 0) return false;
    if (hlen < nlen) return false;
    let first: int = nee[0] as int;
    let last: u64 = hlen - nlen;
    let i: u64 = from;
    while (i <= last) {
        if ((hay[i] as int) == first) {
            let j: u64 = 1;
            while (j < nlen && (hay[i + j] as int) == (nee[j] as int)) { j = j + 1; }
            if (j == nlen) { start[0] = i; return true; }
        }
        i = i + 1;
    }
    return false;
}

// Does this line contain a match? An empty pattern matches every line (like real grep).
function matches(m: Matcher*, line: Str.String*) => bool {
    if (m.plen == 0) return true;
    let text: char* = textFor(m, line);
    let st: u64 = 0;
    let ln: u64 = 0;
    if (m.regex) return RX.find(m.pat.cstr(), text, &st, &ln);
    return litFind(text, lenFor(m, line), m.pat.cstr(), m.plen, 0, &st);
}

// Next match at or after byte `from`: start/len of the match. Used for --color.
// ASSUMPTION: Regex.findFrom reports `start` as an absolute offset into `text`.
function findFrom(m: Matcher*, line: Str.String*, from: u64, outStart: u64*, outLen: u64*) => bool {
    if (m.plen == 0) return false;
    let text: char* = textFor(m, line);
    if (m.regex) return RX.findFrom(m.pat.cstr(), text, from, outStart, outLen);
    if (!litFind(text, lenFor(m, line), m.pat.cstr(), m.plen, from, outStart)) return false;
    outLen[0] = m.plen;
    return true;
}
