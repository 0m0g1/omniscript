// browser/css/css.os  -- TIER 2: stylesheet parser, selector matching, cascade (source order after specificity sort).
// Supports: tag, .class, #id, compound (p.note), descendant combinator, comma lists.
// NEXT: child/sibling combinators, attribute selectors, pseudo-classes, !important, rgb(), shorthand sides,
//       inheritance table, @media, per-property specificity, var().
module U = import "browser/util.os";
module D = import "browser/dom/dom.os";

struct Rule {
    sel: char* = nullptr;
    body: char* = nullptr;
    spec: int = 0;
    next: Rule* = nullptr;
}

let rules: Rule* = nullptr;

function specOf(sel: char*) => int {
    let s: int = 0;
    let n: int = U.slen(sel);
    let i: int = 0;
    let atStart: bool = true;
    while (i < n) {
        let c: int = sel[i] as int;
        if (c == 35) s = s + 100;
        else if (c == 46) s = s + 10;
        else if (atStart && U.isAlpha(c)) s = s + 1;
        atStart = U.isSpace(c);
        i = i + 1;
    }
    return s;
}

function addRule(sel: char*, body: char*) => void {
    let r: Rule* = U.alloc(sizeof(Rule)) as Rule*;
    r.sel = U.trim(sel);
    r.body = body;
    r.spec = specOf(r.sel);
    // stable insert ordered by specificity
    if (rules == nullptr || rules.spec > r.spec) { r.next = rules; rules = r; }
    else {
        let p: Rule* = rules;
        while (p.next != nullptr && p.next.spec <= r.spec) { p = p.next; }
        r.next = p.next;
        p.next = r;
    }
}

function stripComments(s: char*) => char* {
    let out: char* = U.dup(s);
    let n: int = U.slen(s);
    let i: int = 0;
    while (i < n) {
        if ((s[i] as int) == 47 && (s[i + 1] as int) == 42) {
            let e: int = U.indexOf(s, i + 2, "*/");
            if (e < 0) e = n - 2;
            let k: int = i;
            while (k < e + 2) { out[k] = 32 as char; k = k + 1; }
            i = e + 2;
        } else { i = i + 1; }
    }
    return out;
}

function parseSheet(src0: char*) => void {
    let src: char* = stripComments(src0);
    let n: int = U.slen(src);
    let i: int = 0;
    while (i < n) {
        let ob: int = U.indexOf(src, i, "{");
        if (ob < 0) { i = n; }
        else {
            let cb: int = U.indexOf(src, ob, "}");
            if (cb < 0) cb = n;
            let selList: char* = U.sub(src, i, ob);
            let body: char* = U.sub(src, ob + 1, cb);
            // NEXT: skip @media/@font-face blocks properly
            let a: int = 0;
            let m: int = U.slen(selList);
            let k: int = 0;
            while (k <= m) {
                if (k == m || (selList[k] as int) == 44) {
                    addRule(U.sub(selList, a, k), body);
                    a = k + 1;
                }
                k = k + 1;
            }
            i = cb + 1;
        }
    }
}

// ---------- selector matching ----------
function hasClass(cls: char*, name: char*) => bool {
    let n: int = U.slen(cls);
    let i: int = 0;
    let found: bool = false;
    while (i < n) {
        while (i < n && U.isSpace(cls[i] as int)) { i = i + 1; }
        let a: int = i;
        while (i < n && !U.isSpace(cls[i] as int)) { i = i + 1; }
        if (i > a && U.streq(U.sub(cls, a, i), name)) found = true;
    }
    return found;
}

function matchCompound(nd: D.Node*, part: char*) => bool {
    let n: int = U.slen(part);
    let i: int = 0;
    let ok: bool = true;
    if (U.isAlpha(part[0] as int)) {
        while (i < n && (U.isAlnum(part[i] as int) || (part[i] as int) == 45)) { i = i + 1; }
        if (!U.streq(U.lower(U.sub(part, 0, i)), nd.tag)) ok = false;
    } else if ((part[0] as int) == 42) { i = 1; }
    while (i < n) {
        let kind: int = part[i] as int;                         // '.' or '#'
        let a: int = i + 1;
        let j: int = a;
        while (j < n && (U.isAlnum(part[j] as int) || (part[j] as int) == 45)) { j = j + 1; }
        let name: char* = U.sub(part, a, j);
        if (kind == 46) { if (!hasClass(nd.cls, name)) ok = false; }
        else if (kind == 35) { if (!U.streq(nd.id, name)) ok = false; }
        else { ok = false; j = n; }                              // unsupported syntax => no match
        i = j;
    }
    return ok;
}

function partCount(sel: char*) => int {
    let n: int = U.slen(sel);
    let i: int = 0;
    let c: int = 0;
    while (i < n) {
        while (i < n && U.isSpace(sel[i] as int)) { i = i + 1; }
        if (i < n) { c = c + 1; }
        while (i < n && !U.isSpace(sel[i] as int)) { i = i + 1; }
    }
    return c;
}

function partAt(sel: char*, k: int) => char* {
    let n: int = U.slen(sel);
    let i: int = 0;
    let c: int = 0;
    let out: char* = "";
    while (i < n) {
        while (i < n && U.isSpace(sel[i] as int)) { i = i + 1; }
        let a: int = i;
        while (i < n && !U.isSpace(sel[i] as int)) { i = i + 1; }
        if (i > a) { if (c == k) out = U.sub(sel, a, i); c = c + 1; }
    }
    return out;
}

function matchSelector(nd: D.Node*, sel: char*) => bool {
    let np: int = partCount(sel);
    if (np == 0) return false;
    let ok: bool = matchCompound(nd, partAt(sel, np - 1));
    let k: int = np - 2;
    let anc: D.Node* = nd.parent;
    while (ok && k >= 0) {
        let want: char* = partAt(sel, k);
        while (anc != nullptr && !matchCompound(anc, want)) { anc = anc.parent; }
        if (anc == nullptr) ok = false; else { anc = anc.parent; k = k - 1; }
    }
    return ok;
}

// ---------- values ----------
function rgba(r: int, g: int, b: int) => u32 {
    let v: i64 = (r as i64) * 16777216 + (g as i64) * 65536 + (b as i64) * 256 + 255;
    return v as u32;
}

function parseColor(v: char*, out: u32*) => bool {
    let n: int = U.slen(v);
    let ok: bool = true;
    if ((v[0] as int) == 35 && n == 7) {
        out[0] = rgba(U.hexv(v[1] as int) * 16 + U.hexv(v[2] as int), U.hexv(v[3] as int) * 16 + U.hexv(v[4] as int), U.hexv(v[5] as int) * 16 + U.hexv(v[6] as int));
    } else if ((v[0] as int) == 35 && n == 4) {
        out[0] = rgba(U.hexv(v[1] as int) * 17, U.hexv(v[2] as int) * 17, U.hexv(v[3] as int) * 17);
    } else if (U.streq(v, "black")) out[0] = rgba(0, 0, 0);
    else if (U.streq(v, "white")) out[0] = rgba(255, 255, 255);
    else if (U.streq(v, "red")) out[0] = rgba(220, 40, 40);
    else if (U.streq(v, "green")) out[0] = rgba(40, 160, 70);
    else if (U.streq(v, "blue")) out[0] = rgba(40, 90, 220);
    else if (U.streq(v, "yellow")) out[0] = rgba(250, 220, 40);
    else if (U.streq(v, "orange")) out[0] = rgba(250, 150, 30);
    else if (U.streq(v, "purple")) out[0] = rgba(140, 60, 180);
    else if (U.streq(v, "gray") || U.streq(v, "grey")) out[0] = rgba(128, 128, 128);
    else if (U.streq(v, "transparent")) out[0] = 0;
    else ok = false;                                             // NEXT: rgb(), hsl(), full named list
    return ok;
}

// "12px" / "1.5em" / "auto" -> px (auto = -1)
function parseLen(v: char*, fontSize: float) => float {
    if (U.streq(v, "auto")) return 0.0 - 1.0;
    let n: int = U.slen(v);
    let i: int = 0;
    while (i < n && (U.isDigit(v[i] as int) || (v[i] as int) == 46 || (v[i] as int) == 45)) { i = i + 1; }
    let num: float = U.toNum(U.sub(v, 0, i)) as float;
    if (U.streq(U.sub(v, i, n), "em")) return num * fontSize;
    return num;                                                  // px or unitless; NEXT: %, rem, vw
}

function firstWord(v: char*) => char* {
    let n: int = U.slen(v);
    let i: int = 0;
    while (i < n && !U.isSpace(v[i] as int)) { i = i + 1; }
    return U.sub(v, 0, i);
}

function applyProp(st: D.Style*, name: char*, val: char*) => void {
    let c: u32 = 0;
    if (U.streq(name, "color")) { if (parseColor(val, &c)) st.color = c; }
    else if (U.streq(name, "background-color") || U.streq(name, "background")) {
        if (parseColor(firstWord(val), &c)) { st.bg = c; st.bgOn = true; }
    }
    else if (U.streq(name, "font-size")) st.fontSize = parseLen(val, st.fontSize);
    else if (U.streq(name, "width")) st.width = parseLen(val, st.fontSize);
    else if (U.streq(name, "height")) st.height = parseLen(val, st.fontSize);
    else if (U.streq(name, "margin")) { st.margin = parseLen(firstWord(val), st.fontSize); if (st.margin < 0.0) st.margin = 0.0; }
    else if (U.streq(name, "padding")) st.padding = parseLen(firstWord(val), st.fontSize);
    else if (U.streq(name, "border-color")) { if (parseColor(val, &c)) st.borderColor = c; }
    else if (U.streq(name, "border")) {
        // "1px solid #333"  (width first, color last)
        st.border = parseLen(firstWord(val), st.fontSize);
        let sp: int = U.lastIndexOfCh(val, 32);
        if (sp >= 0 && parseColor(U.sub(val, sp + 1, U.slen(val)), &c)) st.borderColor = c;
    }
    else if (U.streq(name, "display")) {
        if (U.streq(val, "none")) st.display = 0;
        else if (U.streq(val, "inline")) st.display = 2;
        else if (U.streq(val, "block")) st.display = 1;          // NEXT: flex, inline-block, grid
    }
    // NEXT: font-weight, text-align, line-height, position, float, overflow ...
}

function camelToKebab(s: char*) => char* {
    let n: int = U.slen(s);
    let out: char* = U.newStr(n * 2);
    let i: int = 0;
    let p: int = 0;
    while (i < n) {
        let c: int = s[i] as int;
        if (c >= 65 && c <= 90) { out[p] = 45 as char; p = p + 1; out[p] = (c + 32) as char; p = p + 1; }
        else { out[p] = c as char; p = p + 1; }
        i = i + 1;
    }
    return out;
}

function applyDecls(st: D.Style*, decls: char*) => void {
    let n: int = U.slen(decls);
    let a: int = 0;
    let k: int = 0;
    while (k <= n) {
        if (k == n || (decls[k] as int) == 59) {
            let d: char* = U.sub(decls, a, k);
            let col: int = U.indexOf(d, 0, ":");
            if (col > 0) applyProp(st, U.lower(U.trim(U.sub(d, 0, col))), U.trim(U.sub(d, col + 1, U.slen(d))));
            a = k + 1;
        }
        k = k + 1;
    }
}

function uaStyle(nd: D.Node*) => void {
    let t: char* = nd.tag;
    let st: D.Style* = &nd.style;
    st.display = 1;
    if (U.streq(t, "span") || U.streq(t, "a") || U.streq(t, "b") || U.streq(t, "i") || U.streq(t, "em") || U.streq(t, "strong") || U.streq(t, "br")) st.display = 2;
    if (U.streq(t, "head") || U.streq(t, "style") || U.streq(t, "script") || U.streq(t, "title") || U.streq(t, "meta") || U.streq(t, "link")) st.display = 0;
    if (U.streq(t, "body")) st.margin = 8.0;
    if (U.streq(t, "h1")) { st.fontSize = 32.0; st.margin = 12.0; }
    if (U.streq(t, "h2")) { st.fontSize = 24.0; st.margin = 10.0; }
    if (U.streq(t, "h3")) { st.fontSize = 19.0; st.margin = 8.0; }
    if (U.streq(t, "p")) st.margin = 8.0;
    if (U.streq(t, "ul")) { st.margin = 8.0; st.padding = 16.0; }
    if (U.streq(t, "a")) st.color = rgba(30, 80, 220);
}

// Resolve computed style for the whole tree. Inherits color + font-size.
function styleTree(nd: D.Node*, parent: D.Node*) => void {
    if (parent != nullptr) {
        nd.style.color = parent.style.color;
        nd.style.fontSize = parent.style.fontSize;
    }
    if (nd.kind == 1) {
        uaStyle(nd);
        let r: Rule* = rules;
        while (r != nullptr) {
            if (matchSelector(nd, r.sel)) applyDecls(&nd.style, r.body);
            r = r.next;
        }
        applyDecls(&nd.style, nd.attrStyle);
        applyDecls(&nd.style, nd.inlineStyle);
    }
    let c: D.Node* = nd.first;
    while (c != nullptr) { styleTree(c, nd); c = c.next; }
}
