// browser/js/interpreter.os  -- AST tree-walking interpreter (V8's "interpreter/", Ignition's role): scopes, closures,
// control flow, calls. Property access is routed by value kind: objects/arrays here, strings/Math/console to
// builtins.os, window/document/elements/style to dom.os.
// NEXT: try/catch/throw, `this` + `new` + prototypes, for-of/for-in, switch, Array.forEach/map/filter (callbacks live here),
//       getters/setters, TDZ, a bytecode compiler behind the same eval/call entry points.
module U = import "util.os";
module Dbg = import "dbg.os";
module OBJ = import "js/objects.os";
module AST = import "js/ast.os";
module P = import "js/parser.os";
module BI = import "js/builtins.os";
module DOM = import "js/dom.os";

struct Scope {
    names: char** = nullptr;
    vals: OBJ.Value* = nullptr;
    n: int = 0;
    cap: int = 0;
    parent: Scope* = nullptr;
}

struct Closure {
    fn: AST.Node* = nullptr;
    env: Scope* = nullptr;
    proto: OBJ.Obj* = nullptr;     // F.prototype, created on first use
    props: OBJ.Obj* = nullptr;     // static properties (F.name = ...)
}

let STEP_LIMIT: int = 400000;     // loop iterations + calls per entry from the host: protects the UI thread from runaway scripts
let DEPTH_LIMIT: int = 120;

let ctl: int = 0;                 // 0 normal, 1 break, 2 continue, 3 return, 4 error (unwinds to the host entry point)
let retv: OBJ.Value;
let steps: int = 0;
let depth: int = 0;
let argStack: OBJ.Value* = nullptr;
let argTop: int = 0;
let curNode: AST.Node* = nullptr;     // node being evaluated, for error positions
let globalScope: Scope* = nullptr;
let calleeName: char* = "";       // name of the function being called, for "is not a function" errors
let tryDepth: int = 0;            // >0 while inside a try block: errors are caught, not logged
let thrown: OBJ.Value;            // the value of the error being unwound (fail() stores its message here)
let curThis: OBJ.Value;           // `this` for the running call (set by new and by obj.method())

function init() => void {
    argStack = U.alloc(sizeof(OBJ.Value) * 512) as OBJ.Value*;
    argTop = 0;
    globalScope = newScope(nullptr);
    ctl = 0; steps = 0; depth = 0; tryDepth = 0;
    OBJ.setUndef(&curThis); OBJ.setUndef(&thrown);
}

function fail(msg: char*) => void {
    if (ctl != 4) {
        OBJ.setStr(&thrown, msg);
        let at: char* = "";
        if (curNode != nullptr && curNode.file != nullptr) at = U.cat(" [", U.cat(curNode.file, U.cat(":", U.cat(U.fromInt(curNode.line), "]"))));
        if (tryDepth == 0) Dbg.error(U.cat("js: ", U.cat(msg, at)));
    }
    ctl = 4;
}

// ---------- scopes ----------
function newScope(parent: Scope*) => Scope* {
    let s: Scope* = U.alloc(sizeof(Scope)) as Scope*;
    s.cap = 4; s.n = 0; s.parent = parent;
    s.names = U.alloc(8 * 4) as char**;
    s.vals = U.alloc(sizeof(OBJ.Value) * 4) as OBJ.Value*;
    return s;
}

function findLocal(s: Scope*, name: char*) => int {
    let i: int = 0;
    while (i < s.n) { if (U.streq(s.names[i], name)) return i; i = i + 1; }
    return 0 - 1;
}

function declare(s: Scope*, name: char*, v: OBJ.Value*) => void {
    let i: int = findLocal(s, name);
    if (i >= 0) { OBJ.copyVal(&s.vals[i], v); return; }
    if (s.n >= s.cap) {
        let nc: int = s.cap * 2;
        let nn: char** = U.alloc(8 * nc) as char**;
        let nv: OBJ.Value* = U.alloc(sizeof(OBJ.Value) * nc) as OBJ.Value*;
        let j: int = 0;
        while (j < s.n) { nn[j] = s.names[j]; OBJ.copyVal(&nv[j], &s.vals[j]); j = j + 1; }
        s.names = nn; s.vals = nv; s.cap = nc;
    }
    s.names[s.n] = name;
    OBJ.copyVal(&s.vals[s.n], v);
    s.n = s.n + 1;
}

function lookup(s: Scope*, name: char*) => OBJ.Value* {
    let c: Scope* = s;
    while (c != nullptr) {
        let i: int = findLocal(c, name);
        if (i >= 0) return &c.vals[i];
        c = c.parent;
    }
    return nullptr;
}

function assignVar(s: Scope*, name: char*, v: OBJ.Value*) => void {
    let slot: OBJ.Value* = lookup(s, name);
    if (slot != nullptr) OBJ.copyVal(slot, v); else declare(globalScope, name, v);   // sloppy-mode implicit global
}

// ---------- property access ----------
function objGetChain(o: OBJ.Obj*, name: char*, out: OBJ.Value*) => bool {   // own property, then the prototype chain
    let c: OBJ.Obj* = o;
    let guard: int = 0;
    let found: bool = false;
    while (c != nullptr && guard < 64 && !found) {
        if (OBJ.objGet(c, name, out)) found = true;
        else c = c.proto;
        guard = guard + 1;
    }
    if (!found) OBJ.setUndef(out);
    return found;
}

function funcProto(c: Closure*) => OBJ.Obj* {
    if (c.proto == nullptr) c.proto = OBJ.newObj(false);
    return c.proto;
}

function funcGet(c: Closure*, obj: OBJ.Value*, name: char*, out: OBJ.Value*) => void {
    if (U.streq(name, "prototype")) OBJ.setRef(out, OBJ.K_OBJECT, funcProto(c) as void*);
    else if (c.props != nullptr && OBJ.objGet(c.props, name, out)) { }
    else if (U.streq(name, "call") || U.streq(name, "apply") || U.streq(name, "bind")) BI.method(out, obj, name);
}

function getProp(obj: OBJ.Value*, name: char*, out: OBJ.Value*) => void {
    OBJ.setUndef(out);
    let k: int = obj.k;
    if (k == OBJ.K_OBJECT) objGetChain(obj.p as OBJ.Obj*, name, out);
    else if (k == OBJ.K_FUNC) funcGet(obj.p as Closure*, obj, name, out);
    else if (k == OBJ.K_ARRAY || k == OBJ.K_STR || k == OBJ.K_MATH || k == OBJ.K_CONSOLE || k == OBJ.K_STORAGE || k == OBJ.K_CTOR) BI.get(obj, name, out);
    else if (k == OBJ.K_UNDEF || k == OBJ.K_NULL) fail(U.cat("cannot read property '", U.cat(name, U.cat("' of ", OBJ.toStr(obj)))));
    else DOM.get(obj, name, out);
}

function setProp(obj: OBJ.Value*, name: char*, v: OBJ.Value*) => void {
    let k: int = obj.k;
    if (k == OBJ.K_OBJECT) OBJ.objSet(obj.p as OBJ.Obj*, name, v);
    else if (k == OBJ.K_FUNC) {
        let c: Closure* = obj.p as Closure*;
        if (U.streq(name, "prototype") && v.k == OBJ.K_OBJECT) c.proto = v.p as OBJ.Obj*;
        else { if (c.props == nullptr) c.props = OBJ.newObj(false); OBJ.objSet(c.props, name, v); }
    }
    else if (k == OBJ.K_UNDEF || k == OBJ.K_NULL) fail(U.cat("cannot set property '", U.cat(name, "' of undefined")));
    else DOM.set(obj, name, v);
}

function getIndex(obj: OBJ.Value*, key: OBJ.Value*, out: OBJ.Value*) => void {
    OBJ.setUndef(out);
    if (obj.k == OBJ.K_ARRAY && key.k == OBJ.K_NUM) OBJ.arrGet(obj.p as OBJ.Obj*, key.n as int, out);
    else if (obj.k == OBJ.K_STR && key.k == OBJ.K_NUM) {
        let i: int = key.n as int;
        if (i >= 0 && i < U.slen(obj.s)) OBJ.setStr(out, U.sub(obj.s, i, i + 1));
    }
    else getProp(obj, OBJ.toStr(key), out);
}

function setIndex(obj: OBJ.Value*, key: OBJ.Value*, v: OBJ.Value*) => void {
    if (obj.k == OBJ.K_ARRAY && key.k == OBJ.K_NUM) OBJ.arrSet(obj.p as OBJ.Obj*, key.n as int, v);
    else setProp(obj, OBJ.toStr(key), v);
}

// ---------- operators ----------
function cmpStr(a: char*, b: char*) => int {
    let i: int = 0;
    while ((a[i] as int) != 0 && (a[i] as int) == (b[i] as int)) i = i + 1;
    return (a[i] as int) - (b[i] as int);
}

function modD(a: double, b: double) => double {
    if (b == 0.0) return 0.0 / 0.0;
    let q: double = a / b;
    let t: double = OBJ.floorD(q);
    if (q < 0.0) t = 0.0 - OBJ.floorD(0.0 - q);
    return a - t * b;
}

// ---------- bitwise (32-bit, done with double arithmetic) ----------
function toU32(x: double) => double {
    if (x != x) return 0.0;
    let t: double = OBJ.floorD(x);
    if (x < 0.0) t = 0.0 - OBJ.floorD(0.0 - x);
    let m: double = modD(t, 4294967296.0);
    if (m < 0.0) m = m + 4294967296.0;
    return m;
}

function toI32(x: double) => double {
    let u: double = toU32(x);
    if (u >= 2147483648.0) u = u - 4294967296.0;
    return u;
}

function bits16(op: int, a: double, b: double) => double {      // a, b in 0..65535
    let res: double = 0.0;
    let pw: double = 1.0;
    let k: int = 0;
    while (k < 16) {
        let qa: double = OBJ.floorD(a / 2.0);
        let qb: double = OBJ.floorD(b / 2.0);
        let ba: double = a - qa * 2.0;
        let bb: double = b - qb * 2.0;
        let r: double = ba + bb;
        if (op == AST.OP_BAND) r = ba * bb;
        else if (op == AST.OP_BOR) { if (r > 1.0) r = 1.0; }
        else { if (r > 1.5) r = 0.0; }                          // xor
        res = res + r * pw;
        pw = pw * 2.0;
        a = qa; b = qb;
        k = k + 1;
    }
    return res;
}

function bitop(op: int, x: double, y: double) => double {
    let ua: double = toU32(x);
    let ub: double = toU32(y);
    let ah: double = OBJ.floorD(ua / 65536.0);
    let bh: double = OBJ.floorD(ub / 65536.0);
    let al: double = ua - ah * 65536.0;
    let bl: double = ub - bh * 65536.0;
    return toI32(bits16(op, ah, bh) * 65536.0 + bits16(op, al, bl));
}

function shiftop(op: int, x: double, y: double) => double {
    let cnt: int = toU32(y) as int;
    cnt = cnt - (cnt / 32) * 32;
    let k: int = 0;
    if (op == AST.OP_SHL) {
        let u: double = toU32(x);
        while (k < cnt) { u = u * 2.0; if (u >= 4294967296.0) u = u - 4294967296.0; k = k + 1; }
        return toI32(u);
    }
    if (op == AST.OP_SHR) {
        let v: double = toI32(x);
        while (k < cnt) { v = OBJ.floorD(v / 2.0); k = k + 1; }
        return v;
    }
    let w: double = toU32(x);                                    // >>>
    while (k < cnt) {
        if (w >= 2147483648.0) w = OBJ.floorD((w - 2147483648.0) / 2.0) + 1073741824.0;
        else w = OBJ.floorD(w / 2.0);
        k = k + 1;
    }
    return w;
}

function binary(op: int, l: OBJ.Value*, r: OBJ.Value*, out: OBJ.Value*) => void {
    if (op == AST.OP_ADD) {
        if (l.k == OBJ.K_STR || r.k == OBJ.K_STR || l.k >= OBJ.K_FUNC && l.k != OBJ.K_NULL || r.k >= OBJ.K_FUNC && r.k != OBJ.K_NULL)
            OBJ.setStr(out, U.cat(OBJ.toStr(l), OBJ.toStr(r)));
        else OBJ.setNum(out, OBJ.toNum(l) + OBJ.toNum(r));
    }
    else if (op == AST.OP_SUB) OBJ.setNum(out, OBJ.toNum(l) - OBJ.toNum(r));
    else if (op == AST.OP_MUL) OBJ.setNum(out, OBJ.toNum(l) * OBJ.toNum(r));
    else if (op == AST.OP_DIV) OBJ.setNum(out, OBJ.toNum(l) / OBJ.toNum(r));
    else if (op == AST.OP_MOD) OBJ.setNum(out, modD(OBJ.toNum(l), OBJ.toNum(r)));
    else if (op == AST.OP_EQ) OBJ.setBool(out, OBJ.valsEqual(l, r));
    else if (op == AST.OP_NE) OBJ.setBool(out, !OBJ.valsEqual(l, r));
    else if (op == AST.OP_BOR || op == AST.OP_BAND || op == AST.OP_BXOR) OBJ.setNum(out, bitop(op, OBJ.toNum(l), OBJ.toNum(r)));
    else if (op == AST.OP_SHL || op == AST.OP_SHR || op == AST.OP_USHR) OBJ.setNum(out, shiftop(op, OBJ.toNum(l), OBJ.toNum(r)));
    else {
        let a: double = OBJ.toNum(l);
        let b: double = OBJ.toNum(r);
        if (l.k == OBJ.K_STR && r.k == OBJ.K_STR) { a = cmpStr(l.s, r.s) as double; b = 0.0; }   // lexicographic
        if (op == AST.OP_LT) OBJ.setBool(out, a < b);
        else if (op == AST.OP_GT) OBJ.setBool(out, a > b);
        else if (op == AST.OP_LE) OBJ.setBool(out, a <= b);
        else OBJ.setBool(out, a >= b);
    }
}

// ---------- destructuring ----------
function bindPattern(pat: AST.Node*, v: OBJ.Value*, sc: Scope*) => void {
    if (OBJ.looseNullish(v)) { fail("cannot destructure undefined or null"); return; }
    let pr: AST.Node* = pat.a;
    while (pr != nullptr && ctl == 0) {
        let x: OBJ.Value;
        OBJ.setUndef(&x);
        if (pat.op == 1) {
            let ix: OBJ.Value;
            OBJ.setNum(&ix, pr.n);
            getIndex(v, &ix, &x);
        }
        else getProp(v, pr.s, &x);
        if (ctl == 0 && x.k == OBJ.K_UNDEF && pr.a != nullptr) evalExpr(pr.a, sc, &x);   // default value
        if (ctl == 0) declare(sc, pr.b.s, &x);
        pr = pr.next;
    }
}

// ---------- functions ----------
function makeFunc(fn: AST.Node*, env: Scope*, out: OBJ.Value*) => void {
    let c: Closure* = U.alloc(sizeof(Closure)) as Closure*;
    c.fn = fn; c.env = env;
    OBJ.setRef(out, OBJ.K_FUNC, c as void*);
}

function callClosure(c: Closure*, args: OBJ.Value*, argc: int, out: OBJ.Value*) => void {
    OBJ.setUndef(out);
    if (depth >= DEPTH_LIMIT) { fail("call stack too deep"); return; }
    steps = steps + 1;
    if (steps > STEP_LIMIT) { fail("script ran too long"); return; }
    depth = depth + 1;
    let sc: Scope* = newScope(c.env);
    let p: AST.Node* = c.fn.a;
    let i: int = 0;
    while (p != nullptr && ctl == 0) {
        let a: OBJ.Value;
        OBJ.setUndef(&a);
        if (p.op == 1) {                                                     // ...rest
            let rest: OBJ.Obj* = OBJ.newObj(true);
            let j: int = i;
            while (j < argc) { OBJ.arrPush(rest, &args[j]); j = j + 1; }
            OBJ.setRef(&a, OBJ.K_ARRAY, rest as void*);
        }
        else if (i < argc) OBJ.copyVal(&a, &args[i]);
        if (a.k == OBJ.K_UNDEF && p.a != nullptr) evalExpr(p.a, sc, &a);   // default parameter
        if (p.c != nullptr) bindPattern(p.c, &a, sc);
        else declare(sc, p.s, &a);
        i = i + 1;
        p = p.next;
    }
    if (ctl == 4) { depth = depth - 1; return; }
    if (c.fn.n != 0.0) evalExpr(c.fn.b, sc, out);
    else {
        execList(c.fn.b.a, sc);
        if (ctl == 3) { OBJ.copyVal(out, &retv); ctl = 0; }
    }
    depth = depth - 1;
}

function callValue(callee: OBJ.Value*, args: OBJ.Value*, argc: int, out: OBJ.Value*) => void {
    OBJ.setUndef(out);
    if (callee.k == OBJ.K_FUNC) {
        let cl: Closure* = callee.p as Closure*;     // annotated local: a bare cast is not module-qualified, so it cannot bind to a parameter
        callClosure(cl, args, argc, out);
    }
    else if (callee.k == OBJ.K_CTOR) BI.callGlobal(callee.s, args, argc, out);
    else if (callee.k == OBJ.K_METHOD) {
        let rk: int = callee.n as int;
        if (rk == OBJ.K_FUNC) funcMethod(callee, args, argc, out);
        else if (rk == OBJ.K_WINDOW || rk == OBJ.K_DOC || rk == OBJ.K_ELEM || rk == OBJ.K_STYLE) DOM.call(callee, args, argc, out);
        else BI.call(callee, args, argc, out);
    }
    else {
        if (U.slen(calleeName) > 0) fail(U.cat(calleeName, " is not a function")); else fail("value is not a function");
    }
}

function funcMethod(m: OBJ.Value*, args: OBJ.Value*, argc: int, out: OBJ.Value*) => void {   // f.call / f.apply / f.bind
    let cl: Closure* = m.p as Closure*;
    let saved: OBJ.Value;
    OBJ.copyVal(&saved, &curThis);
    if (U.streq(m.s, "call")) {
        if (argc > 0) OBJ.copyVal(&curThis, &args[0]); else OBJ.setUndef(&curThis);
        if (argc > 1) callClosure(cl, &args[1], argc - 1, out); else callClosure(cl, args, 0, out);
    }
    else if (U.streq(m.s, "apply")) {
        if (argc > 0) OBJ.copyVal(&curThis, &args[0]); else OBJ.setUndef(&curThis);
        if (argc > 1 && args[1].k == OBJ.K_ARRAY) { let ao: OBJ.Obj* = args[1].p as OBJ.Obj*; callClosure(cl, ao.vals, ao.n, out); }
        else callClosure(cl, args, 0, out);
    }
    else OBJ.setRef(out, OBJ.K_FUNC, cl as void*);          // bind: returned unbound (no bound-this yet)
    OBJ.copyVal(&curThis, &saved);
}

// Host entry point (events, timers, rAF): runs `fn` with up to two arguments. Errors stay inside this call.
function callFunction(fn: OBJ.Value*, a0: OBJ.Value*, a1: OBJ.Value*, argc: int) => void {
    ctl = 0; steps = 0; depth = 0; argTop = 0; tryDepth = 0;
    if (argc > 0) OBJ.copyVal(&argStack[0], a0);
    if (argc > 1) OBJ.copyVal(&argStack[1], a1);
    argTop = argc;
    let r: OBJ.Value;
    callValue(fn, argStack, argc, &r);
    ctl = 0;
}

// ---------- expressions ----------
function evalArgs(list: AST.Node*, sc: Scope*) => int {   // evaluates into argStack[base..]; returns count (base = argTop before the call)
    let cnt: int = 0;
    let t: AST.Node* = list;
    while (t != nullptr) { cnt = cnt + 1; t = t.next; }
    let base: int = argTop;
    if (base + cnt >= 512) { fail("too many arguments"); return 0; }
    argTop = base + cnt;                                  // reserve first: nested calls while evaluating use slots above
    let i: int = 0;
    t = list;
    while (t != nullptr) {
        let tmp: OBJ.Value;
        evalExpr(t, sc, &tmp);
        OBJ.copyVal(&argStack[base + i], &tmp);
        i = i + 1;
        t = t.next;
    }
    return cnt;
}

function globalLookup(name: char*, out: OBJ.Value*) => bool {
    if (BI.global(name, out)) return true;
    let w: OBJ.Value;
    OBJ.setRef(&w, OBJ.K_WINDOW, nullptr);
    return DOM.get(&w, name, out);                        // document, window, setTimeout, requestAnimationFrame, innerWidth...
}

function evalAssign(n: AST.Node*, sc: Scope*, out: OBJ.Value*) => void {
    let t: AST.Node* = n.a;
    let obj: OBJ.Value;
    let key: OBJ.Value;
    let cur: OBJ.Value;
    let rhs: OBJ.Value;
    OBJ.setUndef(&obj); OBJ.setUndef(&key); OBJ.setUndef(&cur);
    if (t.k == AST.N_MEMBER) evalExpr(t.a, sc, &obj);
    else if (t.k == AST.N_INDEX) { evalExpr(t.a, sc, &obj); evalExpr(t.b, sc, &key); }
    if (ctl == 4) return;
    evalExpr(n.b, sc, &rhs);
    if (ctl == 4) return;
    if (n.op != AST.OP_ASSIGN) {                          // compound: read, combine
        if (t.k == AST.N_IDENT) evalExpr(t, sc, &cur);
        else if (t.k == AST.N_MEMBER) getProp(&obj, t.s, &cur);
        else getIndex(&obj, &key, &cur);
        let res: OBJ.Value;
        binary(n.op, &cur, &rhs, &res);
        OBJ.copyVal(&rhs, &res);
    }
    if (t.k == AST.N_IDENT) assignVar(sc, t.s, &rhs);
    else if (t.k == AST.N_MEMBER) setProp(&obj, t.s, &rhs);
    else setIndex(&obj, &key, &rhs);
    OBJ.copyVal(out, &rhs);
}

function evalUpdate(n: AST.Node*, sc: Scope*, out: OBJ.Value*) => void {
    let t: AST.Node* = n.a;
    let obj: OBJ.Value;
    let key: OBJ.Value;
    let cur: OBJ.Value;
    OBJ.setUndef(&obj); OBJ.setUndef(&key);
    if (t.k == AST.N_IDENT) evalExpr(t, sc, &cur);
    else if (t.k == AST.N_MEMBER) { evalExpr(t.a, sc, &obj); getProp(&obj, t.s, &cur); }
    else if (t.k == AST.N_INDEX) { evalExpr(t.a, sc, &obj); evalExpr(t.b, sc, &key); getIndex(&obj, &key, &cur); }
    else { fail("invalid update target"); return; }
    let old: double = OBJ.toNum(&cur);
    let nv: double = old + 1.0;
    if (n.op == AST.OP_DEC) nv = old - 1.0;
    let v: OBJ.Value;
    OBJ.setNum(&v, nv);
    if (t.k == AST.N_IDENT) assignVar(sc, t.s, &v);
    else if (t.k == AST.N_MEMBER) setProp(&obj, t.s, &v);
    else setIndex(&obj, &key, &v);
    if (n.n != 0.0) OBJ.copyVal(out, &v); else OBJ.setNum(out, old);
}

function evalCall(n: AST.Node*, sc: Scope*, out: OBJ.Value*) => void {
    let callee: OBJ.Value;
    let recv: OBJ.Value;
    OBJ.setUndef(&recv);
    let hasRecv: bool = false;
    if (n.a.k == AST.N_MEMBER) {                          // obj.method(...): evaluate obj once, keep it as `this`
        evalExpr(n.a.a, sc, &recv);
        if (ctl == 4) return;
        if (n.a.op == 1 && OBJ.looseNullish(&recv)) return;
        OBJ.setUndef(&callee);
        getProp(&recv, n.a.s, &callee);
        hasRecv = true;
    }
    else evalExpr(n.a, sc, &callee);
    if (ctl == 4) return;
    let base: int = argTop;
    let cnt: int = evalArgs(n.b, sc);
    calleeName = "";
    if (n.a.k == AST.N_MEMBER || n.a.k == AST.N_IDENT) calleeName = n.a.s;
    if (ctl != 4) {
        let saved: OBJ.Value;
        OBJ.copyVal(&saved, &curThis);
        if (hasRecv) OBJ.copyVal(&curThis, &recv);
        callValue(&callee, &argStack[base], cnt, out);
        OBJ.copyVal(&curThis, &saved);
    }
    argTop = base;
}

function evalNew(n: AST.Node*, sc: Scope*, out: OBJ.Value*) => void {
    let callee: OBJ.Value;
    evalExpr(n.a, sc, &callee);
    if (ctl == 4) return;
    let base: int = argTop;
    let cnt: int = evalArgs(n.b, sc);
    if (ctl == 4) { argTop = base; return; }
    let o: OBJ.Obj* = OBJ.newObj(false);
    let fresh: OBJ.Value;
    OBJ.setRef(&fresh, OBJ.K_OBJECT, o as void*);
    if (callee.k == OBJ.K_FUNC) {
        let saved: OBJ.Value;
        OBJ.copyVal(&saved, &curThis);
        OBJ.copyVal(&curThis, &fresh);
        o.proto = funcProto(callee.p as Closure*);
        let ret: OBJ.Value;
        callValue(&callee, &argStack[base], cnt, &ret);
        OBJ.copyVal(&curThis, &saved);
        if (ret.k == OBJ.K_OBJECT || ret.k == OBJ.K_ARRAY) { OBJ.copyVal(out, &ret); argTop = base; return; }
    }
    else if (callee.k == OBJ.K_CTOR) { BI.construct(callee.s, &argStack[base], cnt, out); argTop = base; return; }
    else if (callee.k != OBJ.K_METHOD) { fail("value is not a constructor"); argTop = base; return; }
    OBJ.copyVal(out, &fresh);                               // native constructors (Date, Map...) are not implemented: an empty object
    argTop = base;
}

function evalExpr(n: AST.Node*, sc: Scope*, out: OBJ.Value*) => void {
    OBJ.setUndef(out);
    if (ctl == 4) return;
    curNode = n;
    let k: int = n.k;
    if (k == AST.N_IDENT) {
        let slot: OBJ.Value* = lookup(sc, n.s);
        if (slot == nullptr && U.streq(n.s, "this")) OBJ.copyVal(out, &curThis);
        else if (slot != nullptr) OBJ.copyVal(out, slot);
        else if (!globalLookup(n.s, out)) fail(U.cat(n.s, " is not defined"));
    }
    else if (k == AST.N_NUM) OBJ.setNum(out, n.n);
    else if (k == AST.N_STR) OBJ.setStr(out, n.s);
    else if (k == AST.N_MEMBER) {
        let obj: OBJ.Value;
        evalExpr(n.a, sc, &obj);
        if (n.op == 1 && OBJ.looseNullish(&obj)) return;           // a?.b
        if (ctl != 4) getProp(&obj, n.s, out);
    }
    else if (k == AST.N_CALL) evalCall(n, sc, out);
    else if (k == AST.N_ASSIGN) evalAssign(n, sc, out);
    else if (k == AST.N_BINARY) {
        let l: OBJ.Value; let r: OBJ.Value;
        evalExpr(n.a, sc, &l);
        evalExpr(n.b, sc, &r);
        if (ctl != 4) binary(n.op, &l, &r, out);
    }
    else if (k == AST.N_TPL) {
        let s: char* = "";
        let p: AST.Node* = n.a;
        while (p != nullptr) {
            let part: OBJ.Value;
            evalExpr(p, sc, &part);
            s = U.cat(s, OBJ.toStr(&part));
            p = p.next;
        }
        OBJ.setStr(out, s);
    }
    else if (k == AST.N_COND) {
        let c: OBJ.Value;
        evalExpr(n.a, sc, &c);
        if (OBJ.truthy(&c)) evalExpr(n.b, sc, out); else evalExpr(n.c, sc, out);
    }
    else if (k == AST.N_FUNC) makeFunc(n, sc, out);
    else if (k == AST.N_BOOL) OBJ.setBool(out, n.n != 0.0);
    else if (k == AST.N_NULL) OBJ.setNull(out);
    else if (k == AST.N_UNDEF) { }
    else if (k == AST.N_LOGICAL) {
        evalExpr(n.a, sc, out);
        if (n.op == AST.OP_AND) { if (OBJ.truthy(out)) evalExpr(n.b, sc, out); }
        else if (n.op == AST.OP_OR) { if (!OBJ.truthy(out)) evalExpr(n.b, sc, out); }
        else { if (OBJ.looseNullish(out)) evalExpr(n.b, sc, out); }
    }
    else if (k == AST.N_UNARY) {
        let v: OBJ.Value;
        if (n.op == AST.OP_TYPEOF && n.a.k == AST.N_IDENT && lookup(sc, n.a.s) == nullptr && !globalLookup(n.a.s, &v)) { OBJ.setStr(out, "undefined"); return; }
        evalExpr(n.a, sc, &v);
        if (n.op == AST.OP_NOT) OBJ.setBool(out, !OBJ.truthy(&v));
        else if (n.op == AST.OP_NEG) OBJ.setNum(out, 0.0 - OBJ.toNum(&v));
        else if (n.op == AST.OP_POS) OBJ.setNum(out, OBJ.toNum(&v));
        else if (n.op == AST.OP_BNOT) OBJ.setNum(out, 0.0 - toI32(OBJ.toNum(&v)) - 1.0);
        else if (n.op == AST.OP_VOID) OBJ.setUndef(out);
        else OBJ.setStr(out, OBJ.typeOf(&v));
    }
    else if (k == AST.N_UPDATE) evalUpdate(n, sc, out);
    else if (k == AST.N_SEQ) { let skipv: OBJ.Value; evalExpr(n.a, sc, &skipv); evalExpr(n.b, sc, out); }
    else if (k == AST.N_INDEX) {
        let obj: OBJ.Value; let key: OBJ.Value;
        evalExpr(n.a, sc, &obj);
        evalExpr(n.b, sc, &key);
        if (ctl != 4) getIndex(&obj, &key, out);
    }
    else if (k == AST.N_ARRAY) {
        let arr: OBJ.Obj* = OBJ.newObj(true);
        let e: AST.Node* = n.a;
        while (e != nullptr) { let v: OBJ.Value; evalExpr(e, sc, &v); OBJ.arrPush(arr, &v); e = e.next; }
        OBJ.setRef(out, OBJ.K_ARRAY, arr as void*);
    }
    else if (k == AST.N_NEW) evalNew(n, sc, out);
    else if (k == AST.N_REGEX) {                              // no regex engine yet: an object carrying .source
        let ro: OBJ.Obj* = OBJ.newObj(false);
        let sv: OBJ.Value;
        OBJ.setStr(&sv, n.s);
        OBJ.objSet(ro, "source", &sv);
        OBJ.setRef(out, OBJ.K_OBJECT, ro as void*);
    }
    else if (k == AST.N_OBJECT) {
        let o: OBJ.Obj* = OBJ.newObj(false);
        let pr: AST.Node* = n.a;
        while (pr != nullptr) { let v: OBJ.Value; evalExpr(pr.a, sc, &v); OBJ.objSet(o, pr.s, &v); pr = pr.next; }
        OBJ.setRef(out, OBJ.K_OBJECT, o as void*);
    }
    else fail("unsupported expression");
}

// ---------- statements ----------
function hoist(list: AST.Node*, sc: Scope*) => void {      // function declarations are visible in the whole block
    let s: AST.Node* = list;
    while (s != nullptr) {
        if (s.k == AST.N_FUNCDECL) { let f: OBJ.Value; makeFunc(s.a, sc, &f); declare(sc, s.s, &f); }
        s = s.next;
    }
}

function execList(list: AST.Node*, sc: Scope*) => void {
    hoist(list, sc);
    let s: AST.Node* = list;
    while (s != nullptr && ctl == 0) { execStmt(s, sc); s = s.next; }
}

function loopBody(body: AST.Node*, sc: Scope*) => bool {   // runs one iteration; false = leave the loop
    steps = steps + 1;
    if (steps > STEP_LIMIT) { fail("script ran too long"); return false; }
    execStmt(body, sc);
    if (ctl == 1) { ctl = 0; return false; }
    if (ctl == 2) ctl = 0;
    return ctl == 0;
}

// for (x of arr|str) / for (k in obj|arr): a fresh scope per iteration, so closures capture each value
function forEachStep(n: AST.Node*, sc: Scope*, v: OBJ.Value*) => bool {
    let iter: Scope* = newScope(sc);
    if (n.n != 0.0) declare(iter, n.s, v); else assignVar(sc, n.s, v);
    return loopBody(n.d, iter);
}

function execForEach(n: AST.Node*, sc: Scope*) => void {
    let src: OBJ.Value;
    evalExpr(n.a, sc, &src);
    if (ctl != 0) return;
    let go: bool = true;
    let i: int = 0;
    if (n.op == 1) {                                         // of
        if (src.k == OBJ.K_ARRAY) {
            let o: OBJ.Obj* = src.p as OBJ.Obj*;
            while (go && ctl == 0 && i < o.n) { let it: OBJ.Value; OBJ.copyVal(&it, &o.vals[i]); go = forEachStep(n, sc, &it); i = i + 1; }
        }
        else if (src.k == OBJ.K_STR) {
            while (go && ctl == 0 && i < U.slen(src.s)) { let ch: OBJ.Value; OBJ.setStr(&ch, U.sub(src.s, i, i + 1)); go = forEachStep(n, sc, &ch); i = i + 1; }
        }
        else if (!OBJ.looseNullish(&src)) fail("value is not iterable");
        else fail("cannot iterate undefined or null");
    }
    else {                                                   // in
        if (src.k == OBJ.K_OBJECT) {
            let o2: OBJ.Obj* = src.p as OBJ.Obj*;
            while (go && ctl == 0 && i < o2.n) {
                if (o2.keys[i] != nullptr && o2.vals[i].k != OBJ.K_UNDEF) { let kv: OBJ.Value; OBJ.setStr(&kv, o2.keys[i]); go = forEachStep(n, sc, &kv); }
                i = i + 1;
            }
        }
        else if (src.k == OBJ.K_ARRAY) {
            let o3: OBJ.Obj* = src.p as OBJ.Obj*;
            while (go && ctl == 0 && i < o3.n) { let iv: OBJ.Value; OBJ.setStr(&iv, U.fromInt(i)); go = forEachStep(n, sc, &iv); i = i + 1; }
        }
    }
}

function execStmt(n: AST.Node*, sc: Scope*) => void {
    curNode = n;
    let k: int = n.k;
    if (k == AST.N_EXPR) { let v: OBJ.Value; evalExpr(n.a, sc, &v); }
    else if (k == AST.N_VAR) {
        let d: AST.Node* = n.a;
        while (d != nullptr && ctl == 0) {
            let v: OBJ.Value;
            OBJ.setUndef(&v);
            if (d.a != nullptr) evalExpr(d.a, sc, &v);
            if (ctl == 0) declare(sc, d.s, &v);
            d = d.next;
        }
    }
    else if (k == AST.N_BLOCK) { let inner: Scope* = newScope(sc); execList(n.a, inner); }
    else if (k == AST.N_IF) {
        let c: OBJ.Value;
        evalExpr(n.a, sc, &c);
        if (ctl == 0) { if (OBJ.truthy(&c)) execStmt(n.b, sc); else if (n.c != nullptr) execStmt(n.c, sc); }
    }
    else if (k == AST.N_RETURN) {
        let v: OBJ.Value;
        OBJ.setUndef(&v);
        if (n.a != nullptr) evalExpr(n.a, sc, &v);
        if (ctl == 0) { OBJ.copyVal(&retv, &v); ctl = 3; }
    }
    else if (k == AST.N_WHILE) {
        let go: bool = true;
        while (go && ctl == 0) {
            let c: OBJ.Value;
            evalExpr(n.a, sc, &c);
            if (ctl == 0 && OBJ.truthy(&c)) go = loopBody(n.b, sc); else go = false;
        }
    }
    else if (k == AST.N_FOR) {
        let ls: Scope* = newScope(sc);
        if (n.a != nullptr) execStmt(n.a, ls);
        let go: bool = true;
        while (go && ctl == 0) {
            let ok: bool = true;
            if (n.b != nullptr) { let c: OBJ.Value; evalExpr(n.b, ls, &c); ok = ctl == 0 && OBJ.truthy(&c); }
            if (!ok) go = false;
            else {
                go = loopBody(n.d, ls);
                if (go && n.c != nullptr) { let u: OBJ.Value; evalExpr(n.c, ls, &u); }
            }
        }
    }
    else if (k == AST.N_FOREACH) execForEach(n, sc);
    else if (k == AST.N_THROW) {
        let tv: OBJ.Value;
        evalExpr(n.a, sc, &tv);
        if (ctl == 0) {
            if (tryDepth == 0) Dbg.error(U.cat("js: uncaught ", OBJ.toStr(&tv)));
            OBJ.copyVal(&thrown, &tv);
            ctl = 4;
        }
    }
    else if (k == AST.N_TRY) {
        tryDepth = tryDepth + 1;
        execStmt(n.a, sc);
        tryDepth = tryDepth - 1;
        if (ctl == 4 && n.b != nullptr) {
            ctl = 0;
            let cs: Scope* = newScope(sc);
            if (U.slen(n.s) > 0) declare(cs, n.s, &thrown);
            execStmt(n.b, cs);
        }
        if (n.c != nullptr) {
            let savedCtl: int = ctl;
            let savedRet: OBJ.Value;
            OBJ.copyVal(&savedRet, &retv);
            ctl = 0;
            execStmt(n.c, sc);
            if (ctl == 0) { ctl = savedCtl; OBJ.copyVal(&retv, &savedRet); }
        }
    }
    else if (k == AST.N_BREAK) ctl = 1;
    else if (k == AST.N_CONTINUE) ctl = 2;
    // N_FUNCDECL was hoisted; N_EMPTY does nothing
}

// ---------- program entry ----------
// Parses and runs `src` in the page's global scope. A syntax error keeps the statements before it; a runtime
// error stops this script only.
function runSource(src: char*) => void {
    let prog: AST.Node* = P.parse(src);
    ctl = 0; steps = 0; depth = 0; argTop = 0; tryDepth = 0;
    hoist(prog.a, globalScope);
    let s: AST.Node* = prog.a;
    while (s != nullptr) {                                // each top-level statement is isolated: an error ends that statement only
        ctl = 0; steps = 0; depth = 0; argTop = 0;
        execStmt(s, globalScope);
        s = s.next;
    }
    ctl = 0;
}
