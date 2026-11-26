// browser/js/selector.os  -- CSS selectors for querySelector / querySelectorAll.
// Supports: tag, *, #id, .class (several), [attr], [attr=value] (quoted or not), descendant (space) and child (>)
// combinators, and comma lists. Unsupported syntax (pseudo-classes, +, ~) makes that selector match nothing.
module U = import "util.os";
module UI = import "standard/1/Ui.os";
module A = import "uiapi.os";

struct Step {
    tag: char* = nullptr;       // nullptr = any
    id: char* = nullptr;
    cls: char** = nullptr;      // up to 6 class names
    ncls: int = 0;
    attr: char* = nullptr;
    attrVal: char* = nullptr;   // nullptr = presence only
    child: bool = false;        // combinator linking this step to the previous one: true '>', false ' '
    bad: bool = false;
}

struct Sel {
    steps: Step* = nullptr;
    n: int = 0;
    bad: bool = false;
}

function isNameChar(c: int) => bool { return U.isAlnum(c) || c == 45 || c == 95; }

function lower(s: char*) => char* {
    let n: int = U.slen(s);
    let out: char* = U.newStr(n + 1);
    let i: int = 0;
    while (i < n) {
        let c: int = s[i] as int;
        if (c >= 65 && c <= 90) c = c + 32;
        out[i] = c as char;
        i = i + 1;
    }
    out[n] = 0 as char;
    return out;
}

function readName(s: char*, i: int, n: int) => int { let j: int = i; while (j < n && isNameChar(s[j] as int)) j = j + 1; return j; }

// parses one comma-free selector text into steps
function parseOne(text: char*) => Sel* {
    let sel: Sel* = U.alloc(sizeof(Sel)) as Sel*;
    sel.steps = U.alloc(sizeof(Step) * 8) as Step*;
    sel.n = 0; sel.bad = false;
    let n: int = U.slen(text);
    let i: int = 0;
    let child: bool = false;
    while (i < n && !sel.bad) {
        while (i < n && U.isSpace(text[i] as int)) i = i + 1;
        if (i < n && (text[i] as int) == 62) { child = true; i = i + 1; while (i < n && U.isSpace(text[i] as int)) i = i + 1; }
        if (i >= n) { i = n; }
        else if (sel.n >= 8) { sel.bad = true; }
        else {
            let st: Step* = &sel.steps[sel.n];
            st.tag = nullptr; st.id = nullptr; st.attr = nullptr; st.attrVal = nullptr; st.ncls = 0; st.bad = false;
            st.cls = U.alloc(8 * 6) as char**;
            st.child = child;
            child = false;
            let any: bool = false;
            let more: bool = true;
            while (more && i < n && !st.bad) {
                let c: int = text[i] as int;
                if (c == 42) { i = i + 1; any = true; }
                else if (isNameChar(c) && !any && st.tag == nullptr && st.id == nullptr && st.ncls == 0 && st.attr == nullptr) {
                    let j: int = readName(text, i, n);
                    st.tag = lower(U.sub(text, i, j)); i = j; any = true;
                }
                else if (c == 35) { let j: int = readName(text, i + 1, n); st.id = U.sub(text, i + 1, j); i = j; any = true; }
                else if (c == 46) {
                    let j: int = readName(text, i + 1, n);
                    if (st.ncls < 6) { st.cls[st.ncls] = U.sub(text, i + 1, j); st.ncls = st.ncls + 1; }
                    i = j; any = true;
                }
                else if (c == 91) {
                    let j: int = i + 1;
                    let k: int = readName(text, j, n);
                    st.attr = U.sub(text, j, k);
                    j = k;
                    if (j < n && (text[j] as int) == 61) {
                        j = j + 1;
                        let q: int = 0;
                        if (j < n && ((text[j] as int) == 34 || (text[j] as int) == 39)) { q = text[j] as int; j = j + 1; }
                        let a: int = j;
                        while (j < n && (q == 0 && (text[j] as int) != 93 || q != 0 && (text[j] as int) != q)) j = j + 1;
                        st.attrVal = U.sub(text, a, j);
                        if (q != 0) j = j + 1;
                    }
                    if (j < n && (text[j] as int) == 93) j = j + 1; else st.bad = true;
                    i = j; any = true;
                }
                else { more = false; }
            }
            if (!any) st.bad = true;                         // e.g. ':hover', '+', '~'
            if (st.bad) sel.bad = true;
            sel.n = sel.n + 1;
        }
    }
    if (sel.n == 0) sel.bad = true;
    return sel;
}

function hasClass(nd: UI.Node*, cls: char*) => bool {
    let list: char* = A.attr(nd, "class");
    if (list == nullptr) return false;
    let n: int = U.slen(list);
    let cl: int = U.slen(cls);
    let i: int = 0;
    let found: bool = false;
    while (i < n && !found) {
        while (i < n && U.isSpace(list[i] as int)) i = i + 1;
        let a: int = i;
        while (i < n && !U.isSpace(list[i] as int)) i = i + 1;
        if (i - a == cl && U.streq(U.sub(list, a, i), cls)) found = true;
    }
    return found;
}

function stepMatches(nd: UI.Node*, st: Step*) => bool {
    if (st.tag != nullptr && !U.streq(lower(A.tagOf(nd)), st.tag)) return false;
    if (st.id != nullptr) { let id: char* = A.attr(nd, "id"); if (id == nullptr || !U.streq(id, st.id)) return false; }
    let i: int = 0;
    while (i < st.ncls) { if (!hasClass(nd, st.cls[i])) return false; i = i + 1; }
    if (st.attr != nullptr) {
        let v: char* = A.attr(nd, st.attr);
        if (v == nullptr || (st.attrVal == nullptr && U.slen(v) == 0)) return false;
        if (st.attrVal != nullptr && !U.streq(v, st.attrVal)) return false;
    }
    return true;
}

// does nd match steps[0..idx] (right-to-left, with backtracking for descendant combinators)?
function matchFrom(nd: UI.Node*, sel: Sel*, idx: int) => bool {
    if (!stepMatches(nd, &sel.steps[idx])) return false;
    if (idx == 0) return true;
    let par: UI.Node* = nd.parent;
    if (sel.steps[idx].child) { if (par == nullptr) return false; return matchFrom(par, sel, idx - 1); }
    let ok: bool = false;
    while (par != nullptr && !ok) { ok = matchFrom(par, sel, idx - 1); par = par.parent; }
    return ok;
}

// splits `a, b > c` into selectors; returns how many (max 8)
function parseList(text: char*, out: Sel**) => int {
    let n: int = U.slen(text);
    let cnt: int = 0;
    let a: int = 0;
    let i: int = 0;
    while (i <= n) {
        if (i == n || (text[i] as int) == 44) {
            if (cnt < 8) { out[cnt] = parseOne(U.sub(text, a, i)); cnt = cnt + 1; }
            a = i + 1;
        }
        i = i + 1;
    }
    return cnt;
}

function matchesAny(nd: UI.Node*, sels: Sel**, cnt: int) => bool {
    let i: int = 0;
    while (i < cnt) {
        if (!sels[i].bad && matchFrom(nd, sels[i], sels[i].n - 1)) return true;
        i = i + 1;
    }
    return false;
}

// depth-first, document order; stops after `limit` hits (0 = all). Fills `hits`, returns the count.
function walk(root: UI.Node*, sels: Sel**, cnt: int, hits: UI.Node**, found: int, limit: int) => int {
    let c: UI.Node* = root.firstEl();
    let f: int = found;
    while (c != nullptr && (limit == 0 || f < limit) && f < 256) {
        if (matchesAny(c, sels, cnt)) { hits[f] = c; f = f + 1; }
        f = walk(c, sels, cnt, hits, f, limit);
        c = c.nextEl();
    }
    return f;
}

function queryAll(root: UI.Node*, text: char*, hits: UI.Node**, limit: int) => int {
    if (root == nullptr) return 0;
    let sels: Sel** = U.alloc(8 * 8) as Sel**;
    let cnt: int = parseList(text, sels);
    return walk(root, sels, cnt, hits, 0, limit);
}

function queryFirst(root: UI.Node*, text: char*) => UI.Node* {
    let hits: UI.Node** = U.alloc(8 * 4) as UI.Node**;
    if (queryAll(root, text, hits, 1) > 0) return hits[0];
    return nullptr;
}
