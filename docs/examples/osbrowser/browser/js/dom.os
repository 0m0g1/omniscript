// browser/js/dom.os  -- host objects (V8's embedder API / Blink bindings): window, document, Element, element.style.
// Sits on top of Ui nodes. It never calls back into the interpreter: callbacks are only stored in eventloop.os.
//   window:   innerWidth innerHeight pageYOffset scrollY document addEventListener requestAnimationFrame
//             cancelAnimationFrame setTimeout clearTimeout matchMedia
//   document: querySelector querySelectorAll getElementById addEventListener body documentElement
//   element:  querySelector querySelectorAll getAttribute setAttribute addEventListener
//             innerText textContent innerHTML(text only) id className href src value checked style.<prop>
// NEXT: classList, children/parentElement, createElement/appendChild, dataset, getBoundingClientRect, computed style.
module U = import "util.os";
module UI = import "standard/1/Ui.os";
module A = import "uiapi.os";
module OBJ = import "js/objects.os";
module BI = import "js/builtins.os";
module SEL = import "js/selector.os";
module EL = import "js/eventloop.os";

let docRoot: UI.Node* = nullptr;
let viewW: double = 1000.0;
let viewH: double = 700.0;
let scrollY: double = 0.0;

function setRoot(root: UI.Node*) => void { docRoot = root; }
function setViewport(w: double, h: double) => void { if (w > 0.0) viewW = w; if (h > 0.0) viewH = h; }
function setScroll(y: double) => void { scrollY = y; }
function viewportW() => double { return viewW; }
function viewportH() => double { return viewH; }
function scrollPos() => double { return scrollY; }

function wrapEl(nd: UI.Node*, out: OBJ.Value*) => void {
    if (nd == nullptr) OBJ.setNull(out); else OBJ.setRef(out, OBJ.K_ELEM, nd as void*);
}

function isWindowMethod(name: char*) => bool {
    return U.streq(name, "addEventListener") || U.streq(name, "removeEventListener") || U.streq(name, "requestAnimationFrame")
        || U.streq(name, "cancelAnimationFrame") || U.streq(name, "setTimeout") || U.streq(name, "clearTimeout")
        || U.streq(name, "matchMedia");
}

// ---------- reads ----------
function getWindow(obj: OBJ.Value*, name: char*, out: OBJ.Value*) => bool {
    if (U.streq(name, "innerWidth")) OBJ.setNum(out, viewW);
    else if (U.streq(name, "innerHeight")) OBJ.setNum(out, viewH);
    else if (U.streq(name, "pageYOffset") || U.streq(name, "scrollY")) OBJ.setNum(out, scrollY);
    else if (U.streq(name, "pageXOffset") || U.streq(name, "scrollX")) OBJ.setNum(out, 0.0);
    else if (U.streq(name, "document")) OBJ.setRef(out, OBJ.K_DOC, nullptr);
    else if (U.streq(name, "window") || U.streq(name, "self")) OBJ.setRef(out, OBJ.K_WINDOW, nullptr);
    else if (U.streq(name, "localStorage") || U.streq(name, "sessionStorage")) return BI.global(name, out);
    else if (isWindowMethod(name)) BI.method(out, obj, name);
    else return false;
    return true;
}

function get(obj: OBJ.Value*, name: char*, out: OBJ.Value*) => bool {
    OBJ.setUndef(out);
    if (obj.k == OBJ.K_WINDOW) return getWindow(obj, name, out);
    if (obj.k == OBJ.K_DOC) {
        if (U.streq(name, "body")) { if (docRoot != nullptr) wrapEl(docRoot.findTag("body"), out); else OBJ.setNull(out); }
        else if (U.streq(name, "documentElement")) wrapEl(docRoot, out);
        else BI.method(out, obj, name);
        return true;
    }
    if (obj.k == OBJ.K_ELEM) {
        let nd: UI.Node* = obj.p as UI.Node*;
        if (U.streq(name, "textContent") || U.streq(name, "innerText") || U.streq(name, "innerHTML")) OBJ.setStr(out, A.textOf(nd));
        else if (U.streq(name, "id")) OBJ.setStr(out, A.attr(nd, "id"));
        else if (U.streq(name, "className")) OBJ.setStr(out, A.attr(nd, "class"));
        else if (U.streq(name, "href") || U.streq(name, "src")) OBJ.setStr(out, A.attr(nd, name));
        else if (U.streq(name, "value")) { let v: char* = nd.getValue(); if (v == nullptr) v = ""; OBJ.setStr(out, v); }
        else if (U.streq(name, "checked")) OBJ.setBool(out, nd.checked);
        else if (U.streq(name, "style")) OBJ.setRef(out, OBJ.K_STYLE, obj.p);
        else if (U.streq(name, "tagName") || U.streq(name, "nodeName")) OBJ.setStr(out, A.tagOf(nd));
        else BI.method(out, obj, name);
        return true;
    }
    if (obj.k == OBJ.K_STYLE) { OBJ.setStr(out, ""); return true; }      // NEXT: read back inline / computed style
    return false;
}

// ---------- writes ----------
// Inline style declarations are appended by Ui, so identical consecutive writes (a scroll handler resetting
// `transform: none`) are dropped instead of piling up.
let sNode: UI.Node** = nullptr;
let sKey: char** = nullptr;
let sVal: char** = nullptr;
let sN: int = 0;

function kebab(name: char*) => char* {                 // backgroundImage -> background-image
    let n: int = U.slen(name);
    let out: char* = U.newStr(n * 2 + 1);
    let i: int = 0;
    let p: int = 0;
    while (i < n) {
        let c: int = name[i] as int;
        if (c >= 65 && c <= 90) { out[p] = 45 as char; p = p + 1; out[p] = (c + 32) as char; p = p + 1; }
        else { out[p] = c as char; p = p + 1; }
        i = i + 1;
    }
    out[p] = 0 as char;
    return out;
}

function setStyle(nd: UI.Node*, prop: char*, v: OBJ.Value*) => void {
    let css: char* = kebab(prop);
    let val: char* = OBJ.toStr(v);
    if (v.k == OBJ.K_NUM && !U.streq(css, "opacity") && !U.streq(css, "z-index") && !U.streq(css, "flex-grow")) val = U.cat(val, "px");
    if (sNode == nullptr) { sNode = U.alloc(8 * 64) as UI.Node**; sKey = U.alloc(8 * 64) as char**; sVal = U.alloc(8 * 64) as char**; sN = 0; }
    let i: int = 0;
    let slot: int = 0 - 1;
    while (i < sN && slot < 0) { if (sNode[i] == nd && U.streq(sKey[i], css)) slot = i; i = i + 1; }
    if (slot >= 0 && U.streq(sVal[slot], val)) return;
    if (slot < 0 && sN < 64) { slot = sN; sN = sN + 1; sNode[slot] = nd; sKey[slot] = css; }
    if (slot >= 0) sVal[slot] = val;
    nd.style(U.cat(css, U.cat(":", U.cat(val, ";"))));
}

function resetStyleCache() => void { sN = 0; }

function set(obj: OBJ.Value*, name: char*, v: OBJ.Value*) => void {
    if (obj.k == OBJ.K_ELEM) {
        let nd: UI.Node* = obj.p as UI.Node*;
        if (U.streq(name, "textContent") || U.streq(name, "innerText") || U.streq(name, "innerHTML")) nd.setText(OBJ.toStr(v));
        else if (U.streq(name, "className")) nd.setAttr("class", OBJ.toStr(v));
        else if (U.streq(name, "id") || U.streq(name, "href") || U.streq(name, "src")) nd.setAttr(name, OBJ.toStr(v));
        else if (U.streq(name, "value")) nd.setValue(OBJ.toStr(v));
        else if (U.streq(name, "checked")) nd.setChecked(OBJ.truthy(v));
    }
    else if (obj.k == OBJ.K_STYLE) setStyle(obj.p as UI.Node*, name, v);
}

// ---------- calls ----------
function queryList(root: UI.Node*, sel: char*, out: OBJ.Value*) => void {
    let hits: UI.Node** = U.alloc(8 * 256) as UI.Node**;
    let n: int = SEL.queryAll(root, sel, hits, 0);
    let arr: OBJ.Obj* = OBJ.newObj(true);
    let i: int = 0;
    while (i < n) { let e: OBJ.Value; wrapEl(hits[i], &e); OBJ.arrPush(arr, &e); i = i + 1; }
    OBJ.setRef(out, OBJ.K_ARRAY, arr as void*);
}

function argStr(args: OBJ.Value*, argc: int, i: int) => char* {
    if (i >= argc) return "";
    return OBJ.toStr(&args[i]);
}

function mediaQuery(q: char*, out: OBJ.Value*) => void {
    let o: OBJ.Obj* = OBJ.newObj(false);
    let m: OBJ.Value;
    OBJ.setBool(&m, false);                            // NEXT: evaluate width / prefers-* features
    OBJ.objSet(o, "matches", &m);
    let s: OBJ.Value;
    OBJ.setStr(&s, q);
    OBJ.objSet(o, "media", &s);
    OBJ.setRef(out, OBJ.K_OBJECT, o as void*);
}

function call(m: OBJ.Value*, args: OBJ.Value*, argc: int, out: OBJ.Value*) => void {
    OBJ.setUndef(out);
    let kind: int = m.n as int;
    let name: char* = m.s;
    if (kind == OBJ.K_WINDOW) {
        if (U.streq(name, "addEventListener") && argc >= 2) EL.addListener(EL.T_WINDOW, nullptr, argStr(args, argc, 0), &args[1]);
        else if (U.streq(name, "requestAnimationFrame") && argc >= 1) OBJ.setNum(out, EL.addRaf(&args[0]) as double);
        else if (U.streq(name, "setTimeout") && argc >= 1) {
            let ms: double = 0.0;
            if (argc >= 2) ms = OBJ.toNum(&args[1]);
            OBJ.setNum(out, EL.addTimer(&args[0], ms) as double);
        }
        else if ((U.streq(name, "cancelAnimationFrame") || U.streq(name, "clearTimeout")) && argc >= 1) EL.cancel(OBJ.toNum(&args[0]) as int);
        else if (U.streq(name, "matchMedia")) mediaQuery(argStr(args, argc, 0), out);
    }
    else if (kind == OBJ.K_DOC) {
        if (U.streq(name, "querySelector") && argc >= 1) wrapEl(SEL.queryFirst(docRoot, argStr(args, argc, 0)), out);
        else if (U.streq(name, "querySelectorAll") && argc >= 1) queryList(docRoot, argStr(args, argc, 0), out);
        else if (U.streq(name, "getElementById") && argc >= 1) {
            let nd: UI.Node* = nullptr;
            if (docRoot != nullptr) nd = docRoot.findId(argStr(args, argc, 0));
            wrapEl(nd, out);
        }
        else if (U.streq(name, "addEventListener") && argc >= 2) EL.addListener(EL.T_DOCUMENT, nullptr, argStr(args, argc, 0), &args[1]);
    }
    else if (kind == OBJ.K_ELEM) {
        let nd: UI.Node* = m.p as UI.Node*;
        if (U.streq(name, "addEventListener") && argc >= 2) EL.addListener(EL.T_ELEMENT, nd, argStr(args, argc, 0), &args[1]);
        else if (U.streq(name, "getAttribute") && argc >= 1) OBJ.setStr(out, A.attr(nd, argStr(args, argc, 0)));
        else if (U.streq(name, "setAttribute") && argc >= 2) nd.setAttr(argStr(args, argc, 0), OBJ.toStr(&args[1]));
        else if (U.streq(name, "querySelector") && argc >= 1) wrapEl(SEL.queryFirst(nd, argStr(args, argc, 0)), out);
        else if (U.streq(name, "querySelectorAll") && argc >= 1) queryList(nd, argStr(args, argc, 0), out);
    }
}
