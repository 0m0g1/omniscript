// browser/js/objects.os  -- runtime object model (V8's "objects/" layer): tagged Value, JSObject/JSArray, conversions.
// Nothing in here knows about the parser, the interpreter or the DOM, so every other layer can import it.
module U = import "util.os";

// Value kinds (Value.k)
let K_UNDEF: int = 0;
let K_NUM: int = 1;
let K_STR: int = 2;
let K_BOOL: int = 3;
let K_FUNC: int = 4;       // user closure            p = Closure*
let K_ELEM: int = 5;       // DOM element             p = UI.Node*
let K_STYLE: int = 6;      // element.style           p = UI.Node*
let K_DOC: int = 7;        // document
let K_CONSOLE: int = 8;
let K_NULL: int = 9;
let K_NATIVE: int = 10;    // reserved: free-standing native function (n = id)
let K_METHOD: int = 11;    // bound native method     s = name, n = receiver kind, p = receiver payload
let K_OBJECT: int = 12;    // plain object            p = Obj*
let K_ARRAY: int = 13;     // array                   p = Obj*
let K_WINDOW: int = 14;
let K_MATH: int = 15;
let K_STORAGE: int = 16;   // localStorage / sessionStorage (in memory)

struct Value {
    k: int = 0;
    n: double = 0.0;
    s: char* = nullptr;
    p: void* = nullptr;
}

// One container for both objects (keys + vals) and arrays (vals only).
struct Obj {
    isArr: bool = false;
    n: int = 0;
    cap: int = 0;
    keys: char** = nullptr;
    vals: Value* = nullptr;
}

function setUndef(v: Value*) => void { v.k = 0; v.n = 0.0; v.s = nullptr; v.p = nullptr; }
function setNull(v: Value*) => void { v.k = 9; v.n = 0.0; v.s = nullptr; v.p = nullptr; }
function setNum(v: Value*, n: double) => void { v.k = 1; v.n = n; v.s = nullptr; v.p = nullptr; }
function setStr(v: Value*, s: char*) => void { v.k = 2; v.n = 0.0; v.s = s; v.p = nullptr; }
function setBool(v: Value*, b: bool) => void { v.k = 3; v.n = 0.0; v.s = nullptr; v.p = nullptr; if (b) v.n = 1.0; }
function copyVal(dst: Value*, src: Value*) => void { dst.k = src.k; dst.n = src.n; dst.s = src.s; dst.p = src.p; }
function setRef(v: Value*, k: int, p: void*) => void { v.k = k; v.n = 0.0; v.s = nullptr; v.p = p; }

// ---------- containers ----------
function newObj(isArr: bool) => Obj* {
    let o: Obj* = U.alloc(sizeof(Obj)) as Obj*;
    o.isArr = isArr; o.n = 0; o.cap = 8;
    o.keys = U.alloc(8 * 8) as char**;
    o.vals = U.alloc(sizeof(Value) * 8) as Value*;
    return o;
}

function grow(o: Obj*) => void {
    let nc: int = o.cap * 2;
    let nk: char** = U.alloc(8 * nc) as char**;
    let nv: Value* = U.alloc(sizeof(Value) * nc) as Value*;
    let i: int = 0;
    while (i < o.n) { nk[i] = o.keys[i]; copyVal(&nv[i], &o.vals[i]); i = i + 1; }
    o.keys = nk; o.vals = nv; o.cap = nc;
}

function objFind(o: Obj*, key: char*) => int {
    let i: int = 0;
    let found: int = 0 - 1;
    while (i < o.n && found < 0) { if (o.keys[i] != nullptr && U.streq(o.keys[i], key)) found = i; i = i + 1; }
    return found;
}

function objGet(o: Obj*, key: char*, out: Value*) => bool {
    let i: int = objFind(o, key);
    if (i < 0) { setUndef(out); return false; }
    copyVal(out, &o.vals[i]);
    return true;
}

function objSet(o: Obj*, key: char*, v: Value*) => void {
    let i: int = objFind(o, key);
    if (i >= 0) { copyVal(&o.vals[i], v); return; }
    if (o.n >= o.cap) grow(o);
    o.keys[o.n] = U.dup(key);
    copyVal(&o.vals[o.n], v);
    o.n = o.n + 1;
}

function arrPush(o: Obj*, v: Value*) => void {
    if (o.n >= o.cap) grow(o);
    o.keys[o.n] = nullptr;
    copyVal(&o.vals[o.n], v);
    o.n = o.n + 1;
}

function arrGet(o: Obj*, i: int, out: Value*) => void {
    if (i < 0 || i >= o.n) setUndef(out); else copyVal(out, &o.vals[i]);
}

function arrSet(o: Obj*, i: int, v: Value*) => void {
    if (i < 0) return;
    let u: Value;
    setUndef(&u);
    while (o.n <= i) arrPush(o, &u);
    copyVal(&o.vals[i], v);
}

// ---------- conversions ----------
function isNaN(x: double) => bool { return x != x; }

function digits(x: double) => char* {                  // x >= 0 and < 2e9, up to 6 fractional digits, trimmed
    let y: double = x + 0.0000005;
    let ip: int = y as int;
    let frac: double = y - (ip as double);
    let out: char* = U.fromInt(ip);
    let tail: char* = "";
    let nz: int = 0;
    let i: int = 0;
    while (i < 6) {
        frac = frac * 10.0;
        let d: int = frac as int;
        frac = frac - (d as double);
        tail = U.cat(tail, U.fromInt(d));
        if (d != 0) nz = i + 1;
        i = i + 1;
    }
    if (nz > 0) out = U.cat(out, U.cat(".", U.sub(tail, 0, nz)));
    return out;
}

function numStr(x: double) => char* {
    if (x != x) return "NaN";
    if (x < 0.0) {
        if (x < 0.0 - 2000000000.0) return U.fromNum(x);
        return U.cat("-", digits(0.0 - x));
    }
    if (x >= 2000000000.0) return U.fromNum(x);
    return digits(x);
}

function truthy(v: Value*) => bool {
    if (v.k == 0 || v.k == 9) return false;
    if (v.k == 1 || v.k == 3) return v.n != 0.0 && v.n == v.n;
    if (v.k == 2) return (v.s[0] as int) != 0;
    return true;
}

function toStr(v: Value*) => char* {
    if (v.k == 2) return v.s;
    if (v.k == 1) return numStr(v.n);
    if (v.k == 3) { if (v.n != 0.0) return "true"; return "false"; }
    if (v.k == 0) return "undefined";
    if (v.k == 9) return "null";
    if (v.k == 13) {
        let o: Obj* = v.p as Obj*;
        let out: char* = "";
        let i: int = 0;
        while (i < o.n) {
            if (i > 0) out = U.cat(out, ",");
            if (o.vals[i].k != 0 && o.vals[i].k != 9) out = U.cat(out, toStr(&o.vals[i]));
            i = i + 1;
        }
        return out;
    }
    if (v.k == 4) return "function () { [code] }";
    if (v.k == 5) return "[object HTMLElement]";
    return "[object Object]";
}

function toNum(v: Value*) => double {
    if (v.k == 1 || v.k == 3) return v.n;
    if (v.k == 9) return 0.0;
    if (v.k == 2) {
        if (U.slen(v.s) == 0) return 0.0;
        return U.toNum(v.s);
    }
    return 0.0 / 0.0;
}

function looseNullish(v: Value*) => bool { return v.k == 0 || v.k == 9; }

function valsEqual(a: Value*, b: Value*) => bool {
    if (looseNullish(a) && looseNullish(b)) return true;
    if (a.k != b.k) return false;
    if (a.k == 2) return U.streq(a.s, b.s);
    if (a.k == 1 || a.k == 3) return a.n == b.n;
    if (a.k == 11) return a.p == b.p && U.streq(a.s, b.s);
    return a.p == b.p;
}

function typeOf(v: Value*) => char* {
    if (v.k == 0) return "undefined";
    if (v.k == 1) return "number";
    if (v.k == 2) return "string";
    if (v.k == 3) return "boolean";
    if (v.k == 4 || v.k == 10 || v.k == 11) return "function";
    return "object";
}

function floorD(x: double) => double {
    if (x != x) return x;
    if (x > 2000000000.0 || x < 0.0 - 2000000000.0) return x;
    let i: int = x as int;
    let d: double = i as double;
    if (d > x) d = d - 1.0;
    return d;
}
