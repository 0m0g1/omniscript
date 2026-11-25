// standard/1/Ui/Css/Values/Var.os
// Custom properties (--name) and var(--name, fallback).
//
// Model (same shape as Blink's StyleCascade, minus the computed-value cache):
//   * an element that declares any --x gets a VarScope {props, up}; `up` is the nearest ancestor scope.
//   * cascadeNode pushes the scope before it applies the element's own declarations and pops it after its children,
//     so gScope is always "the custom properties visible to the element being cascaded".
//   * applyDeclsRec substitutes var() in a value (varSubstDepth) right before applyProp parses it.
//   * a var() with no value and no fallback makes the declaration invalid (StyleNote INVALID), as in CSS.
// Custom properties inherit by construction (the scope chain). Values are not typed (@property is ignored).

function isCustomName(nm: char*) => bool {
    let r: bool = false;
    if ((nm[0] as int) == 45 && (nm[1] as int) == 45) r = true;                          // --x
    else if ((nm[0] as int) == 33 && (nm[1] as int) == 45 && (nm[2] as int) == 45) r = true;   // !--x (important)
    return r;
}

function declsHaveCustom(a0: Attr*) => bool {
    let a: Attr* = a0; let hit: bool = false;
    while (a != nullptr && !hit) { if (isCustomName(a.name)) hit = true; a = a.next; }
    return hit;
}

function attrListAdd(l: AttrList*, name: char*, value: char*) => void {
    let a: Attr* = CMem.malloc(sizeof(Attr)) as Attr*;
    a.name = name; a.value = value; a.next = nullptr;
    if (l.tail != nullptr) l.tail.next = a; else l.head = a;
    l.tail = a;
}

// copies the custom properties of a declaration list into `l` (name without the "!")
function varAddCustom(decls: Attr*, l: AttrList*) => void {
    let a: Attr* = decls;
    while (a != nullptr) {
        if (isCustomName(a.name)) {
            let nm: char* = strDup(a.name);
            if ((a.name[0] as int) == 33) { strFree(nm); nm = strSub(a.name, 1, strLen(a.name) - 1); }
            attrListAdd(l, nm, strDup(a.value));
        }
        a = a.next;
    }
}

// own custom properties of `n`, in cascade order (rules by specificity / source order, then style=""); later wins
function varCollect(n: Node*, sheet: Sheet*, l: AttrList*) => void {
    let r: Rule* = sheet.head;
    while (r != nullptr) {
        if (declsHaveCustom(r.decls) && matchChain(n, r.sel)) varAddCustom(r.decls, l);
        r = r.next;
    }
    if (n.inlineCss != nullptr && containsStr(n.inlineCss, "--")) {
        let d: Attr* = parseDecls(n.inlineCss);
        varAddCustom(d, l);
        freeAttrs(d);
    }
}

// value of --name in the current scope chain; not owned by the caller. nullptr when undefined.
function varLookup(name: char*) => char* {
    let r: char* = nullptr;
    let s: VarScope* = gScope;
    while (s != nullptr && r == nullptr) {
        let a: Attr* = s.props;
        while (a != nullptr) { if (strEq(a.name, name)) r = a.value; a = a.next; }      // last declaration wins
        s = s.up;
    }
    return r;
}

// Returns a new string with every var(--x, fallback) replaced; nullptr when one has no value and no fallback
// (or the chain is cyclic / deeper than 8). Self recursion only.
function varSubstDepth(text: char*, depth: int) => char* {
    if (depth > 8) return nullptr;
    let n: int = strLen(text);
    let out: char* = strDup("");
    let i: int = 0; let a: int = 0; let bad: bool = false;
    while (i < n && !bad) {
        if (eqAtI(text, i, "var(")) {
            let lit: char* = strSub(text, a, i - a);
            let o2: char* = strCat(out, lit); strFree(lit); strFree(out); out = o2;
            let j: int = i + 4; let d: int = 1; let comma: int = 0 - 1;
            while (j < n && d > 0) {
                let c: int = text[j] as int;
                if (c == 40) d = d + 1;
                else if (c == 41) d = d - 1;
                else if (c == 44 && d == 1 && comma < 0) comma = j;
                if (d > 0) j = j + 1;
            }
            let nameEnd: int = j;
            if (comma >= 0) nameEnd = comma;
            let nm: char* = trimDup(text, i + 4, nameEnd);
            let val: char* = varLookup(nm);
            let piece: char* = nullptr;
            if (val != nullptr) { piece = varSubstDepth(val, depth + 1); }
            else if (comma >= 0) {
                let fb: char* = trimDup(text, comma + 1, j);
                piece = varSubstDepth(fb, depth + 1);
                strFree(fb);
            }
            strFree(nm);
            if (piece == nullptr) { bad = true; }
            else { let o3: char* = strCat(out, piece); strFree(piece); strFree(out); out = o3; }
            i = j + 1; a = i;
        }
        else { i = i + 1; }
    }
    if (!bad && a < n) {
        let tail: char* = strSub(text, a, n - a);
        let o4: char* = strCat(out, tail); strFree(tail); strFree(out); out = o4;
    }
    if (bad) { strFree(out); out = nullptr; }
    return out;
}

// Pushes `n`'s scope when it declares custom properties. true when gScope changed (pair with varPop).
function varPush(n: Node*, sheet: Sheet*) => bool {
    let l: AttrList; l.head = nullptr; l.tail = nullptr;
    varCollect(n, sheet, &l);
    let pushed: bool = false;
    if (l.head != nullptr) {
        let s: VarScope* = CMem.malloc(sizeof(VarScope)) as VarScope*;
        s.props = l.head; s.up = gScope;
        gScope = s;
        pushed = true;
        // --a: var(--b) resolves now, against this scope (siblings declared on the same element included)
        let a: Attr* = s.props;
        while (a != nullptr) {
            if (containsStr(a.value, "var(")) {
                let nv: char* = varSubstDepth(a.value, 1);
                if (nv != nullptr) { strFree(a.value); a.value = nv; }
            }
            a = a.next;
        }
    }
    return pushed;
}

function varPop() => void {
    let s: VarScope* = gScope;
    if (s != nullptr) {
        gScope = s.up;
        freeAttrs(s.props);
        CMem.free(s as void*);
    }
}

// Scope visible to the children of `n` (rebuilds the chain from the root). Used when cascadeNode is entered below
// the root (partial restyle) and no scope is active. Free the result with varFreeChain.
function varChain(n: Node*, sheet: Sheet*) => VarScope* {
    if (n == nullptr) return nullptr;
    let up: VarScope* = varChain(n.parent, sheet);
    let saved: VarScope* = gScope;
    gScope = up;
    let did: bool = false;
    if (n.kind != NK_TEXT) did = varPush(n, sheet);
    let res: VarScope* = gScope;
    gScope = saved;
    return res;
}

function varFreeChain(s0: VarScope*) => void {
    let s: VarScope* = s0;
    while (s != nullptr) {
        let up: VarScope* = s.up;
        freeAttrs(s.props);
        CMem.free(s as void*);
        s = up;
    }
}
