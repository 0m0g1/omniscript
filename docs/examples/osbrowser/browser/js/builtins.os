// browser/js/builtins.os  -- ECMAScript built-ins that need no engine callbacks (V8's "builtins/"):
// Math, console, string and array methods. Methods taking callbacks (forEach/map/...) belong in interpreter.os
// because they must call back into it.   NEXT: Number, String(), parseInt/parseFloat, JSON, Date, Object.keys.
module U = import "util.os";
module Dbg = import "dbg.os";
module OBJ = import "js/objects.os";

let rngS: double = 0.0;
let store: OBJ.Obj* = nullptr;      // backing map for localStorage and sessionStorage

// Park-Miller minimal standard generator: s' = s * 16807 mod (2^31 - 1). Products stay below 2^53, so doubles are exact.
function rngStep() => double {
    let p: double = rngS * 16807.0;
    let q: int = (p / 2147483647.0) as int;
    rngS = p - (q as double) * 2147483647.0;
    if (rngS < 1.0) rngS = 1.0;
    return rngS;
}

// Mix the clock in on every page so two loads do not start from the same state, then discard the first outputs
// (low-entropy seeds make the first draws similar).
function seed(ms: double) => void {
    let s: double = rngS + ms * 7919.0 + 12345.0;
    let q: int = (s / 2147483647.0) as int;
    s = s - (q as double) * 2147483647.0;
    if (s < 1.0) s = 1.0;
    rngS = s;
    let i: int = 0;
    while (i < 8) { rngStep(); i = i + 1; }
}

function random() => double {
    if (rngS == 0.0) seed(1.0);
    return rngStep() / 2147483647.0;
}

function method(out: OBJ.Value*, recv: OBJ.Value*, name: char*) => void {
    out.k = OBJ.K_METHOD; out.s = name; out.n = recv.k as double; out.p = recv.p;
    if (recv.k == OBJ.K_STR) out.p = recv.s as void*;
}

// ---------- global names owned by this layer ----------
function global(name: char*, out: OBJ.Value*) => bool {
    if (U.streq(name, "Math")) { OBJ.setRef(out, OBJ.K_MATH, nullptr); return true; }
    if (U.streq(name, "console")) { OBJ.setRef(out, OBJ.K_CONSOLE, nullptr); return true; }
    if (U.streq(name, "localStorage") || U.streq(name, "sessionStorage")) { OBJ.setRef(out, OBJ.K_STORAGE, nullptr); return true; }
    if (U.streq(name, "NaN")) { OBJ.setNum(out, 0.0 / 0.0); return true; }
    if (U.streq(name, "Infinity")) { OBJ.setNum(out, 1.0e300 * 1.0e300); return true; }
    return false;
}

// ---------- property reads ----------
function get(obj: OBJ.Value*, name: char*, out: OBJ.Value*) => bool {
    OBJ.setUndef(out);
    if (obj.k == OBJ.K_MATH) {
        if (U.streq(name, "PI")) OBJ.setNum(out, 3.141592653589793);
        else if (U.streq(name, "E")) OBJ.setNum(out, 2.718281828459045);
        else method(out, obj, name);
        return true;
    }
    if (obj.k == OBJ.K_CONSOLE || obj.k == OBJ.K_STORAGE) { method(out, obj, name); return true; }
    if (obj.k == OBJ.K_STR) {
        if (U.streq(name, "length")) OBJ.setNum(out, U.slen(obj.s) as double);
        else method(out, obj, name);
        return true;
    }
    if (obj.k == OBJ.K_ARRAY) {
        let o: OBJ.Obj* = obj.p as OBJ.Obj*;
        if (U.streq(name, "length")) OBJ.setNum(out, o.n as double);
        else method(out, obj, name);
        return true;
    }
    return false;
}

// ---------- calls ----------
function argNum(args: OBJ.Value*, argc: int, i: int) => double {
    if (i >= argc) return 0.0 / 0.0;
    return OBJ.toNum(&args[i]);
}

function callMath(name: char*, args: OBJ.Value*, argc: int, out: OBJ.Value*) => void {
    let x: double = argNum(args, argc, 0);
    if (U.streq(name, "random")) OBJ.setNum(out, random());
    else if (U.streq(name, "floor")) OBJ.setNum(out, OBJ.floorD(x));
    else if (U.streq(name, "ceil")) OBJ.setNum(out, 0.0 - OBJ.floorD(0.0 - x));
    else if (U.streq(name, "round")) OBJ.setNum(out, OBJ.floorD(x + 0.5));
    else if (U.streq(name, "trunc")) { if (x < 0.0) OBJ.setNum(out, 0.0 - OBJ.floorD(0.0 - x)); else OBJ.setNum(out, OBJ.floorD(x)); }
    else if (U.streq(name, "abs")) { if (x < 0.0) OBJ.setNum(out, 0.0 - x); else OBJ.setNum(out, x); }
    else if (U.streq(name, "sqrt")) {
        let r: double = x;
        if (x > 0.0) { r = x; let i: int = 0; while (i < 40) { r = 0.5 * (r + x / r); i = i + 1; } }
        OBJ.setNum(out, r);
    }
    else if (U.streq(name, "pow")) {
        let y: double = argNum(args, argc, 1);
        let r: double = 1.0;
        let n: int = y as int;
        let neg: bool = n < 0;
        if (neg) n = 0 - n;
        let i: int = 0;
        while (i < n && i < 1100) { r = r * x; i = i + 1; }
        if (neg) r = 1.0 / r;
        OBJ.setNum(out, r);
    }
    else if (U.streq(name, "min") || U.streq(name, "max")) {
        let isMin: bool = U.streq(name, "min");
        let r: double = 1.0e300 * 1.0e300;
        if (!isMin) r = 0.0 - r;
        let i: int = 0;
        while (i < argc) {
            let v: double = OBJ.toNum(&args[i]);
            if (isMin && v < r) r = v;
            if (!isMin && v > r) r = v;
            i = i + 1;
        }
        OBJ.setNum(out, r);
    }
}

function callString(s: char*, name: char*, args: OBJ.Value*, argc: int, out: OBJ.Value*) => void {
    let n: int = U.slen(s);
    if (U.streq(name, "charAt")) {
        let i: int = argNum(args, argc, 0) as int;
        if (i >= 0 && i < n) OBJ.setStr(out, U.sub(s, i, i + 1)); else OBJ.setStr(out, "");
    }
    else if (U.streq(name, "indexOf")) {
        let needle: char* = "";
        if (argc > 0) needle = OBJ.toStr(&args[0]);
        let nl: int = U.slen(needle);
        let i: int = 0;
        let found: int = 0 - 1;
        while (i + nl <= n && found < 0) { if (U.streq(U.sub(s, i, i + nl), needle)) found = i; i = i + 1; }
        OBJ.setNum(out, found as double);
    }
    else if (U.streq(name, "slice") || U.streq(name, "substring")) {
        let a: int = 0;
        let b: int = n;
        if (argc > 0) a = argNum(args, argc, 0) as int;
        if (argc > 1) b = argNum(args, argc, 1) as int;
        if (a < 0) a = n + a;
        if (b < 0) b = n + b;
        if (a < 0) a = 0;
        if (b > n) b = n;
        if (a > b) a = b;
        OBJ.setStr(out, U.sub(s, a, b));
    }
    else if (U.streq(name, "toUpperCase") || U.streq(name, "toLowerCase")) {
        let up: bool = U.streq(name, "toUpperCase");
        let r: char* = U.newStr(n + 1);
        let i: int = 0;
        while (i < n) {
            let c: int = s[i] as int;
            if (up && c >= 97 && c <= 122) c = c - 32;
            if (!up && c >= 65 && c <= 90) c = c + 32;
            r[i] = c as char;
            i = i + 1;
        }
        r[n] = 0 as char;
        OBJ.setStr(out, r);
    }
}

function callArray(o: OBJ.Obj*, name: char*, args: OBJ.Value*, argc: int, out: OBJ.Value*) => void {
    if (U.streq(name, "push")) {
        let i: int = 0;
        while (i < argc) { OBJ.arrPush(o, &args[i]); i = i + 1; }
        OBJ.setNum(out, o.n as double);
    }
    else if (U.streq(name, "pop")) {
        if (o.n > 0) { OBJ.copyVal(out, &o.vals[o.n - 1]); o.n = o.n - 1; }
    }
    else if (U.streq(name, "join")) {
        let sep: char* = ",";
        if (argc > 0 && args[0].k != OBJ.K_UNDEF) sep = OBJ.toStr(&args[0]);
        let r: char* = "";
        let i: int = 0;
        while (i < o.n) {
            if (i > 0) r = U.cat(r, sep);
            if (o.vals[i].k != OBJ.K_UNDEF && o.vals[i].k != OBJ.K_NULL) r = U.cat(r, OBJ.toStr(&o.vals[i]));
            i = i + 1;
        }
        OBJ.setStr(out, r);
    }
    else if (U.streq(name, "indexOf")) {
        let found: int = 0 - 1;
        let i: int = 0;
        while (i < o.n && found < 0 && argc > 0) { if (OBJ.valsEqual(&o.vals[i], &args[0])) found = i; i = i + 1; }
        OBJ.setNum(out, found as double);
    }
}

function callStorage(name: char*, args: OBJ.Value*, argc: int, out: OBJ.Value*) => void {
    if (store == nullptr) store = OBJ.newObj(false);
    if (argc < 1) return;
    let key: char* = OBJ.toStr(&args[0]);
    if (U.streq(name, "getItem")) {
        let v: OBJ.Value;
        if (OBJ.objGet(store, key, &v) && v.k != OBJ.K_UNDEF) OBJ.copyVal(out, &v); else OBJ.setNull(out);
    }
    else if (U.streq(name, "setItem") && argc >= 2) { let sv: OBJ.Value; OBJ.setStr(&sv, OBJ.toStr(&args[1])); OBJ.objSet(store, key, &sv); }
    else if (U.streq(name, "removeItem")) { let uv: OBJ.Value; OBJ.setUndef(&uv); OBJ.objSet(store, key, &uv); }
}

function call(m: OBJ.Value*, args: OBJ.Value*, argc: int, out: OBJ.Value*) => void {
    OBJ.setUndef(out);
    let kind: int = m.n as int;
    if (kind == OBJ.K_STORAGE) { callStorage(m.s, args, argc, out); return; }
    if (kind == OBJ.K_MATH) callMath(m.s, args, argc, out);
    else if (kind == OBJ.K_CONSOLE) {
        let line: char* = "";
        let i: int = 0;
        while (i < argc) { if (i > 0) line = U.cat(line, " "); line = U.cat(line, OBJ.toStr(&args[i])); i = i + 1; }
        if (U.streq(m.s, "error")) Dbg.error(line); else Dbg.log(line);
    }
    else if (kind == OBJ.K_STR) callString(m.p as char*, m.s, args, argc, out);
    else if (kind == OBJ.K_ARRAY) callArray(m.p as OBJ.Obj*, m.s, args, argc, out);
}
