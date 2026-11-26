// browser/js/eventloop.os  -- task queues (V8 itself has none; this is the embedder's job): listeners, timers, rAF.
// Stores callbacks only. The facade (js.os) pops them and runs them through the interpreter.
module UI = import "standard/1/Ui.os";
module OBJ = import "js/objects.os";
module U = import "util.os";

let T_WINDOW: int = 0;
let T_DOCUMENT: int = 1;
let T_ELEMENT: int = 2;

let L_MAX: int = 64;
let Q_MAX: int = 32;

let lTarget: int* = nullptr;
let lNode: UI.Node** = nullptr;
let lType: char** = nullptr;
let lFn: OBJ.Value* = nullptr;
let lN: int = 0;

let rFn: OBJ.Value* = nullptr;      // requestAnimationFrame
let rId: int* = nullptr;
let rN: int = 0;

let tFn: OBJ.Value* = nullptr;      // setTimeout
let tDue: double* = nullptr;
let tId: int* = nullptr;
let tN: int = 0;

let nextId: int = 1;
let nowMs: double = 0.0;

function reset() => void {
    lTarget = U.alloc(4 * 64) as int*;
    lNode = U.alloc(8 * 64) as UI.Node**;
    lType = U.alloc(8 * 64) as char**;
    lFn = U.alloc(sizeof(OBJ.Value) * 64) as OBJ.Value*;
    rFn = U.alloc(sizeof(OBJ.Value) * 32) as OBJ.Value*;
    rId = U.alloc(4 * 32) as int*;
    tFn = U.alloc(sizeof(OBJ.Value) * 32) as OBJ.Value*;
    tDue = U.alloc(8 * 32) as double*;
    tId = U.alloc(4 * 32) as int*;
    lN = 0; rN = 0; tN = 0; nextId = 1;
}

function setNow(ms: double) => void { nowMs = ms; }
function now() => double { return nowMs; }

function addListener(target: int, node: UI.Node*, type: char*, fn: OBJ.Value*) => void {
    if (lN >= L_MAX || fn.k != OBJ.K_FUNC) return;
    lTarget[lN] = target; lNode[lN] = node; lType[lN] = U.dup(type);
    OBJ.copyVal(&lFn[lN], fn);
    lN = lN + 1;
}

function addRaf(fn: OBJ.Value*) => int {
    if (rN >= Q_MAX || fn.k != OBJ.K_FUNC) return 0;
    let id: int = nextId; nextId = nextId + 1;
    OBJ.copyVal(&rFn[rN], fn); rId[rN] = id; rN = rN + 1;
    return id;
}

function addTimer(fn: OBJ.Value*, ms: double) => int {
    if (tN >= Q_MAX || fn.k != OBJ.K_FUNC) return 0;
    let id: int = nextId; nextId = nextId + 1;
    OBJ.copyVal(&tFn[tN], fn); tDue[tN] = nowMs + ms; tId[tN] = id; tN = tN + 1;
    return id;
}

function cancel(id: int) => void {                       // cancelAnimationFrame / clearTimeout
    let i: int = 0;
    while (i < rN) { if (rId[i] == id) rFn[i].k = OBJ.K_UNDEF; i = i + 1; }
    i = 0;
    while (i < tN) { if (tId[i] == id) tFn[i].k = OBJ.K_UNDEF; i = i + 1; }
}

// listeners
function listenerCount() => int { return lN; }
function listenerTarget(i: int) => int { return lTarget[i]; }
function listenerNode(i: int) => UI.Node* { return lNode[i]; }
function listenerType(i: int) => char* { return lType[i]; }
function listenerFn(i: int) => OBJ.Value* { return &lFn[i]; }

// rAF: callbacks registered while a batch runs belong to the next batch
function rafCount() => int { return rN; }
function rafFn(i: int) => OBJ.Value* { return &rFn[i]; }
function rafShift(n: int) => void {
    let i: int = n;
    while (i < rN) { OBJ.copyVal(&rFn[i - n], &rFn[i]); rId[i - n] = rId[i]; i = i + 1; }
    rN = rN - n;
    if (rN < 0) rN = 0;
}

// timers
function timerCount() => int { return tN; }
function timerDue(i: int) => bool { return tFn[i].k == OBJ.K_FUNC && tDue[i] <= nowMs; }
function timerFn(i: int) => OBJ.Value* { return &tFn[i]; }
function timerFired(i: int) => void { tFn[i].k = OBJ.K_UNDEF; }
function timerCompact() => void {                        // drop fired/cancelled entries once nothing is running
    let w: int = 0;
    let i: int = 0;
    while (i < tN) {
        if (tFn[i].k == OBJ.K_FUNC) { if (w != i) { OBJ.copyVal(&tFn[w], &tFn[i]); tDue[w] = tDue[i]; tId[w] = tId[i]; } w = w + 1; }
        i = i + 1;
    }
    tN = w;
}
