// browser/js/js.os  -- TIER 6+7: tiny JS engine + DOM/console/timer bindings.
// Design: lexer -> token array; a token-walking interpreter (parses and evaluates in one pass; `skip` > 0 means
// "parse only, no side effects"). Functions are stored as token indices. Bindings are the HOST section at the bottom.
// Supported: let/const/var, numbers, strings, booleans, + - * / < > <= >= == != && ||, =, +=, function decls + calls,
//   if/else, while, return, console.log, document.getElementById, el.textContent, el.style.<prop>, el.addEventListener,
//   setTimeout.
// NEXT: real AST, closures/scopes, objects/arrays, for loops, ++/--, arrow functions, prototypes, exceptions,
//       split HOST section into js/bindings.os once Value is in its own module.
module U = import "browser/util.os";
module UI = import "standard/1/Ui.os";
module A = import "browser/uiapi.os";
module Console = import "standard/1/Console.os";
module Dbg = import "browser/dbg.os";

// ---------- values ----------
// k: 0 undefined, 1 number, 2 string, 3 bool, 4 user function, 5 element, 6 style object, 7 document,
//    8 console, 9 null, 10 builtin function, 11 bound method
struct Value {
    k: int = 0;
    n: double = 0.0;
    s: char* = nullptr;
    p: void* = nullptr;
}

function setUndef(v: Value*) => void { v.k = 0; v.n = 0.0; v.s = nullptr; v.p = nullptr; }
function setNum(v: Value*, n: double) => void { v.k = 1; v.n = n; }
function setStr(v: Value*, s: char*) => void { v.k = 2; v.s = s; }
function setBool(v: Value*, b: bool) => void { v.k = 3; v.n = 0.0; if (b) v.n = 1.0; }
function copyVal(dst: Value*, src: Value*) => void { dst.k = src.k; dst.n = src.n; dst.s = src.s; dst.p = src.p; }

function truthy(v: Value*) => bool {
    let r: bool = true;
    if (v.k == 0 || v.k == 9) r = false;
    else if (v.k == 1 || v.k == 3) r = v.n != 0.0;
    else if (v.k == 2) r = (v.s[0] as int) != 0;
    return r;
}

function toStr(v: Value*) => char* {
    if (v.k == 2) return v.s;
    if (v.k == 1) return U.fromNum(v.n);
    if (v.k == 3) { if (v.n != 0.0) return "true"; return "false"; }
    if (v.k == 0) return "undefined";
    if (v.k == 9) return "null";
    return "[object]";
}

// ---------- lexer ----------
struct Tok {
    t: int = 0;                  // 0 eof, 1 num, 2 str, 3 id, 4 punct
    s: char* = nullptr;
    n: double = 0.0;
}

let toks: Tok* = nullptr;
let ntok: int = 0;
let pos: int = 0;
let skip: int = 0;
let halted: bool = false;
let dirty: bool = false;
let docRoot: UI.Node* = nullptr;

function err(msg: char*) => void {
    if (!halted) { Dbg.error(U.cat("js: ", msg)); }
    halted = true;
    pos = ntok - 1;
}

function addTok(t: int, s: char*, n: double) => void {
    toks[ntok].t = t; toks[ntok].s = s; toks[ntok].n = n;
    ntok = ntok + 1;
}

function lex(src: char*) => void {
    let n: int = U.slen(src);
    toks = U.alloc(sizeof(Tok) * (n + 2)) as Tok*;
    ntok = 0;
    let i: int = 0;
    while (i < n) {
        let c: int = src[i] as int;
        if (U.isSpace(c)) { i = i + 1; }
        else if (c == 47 && (src[i + 1] as int) == 47) {                    // // comment
            while (i < n && (src[i] as int) != 10) { i = i + 1; }
        }
        else if (U.isDigit(c)) {
            let a: int = i;
            while (i < n && (U.isDigit(src[i] as int) || (src[i] as int) == 46)) { i = i + 1; }
            addTok(1, "", U.toNum(U.sub(src, a, i)));
        }
        else if (U.isAlpha(c)) {
            let a: int = i;
            while (i < n && U.isAlnum(src[i] as int)) { i = i + 1; }
            addTok(3, U.sub(src, a, i), 0.0);
        }
        else if (c == 34 || c == 39) {
            let a: int = i + 1;
            i = a;
            while (i < n && (src[i] as int) != c) { i = i + 1; }
            addTok(2, U.sub(src, a, i), 0.0);                                // NEXT: escapes
            i = i + 1;
        }
        else {
            let two: char* = U.sub(src, i, i + 2);
            let three: char* = U.sub(src, i, i + 3);
            if (U.streq(three, "===")) { addTok(4, "==", 0.0); i = i + 3; }
            else if (U.streq(three, "!==")) { addTok(4, "!=", 0.0); i = i + 3; }
            else if (U.streq(two, "==") || U.streq(two, "!=") || U.streq(two, "<=") || U.streq(two, ">=") || U.streq(two, "&&") || U.streq(two, "||") || U.streq(two, "+=")) {
                addTok(4, two, 0.0); i = i + 2;
            }
            else { addTok(4, U.sub(src, i, i + 1), 0.0); i = i + 1; }
        }
    }
    addTok(0, "", 0.0);
}

function isP(s: char*) => bool { return toks[pos].t == 4 && U.streq(toks[pos].s, s); }
function isKw(s: char*) => bool { return toks[pos].t == 3 && U.streq(toks[pos].s, s); }
function eatP(s: char*) => bool { if (isP(s)) { pos = pos + 1; return true; } return false; }
function expectP(s: char*) => void { if (!eatP(s)) err(U.cat("expected ", s)); }

// ---------- variables, functions ----------
let vNames: char** = nullptr;
let vVals: Value* = nullptr;
let vTop: int = 0;
let fBase: int = 0;
let gCount: int = 0;

let fnName: char** = nullptr;
let fnParams: int* = nullptr;
let fnBody: int* = nullptr;
let fnCount: int = 0;

let argStack: Value* = nullptr;
let argTop: int = 0;
let returning: bool = false;
let retVal: Value;

// timers
let tFn: int* = nullptr;
let tDue: double* = nullptr;
let tCount: int = 0;

function initEngine() => void {
    vNames = U.alloc(8 * 512) as char**;
    vVals = U.alloc(sizeof(Value) * 512) as Value*;
    fnName = U.alloc(8 * 64) as char**;
    fnParams = U.alloc(4 * 64) as int*;
    fnBody = U.alloc(4 * 64) as int*;
    argStack = U.alloc(sizeof(Value) * 256) as Value*;
    tFn = U.alloc(4 * 16) as int*;
    tDue = U.alloc(8 * 16) as double*;
    hNodes = U.alloc(8 * 64) as UI.Node**;
    hFns = U.alloc(4 * 64) as int*;
    hCount = 0;
    vTop = 0; fBase = 0; gCount = 0; fnCount = 0; argTop = 0; tCount = 0;
}

function findVar(name: char*) => int {
    let i: int = vTop - 1;
    let found: int = 0 - 1;
    while (i >= fBase && found < 0) { if (U.streq(vNames[i], name)) found = i; i = i - 1; }
    if (found < 0 && fBase > 0) {
        i = gCount - 1;
        while (i >= 0 && found < 0) { if (U.streq(vNames[i], name)) found = i; i = i - 1; }
    }
    return found;
}

function declVar(name: char*, v: Value*) => void {
    let i: int = findVar(name);
    if (i >= fBase && i >= 0 && fBase == 0) { copyVal(&vVals[i], v); }
    else if (vTop < 512) {
        vNames[vTop] = name;
        copyVal(&vVals[vTop], v);
        vTop = vTop + 1;
        if (fBase == 0) gCount = vTop;
    }
}

function findFn(name: char*) => int {
    let i: int = 0;
    let found: int = 0 - 1;
    while (i < fnCount) { if (U.streq(fnName[i], name)) found = i; i = i + 1; }
    return found;
}

// ---------- expression parser/evaluator ----------
let lvKind: int = 0;             // 0 none, 1 variable, 2 property
let lvName: char* = nullptr;
let lvObj: Value;

function pPrimary(out: Value*) => void {
    setUndef(out);
    lvKind = 0;
    let t: Tok* = &toks[pos];
    if (t.t == 1) { setNum(out, t.n); pos = pos + 1; }
    else if (t.t == 2) { setStr(out, t.s); pos = pos + 1; }
    else if (t.t == 3) {
        pos = pos + 1;
        let name: char* = t.s;
        lvKind = 1; lvName = name;
        if (U.streq(name, "true")) { setBool(out, true); lvKind = 0; }
        else if (U.streq(name, "false")) { setBool(out, false); lvKind = 0; }
        else if (U.streq(name, "null")) { out.k = 9; lvKind = 0; }
        else if (U.streq(name, "undefined")) { lvKind = 0; }
        else if (skip == 0) {
            let vi: int = findVar(name);
            if (vi >= 0) copyVal(out, &vVals[vi]);
            else {
                let fi: int = findFn(name);
                if (fi >= 0) { out.k = 4; out.n = fi as double; }
                else if (U.streq(name, "document")) out.k = 7;
                else if (U.streq(name, "console")) out.k = 8;
                else if (U.streq(name, "setTimeout")) { out.k = 10; out.n = 1.0; }
            }
        }
    }
    else if (isP("(")) { pos = pos + 1; pAssign(out); expectP(")"); lvKind = 0; }
    else { err(U.cat("unexpected token ", t.s)); }
}

function pPostfix(out: Value*) => void {
    pPrimary(out);
    let more: bool = true;
    while (more && !halted) {
        if (isP(".")) {
            pos = pos + 1;
            let name: char* = toks[pos].s;
            pos = pos + 1;
            let obj: Value;
            copyVal(&obj, out);
            copyVal(&lvObj, &obj);
            lvKind = 2; lvName = name;
            if (skip == 0) hostGet(&obj, name, out); else setUndef(out);
        }
        else if (isP("(")) {
            pos = pos + 1;
            let base: int = argTop;
            let cnt: int = 0;
            while (!isP(")") && !halted && toks[pos].t != 0) {
                let slot: int = argTop;
                argTop = argTop + 1;
                pAssign(&argStack[slot]);
                cnt = cnt + 1;
                if (!eatP(",")) { }
            }
            expectP(")");
            let callee: Value;
            copyVal(&callee, out);
            setUndef(out);
            if (skip == 0) callValue(&callee, base, cnt, out);
            argTop = base;
            lvKind = 0;
        }
        else { more = false; }
    }
}

function pUnary(out: Value*) => void {
    if (isP("!")) { pos = pos + 1; pUnary(out); if (skip == 0) setBool(out, !truthy(out)); lvKind = 0; }
    else if (isP("-")) { pos = pos + 1; pUnary(out); if (skip == 0) setNum(out, 0.0 - out.n); lvKind = 0; }
    else pPostfix(out);
}

function pMul(out: Value*) => void {
    pUnary(out);
    while ((isP("*") || isP("/")) && !halted) {
        let mul: bool = isP("*");
        pos = pos + 1;
        let r: Value;
        pUnary(&r);
        if (skip == 0) { if (mul) setNum(out, out.n * r.n); else setNum(out, out.n / r.n); }
        lvKind = 0;
    }
}

function addVals(out: Value*, r: Value*) => void {
    if (out.k == 2 || r.k == 2) setStr(out, U.cat(toStr(out), toStr(r)));
    else setNum(out, out.n + r.n);
}

function pAdd(out: Value*) => void {
    pMul(out);
    while ((isP("+") || isP("-")) && !halted) {
        let plus: bool = isP("+");
        pos = pos + 1;
        let r: Value;
        pMul(&r);
        if (skip == 0) { if (plus) addVals(out, &r); else setNum(out, out.n - r.n); }
        lvKind = 0;
    }
}

function pRel(out: Value*) => void {
    pAdd(out);
    while ((isP("<") || isP(">") || isP("<=") || isP(">=")) && !halted) {
        let op: char* = toks[pos].s;
        pos = pos + 1;
        let r: Value;
        pAdd(&r);
        if (skip == 0) {
            let res: bool = false;
            if (U.streq(op, "<")) res = out.n < r.n;
            else if (U.streq(op, ">")) res = out.n > r.n;
            else if (U.streq(op, "<=")) res = out.n <= r.n;
            else res = out.n >= r.n;
            setBool(out, res);
        }
        lvKind = 0;
    }
}

function valsEqual(a: Value*, b: Value*) => bool {
    if (a.k != b.k) return false;
    if (a.k == 2) return U.streq(a.s, b.s);
    if (a.k == 1 || a.k == 3) return a.n == b.n;
    return a.p == b.p;
}

function pEq(out: Value*) => void {
    pRel(out);
    while ((isP("==") || isP("!=")) && !halted) {
        let eq: bool = isP("==");
        pos = pos + 1;
        let r: Value;
        pRel(&r);
        if (skip == 0) { let same: bool = valsEqual(out, &r); if (eq) setBool(out, same); else setBool(out, !same); }
        lvKind = 0;
    }
}

function pAnd(out: Value*) => void {
    pEq(out);
    while (isP("&&") && !halted) {
        pos = pos + 1;
        let r: Value;
        pEq(&r);
        if (skip == 0) setBool(out, truthy(out) && truthy(&r));
        lvKind = 0;
    }
}

function pOr(out: Value*) => void {
    pAnd(out);
    while (isP("||") && !halted) {
        pos = pos + 1;
        let r: Value;
        pAnd(&r);
        if (skip == 0) setBool(out, truthy(out) || truthy(&r));
        lvKind = 0;
    }
}

function pAssign(out: Value*) => void {
    pOr(out);
    if ((isP("=") || isP("+=")) && lvKind != 0 && !halted) {
        let compound: bool = isP("+=");
        let kind: int = lvKind;
        let name: char* = lvName;
        let obj: Value;
        copyVal(&obj, &lvObj);
        pos = pos + 1;
        let r: Value;
        pAssign(&r);
        if (skip == 0) {
            if (compound) { let cur: Value; copyVal(&cur, out); addVals(&cur, &r); copyVal(&r, &cur); }
            if (kind == 1) {
                let vi: int = findVar(name);
                if (vi >= 0) copyVal(&vVals[vi], &r); else declVar(name, &r);
            } else hostSet(&obj, name, &r);
            copyVal(out, &r);
        }
        lvKind = 0;
    }
}

// ---------- statements ----------
function skipBlock() => void {
    // pos is at '{'
    let depth: int = 0;
    let more: bool = true;
    while (more && toks[pos].t != 0) {
        if (isP("{")) depth = depth + 1;
        if (isP("}")) { depth = depth - 1; if (depth == 0) more = false; }
        pos = pos + 1;
    }
}

function execBlock() => void {
    expectP("{");
    while (!isP("}") && toks[pos].t != 0 && !halted && !returning) { execStmt(); }
    if (returning && skip == 0) { skip = skip + 1; while (!isP("}") && toks[pos].t != 0) { execStmt(); } skip = skip - 1; }
    expectP("}");
}

function execStmt() => void {
    if (isP("{")) { execBlock(); }
    else if (isP(";")) { pos = pos + 1; }
    else if (isKw("let") || isKw("const") || isKw("var")) {
        pos = pos + 1;
        let name: char* = toks[pos].s;
        pos = pos + 1;
        let v: Value;
        setUndef(&v);
        if (eatP("=")) pAssign(&v);
        if (skip == 0) declVar(name, &v);
        eatP(";");
    }
    else if (isKw("function")) {
        pos = pos + 1;
        let name: char* = toks[pos].s;
        pos = pos + 1;
        expectP("(");
        let ps: int = pos;
        while (!isP(")") && toks[pos].t != 0) { pos = pos + 1; }
        expectP(")");
        if (skip == 0 && fnCount < 64) {
            fnName[fnCount] = name; fnParams[fnCount] = ps; fnBody[fnCount] = pos; fnCount = fnCount + 1;
        }
        skipBlock();
    }
    else if (isKw("if")) {
        pos = pos + 1;
        expectP("(");
        let c: Value;
        pAssign(&c);
        expectP(")");
        let take: bool = false;
        if (skip == 0) take = truthy(&c);
        if (!take) skip = skip + 1;
        execStmt();
        if (!take) skip = skip - 1;
        if (isKw("else")) {
            pos = pos + 1;
            if (take) skip = skip + 1;
            execStmt();
            if (take) skip = skip - 1;
        }
    }
    else if (isKw("while")) {
        let start: int = pos;
        let guard: int = 0;
        let go: bool = true;
        while (go && guard < 10000) {
            pos = start + 1;
            expectP("(");
            let c: Value;
            pAssign(&c);
            expectP(")");
            if (skip == 0 && truthy(&c) && !halted && !returning) { execStmt(); guard = guard + 1; }
            else { skip = skip + 1; execStmt(); skip = skip - 1; go = false; }
        }
    }
    else if (isKw("return")) {
        pos = pos + 1;
        let v: Value;
        setUndef(&v);
        if (!isP(";") && !isP("}")) pAssign(&v);
        if (skip == 0) { copyVal(&retVal, &v); returning = true; }
        eatP(";");
    }
    else {
        let v: Value;
        pAssign(&v);
        eatP(";");
    }
}

function callUser(idx: int, base: int, cnt: int, out: Value*) => void {
    let savePos: int = pos;
    let saveBase: int = fBase;
    fBase = vTop;
    let p: int = fnParams[idx];
    let i: int = 0;
    while (toks[p].t == 3) {
        let a: Value;
        setUndef(&a);
        if (i < cnt) copyVal(&a, &argStack[base + i]);
        declVar(toks[p].s, &a);
        i = i + 1; p = p + 1;
        if (toks[p].t == 4 && U.streq(toks[p].s, ",")) p = p + 1;
    }
    pos = fnBody[idx];
    returning = false;
    execBlock();
    if (returning) copyVal(out, &retVal); else setUndef(out);
    returning = false;
    vTop = fBase;
    fBase = saveBase;
    pos = savePos;
}

function callValue(callee: Value*, base: int, cnt: int, out: Value*) => void {
    if (callee.k == 4) callUser(callee.n as int, base, cnt, out);
    else if (callee.k == 10) {
        // setTimeout(fn, ms)
        if (tCount < 16 && cnt >= 1) {
            tFn[tCount] = argStack[base].n as int;
            let ms: double = 0.0;
            if (cnt >= 2) ms = argStack[base + 1].n;
            tDue[tCount] = nowMs + ms;
            tCount = tCount + 1;
        }
    }
    else if (callee.k == 11) hostCall(callee, base, cnt, out);
    else err("not a function");
}

// ---------- HOST BINDINGS (document / element / style / console), on top of Ui nodes ----------
let nowMs: double = 0.0;
let hNodes: UI.Node** = nullptr;      // click listeners: node + user-function index
let hFns: int* = nullptr;
let hCount: int = 0;

function reset() => void { hCount = 0; tCount = 0; }

function methodVal(out: Value*, recv: Value*, name: char*) => void {
    out.k = 11; out.s = name; out.p = recv.p; out.n = recv.k as double;
}

function hostGet(obj: Value*, name: char*, out: Value*) => void {
    setUndef(out);
    if (obj.k == 7 || obj.k == 8) { methodVal(out, obj, name); }
    else if (obj.k == 5) {
        let nd: UI.Node* = obj.p as UI.Node*;
        if (U.streq(name, "textContent") || U.streq(name, "innerText")) setStr(out, A.textOf(nd));
        else if (U.streq(name, "id")) setStr(out, A.attr(nd, "id"));
        else if (U.streq(name, "className")) setStr(out, A.attr(nd, "class"));
        else if (U.streq(name, "value")) { let v: char* = nd.getValue(); if (v == nullptr) v = ""; setStr(out, v); }
        else if (U.streq(name, "checked")) setBool(out, nd.checked);
        else if (U.streq(name, "style")) { out.k = 6; out.p = obj.p; }
        else methodVal(out, obj, name);                                     // addEventListener, ...
    }
    else if (obj.k == 6) { setStr(out, ""); }                               // NEXT: read computed style
}

function hostSet(obj: Value*, name: char*, v: Value*) => void {
    if (obj.k == 5) {
        let nd: UI.Node* = obj.p as UI.Node*;
        if (U.streq(name, "textContent") || U.streq(name, "innerText") || U.streq(name, "innerHTML")) nd.setText(toStr(v));   // NEXT: innerHTML should parse HTML
        else if (U.streq(name, "className")) nd.setAttr("class", toStr(v));
        else if (U.streq(name, "id")) nd.setAttr("id", toStr(v));
        else if (U.streq(name, "value")) nd.setValue(toStr(v));
        else if (U.streq(name, "checked")) nd.setChecked(truthy(v));
    }
    else if (obj.k == 6) {
        let nd: UI.Node* = obj.p as UI.Node*;
        let n: int = U.slen(name);
        let kebab: char* = U.newStr(n * 2);                                 // backgroundColor -> background-color
        let i: int = 0;
        let p: int = 0;
        while (i < n) {
            let c: int = name[i] as int;
            if (c >= 65 && c <= 90) { kebab[p] = 45 as char; p = p + 1; kebab[p] = (c + 32) as char; p = p + 1; }
            else { kebab[p] = c as char; p = p + 1; }
            i = i + 1;
        }
        let val: char* = toStr(v);
        if (v.k == 1) val = U.cat(val, "px");                               // el.style.width = 100  (lenient)
        nd.style(U.cat(kebab, U.cat(":", U.cat(val, ";"))));                // Ui appends inline declarations
    }
}

function hostCall(m: Value*, base: int, cnt: int, out: Value*) => void {
    setUndef(out);
    let recvKind: int = m.n as int;
    if (recvKind == 8 && U.streq(m.s, "log")) {
        let line: char* = "";
        let i: int = 0;
        while (i < cnt) {
            if (i > 0) line = U.cat(line, " ");
            line = U.cat(line, toStr(&argStack[base + i]));
            i = i + 1;
        }
        Dbg.log(line);
    }
    else if (recvKind == 7 && U.streq(m.s, "getElementById") && cnt >= 1) {
        let nd: UI.Node* = nullptr;
        if (docRoot != nullptr) nd = docRoot.findId(toStr(&argStack[base]));
        if (nd == nullptr) out.k = 9; else { out.k = 5; out.p = nd as void*; }
    }
    else if (recvKind == 5 && U.streq(m.s, "addEventListener") && cnt >= 2) {
        if (U.streq(toStr(&argStack[base]), "click") && argStack[base + 1].k == 4 && hCount < 64) {
            hNodes[hCount] = m.p as UI.Node*;
            hFns[hCount] = argStack[base + 1].n as int;
            hCount = hCount + 1;
        }
    }
}

// ---------- public API ----------
function run(root: UI.Node*, src: char*) => void {
    docRoot = root;
    initEngine();
    halted = false;
    skip = 0;
    lex(src);
    pos = 0;
    while (toks[pos].t != 0 && !halted) { execStmt(); }
}

// Ui sets `clicked` on the clicked node and every ancestor for one frame, so listeners bubble for free.
// Call right after ui.frame() when a click event was seen.
function pumpClicks(ms: double) => void {
    nowMs = ms;
    let i: int = 0;
    while (i < hCount) {
        if (hNodes[i].clicked && !halted) {
            let r: Value;
            let saved: int = pos;
            skip = 0;
            callUser(hFns[i], 0, 0, &r);
            pos = saved;
        }
        i = i + 1;
    }
}

function tick(ms: double) => void {
    nowMs = ms;
    let i: int = 0;
    while (i < tCount) {
        if (tFn[i] >= 0 && tDue[i] <= ms && !halted) {
            let fi: int = tFn[i];
            tFn[i] = 0 - 1;
            let r: Value;
            callUser(fi, 0, 0, &r);
        }
        i = i + 1;
    }
}
