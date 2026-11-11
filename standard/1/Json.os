// standard/1/Json.os
// Tier 4: JSON DOM. Json.parse(text, &root) / root.free(); build with newObject/newArray + set/push; stringify.
// Values own their children. Never copy a JsonValue with `=`.
module Str = import "standard/1/String.os";
module L = import "standard/1/List.os";
module CMem = import "standard/1/c/mem.os";
module CStr = import "standard/1/c/string.os";
module CIO = import "standard/1/c/stdio_extra.os";
module Text = import "standard/1/Text.os";

let J_NULL: int = 0;
let J_BOOL: int = 1;
let J_NUM: int = 2;
let J_STR: int = 3;
let J_ARR: int = 4;
let J_OBJ: int = 5;

struct JsonValue {
    kind: int = 0;
    b: bool = false;
    n: double = 0.0;
    s: Str.String;
    items: L.List<JsonValue*>;     // array items, or object values
    keys: L.List<Str.String>;      // object keys (parallel to items)

    free() => void {
        let i: u64 = 0;
        while (i < this.items.len) {
            let c: JsonValue* = this.items.data[i];
            c.free();
            CMem.free(c as void*);
            i = i + 1;
        }
        this.items.free();
        i = 0;
        while (i < this.keys.len) { this.keys.data[i].free(); i = i + 1; }
        this.keys.free();
        this.s.free();
    }

    isNull() => bool { return this.kind == J_NULL; }
    isObject() => bool { return this.kind == J_OBJ; }
    isArray() => bool { return this.kind == J_ARR; }
    length() => u64 { return this.items.len; }

    // object lookup; nullptr if missing
    get(key: char*) => JsonValue* {
        let i: u64 = 0;
        while (i < this.keys.len) {
            if (this.keys.data[i].equalsC(key)) return this.items.data[i];
            i = i + 1;
        }
        return nullptr;
    }
    at(i: u64) => JsonValue* { if (i >= this.items.len) return nullptr; return this.items.data[i]; }

    asStr(def: char*) => char* { if (this.kind == J_STR) return this.s.cstr(); return def; }
    asNum(def: double) => double { if (this.kind == J_NUM) return this.n; return def; }
    asBool(def: bool) => bool { if (this.kind == J_BOOL) return this.b; return def; }

    // building
    push(v: JsonValue*) => bool { return this.items.push(v); }
    set(key: char*, v: JsonValue*) => bool {
        let k: Str.String; k.set(key);
        if (!this.keys.push(k)) return false;
        return this.items.push(v);
    }
}

function alloc(kind: int) => JsonValue* {
    let v = CMem.calloc(1, sizeof(JsonValue)) as JsonValue*;
    v.kind = kind;
    return v;
}
function newNull() => JsonValue* { return alloc(J_NULL); }
function newBool(b: bool) => JsonValue* { let v = alloc(J_BOOL); v.b = b; return v; }
function newNum(n: double) => JsonValue* { let v = alloc(J_NUM); v.n = n; return v; }
function newStr(s: char*) => JsonValue* { let v = alloc(J_STR); v.s.set(s); return v; }
function newArray() => JsonValue* { return alloc(J_ARR); }
function newObject() => JsonValue* { return alloc(J_OBJ); }

struct Parser { src: char* = nullptr; pos: u64 = 0; err: bool = false; }

function ws(p: Parser*) => void {
    while (Text.isSpace(p.src[p.pos] as int)) p.pos = p.pos + 1;
}

function parseString(p: Parser*, out: Str.String*) => bool {
    p.pos = p.pos + 1; // opening quote
    while (true) {
        let c: int = p.src[p.pos] as int;
        if (c == 0) return false;
        p.pos = p.pos + 1;
        if (c == 34) return true;
        if (c == 92) {
            let e: int = p.src[p.pos] as int;
            p.pos = p.pos + 1;
            if (e == 110) out.appendChar(10 as char);
            else if (e == 116) out.appendChar(9 as char);
            else if (e == 114) out.appendChar(13 as char);
            else if (e == 98) out.appendChar(8 as char);
            else if (e == 102) out.appendChar(12 as char);
            else if (e == 117) {
                let hex: [5]char;
                let k: u64 = 0;
                while (k < 4) { hex[k] = p.src[p.pos + k]; k = k + 1; }
                hex[4] = 0 as char;
                let cp: u64 = CStr.strtoull(&hex as char*, nullptr, 16);
                p.pos = p.pos + 4;
                Text.appendUtf8(cp as u32, out);
            } else out.appendChar(e as char);
        } else out.appendChar(c as char);
    }
    return false;
}

function parseValue(p: Parser*) => JsonValue* {
    ws(p);
    let c: int = p.src[p.pos] as int;
    if (c == 123) { // {
        let o = newObject();
        p.pos = p.pos + 1; ws(p);
        if ((p.src[p.pos] as int) == 125) { p.pos = p.pos + 1; return o; }
        while (true) {
            ws(p);
            if ((p.src[p.pos] as int) != 34) { p.err = true; return o; }
            let k: Str.String;
            if (!parseString(p, &k)) { p.err = true; return o; }
            ws(p);
            if ((p.src[p.pos] as int) != 58) { k.free(); p.err = true; return o; }
            p.pos = p.pos + 1;
            let v = parseValue(p);
            o.keys.push(k); o.items.push(v);
            if (p.err) return o;
            ws(p);
            let d: int = p.src[p.pos] as int;
            p.pos = p.pos + 1;
            if (d == 125) return o;
            if (d != 44) { p.err = true; return o; }
        }
    }
    if (c == 91) { // [
        let a = newArray();
        p.pos = p.pos + 1; ws(p);
        if ((p.src[p.pos] as int) == 93) { p.pos = p.pos + 1; return a; }
        while (true) {
            let v = parseValue(p);
            a.items.push(v);
            if (p.err) return a;
            ws(p);
            let d: int = p.src[p.pos] as int;
            p.pos = p.pos + 1;
            if (d == 93) return a;
            if (d != 44) { p.err = true; return a; }
        }
    }
    if (c == 34) {
        let v = alloc(J_STR);
        if (!parseString(p, &v.s)) p.err = true;
        return v;
    }
    if (CStr.strncmp((p.src as u64 + p.pos) as char*, "true", 4) == 0) { p.pos = p.pos + 4; return newBool(true); }
    if (CStr.strncmp((p.src as u64 + p.pos) as char*, "false", 5) == 0) { p.pos = p.pos + 5; return newBool(false); }
    if (CStr.strncmp((p.src as u64 + p.pos) as char*, "null", 4) == 0) { p.pos = p.pos + 4; return newNull(); }
    let end: char* = nullptr;
    let n: double = CStr.strtod((p.src as u64 + p.pos) as char*, &end);
    if (end == ((p.src as u64 + p.pos) as char*)) { p.err = true; return newNull(); }
    p.pos = (end as u64) - (p.src as u64);
    return newNum(n);
}

// Returns nullptr on syntax error. Caller: root.free(); CMem.free(root).
function parse(text: char*) => JsonValue* {
    let p: Parser;
    p.src = text;
    let v = parseValue(&p);
    if (p.err) { v.free(); CMem.free(v as void*); return nullptr; }
    return v;
}

function write(v: JsonValue*, out: Str.String*, pretty: bool, depth: u64) => void {
    if (v.kind == J_NULL) { out.append("null"); return; }
    if (v.kind == J_BOOL) { if (v.b) out.append("true"); else out.append("false"); return; }
    if (v.kind == J_NUM) {
        let buf: [64]char;
        CIO.snprintf(&buf as char*, 64, "%.17g", v.n);
        out.append(&buf as char*);
        return;
    }
    if (v.kind == J_STR) { out.append("\""); Text.escape(v.s.cstr(), out); out.append("\""); return; }
    let isObj: bool = v.kind == J_OBJ;
    if (isObj) out.append("{"); else out.append("[");
    let i: u64 = 0;
    while (i < v.items.len) {
        if (i > 0) out.append(",");
        if (pretty) { out.append("\n"); Text.repeat("  ", depth + 1, out); }
        if (isObj) { out.append("\""); Text.escape(v.keys.data[i].cstr(), out); out.append("\":"); if (pretty) out.append(" "); }
        write(v.items.data[i], out, pretty, depth + 1);
        i = i + 1;
    }
    if (pretty && v.items.len > 0) { out.append("\n"); Text.repeat("  ", depth, out); }
    if (isObj) out.append("}"); else out.append("]");
}

function stringify(v: JsonValue*, out: Str.String*, pretty: bool) => void { write(v, out, pretty, 0); }
