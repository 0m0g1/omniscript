// standard/1/Yaml.os
// Tier 4: YAML subset parser/emitter over the Json.JsonValue DOM (same tree as Json.os / Toml.os).
//
//   let err: Yaml.ParseError;
//   let root = Yaml.parse(text, &err);           // nullptr on error: err.line / err.col / err.msg
//   if (root == nullptr) { printf("%d:%d %s\n", err.line, err.col, err.msg); }
//   let name = root.get("name").asStr("unnamed");
//   Yaml.freeValue(root);                         // frees children, then the root itself
//
//   let out: Str.String;
//   Yaml.stringify(root, &out);                   // block style, 2-space indent
//   out.free();
//
// Supported: block mappings and sequences (nested, "- key: v" compact form, sequence at the same indent as
// its parent key), plain / 'single' / "double" quoted scalars (escapes, multi-line folding), flow [..] and
// {..} (may span lines), literal | and folded > block scalars with -/+ chomping and indent digit, comments,
// anchors &a / aliases *a (deep-copied), merge keys "<<: *a" (and a list of aliases), a leading "---",
// %directives (skipped), end at the second "---" or "...".
// Scalars: null/~/empty, true/false, ints (decimal, 0x, 0o), floats, .inf/-.inf/.nan, otherwise strings.
// Not supported: tags (!!str etc. are parsed and ignored), complex "? key" keys, multiple documents (only the
// first is read), multi-line plain scalars, tab indentation, yes/no/on/off booleans (YAML 1.2: they are strings;
// stringify quotes them so other parsers agree).
// Numbers are doubles: integers above 2^53 lose precision.
// Not compiled/run yet. Written to avoid `return` inside loop bodies (compiler quirk noted in Canvas.os).
module Json = import "standard/1/Json.os";
module Str = import "standard/1/String.os";
module L = import "standard/1/List.os";
module CMem = import "standard/1/c/mem.os";
module CStr = import "standard/1/c/string.os";
module CIO = import "standard/1/c/stdio_extra.os";
module Text = import "standard/1/Text.os";

let K_NULL: int = 0;
let K_BOOL: int = 1;
let K_NUM: int = 2;
let K_STR: int = 3;
let K_ARR: int = 4;
let K_OBJ: int = 5;

struct ParseError { line: int = 0; col: int = 0; msg: char* = nullptr; }

// One physical line. start..end is the content (after the indent, without CR/LF). A "virtual" line is a
// line whose start/indent were moved right to hide a leading "- " or "&anchor ".
struct Line {
    raw: u64 = 0;
    start: u64 = 0;
    end: u64 = 0;
    indent: int = 0;
    blank: bool = false;      // empty or comment-only
}

struct Anchor { nameStart: u64 = 0; nameLen: u64 = 0; v: Json.JsonValue* = nullptr; }

struct Y {
    src: char* = nullptr;
    n: u64 = 0;
    lines: L.List<Line>;
    anchors: L.List<Anchor>;
    idx: u64 = 0;             // current line (block context)
    pos: u64 = 0;             // current offset (inline / flow context)
    aStart: u64 = 0;          // result of scanProps
    aLen: u64 = 0;
    err: bool = false;
    msg: char* = nullptr;
    errPos: u64 = 0;
}

// ---------------------------------------------------------------------------------------------
// basics
// ---------------------------------------------------------------------------------------------
function fail(p: Y*, msg: char*, pos: u64) => void {
    if (!p.err) { p.err = true; p.msg = msg; p.errPos = pos; }
}

// bounds-safe character read: 0 past the end
function at(p: Y*, i: u64) => int {
    if (i >= p.n) return 0;
    return p.src[i] as int;
}

function isBlankCh(c: int) => bool { return c == 32 || c == 9; }
function isDigit(c: int) => bool { return c >= 48 && c <= 57; }

function hexVal(c: int) => int {
    if (c >= 48 && c <= 57) return c - 48;
    if (c >= 97 && c <= 102) return c - 87;
    if (c >= 65 && c <= 70) return c - 55;
    return 0 - 1;
}

function freeNode(v: Json.JsonValue*) => void {
    if (v != nullptr) {
        v.free();
        CMem.free(v as void*);
    }
}

function locate(src: char*, pos: u64, err: ParseError*) => void {
    let line: int = 1;
    let col: int = 1;
    let i: u64 = 0;
    while (i < pos) {
        if ((src[i] as int) == 10) { line = line + 1; col = 1; }
        else col = col + 1;
        i = i + 1;
    }
    err.line = line;
    err.col = col;
}

function eqLower(s: char*, lit: char*) => bool {
    let i: u64 = 0;
    let ok: bool = true;
    let go: bool = true;
    while (go) {
        let a: int = s[i] as int;
        let b: int = lit[i] as int;
        if (a >= 65 && a <= 90) a = a + 32;
        if (a != b) { ok = false; go = false; }
        else if (a == 0) go = false;
        else i = i + 1;
    }
    return ok;
}

// ---------------------------------------------------------------------------------------------
// line table
// ---------------------------------------------------------------------------------------------
function isMarker(p: Y*, start: u64, end: u64, c: int) => bool {
    if (end - start < 3) return false;
    if (at(p, start) != c || at(p, start + 1) != c || at(p, start + 2) != c) return false;
    return (end - start == 3) || at(p, start + 3) == 32;
}

function buildLines(p: Y*) => void {
    let i: u64 = 0;
    let seen: bool = false;
    let stop: bool = false;
    while (!stop && i < p.n) {
        let raw: u64 = i;
        let indent: int = 0;
        while (at(p, i) == 32) { i = i + 1; indent = indent + 1; }
        let start: u64 = i;
        let j: u64 = i;
        while (j < p.n && at(p, j) != 10) j = j + 1;
        let end: u64 = j;
        if (end > start && at(p, end - 1) == 13) end = end - 1;
        let next: u64 = j;
        if (j < p.n) next = j + 1;
        let blank: bool = (start == end) || at(p, start) == 35;
        if (!blank) {
            if (indent == 0 && isMarker(p, start, end, 45)) {
                if (!seen) {
                    seen = true;
                    let rest: u64 = start + 3;
                    while (at(p, rest) == 32) rest = rest + 1;
                    if (rest >= end || at(p, rest) == 35) blank = true;
                    else start = rest;
                } else {
                    stop = true;
                }
            }
            else if (indent == 0 && isMarker(p, start, end, 46)) stop = true;
            else if (indent == 0 && !seen && at(p, start) == 37) blank = true;
            else seen = true;
        }
        if (!stop) {
            let ln: Line;
            ln.raw = raw;
            ln.start = start;
            ln.end = end;
            ln.indent = indent;
            ln.blank = blank;
            p.lines.push(ln);
        }
        i = next;
    }
}

function skipBlankLines(p: Y*) => void {
    let go: bool = true;
    while (go && p.idx < p.lines.len) {
        let ln: Line* = p.lines.ptr(p.idx);
        if (ln.blank) p.idx = p.idx + 1;
        else go = false;
    }
}

// Moves idx to the line after the one containing offset pos.
function advanceTo(p: Y*, pos: u64) => void {
    let i: u64 = p.idx;
    let go: bool = true;
    while (go && i < p.lines.len) {
        let ln: Line* = p.lines.ptr(i);
        if (ln.end < pos) i = i + 1;
        else go = false;
    }
    p.idx = i + 1;
}

function isSeqEntry(p: Y*, ln: Line*) => bool {
    if (at(p, ln.start) != 45) return false;
    if (ln.start + 1 >= ln.end) return true;
    return isBlankCh(at(p, ln.start + 1));
}

// Index just after the closing quote of the quoted scalar starting at i, or -1 if it does not close before end.
function skipQuoted(p: Y*, i: u64, end: u64) => i64 {
    let q: int = at(p, i);
    let j: u64 = i + 1;
    let res: i64 = -1;
    let go: bool = true;
    while (go && j < end) {
        let c: int = at(p, j);
        if (q == 34 && c == 92) j = j + 2;
        else if (c == q) {
            if (q == 39 && at(p, j + 1) == 39) j = j + 2;
            else { res = (j + 1) as i64; go = false; }
        }
        else j = j + 1;
    }
    return res;
}

// Offset of the ':' that ends a mapping key on this line, or -1 when the line is not "key: ...".
function findMapColon(p: Y*, from: u64, end: u64) => i64 {
    if (from >= end) return -1;
    let c0: int = at(p, from);
    if (c0 == 91 || c0 == 123) return -1;
    let res: i64 = -1;
    if (c0 == 34 || c0 == 39) {
        let q: i64 = skipQuoted(p, from, end);
        if (q >= 0) {
            let k: u64 = q as u64;
            while (at(p, k) == 32) k = k + 1;
            if (k < end && at(p, k) == 58 && (k + 1 >= end || isBlankCh(at(p, k + 1)))) res = k as i64;
        }
        return res;
    }
    let i: u64 = from;
    let go: bool = true;
    while (go && i < end) {
        let c: int = at(p, i);
        if (c == 58 && (i + 1 >= end || isBlankCh(at(p, i + 1)))) { res = i as i64; go = false; }
        else if (c == 35 && i > from && isBlankCh(at(p, i - 1))) go = false;
        else i = i + 1;
    }
    return res;
}

// ---------------------------------------------------------------------------------------------
// anchors, aliases, merge keys
// ---------------------------------------------------------------------------------------------
function cloneValue(v: Json.JsonValue*) => Json.JsonValue* {
    let c = Json.alloc(v.kind);
    c.b = v.b;
    c.n = v.n;
    v.s.clone(&c.s);
    let i: u64 = 0;
    while (i < v.items.len) {
        c.items.push(cloneValue(v.items.data[i]));
        i = i + 1;
    }
    let j: u64 = 0;
    while (j < v.keys.len) {
        let k: Str.String;
        v.keys.data[j].clone(&k);
        c.keys.push(k);
        j = j + 1;
    }
    return c;
}

function addAnchor(p: Y*, s: u64, len: u64, v: Json.JsonValue*) => void {
    let a: Anchor;
    a.nameStart = s;
    a.nameLen = len;
    a.v = v;
    p.anchors.push(a);
}

// The most recent anchor with this name, or nullptr.
function findAnchor(p: Y*, s: u64, len: u64) => Json.JsonValue* {
    let found: Json.JsonValue* = nullptr;
    let i: u64 = 0;
    while (i < p.anchors.len) {
        let a: Anchor* = p.anchors.ptr(i);
        if (a.nameLen == len) {
            if (CMem.memcmp((&p.src[a.nameStart]) as void*, (&p.src[s]) as void*, len) == 0) found = a.v;
        }
        i = i + 1;
    }
    return found;
}

// Copies entries of `from` that `obj` does not have yet (an object, or an array of objects).
function mergeInto(obj: Json.JsonValue*, from: Json.JsonValue*) => void {
    if (from.kind == K_OBJ) {
        let i: u64 = 0;
        while (i < from.items.len) {
            if (obj.get(from.keys.data[i].cstr()) == nullptr) {
                obj.set(from.keys.data[i].cstr(), cloneValue(from.items.data[i]));
            }
            i = i + 1;
        }
    } else if (from.kind == K_ARR) {
        let j: u64 = 0;
        while (j < from.items.len) {
            let it = from.items.data[j];
            if (it.kind == K_OBJ) mergeInto(obj, it);
            j = j + 1;
        }
    }
}

// ---------------------------------------------------------------------------------------------
// scalars
// ---------------------------------------------------------------------------------------------
function numericLike(s: char*) => bool {
    let i: u64 = 0;
    let c0: int = s[0] as int;
    if (c0 == 43 || c0 == 45) i = 1;
    let c: int = s[i] as int;
    if (isDigit(c)) return true;
    if (c == 46 && isDigit(s[i + 1] as int)) return true;
    return false;
}

function resolvePlain(s: Str.String*) => Json.JsonValue* {
    let t: char* = s.cstr();
    if (s.len == 0 || eqLower(t, "~") || eqLower(t, "null")) return Json.newNull();
    if (eqLower(t, "true")) return Json.newBool(true);
    if (eqLower(t, "false")) return Json.newBool(false);

    let zero: double = 0.0;
    if (eqLower(t, ".inf") || eqLower(t, "+.inf")) return Json.newNum(1.0 / zero);
    if (eqLower(t, "-.inf")) return Json.newNum(0.0 - (1.0 / zero));
    if (eqLower(t, ".nan")) return Json.newNum(zero / zero);

    if (numericLike(t)) {
        let end: char* = nullptr;
        if (s.len > 2 && (s.data[0] as int) == 48 && ((s.data[1] as int) == 120 || (s.data[1] as int) == 111)) {
            let base: int = 16;
            if ((s.data[1] as int) == 111) base = 8;
            let iv: u64 = CStr.strtoull(&s.data[2], &end, base);
            if ((*end as int) == 0) return Json.newNum(iv as double);
        } else {
            let dv: double = CStr.strtod(t, &end);
            if (end != t && (*end as int) == 0) return Json.newNum(dv);
        }
    }
    return Json.newStr(t);
}

function isNameEnd(c: int, inFlow: bool) => bool {
    if (c == 0 || c == 10 || c == 13 || c == 32 || c == 9) return true;
    if (inFlow && (c == 44 || c == 93 || c == 125)) return true;
    return false;
}

// Consumes leading &anchor / !tag tokens at pos. Returns the offset of what follows (spaces skipped).
// The last anchor seen is left in p.aStart / p.aLen (aLen == 0 when none).
function scanProps(p: Y*, posIn: u64) => u64 {
    p.aStart = 0;
    p.aLen = 0;
    let pos: u64 = posIn;
    let more: bool = true;
    while (more) {
        let c: int = at(p, pos);
        if (c == 38 || c == 33) {
            let s: u64 = pos + 1;
            let e: u64 = s;
            while (!isNameEnd(at(p, e), false)) e = e + 1;
            if (c == 38) { p.aStart = s; p.aLen = e - s; }
            pos = e;
            while (isBlankCh(at(p, pos))) pos = pos + 1;
        } else {
            more = false;
        }
    }
    return pos;
}

// After a raw line break inside a quoted scalar: trailing blanks go, the break folds to one space, and
// each extra blank line becomes a newline. Leading blanks of the next line are skipped.
function foldBreak(p: Y*, out: Str.String*) => void {
    while (out.len > 0 && isBlankCh(out.data[out.len - 1] as int)) out.truncate(out.len - 1);
    let nl: int = 0;
    let go: bool = true;
    while (go) {
        let c: int = at(p, p.pos);
        if (c == 10) { nl = nl + 1; p.pos = p.pos + 1; }
        else if (c == 13 || c == 32 || c == 9) p.pos = p.pos + 1;
        else go = false;
        if (p.pos >= p.n) go = false;
    }
    if (nl <= 1) out.appendChar(32 as char);
    else {
        let k: int = 1;
        while (k < nl) { out.appendChar(10 as char); k = k + 1; }
    }
}

function parseDoubleQuoted(p: Y*) => Json.JsonValue* {
    let v = Json.alloc(K_STR);
    let out: Str.String* = &v.s;
    p.pos = p.pos + 1;
    let ok: bool = true;
    let done: bool = false;
    while (ok && !done) {
        let c: int = at(p, p.pos);
        if (p.pos >= p.n) { fail(p, "unterminated double-quoted string", p.pos); ok = false; }
        else if (c == 34) { p.pos = p.pos + 1; done = true; }
        else if (c == 10 || c == 13) foldBreak(p, out);
        else if (c == 92) {
            p.pos = p.pos + 1;
            let e: int = at(p, p.pos);
            if (e == 110) { out.appendChar(10 as char); p.pos = p.pos + 1; }
            else if (e == 116) { out.appendChar(9 as char); p.pos = p.pos + 1; }
            else if (e == 114) { out.appendChar(13 as char); p.pos = p.pos + 1; }
            else if (e == 48) { p.pos = p.pos + 1; }
            else if (e == 97) { out.appendChar(7 as char); p.pos = p.pos + 1; }
            else if (e == 98) { out.appendChar(8 as char); p.pos = p.pos + 1; }
            else if (e == 101) { out.appendChar(27 as char); p.pos = p.pos + 1; }
            else if (e == 102) { out.appendChar(12 as char); p.pos = p.pos + 1; }
            else if (e == 118) { out.appendChar(11 as char); p.pos = p.pos + 1; }
            else if (e == 32 || e == 34 || e == 47 || e == 92) { out.appendChar(e as char); p.pos = p.pos + 1; }
            else if (e == 78) { Text.appendUtf8(133 as u32, out); p.pos = p.pos + 1; }
            else if (e == 95) { Text.appendUtf8(160 as u32, out); p.pos = p.pos + 1; }
            else if (e == 120 || e == 117 || e == 85) {
                let nd: u64 = 2;
                if (e == 117) nd = 4;
                if (e == 85) nd = 8;
                p.pos = p.pos + 1;
                let cp: u64 = 0;
                let k: u64 = 0;
                while (k < nd && ok) {
                    let h: int = hexVal(at(p, p.pos));
                    if (h < 0) { fail(p, "bad hex escape", p.pos); ok = false; }
                    else { cp = cp * 16 + (h as u64); p.pos = p.pos + 1; }
                    k = k + 1;
                }
                if (ok) Text.appendUtf8(cp as u32, out);
            }
            else if (e == 10 || e == 13) {
                // backslash at end of line: join with the next line, dropping its indentation
                while (at(p, p.pos) == 13 || at(p, p.pos) == 10) p.pos = p.pos + 1;
                while (isBlankCh(at(p, p.pos))) p.pos = p.pos + 1;
            }
            else { fail(p, "invalid escape sequence", p.pos); ok = false; }
        }
        else { out.appendChar(c as char); p.pos = p.pos + 1; }
    }
    if (!ok) { freeNode(v); return nullptr; }
    return v;
}

function parseSingleQuoted(p: Y*) => Json.JsonValue* {
    let v = Json.alloc(K_STR);
    let out: Str.String* = &v.s;
    p.pos = p.pos + 1;
    let ok: bool = true;
    let done: bool = false;
    while (ok && !done) {
        let c: int = at(p, p.pos);
        if (p.pos >= p.n) { fail(p, "unterminated single-quoted string", p.pos); ok = false; }
        else if (c == 39) {
            if (at(p, p.pos + 1) == 39) { out.appendChar(39 as char); p.pos = p.pos + 2; }
            else { p.pos = p.pos + 1; done = true; }
        }
        else if (c == 10 || c == 13) foldBreak(p, out);
        else { out.appendChar(c as char); p.pos = p.pos + 1; }
    }
    if (!ok) { freeNode(v); return nullptr; }
    return v;
}

// Plain scalar at p.pos, to the end of the line (or the next flow delimiter), without a trailing comment.
function parsePlain(p: Y*, inFlow: bool) => Json.JsonValue* {
    let s: u64 = p.pos;
    let i: u64 = s;
    let go: bool = true;
    while (go) {
        let c: int = at(p, i);
        let nx: int = at(p, i + 1);
        if (c == 0 || c == 10 || c == 13) go = false;
        else if (c == 35 && i > s && isBlankCh(at(p, i - 1))) go = false;
        else if (inFlow && (c == 44 || c == 93 || c == 125)) go = false;
        else if (inFlow && c == 58 && (isBlankCh(nx) || nx == 0 || nx == 10 || nx == 13 || nx == 44 || nx == 93 || nx == 125)) go = false;
        else i = i + 1;
    }
    let e: u64 = i;
    while (e > s && isBlankCh(at(p, e - 1))) e = e - 1;
    p.pos = i;
    let str: Str.String;
    if (e > s) str.appendN(&p.src[s], e - s);
    let v = resolvePlain(&str);
    str.free();
    return v;
}

// ---------------------------------------------------------------------------------------------
// inline and flow nodes (operate on p.pos)
// ---------------------------------------------------------------------------------------------
function skipFlowWs(p: Y*) => void {
    let more: bool = true;
    while (more) {
        let c: int = at(p, p.pos);
        if (p.pos >= p.n) more = false;
        else if (c == 32 || c == 9 || c == 10 || c == 13) p.pos = p.pos + 1;
        else if (c == 35) {
            while (p.pos < p.n && at(p, p.pos) != 10) p.pos = p.pos + 1;
        }
        else more = false;
    }
}

function parseFlowSeq(p: Y*) => Json.JsonValue* {
    let a = Json.newArray();
    p.pos = p.pos + 1;
    let ok: bool = true;
    let done: bool = false;
    while (ok && !done) {
        skipFlowWs(p);
        let c: int = at(p, p.pos);
        if (p.pos >= p.n) { fail(p, "unterminated flow sequence", p.pos); ok = false; }
        else if (c == 93) { p.pos = p.pos + 1; done = true; }
        else {
            let v = parseNode(p, true);
            if (v == nullptr) ok = false;
            else {
                a.push(v);
                skipFlowWs(p);
                let d: int = at(p, p.pos);
                if (d == 44) p.pos = p.pos + 1;
                else if (d == 93) { p.pos = p.pos + 1; done = true; }
                else { fail(p, "expected ',' or ']' in flow sequence", p.pos); ok = false; }
            }
        }
    }
    if (!ok) { freeNode(a); return nullptr; }
    return a;
}

function parseFlowMap(p: Y*) => Json.JsonValue* {
    let o = Json.newObject();
    p.pos = p.pos + 1;
    let ok: bool = true;
    let done: bool = false;
    while (ok && !done) {
        skipFlowWs(p);
        let c: int = at(p, p.pos);
        if (p.pos >= p.n) { fail(p, "unterminated flow mapping", p.pos); ok = false; }
        else if (c == 125) { p.pos = p.pos + 1; done = true; }
        else {
            let ks: u64 = p.pos;
            let k = parseNode(p, true);
            if (k == nullptr) ok = false;
            else {
                let ke: u64 = p.pos;
                while (ke > ks && isBlankCh(at(p, ke - 1))) ke = ke - 1;
                let key: Str.String;
                if (k.kind == K_STR) k.s.clone(&key);
                else if (ke > ks) key.appendN(&p.src[ks], ke - ks);
                freeNode(k);

                skipFlowWs(p);
                let v: Json.JsonValue* = nullptr;
                if (at(p, p.pos) == 58) {
                    p.pos = p.pos + 1;
                    skipFlowWs(p);
                    let d: int = at(p, p.pos);
                    if (d == 44 || d == 125) v = Json.newNull();
                    else v = parseNode(p, true);
                } else {
                    v = Json.newNull();
                }
                if (v == nullptr) ok = false;
                else if (o.get(key.cstr()) != nullptr) { fail(p, "duplicate key", ks); freeNode(v); ok = false; }
                else o.set(key.cstr(), v);
                key.free();

                if (ok) {
                    skipFlowWs(p);
                    let d2: int = at(p, p.pos);
                    if (d2 == 44) p.pos = p.pos + 1;
                    else if (d2 == 125) { p.pos = p.pos + 1; done = true; }
                    else { fail(p, "expected ',' or '}' in flow mapping", p.pos); ok = false; }
                }
            }
        }
    }
    if (!ok) { freeNode(o); return nullptr; }
    return o;
}

// One value at p.pos: alias, anchored value, quoted/flow/plain scalar. Leaves p.pos after it.
function parseNode(p: Y*, inFlow: bool) => Json.JsonValue* {
    let c: int = at(p, p.pos);
    if (c == 42) {
        p.pos = p.pos + 1;
        let s: u64 = p.pos;
        while (!isNameEnd(at(p, p.pos), inFlow)) p.pos = p.pos + 1;
        let a = findAnchor(p, s, p.pos - s);
        if (a == nullptr) { fail(p, "unknown alias", s); return nullptr; }
        return cloneValue(a);
    }
    if (c == 38 || c == 33) {
        let after: u64 = scanProps(p, p.pos);
        let aS: u64 = p.aStart;
        let aL: u64 = p.aLen;
        p.pos = after;
        let d: int = at(p, p.pos);
        let v: Json.JsonValue* = nullptr;
        if (d == 0 || d == 10 || d == 13 || d == 35) v = Json.newNull();
        else v = parseNode(p, inFlow);
        if (v != nullptr && aL > 0) addAnchor(p, aS, aL, v);
        return v;
    }
    if (c == 34) return parseDoubleQuoted(p);
    if (c == 39) return parseSingleQuoted(p);
    if (c == 91) return parseFlowSeq(p);
    if (c == 123) return parseFlowMap(p);
    return parsePlain(p, inFlow);
}

// ---------------------------------------------------------------------------------------------
// block scalars: | and >
// ---------------------------------------------------------------------------------------------
// hp is the offset of '|' / '>'. p.idx must already be the first line after the header line.
function parseBlockScalar(p: Y*, parentIndent: int, hp: u64) => Json.JsonValue* {
    let folded: bool = at(p, hp) == 62;
    let chomp: int = 0;              // 0 clip, 1 strip, 2 keep
    let explicitIndent: int = 0;
    let i: u64 = hp + 1;
    let more: bool = true;
    while (more) {
        let c: int = at(p, i);
        if (c == 45) { chomp = 1; i = i + 1; }
        else if (c == 43) { chomp = 2; i = i + 1; }
        else if (c >= 49 && c <= 57) { explicitIndent = c - 48; i = i + 1; }
        else more = false;
    }

    let blockIndent: int = 0 - 1;
    if (explicitIndent > 0) {
        blockIndent = parentIndent + explicitIndent;
    } else {
        let j: u64 = p.idx;
        let scanning: bool = true;
        while (scanning && j < p.lines.len) {
            let probe: Line* = p.lines.ptr(j);
            if (probe.start == probe.end) j = j + 1;
            else scanning = false;
        }
        if (j < p.lines.len) {
            let first: Line* = p.lines.ptr(j);
            if (first.indent > parentIndent) blockIndent = first.indent;
        }
    }

    let v = Json.alloc(K_STR);
    let out: Str.String* = &v.s;
    let hasContent: bool = false;
    let pending: int = 0;
    let prevMore: bool = false;
    if (blockIndent >= 0) {
        let go: bool = true;
        while (go && p.idx < p.lines.len) {
            let ln: Line* = p.lines.ptr(p.idx);
            if (ln.start == ln.end) {
                pending = pending + 1;
                p.idx = p.idx + 1;
            } else if (ln.indent >= blockIndent) {
                let ts: u64 = ln.raw + (blockIndent as u64);
                let moreIndented: bool = ln.indent > blockIndent;
                if (hasContent) {
                    if (!folded) {
                        out.appendChar(10 as char);
                        let k: int = 0;
                        while (k < pending) { out.appendChar(10 as char); k = k + 1; }
                    } else if (pending > 0) {
                        let k2: int = 0;
                        while (k2 < pending) { out.appendChar(10 as char); k2 = k2 + 1; }
                    } else if (moreIndented || prevMore) {
                        out.appendChar(10 as char);
                    } else {
                        out.appendChar(32 as char);
                    }
                } else {
                    let k3: int = 0;
                    while (k3 < pending) { out.appendChar(10 as char); k3 = k3 + 1; }
                }
                pending = 0;
                out.appendN(&p.src[ts], ln.end - ts);
                hasContent = true;
                prevMore = moreIndented;
                p.idx = p.idx + 1;
            } else {
                go = false;
            }
        }
    }
    if (chomp != 1 && hasContent) out.appendChar(10 as char);
    if (chomp == 2) {
        let k4: int = 0;
        while (k4 < pending) { out.appendChar(10 as char); k4 = k4 + 1; }
    }
    return v;
}

// ---------------------------------------------------------------------------------------------
// block structure
// ---------------------------------------------------------------------------------------------
// After a "key:" or "-" with nothing on the line: the value is the deeper block that follows, or null.
// A sequence at the same indent as its parent key is allowed (allowSeq).
function parseNested(p: Y*, parentIndent: int, allowSeq: bool) => Json.JsonValue* {
    skipBlankLines(p);
    if (p.idx >= p.lines.len) return Json.newNull();
    let ln: Line* = p.lines.ptr(p.idx);
    if (ln.indent > parentIndent) return parseBlock(p, parentIndent);
    if (allowSeq && ln.indent == parentIndent && isSeqEntry(p, ln)) return parseSequence(p, parentIndent);
    return Json.newNull();
}

function parseBlock(p: Y*, parentIndent: int) => Json.JsonValue* {
    skipBlankLines(p);
    if (p.idx >= p.lines.len) return Json.newNull();
    let ln: Line* = p.lines.ptr(p.idx);
    let c0: int = at(p, ln.start);

    // &anchor / !tag in front of the node
    if (c0 == 38 || c0 == 33) {
        let after: u64 = scanProps(p, ln.start);
        let aS: u64 = p.aStart;
        let aL: u64 = p.aLen;
        let v: Json.JsonValue* = nullptr;
        if (after >= ln.end || at(p, after) == 35) {
            p.idx = p.idx + 1;
            v = parseNested(p, parentIndent, false);
        } else {
            ln.indent = ln.indent + ((after - ln.start) as int);
            ln.start = after;
            v = parseBlock(p, parentIndent);
        }
        if (v != nullptr && aL > 0) addAnchor(p, aS, aL, v);
        return v;
    }

    if (isSeqEntry(p, ln)) return parseSequence(p, ln.indent);

    if (c0 == 124 || c0 == 62) {
        p.idx = p.idx + 1;
        return parseBlockScalar(p, parentIndent, ln.start);
    }

    if (findMapColon(p, ln.start, ln.end) >= 0) return parseMapping(p, ln.indent);

    // a scalar, alias or flow collection on its own line(s)
    p.pos = ln.start;
    let v2 = parseNode(p, false);
    if (v2 != nullptr) advanceTo(p, p.pos);
    return v2;
}

function parseSequence(p: Y*, indent: int) => Json.JsonValue* {
    let arr = Json.newArray();
    let ok: bool = true;
    let go: bool = true;
    while (ok && go) {
        skipBlankLines(p);
        if (p.idx >= p.lines.len) go = false;
        else {
            let ln: Line* = p.lines.ptr(p.idx);
            if (ln.indent != indent || !isSeqEntry(p, ln)) go = false;
            else {
                let pos: u64 = ln.start + 1;
                while (at(p, pos) == 32) pos = pos + 1;
                let v: Json.JsonValue* = nullptr;
                if (pos >= ln.end || at(p, pos) == 35) {
                    p.idx = p.idx + 1;
                    v = parseNested(p, indent, false);
                } else {
                    // hide the "- " so "- key: v" / "- - x" / "- |" read as a line of their own
                    ln.indent = indent + ((pos - ln.start) as int);
                    ln.start = pos;
                    v = parseBlock(p, indent);
                }
                if (v == nullptr) ok = false;
                else arr.push(v);
            }
        }
    }
    if (!ok) { freeNode(arr); return nullptr; }
    return arr;
}

// The value after "key:" (restIn is the first non-blank offset after the colon).
function parseMapValue(p: Y*, ln: Line*, restIn: u64, indent: int) => Json.JsonValue* {
    let pos: u64 = scanProps(p, restIn);
    let aS: u64 = p.aStart;
    let aL: u64 = p.aLen;
    let c0: int = at(p, pos);
    let v: Json.JsonValue* = nullptr;
    if (pos >= ln.end || c0 == 35) {
        p.idx = p.idx + 1;
        v = parseNested(p, indent, true);
    } else if (c0 == 124 || c0 == 62) {
        p.idx = p.idx + 1;
        v = parseBlockScalar(p, indent, pos);
    } else {
        p.pos = pos;
        v = parseNode(p, false);
        if (v != nullptr) advanceTo(p, p.pos);
    }
    if (v != nullptr && aL > 0) addAnchor(p, aS, aL, v);
    return v;
}

function parseMapping(p: Y*, indent: int) => Json.JsonValue* {
    let obj = Json.newObject();
    let merges: L.List<Json.JsonValue*>;
    let ok: bool = true;
    let go: bool = true;
    while (ok && go) {
        skipBlankLines(p);
        if (p.idx >= p.lines.len) go = false;
        else {
            let ln: Line* = p.lines.ptr(p.idx);
            if (ln.indent < indent) go = false;
            else if (ln.indent > indent) { fail(p, "unexpected indentation", ln.start); ok = false; }
            else {
                let colon: i64 = findMapColon(p, ln.start, ln.end);
                if (colon < 0) { fail(p, "expected 'key: value'", ln.start); ok = false; }
                else {
                    let key: Str.String;
                    let kc: int = at(p, ln.start);
                    if (kc == 34 || kc == 39) {
                        p.pos = ln.start;
                        let kv: Json.JsonValue* = nullptr;
                        if (kc == 34) kv = parseDoubleQuoted(p);
                        else kv = parseSingleQuoted(p);
                        if (kv == nullptr) ok = false;
                        else { kv.s.clone(&key); freeNode(kv); }
                    } else {
                        let ke: u64 = colon as u64;
                        while (ke > ln.start && isBlankCh(at(p, ke - 1))) ke = ke - 1;
                        key.appendN(&p.src[ln.start], ke - ln.start);
                    }
                    if (ok) {
                        let rest: u64 = (colon as u64) + 1;
                        while (isBlankCh(at(p, rest))) rest = rest + 1;
                        let keyPos: u64 = ln.start;
                        let v = parseMapValue(p, ln, rest, indent);
                        if (v == nullptr) ok = false;
                        else if (key.equalsC("<<") && (v.kind == K_OBJ || v.kind == K_ARR)) merges.push(v);
                        else if (obj.get(key.cstr()) != nullptr) { fail(p, "duplicate key", keyPos); freeNode(v); ok = false; }
                        else obj.set(key.cstr(), v);
                    }
                    key.free();
                }
            }
        }
    }
    // merge keys never override explicit keys, whatever their order
    let m: u64 = 0;
    while (m < merges.len) {
        if (ok) mergeInto(obj, merges.data[m]);
        freeNode(merges.data[m]);
        m = m + 1;
    }
    merges.free();
    if (!ok) { freeNode(obj); return nullptr; }
    return obj;
}

// Parses YAML text (first document) into a JsonValue tree. Returns nullptr on a syntax error (err may be
// nullptr). An empty document gives a null value. Caller releases the tree with Yaml.freeValue(root).
function parse(text: char*, err: ParseError*) => Json.JsonValue* {
    let p: Y;
    p.src = text;
    let n: u64 = 0;
    while ((text[n] as int) != 0) n = n + 1;
    p.n = n;
    buildLines(&p);
    p.idx = 0;

    let v = parseBlock(&p, 0 - 1);
    if (v != nullptr) {
        skipBlankLines(&p);
        if (p.idx < p.lines.len) {
            let ln: Line* = p.lines.ptr(p.idx);
            fail(&p, "unexpected content (bad indentation?)", ln.start);
            freeNode(v);
            v = nullptr;
        }
    }
    p.lines.free();
    p.anchors.free();

    if (v == nullptr) {
        if (err != nullptr) {
            err.msg = p.msg;
            locate(text, p.errPos, err);
        }
        return nullptr;
    }
    if (err != nullptr) { err.msg = nullptr; err.line = 0; err.col = 0; }
    return v;
}

// Frees a tree returned by parse (or built with Json.new*).
function freeValue(v: Json.JsonValue*) => void { freeNode(v); }

// ---------------------------------------------------------------------------------------------
// emitter
// ---------------------------------------------------------------------------------------------
function needsQuote(s: char*) => bool {
    let c0: int = s[0] as int;
    if (c0 == 0) return true;
    // indicators and spaces that cannot start a plain scalar
    if (c0 == 32 || c0 == 45 || c0 == 63 || c0 == 58 || c0 == 44 || c0 == 91 || c0 == 93 || c0 == 123 || c0 == 125) return true;
    if (c0 == 35 || c0 == 38 || c0 == 42 || c0 == 33 || c0 == 124 || c0 == 62 || c0 == 39 || c0 == 34) return true;
    if (c0 == 37 || c0 == 64 || c0 == 96) return true;
    if (numericLike(s)) return true;
    if (eqLower(s, "~") || eqLower(s, "null") || eqLower(s, "true") || eqLower(s, "false")) return true;
    if (eqLower(s, "yes") || eqLower(s, "no") || eqLower(s, "on") || eqLower(s, "off")) return true;
    if (eqLower(s, "y") || eqLower(s, "n")) return true;
    if (eqLower(s, ".inf") || eqLower(s, "+.inf") || eqLower(s, "-.inf") || eqLower(s, ".nan")) return true;

    let bad: bool = false;
    let i: u64 = 0;
    let last: int = 0;
    let c: int = s[0] as int;
    while (c != 0 && !bad) {
        let nx: int = s[i + 1] as int;
        if (c < 32 || c == 127) bad = true;
        else if (c == 58 && (nx == 32 || nx == 0)) bad = true;
        else if (c == 35 && last == 32) bad = true;
        last = c;
        i = i + 1;
        c = s[i] as int;
    }
    if (last == 32) bad = true;
    return bad;
}

function writeStr(s: char*, out: Str.String*) => void {
    if (needsQuote(s)) { out.append("\""); Text.escape(s, out); out.append("\""); }
    else out.append(s);
}

function writeNum(n: double, out: Str.String*) => void {
    if (n != n) { out.append(".nan"); return; }
    if (n - n != 0.0) {
        if (n > 0.0) out.append(".inf"); else out.append("-.inf");
        return;
    }
    if (n < 9000000000000000.0 && n > 0.0 - 9000000000000000.0) {
        let iv: i64 = n as i64;
        if ((iv as double) == n) { out.appendInt(iv); return; }
    }
    let buf: [64]char;
    CIO.snprintf(&buf as char*, 64, "%.17g", n);
    out.append(&buf as char*);
}

function writeScalar(v: Json.JsonValue*, out: Str.String*) => void {
    if (v.kind == K_NULL) out.append("null");
    else if (v.kind == K_BOOL) { if (v.b) out.append("true"); else out.append("false"); }
    else if (v.kind == K_NUM) writeNum(v.n, out);
    else writeStr(v.s.cstr(), out);
}

function isContainer(v: Json.JsonValue*) => bool { return v.kind == K_ARR || v.kind == K_OBJ; }

// Writes "{}" or "[]" for an empty container.
function writeEmpty(v: Json.JsonValue*, out: Str.String*) => void {
    if (v.kind == K_OBJ) out.append("{}"); else out.append("[]");
}

// Mapping entries at `depth` (2 spaces per level). With inlineFirst the caller already wrote "- ".
function writeMap(v: Json.JsonValue*, out: Str.String*, depth: u64, inlineFirst: bool) => void {
    let i: u64 = 0;
    while (i < v.items.len) {
        if (!(i == 0 && inlineFirst)) Text.repeat("  ", depth, out);
        writeStr(v.keys.data[i].cstr(), out);
        out.append(":");
        let item = v.items.data[i];
        if (isContainer(item) && item.items.len == 0) {
            out.append(" ");
            writeEmpty(item, out);
            out.append("\n");
        } else if (item.kind == K_OBJ) {
            out.append("\n");
            writeMap(item, out, depth + 1, false);
        } else if (item.kind == K_ARR) {
            out.append("\n");
            writeSeq(item, out, depth + 1, false);
        } else {
            out.append(" ");
            writeScalar(item, out);
            out.append("\n");
        }
        i = i + 1;
    }
}

function writeSeq(v: Json.JsonValue*, out: Str.String*, depth: u64, inlineFirst: bool) => void {
    let i: u64 = 0;
    while (i < v.items.len) {
        if (!(i == 0 && inlineFirst)) Text.repeat("  ", depth, out);
        out.append("-");
        let item = v.items.data[i];
        if (isContainer(item) && item.items.len == 0) {
            out.append(" ");
            writeEmpty(item, out);
            out.append("\n");
        } else if (item.kind == K_OBJ) {
            out.append(" ");
            writeMap(item, out, depth + 1, true);
        } else if (item.kind == K_ARR) {
            out.append(" ");
            writeSeq(item, out, depth + 1, true);
        } else {
            out.append(" ");
            writeScalar(item, out);
            out.append("\n");
        }
        i = i + 1;
    }
}

// Appends the YAML form of `v` (block style, 2-space indent) to out. Always true for a non-null v.
function stringify(v: Json.JsonValue*, out: Str.String*) => bool {
    if (v == nullptr) return false;
    if (isContainer(v) && v.items.len == 0) { writeEmpty(v, out); out.append("\n"); }
    else if (v.kind == K_OBJ) writeMap(v, out, 0, false);
    else if (v.kind == K_ARR) writeSeq(v, out, 0, false);
    else { writeScalar(v, out); out.append("\n"); }
    return true;
}
