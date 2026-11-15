// standard/1/Toml.os
// Tier 4: TOML 1.0 parser/emitter over the Json.JsonValue DOM (same tree as Json.os / Yaml.os).
//
//   let err: Toml.ParseError;
//   let root = Toml.parse(text, &err);          // nullptr on error: err.line / err.col / err.msg
//   if (root == nullptr) { printf("%d:%d %s\n", err.line, err.col, err.msg); }
//   let port = root.get("server").get("port").asNum(8080.0);
//   Toml.freeValue(root);                        // frees children, then the root itself
//
//   let out: Str.String;
//   Toml.stringify(root, &out);                  // root must be an object; false otherwise
//   out.free();
//
// Supported: comments, bare/quoted/dotted keys, [tables], [[arrays of tables]], inline tables, arrays
// (multi-line, trailing commas, comments), basic/literal strings, multi-line """ and ''' strings,
// \uXXXX / \UXXXXXXXX escapes, integers (_, 0x, 0o, 0b), floats, inf/nan, booleans.
// Limits (by design, because the DOM has no such types):
//   - numbers are doubles, so integers above 2^53 lose precision;
//   - date/times are kept as STRINGS, and stringify writes them back as quoted strings;
//   - null has no TOML form: stringify skips null table values and writes null array items as "".
// Lenient where TOML is strict: a table re-opened by a second [header] is accepted and merged.
// Not compiled/run yet. Written to avoid `return` inside loop bodies (compiler quirk noted in Canvas.os).
module Json = import "standard/1/Json.os";
module Str = import "standard/1/String.os";
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

struct P { src: char* = nullptr; pos: u64 = 0; err: bool = false; msg: char* = nullptr; errPos: u64 = 0; }

// ---------------------------------------------------------------------------------------------
// lexing helpers
// ---------------------------------------------------------------------------------------------
function fail(p: P*, msg: char*) => void {
    if (!p.err) { p.err = true; p.msg = msg; p.errPos = p.pos; }
}

function ch(p: P*) => int { return p.src[p.pos] as int; }

// true when the next three characters are all `q`
function triple(p: P*, q: int) => bool {
    if ((p.src[p.pos] as int) != q) return false;
    if ((p.src[p.pos + 1] as int) != q) return false;
    return (p.src[p.pos + 2] as int) == q;
}

function startsWithLit(p: P*, lit: char*, n: u64) => bool {
    return CStr.strncmp((p.src as u64 + p.pos) as char*, lit, n) == 0;
}

function isDigit(c: int) => bool { return c >= 48 && c <= 57; }

function isBareKeyChar(c: int) => bool {
    return (c >= 65 && c <= 90) || (c >= 97 && c <= 122) || (c >= 48 && c <= 57) || c == 95 || c == 45;
}

function hexVal(c: int) => int {
    if (c >= 48 && c <= 57) return c - 48;
    if (c >= 97 && c <= 102) return c - 87;
    if (c >= 65 && c <= 70) return c - 55;
    return 0 - 1;
}

function skipSpaces(p: P*) => void {
    while (ch(p) == 32 || ch(p) == 9) p.pos = p.pos + 1;
}

function skipComment(p: P*) => void {
    if (ch(p) == 35) {
        while (ch(p) != 0 && ch(p) != 10) p.pos = p.pos + 1;
    }
}

// spaces, tabs, newlines and comments
function skipAll(p: P*) => void {
    let more: bool = true;
    while (more) {
        let c: int = ch(p);
        if (c == 32 || c == 9 || c == 10 || c == 13) p.pos = p.pos + 1;
        else if (c == 35) skipComment(p);
        else more = false;
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

// ---------------------------------------------------------------------------------------------
// strings
// ---------------------------------------------------------------------------------------------

// pos is just after the opening quote(s). Consumes the closing quote(s).
function parseBasicString(p: P*, out: Str.String*, multi: bool) => bool {
    let ok: bool = true;
    let done: bool = false;
    while (ok && !done) {
        let c: int = ch(p);
        if (c == 0) { fail(p, "unterminated string"); ok = false; }
        else if (c == 34) {
            if (!multi) { p.pos = p.pos + 1; done = true; }
            else if (triple(p, 34)) { p.pos = p.pos + 3; done = true; }
            else { out.appendChar(34 as char); p.pos = p.pos + 1; }
        }
        else if (c == 10 && !multi) { fail(p, "newline in single-line string"); ok = false; }
        else if (c == 92) {
            p.pos = p.pos + 1;
            let e: int = ch(p);
            if (e == 110) { out.appendChar(10 as char); p.pos = p.pos + 1; }
            else if (e == 116) { out.appendChar(9 as char); p.pos = p.pos + 1; }
            else if (e == 114) { out.appendChar(13 as char); p.pos = p.pos + 1; }
            else if (e == 98) { out.appendChar(8 as char); p.pos = p.pos + 1; }
            else if (e == 102) { out.appendChar(12 as char); p.pos = p.pos + 1; }
            else if (e == 101) { out.appendChar(27 as char); p.pos = p.pos + 1; }
            else if (e == 34) { out.appendChar(34 as char); p.pos = p.pos + 1; }
            else if (e == 92) { out.appendChar(92 as char); p.pos = p.pos + 1; }
            else if (e == 117 || e == 85) {
                let nd: u64 = 4;
                if (e == 85) nd = 8;
                p.pos = p.pos + 1;
                let cp: u64 = 0;
                let k: u64 = 0;
                while (k < nd && ok) {
                    let h: int = hexVal(ch(p));
                    if (h < 0) { fail(p, "bad \\u escape"); ok = false; }
                    else { cp = cp * 16 + (h as u64); p.pos = p.pos + 1; }
                    k = k + 1;
                }
                if (ok) Text.appendUtf8(cp as u32, out);
            }
            else if (multi && (e == 10 || e == 13 || e == 32 || e == 9)) {
                // line-ending backslash: drop it, the newline and the following whitespace
                while (ch(p) == 32 || ch(p) == 9 || ch(p) == 10 || ch(p) == 13) p.pos = p.pos + 1;
            }
            else { fail(p, "invalid escape sequence"); ok = false; }
        }
        else { out.appendChar(c as char); p.pos = p.pos + 1; }
    }
    return ok;
}

function parseLiteralString(p: P*, out: Str.String*, multi: bool) => bool {
    let ok: bool = true;
    let done: bool = false;
    while (ok && !done) {
        let c: int = ch(p);
        if (c == 0) { fail(p, "unterminated string"); ok = false; }
        else if (c == 39) {
            if (!multi) { p.pos = p.pos + 1; done = true; }
            else if (triple(p, 39)) { p.pos = p.pos + 3; done = true; }
            else { out.appendChar(39 as char); p.pos = p.pos + 1; }
        }
        else if (c == 10 && !multi) { fail(p, "newline in single-line string"); ok = false; }
        else { out.appendChar(c as char); p.pos = p.pos + 1; }
    }
    return ok;
}

// pos is at the opening quote (" or ').
function parseStringAt(p: P*, out: Str.String*) => bool {
    let q: int = ch(p);
    let multi: bool = triple(p, q);
    if (multi) {
        p.pos = p.pos + 3;
        // a newline right after the opening delimiter is trimmed
        if (ch(p) == 13 && ((p.src[p.pos + 1] as int) == 10)) p.pos = p.pos + 2;
        else if (ch(p) == 10) p.pos = p.pos + 1;
    } else {
        p.pos = p.pos + 1;
    }
    if (q == 34) return parseBasicString(p, out, multi);
    return parseLiteralString(p, out, multi);
}

// ---------------------------------------------------------------------------------------------
// keys and tables
// ---------------------------------------------------------------------------------------------

// One key segment (bare or quoted) into seg.
function parseKeySeg(p: P*, seg: Str.String*) => bool {
    seg.clear();
    let c: int = ch(p);
    if (c == 34 || c == 39) {
        p.pos = p.pos + 1;
        if (c == 34) return parseBasicString(p, seg, false);
        return parseLiteralString(p, seg, false);
    }
    let n: u64 = 0;
    while (isBareKeyChar(ch(p))) {
        seg.appendChar(ch(p) as char);
        p.pos = p.pos + 1;
        n = n + 1;
    }
    if (n == 0) { fail(p, "expected a key"); return false; }
    return true;
}

// The table called `key` under `parent`, created when missing. For an array of tables it is the last element.
function enterTable(p: P*, parent: Json.JsonValue*, key: char*) => Json.JsonValue* {
    let existing = parent.get(key);
    if (existing == nullptr) {
        let t = Json.newObject();
        parent.set(key, t);
        return t;
    }
    if (existing.kind == K_OBJ) return existing;
    if (existing.kind == K_ARR && existing.items.len > 0) {
        let last = existing.items.data[existing.items.len - 1];
        if (last.kind == K_OBJ) return last;
    }
    fail(p, "key is already defined and is not a table");
    return nullptr;
}

// Parses `a.b.c =` starting at pos. Walks/creates the intermediate tables below `table`, leaves the final key
// in seg, consumes the '=' and following spaces, and returns the table that will hold the final key.
// nullptr on error.
function parseKeyPath(p: P*, table: Json.JsonValue*, seg: Str.String*) => Json.JsonValue* {
    let t = table;
    let ok: bool = true;
    let done: bool = false;
    while (ok && !done) {
        ok = parseKeySeg(p, seg);
        if (ok) {
            skipSpaces(p);
            if (ch(p) == 46) {
                p.pos = p.pos + 1;
                skipSpaces(p);
                t = enterTable(p, t, seg.cstr());
                if (t == nullptr) ok = false;
            } else {
                done = true;
            }
        }
    }
    if (ok) {
        if (ch(p) != 61) { fail(p, "expected '=' after key"); ok = false; }
    }
    if (ok) {
        p.pos = p.pos + 1;
        skipSpaces(p);
        if (t.get(seg.cstr()) != nullptr) { fail(p, "duplicate key"); ok = false; }
    }
    if (!ok) return nullptr;
    return t;
}

// ---------------------------------------------------------------------------------------------
// values
// ---------------------------------------------------------------------------------------------
function isTokenEnd(c: int) => bool {
    return c == 0 || c == 10 || c == 13 || c == 32 || c == 9 || c == 44 || c == 93 || c == 125 || c == 35;
}

function scanToken(p: P*, tok: Str.String*) => void {
    while (!isTokenEnd(ch(p))) {
        tok.appendChar(ch(p) as char);
        p.pos = p.pos + 1;
    }
}

function tokAt(tok: Str.String*, i: u64) => int {
    if (i >= tok.len) return 0;
    return tok.data[i] as int;
}

function isDateTimeTok(tok: Str.String*) => bool {
    if (tok.len >= 10 && isDigit(tokAt(tok, 0)) && isDigit(tokAt(tok, 3)) && tokAt(tok, 4) == 45 && tokAt(tok, 7) == 45) return true;
    if (tok.len >= 8 && isDigit(tokAt(tok, 0)) && tokAt(tok, 2) == 58 && tokAt(tok, 5) == 58) return true;
    return false;
}

function freeNode(v: Json.JsonValue*) => void {
    if (v != nullptr) {
        v.free();
        CMem.free(v as void*);
    }
}

// Number, inf/nan or date-time token.
function parseScalarToken(p: P*) => Json.JsonValue* {
    let tok: Str.String;
    scanToken(p, &tok);
    // "1979-05-27 07:32:00Z": a single space may separate the date and the time
    if (tok.len == 10 && ch(p) == 32 && isDigit(p.src[p.pos + 1] as int) && tokAt(&tok, 4) == 45 && tokAt(&tok, 7) == 45) {
        tok.appendChar(32 as char);
        p.pos = p.pos + 1;
        scanToken(p, &tok);
    }
    if (tok.len == 0) {
        fail(p, "expected a value");
        tok.free();
        return nullptr;
    }
    if (isDateTimeTok(&tok)) {
        let dv = Json.newStr(tok.cstr());
        tok.free();
        return dv;
    }

    // numbers: drop '_' separators
    let num: Str.String;
    let i: u64 = 0;
    while (i < tok.len) {
        if ((tok.data[i] as int) != 95) num.appendChar(tok.data[i]);
        i = i + 1;
    }
    tok.free();

    let result: Json.JsonValue* = nullptr;
    let end: char* = nullptr;
    if (num.len > 2 && tokAt(&num, 0) == 48 && (tokAt(&num, 1) == 120 || tokAt(&num, 1) == 111 || tokAt(&num, 1) == 98)) {
        let base: int = 16;
        if (tokAt(&num, 1) == 111) base = 8;
        if (tokAt(&num, 1) == 98) base = 2;
        let iv: u64 = CStr.strtoull(&num.data[2], &end, base);
        if ((*end as int) == 0) result = Json.newNum(iv as double);
    } else {
        let dv: double = CStr.strtod(num.cstr(), &end);
        if (end != num.cstr() && (*end as int) == 0) result = Json.newNum(dv);
    }
    if (result == nullptr) fail(p, "invalid value");
    num.free();
    return result;
}

function parseValue(p: P*) => Json.JsonValue* {
    let c: int = ch(p);

    if (c == 34 || c == 39) {
        let sv = Json.alloc(K_STR);
        if (!parseStringAt(p, &sv.s)) { freeNode(sv); return nullptr; }
        return sv;
    }

    if (c == 91) {
        let a = Json.newArray();
        p.pos = p.pos + 1;
        let done: bool = false;
        let ok: bool = true;
        while (ok && !done) {
            skipAll(p);
            if (ch(p) == 93) { p.pos = p.pos + 1; done = true; }
            else {
                let v = parseValue(p);
                if (v == nullptr) ok = false;
                else {
                    a.push(v);
                    skipAll(p);
                    let d: int = ch(p);
                    if (d == 44) p.pos = p.pos + 1;
                    else if (d == 93) { p.pos = p.pos + 1; done = true; }
                    else { fail(p, "expected ',' or ']' in array"); ok = false; }
                }
            }
        }
        if (!ok) { freeNode(a); return nullptr; }
        return a;
    }

    if (c == 123) {
        let o = Json.newObject();
        p.pos = p.pos + 1;
        let done: bool = false;
        let ok: bool = true;
        while (ok && !done) {
            skipAll(p);
            if (ch(p) == 125) { p.pos = p.pos + 1; done = true; }
            else {
                let seg: Str.String;
                let t = parseKeyPath(p, o, &seg);
                if (t == nullptr) ok = false;
                else {
                    let v = parseValue(p);
                    if (v == nullptr) ok = false;
                    else t.set(seg.cstr(), v);
                }
                seg.free();
                if (ok) {
                    skipAll(p);
                    let d: int = ch(p);
                    if (d == 44) p.pos = p.pos + 1;
                    else if (d == 125) { p.pos = p.pos + 1; done = true; }
                    else { fail(p, "expected ',' or '}' in inline table"); ok = false; }
                }
            }
        }
        if (!ok) { freeNode(o); return nullptr; }
        return o;
    }

    if (startsWithLit(p, "true", 4)) { p.pos = p.pos + 4; return Json.newBool(true); }
    if (startsWithLit(p, "false", 5)) { p.pos = p.pos + 5; return Json.newBool(false); }
    return parseScalarToken(p);
}

function parseKeyValue(p: P*, table: Json.JsonValue*) => bool {
    let seg: Str.String;
    let t = parseKeyPath(p, table, &seg);
    let ok: bool = t != nullptr;
    if (ok) {
        let v = parseValue(p);
        if (v == nullptr) ok = false;
        else t.set(seg.cstr(), v);
    }
    seg.free();
    return ok;
}

// ---------------------------------------------------------------------------------------------
// document
// ---------------------------------------------------------------------------------------------

// pos is at '['. Returns the table that following key/values go into, or nullptr on error.
function parseHeader(p: P*, root: Json.JsonValue*) => Json.JsonValue* {
    let isArr: bool = false;
    p.pos = p.pos + 1;
    if (ch(p) == 91) { isArr = true; p.pos = p.pos + 1; }
    skipSpaces(p);

    let t = root;
    let seg: Str.String;
    let ok: bool = true;
    let done: bool = false;
    while (ok && !done) {
        ok = parseKeySeg(p, &seg);
        if (ok) {
            skipSpaces(p);
            if (ch(p) == 46) {
                p.pos = p.pos + 1;
                skipSpaces(p);
                t = enterTable(p, t, seg.cstr());
                if (t == nullptr) ok = false;
            } else {
                done = true;
            }
        }
    }
    if (ok) {
        if (ch(p) != 93) { fail(p, "expected ']' to close table header"); ok = false; }
        else {
            p.pos = p.pos + 1;
            if (isArr) {
                if (ch(p) != 93) { fail(p, "expected ']]' to close array-of-tables header"); ok = false; }
                else p.pos = p.pos + 1;
            }
        }
    }

    let result: Json.JsonValue* = nullptr;
    if (ok) {
        if (isArr) {
            let existing = t.get(seg.cstr());
            let arr: Json.JsonValue* = nullptr;
            if (existing == nullptr) { arr = Json.newArray(); t.set(seg.cstr(), arr); }
            else if (existing.kind == K_ARR) arr = existing;
            else { fail(p, "key is already defined and is not an array"); ok = false; }
            if (ok) {
                let nt = Json.newObject();
                arr.push(nt);
                result = nt;
            }
        } else {
            result = enterTable(p, t, seg.cstr());
            if (result == nullptr) ok = false;
        }
    }
    seg.free();
    if (!ok) return nullptr;
    return result;
}

// Parses TOML text into a JsonValue object tree. Returns nullptr on a syntax error (err may be nullptr).
// Caller releases the tree with Toml.freeValue(root).
function parse(text: char*, err: ParseError*) => Json.JsonValue* {
    let p: P;
    p.src = text;
    let root = Json.newObject();
    let cur = root;
    let ok: bool = true;
    let done: bool = false;
    while (ok && !done) {
        skipAll(&p);
        if (ch(&p) == 0) done = true;
        else {
            if (ch(&p) == 91) {
                let nt = parseHeader(&p, root);
                if (nt == nullptr) ok = false;
                else cur = nt;
            } else {
                ok = parseKeyValue(&p, cur);
            }
            if (ok) {
                skipSpaces(&p);
                skipComment(&p);
                let d: int = ch(&p);
                if (d != 0 && d != 10 && d != 13) { fail(&p, "expected end of line after value"); ok = false; }
            }
        }
    }
    if (!ok) {
        if (err != nullptr) {
            err.msg = p.msg;
            locate(text, p.errPos, err);
        }
        freeNode(root);
        return nullptr;
    }
    if (err != nullptr) { err.msg = nullptr; err.line = 0; err.col = 0; }
    return root;
}

// Frees a tree returned by parse (or built with Json.new*).
function freeValue(v: Json.JsonValue*) => void { freeNode(v); }

// ---------------------------------------------------------------------------------------------
// emitter
// ---------------------------------------------------------------------------------------------
function isBareKey(s: char*) => bool {
    let n: u64 = 0;
    let ok: bool = true;
    while ((s[n] as int) != 0 && ok) {
        if (!isBareKeyChar(s[n] as int)) ok = false;
        n = n + 1;
    }
    return ok && n > 0;
}

function writeKey(k: char*, out: Str.String*) => void {
    if (isBareKey(k)) out.append(k);
    else { out.append("\""); Text.escape(k, out); out.append("\""); }
}

function writeNum(n: double, out: Str.String*) => void {
    if (n != n) { out.append("nan"); return; }
    if (n - n != 0.0) {
        if (n > 0.0) out.append("inf"); else out.append("-inf");
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

// Every item is an object (and there is at least one): written as [[array.of.tables]].
function isTableArray(a: Json.JsonValue*) => bool {
    if (a.kind != K_ARR || a.items.len == 0) return false;
    let ok: bool = true;
    let i: u64 = 0;
    while (i < a.items.len && ok) {
        if (a.items.data[i].kind != K_OBJ) ok = false;
        i = i + 1;
    }
    return ok;
}

function writeInline(v: Json.JsonValue*, out: Str.String*) => void {
    if (v.kind == K_NULL) { out.append("\"\""); return; }
    if (v.kind == K_BOOL) { if (v.b) out.append("true"); else out.append("false"); return; }
    if (v.kind == K_NUM) { writeNum(v.n, out); return; }
    if (v.kind == K_STR) { out.append("\""); Text.escape(v.s.cstr(), out); out.append("\""); return; }
    if (v.kind == K_ARR) {
        out.append("[");
        let i: u64 = 0;
        while (i < v.items.len) {
            if (i > 0) out.append(", ");
            writeInline(v.items.data[i], out);
            i = i + 1;
        }
        out.append("]");
        return;
    }
    out.append("{");
    let first: bool = true;
    let j: u64 = 0;
    while (j < v.items.len) {
        if (v.items.data[j].kind != K_NULL) {
            if (first) out.append(" "); else out.append(", ");
            first = false;
            writeKey(v.keys.data[j].cstr(), out);
            out.append(" = ");
            writeInline(v.items.data[j], out);
        }
        j = j + 1;
    }
    if (!first) out.append(" ");
    out.append("}");
}

function writeTable(v: Json.JsonValue*, path: char*, out: Str.String*) => void {
    // pass 1: plain key/values
    let i: u64 = 0;
    while (i < v.items.len) {
        let item = v.items.data[i];
        let skip: bool = item.kind == K_OBJ || item.kind == K_NULL || isTableArray(item);
        if (!skip) {
            writeKey(v.keys.data[i].cstr(), out);
            out.append(" = ");
            writeInline(item, out);
            out.append("\n");
        }
        i = i + 1;
    }
    // pass 2: sub-tables, pass 3: arrays of tables
    let pass: int = 2;
    while (pass <= 3) {
        let j: u64 = 0;
        while (j < v.items.len) {
            let item = v.items.data[j];
            let want: bool = false;
            if (pass == 2 && item.kind == K_OBJ) want = true;
            if (pass == 3 && isTableArray(item)) want = true;
            if (want) {
                let child: Str.String;
                child.set(path);
                if ((path[0] as int) != 0) child.append(".");
                writeKey(v.keys.data[j].cstr(), &child);
                if (pass == 2) {
                    if (out.len > 0) out.append("\n");
                    out.append("[");
                    out.append(child.cstr());
                    out.append("]\n");
                    writeTable(item, child.cstr(), out);
                } else {
                    let k: u64 = 0;
                    while (k < item.items.len) {
                        if (out.len > 0) out.append("\n");
                        out.append("[[");
                        out.append(child.cstr());
                        out.append("]]\n");
                        writeTable(item.items.data[k], child.cstr(), out);
                        k = k + 1;
                    }
                }
                child.free();
            }
            j = j + 1;
        }
        pass = pass + 1;
    }
}

// Appends the TOML form of `v` to out. The root must be an object; returns false otherwise.
function stringify(v: Json.JsonValue*, out: Str.String*) => bool {
    if (v == nullptr || v.kind != K_OBJ) return false;
    writeTable(v, "", out);
    return true;
}
