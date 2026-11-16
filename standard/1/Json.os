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
    let ok: bool = false;
    let going: bool = true;
    while (going) {
        let c: int = p.src[p.pos] as int;
        if (c == 0) {
            going = false;                       // unterminated string
        } else {
            p.pos = p.pos + 1;
            if (c == 34) {
                ok = true;
                going = false;
            } else if (c == 92) {
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
    }
    return ok;
}

function parseValue(p: Parser*) => JsonValue* {
    ws(p);
    let c: int = p.src[p.pos] as int;
    if (c == 123) { // {
        let o = newObject();
        p.pos = p.pos + 1; ws(p);
        if ((p.src[p.pos] as int) == 125) { p.pos = p.pos + 1; return o; }
        let going: bool = true;
        while (going) {
            ws(p);
            if ((p.src[p.pos] as int) != 34) {
                p.err = true; going = false;
            } else {
                let k: Str.String;
                if (!parseString(p, &k)) {
                    k.free(); p.err = true; going = false;
                } else {
                    ws(p);
                    if ((p.src[p.pos] as int) != 58) {
                        k.free(); p.err = true; going = false;
                    } else {
                        p.pos = p.pos + 1;
                        let v = parseValue(p);
                        o.keys.push(k); o.items.push(v);
                        if (p.err) {
                            going = false;
                        } else {
                            ws(p);
                            let d: int = p.src[p.pos] as int;
                            p.pos = p.pos + 1;
                            if (d == 125) going = false;
                            else if (d != 44) { p.err = true; going = false; }
                        }
                    }
                }
            }
        }
        return o;
    }
    if (c == 91) { // [
        let a = newArray();
        p.pos = p.pos + 1; ws(p);
        if ((p.src[p.pos] as int) == 93) { p.pos = p.pos + 1; return a; }
        let going2: bool = true;
        while (going2) {
            let v2 = parseValue(p);
            a.items.push(v2);
            if (p.err) {
                going2 = false;
            } else {
                ws(p);
                let d2: int = p.src[p.pos] as int;
                p.pos = p.pos + 1;
                if (d2 == 93) going2 = false;
                else if (d2 != 44) { p.err = true; going2 = false; }
            }
        }
        return a;
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

// ---- Json additions: typed getters with defaults, builders, path lookup, remove, deep copy, strict parse ----
// All free functions taking JsonValue*. Ownership rules are unchanged: a value owns its children.

function has(v: JsonValue*, key: char*) => bool {
    return v.get(key) != nullptr;
}

function getStr(v: JsonValue*, key: char*, def: char*) => char* {
    let c: JsonValue* = v.get(key);
    if (c == nullptr) return def;
    return c.asStr(def);
}

function getNum(v: JsonValue*, key: char*, def: double) => double {
    let c: JsonValue* = v.get(key);
    if (c == nullptr) return def;
    return c.asNum(def);
}

function getInt(v: JsonValue*, key: char*, def: i64) => i64 {
    let c: JsonValue* = v.get(key);
    if (c == nullptr) return def;
    if (c.kind != J_NUM) return def;
    return c.n as i64;
}

function getBool(v: JsonValue*, key: char*, def: bool) => bool {
    let c: JsonValue* = v.get(key);
    if (c == nullptr) return def;
    return c.asBool(def);
}

// ---- building (object setters / array pushers that wrap the raw value for you) ----
function setNum(o: JsonValue*, key: char*, n: double) => bool { return o.set(key, newNum(n)); }
function setStr(o: JsonValue*, key: char*, s: char*) => bool { return o.set(key, newStr(s)); }
function setBool(o: JsonValue*, key: char*, b: bool) => bool { return o.set(key, newBool(b)); }
function setNull(o: JsonValue*, key: char*) => bool { return o.set(key, newNull()); }
function pushNum(a: JsonValue*, n: double) => bool { return a.push(newNum(n)); }
function pushStr(a: JsonValue*, s: char*) => bool { return a.push(newStr(s)); }
function pushBool(a: JsonValue*, b: bool) => bool { return a.push(newBool(b)); }

// Looks up a dotted path: object keys and array indexes, e.g. "users.0.name". nullptr if any step is missing.
function getPath(root: JsonValue*, path: char*) => JsonValue* {
    let cur: JsonValue* = root;
    let i: u64 = 0;
    while (cur != nullptr && (path[i] as int) != 0) {
        let seg: Str.String;
        while ((path[i] as int) != 0 && (path[i] as int) != 46) {
            seg.appendChar(path[i]);
            i = i + 1;
        }
        if ((path[i] as int) == 46) i = i + 1;
        if (cur.kind == J_ARR) {
            let idx: i64 = 0;
            if (seg.parseInt(&idx) && idx >= 0) cur = cur.at(idx as u64); else cur = nullptr;
        } else if (cur.kind == J_OBJ) {
            cur = cur.get(seg.cstr());
        } else {
            cur = nullptr;
        }
        seg.free();
    }
    return cur;
}

// Removes key from an object and frees its value. false if the key is not present.
function remove(o: JsonValue*, key: char*) => bool {
    let idx: u64 = 0;
    let found: bool = false;
    while (!found && idx < o.keys.len) {
        if (o.keys.data[idx].equalsC(key)) found = true; else idx = idx + 1;
    }
    if (!found) return false;
    let child: JsonValue* = o.items.data[idx];
    child.free();
    CMem.free(child as void*);
    o.keys.data[idx].free();
    o.keys.removeAt(idx);
    o.items.removeAt(idx);
    return true;
}

// Deep copy. Free with v.free(); CMem.free(v).
function dup(v: JsonValue*) => JsonValue* {
    let r = alloc(v.kind);
    r.b = v.b;
    r.n = v.n;
    r.s.set(v.s.cstr());
    let i: u64 = 0;
    while (i < v.items.len) {
        r.items.push(dup(v.items.data[i]));
        i = i + 1;
    }
    let k: u64 = 0;
    while (k < v.keys.len) {
        let key: Str.String;
        key.set(v.keys.data[k].cstr());
        r.keys.push(key);
        k = k + 1;
    }
    return r;
}

// Like parse(), but also rejects trailing non-whitespace ("{} junk" is an error).
function parseStrict(text: char*) => JsonValue* {
    let p: Parser;
    p.src = text;
    let v = parseValue(&p);
    ws(&p);
    if (p.err || (p.src[p.pos] as int) != 0) {
        v.free();
        CMem.free(v as void*);
        return nullptr;
    }
    return v;
}
