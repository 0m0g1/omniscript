// browser/js/js.os  -- engine facade (V8's "api/" + the Isolate): the only file browser.os imports.
//
//   source -> scanner -> parser -> AST -> interpreter  <->  builtins (Math, console, String, Array)
//                                              ^                     dom (window, document, Element, style)
//                                              |                     selector (CSS matching for querySelector)
//   host loop (this file) <-- eventloop (listeners, timers, requestAnimationFrame)
//
//   objects.os      Value / Obj / conversions           ast.os        node kinds + operators
//   scanner.os      lexer                               parser.os     recursive descent -> AST
//   interpreter.os  scopes, closures, evaluation        builtins.os   Math, console, String/Array methods
//   dom.os          window/document/element bindings    selector.os   CSS selector matcher
//   eventloop.os    task queues
//
// Host contract (called by browser.os / main.os):
//   reset()                      new page: drop listeners, timers and script state
//   setViewport(w, h)            page area in px (window.innerWidth/innerHeight); a change fires "resize"
//   setScroll(y)                 page scroll offset (window.pageYOffset); a change fires "scroll"
//   run(root, src)               run the page's scripts, then fire DOMContentLoaded and load
//   pumpClicks(ms)               after ui.frame() when a click was seen
//   tick(ms)                     every frame: timers, then requestAnimationFrame callbacks
module U = import "util.os";
module UI = import "standard/1/Ui.os";
module OBJ = import "js/objects.os";
module BI = import "js/builtins.os";
module DOM = import "js/dom.os";
module EL = import "js/eventloop.os";
module I = import "js/interpreter.os";

let loaded: bool = false;
let pendingScroll: bool = false;
let pendingResize: bool = false;
let lastW: double = 0.0;
let lastH: double = 0.0;
let lastY: double = 0.0;

function reset() => void {
    EL.reset();
    DOM.resetStyleCache();
    DOM.setRoot(nullptr);
    loaded = false; pendingScroll = false; pendingResize = false;
}

function setViewport(w: double, h: double) => void {
    DOM.setViewport(w, h);
    if (loaded && (w != lastW || h != lastH)) pendingResize = true;
    lastW = w; lastH = h;
}

function setScroll(y: double) => void {
    DOM.setScroll(y);
    if (loaded && y != lastY) pendingScroll = true;
    lastY = y;
}

function makeEvent(type: char*, out: OBJ.Value*) => void {
    let o: OBJ.Obj* = OBJ.newObj(false);
    let t: OBJ.Value;
    OBJ.setStr(&t, type);
    OBJ.objSet(o, "type", &t);
    OBJ.setRef(out, OBJ.K_OBJECT, o as void*);
}

// calls every listener of `type` on a target (window / document); node = nullptr
function fire(target: int, type: char*) => void {
    let ev: OBJ.Value;
    makeEvent(type, &ev);
    let n: int = EL.listenerCount();
    let i: int = 0;
    while (i < n) {
        if (EL.listenerTarget(i) == target && U.streq(EL.listenerType(i), type)) I.callFunction(EL.listenerFn(i), &ev, &ev, 1);
        i = i + 1;
    }
}

function run(root: UI.Node*, src: char*) => void {
    DOM.setRoot(root);
    BI.seed(EL.now());
    I.init();
    I.runSource(src);
    loaded = true;
    fire(EL.T_DOCUMENT, "DOMContentLoaded");
    fire(EL.T_WINDOW, "DOMContentLoaded");
    fire(EL.T_WINDOW, "load");
}

// Ui sets `clicked` on the clicked node and each ancestor for one frame, so element listeners bubble for free.
function pumpClicks(ms: double) => void {
    EL.setNow(ms);
    let ev: OBJ.Value;
    makeEvent("click", &ev);
    let n: int = EL.listenerCount();
    let i: int = 0;
    while (i < n) {
        if (EL.listenerTarget(i) == EL.T_ELEMENT && U.streq(EL.listenerType(i), "click")) {
            let nd: UI.Node* = EL.listenerNode(i);
            if (nd.clicked) I.callFunction(EL.listenerFn(i), &ev, &ev, 1);
        }
        i = i + 1;
    }
}

function tick(ms: double) => void {
    EL.setNow(ms);
    if (!loaded) return;
    if (pendingResize) { pendingResize = false; fire(EL.T_WINDOW, "resize"); }
    if (pendingScroll) { pendingScroll = false; fire(EL.T_WINDOW, "scroll"); }
    let n: int = EL.timerCount();
    let i: int = 0;
    while (i < n) {
        if (EL.timerDue(i)) {
            let fn: OBJ.Value;
            OBJ.copyVal(&fn, EL.timerFn(i));
            EL.timerFired(i);
            I.callFunction(&fn, &fn, &fn, 0);
        }
        i = i + 1;
    }
    EL.timerCompact();
    let r: int = EL.rafCount();                          // callbacks added while these run wait for the next frame
    let ts: OBJ.Value;
    OBJ.setNum(&ts, ms);
    i = 0;
    while (i < r) {
        let fn2: OBJ.Value;
        OBJ.copyVal(&fn2, EL.rafFn(i));
        if (fn2.k == OBJ.K_FUNC) I.callFunction(&fn2, &ts, &ts, 1);
        i = i + 1;
    }
    EL.rafShift(r);
}
