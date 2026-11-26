// browser/css/flatten.os -- CSS pre-processor that runs on every stylesheet BEFORE the Ui's sheetParse():
//   * nesting      .a { color: red; &:hover { color: blue } .b { margin: 0 } }   ->  flat rules (& and descendant forms)
//   * @media       conditions are evaluated against the viewport (min/max-width/height, orientation, ...): a matching
//                  block is inlined, a non-matching one is dropped (also inside rule bodies: `&.x { @media (..) { .. } }`)
//   * @supports / @layer / @container   inlined as if true;  @font-face, @keyframes, @import, ... dropped
//   * var()        `--name: value` declarations are collected (first definition wins, scoping ignored) and every
//                  var(--name, fallback) is replaced by its value.
// Custom properties persist across the stylesheets of one page (call reset() when a new page starts). Use flattenRaw() per
// sheet and resolve() once all sheets are known to resolve vars defined in later sheets (see Browser.publishCss).
// The viewport size is a parameter of flatten(): resizing the window does not re-evaluate @media until the page reloads (F5).
module Str = import "standard/1/String.os";
module U = import "browser/util.os";

// true: custom properties and var() pass through untouched and the Ui resolves them with proper per-element scopes
// (Cascade.os / Values/Var.os, including `--x` set in style="" attributes). false: the old global first-wins flattening below.
let UI_VARS: bool = true;

// prefers-color-scheme for page CSS. Keep in step with the `dark` argument of cssSetMedia() in standard/1/Ui/Root.os.
let PREFERS_DARK: bool = true;

let gVars: char* = "\n";                 // "\n--name:value\n--name2:value\n" -- one custom property per line
let gW: int = 1000;
let gH: int = 700;

function reset() => void { gVars = "\n"; }

// ---------- small text helpers ----------

function stripComments(s: char*) => char* {
    if (U.indexOf(s, 0, "/*") < 0) return s;
    let o: Str.String;
    let n: int = U.slen(s);
    let i: int = 0;
    let q: int = 0;
    while (i < n) {
        let c: int = s[i] as int;
        if (c == 92 && i + 1 < n) {                 // keep escapes as written
            o.appendChar(s[i]); o.appendChar(s[i + 1]); i = i + 2;
        }
        else if (q == 0 && c == 47 && (s[i + 1] as int) == 42) {
            let e: int = U.indexOf(s, i + 2, "*/");
            if (e < 0) i = n; else i = e + 2;
        }
        else {
            if (q != 0) { if (c == q || c == 10) q = 0; }
            else if (c == 34 || c == 39) q = c;
            o.appendChar(s[i]); i = i + 1;
        }
    }
    let r: char* = U.dup(o.cstr());
    o.free();
    return r;
}

// index of the '}' that closes a block whose '{' is just before `from`; the length of s when unbalanced
function closeIdx(s: char*, from: int) => int {
    let n: int = U.slen(s);
    let i: int = from;
    let depth: int = 1;
    let q: int = 0;
    let found: int = n;
    while (i < n && found == n) {
        let c: int = s[i] as int;
        if (q != 0) {
            if (c == 92) i = i + 1;
            else if (c == q) q = 0;
        }
        else if (c == 34 || c == 39) q = c;
        else if (c == 123) depth = depth + 1;
        else if (c == 125) { depth = depth - 1; if (depth == 0) found = i; }
        i = i + 1;
    }
    return found;
}

// index of the next comma at bracket depth 0 at or after `from`, or the length of s
function nextComma(s: char*, from: int) => int {
    let n: int = U.slen(s);
    let i: int = from;
    let depth: int = 0;
    let found: int = n;
    let done: bool = false;
    while (i < n && !done) {
        let c: int = s[i] as int;
        if (c == 40 || c == 91) depth = depth + 1;
        else if (c == 41 || c == 93) depth = depth - 1;
        else if (c == 44 && depth <= 0) { found = i; done = true; }
        i = i + 1;
    }
    return found;
}

// ---------- @media ----------

function lenPx(v: char*) => double {
    let t: char* = U.lower(U.trim(v));
    let num: double = U.toNum(t);
    if (U.indexOf(t, 0, "em") >= 0) return num * 16.0;        // em and rem
    return num;
}

function featureTrue(name: char*, val: char*) => bool {
    let nm: char* = U.lower(U.trim(name));
    let v: char* = U.lower(U.trim(val));
    let w: double = gW as double;
    let h: double = gH as double;
    if (U.streq(nm, "min-width")) return w >= lenPx(v);
    if (U.streq(nm, "max-width")) return w <= lenPx(v);
    if (U.streq(nm, "min-height")) return h >= lenPx(v);
    if (U.streq(nm, "max-height")) return h <= lenPx(v);
    if (U.streq(nm, "width")) return w == lenPx(v);
    if (U.streq(nm, "height")) return h == lenPx(v);
    if (U.streq(nm, "orientation")) {
        if (U.streq(v, "landscape")) return gW >= gH;
        return gW < gH;
    }
    if (U.streq(nm, "hover") || U.streq(nm, "any-hover")) return U.streq(v, "hover") || U.slen(v) == 0;
    if (U.streq(nm, "pointer") || U.streq(nm, "any-pointer")) return U.streq(v, "fine") || U.slen(v) == 0;
    if (U.streq(nm, "prefers-color-scheme")) {
        if (PREFERS_DARK) return U.streq(v, "dark");
        return U.streq(v, "light");
    }
    return false;                                              // prefers-reduced-motion, resolution, ... : not supported
}

// one media query: "screen and (min-width: 600px) and (max-width: 900px)", "print", "not print"
function queryTrue(q: char*) => bool {
    let t: char* = U.lower(U.trim(q));
    let neg: bool = U.startsAt(t, 0, "not ");
    let ok: bool = true;
    if (U.indexOf(t, 0, "print") >= 0 || U.indexOf(t, 0, "speech") >= 0) ok = false;
    let n: int = U.slen(t);
    let i: int = 0;
    while (i < n) {
        if ((t[i] as int) == 40) {
            let e: int = U.indexOf(t, i + 1, ")");
            if (e < 0) e = n;
            let inner: char* = U.sub(t, i + 1, e);
            let colon: int = U.indexOf(inner, 0, ":");
            if (colon < 0) { if (!featureTrue(inner, "")) ok = false; }
            else if (!featureTrue(U.sub(inner, 0, colon), U.sub(inner, colon + 1, U.slen(inner)))) ok = false;
            i = e + 1;
        }
        else i = i + 1;
    }
    if (neg) return !ok;
    return ok;
}

// comma-separated list of queries: true when any matches
function mediaTrue(cond: char*) => bool {
    if (U.slen(U.trim(cond)) == 0) return true;
    let n: int = U.slen(cond);
    let a: int = 0;
    let any: bool = false;
    while (a <= n && !any) {
        let e: int = nextComma(cond, a);
        if (queryTrue(U.sub(cond, a, e))) any = true;
        a = e + 1;
    }
    return any;
}

// ---------- custom properties ----------

function hasVar(name: char*) => bool {
    let key: char* = U.cat("\n", U.cat(name, ":"));
    return U.indexOf(gVars, 0, key) >= 0;
}

// value of a custom property, "" when it is not defined (check hasVar first)
function getVar(name: char*) => char* {
    let key: char* = U.cat("\n", U.cat(name, ":"));
    let at: int = U.indexOf(gVars, 0, key);
    if (at < 0) return "";
    let a: int = at + U.slen(key);
    let e: int = U.indexOf(gVars, a, "\n");
    if (e < 0) e = U.slen(gVars);
    return U.sub(gVars, a, e);
}

// "--name: value [!important]" -> table (first definition wins)
function registerVar(decl: char*) => void {
    let colon: int = U.indexOf(decl, 0, ":");
    if (colon < 0) return;
    let name: char* = U.trim(U.sub(decl, 0, colon));
    let val: char* = U.trim(U.sub(decl, colon + 1, U.slen(decl)));
    let imp: int = U.indexOf(val, 0, "!");
    if (imp >= 0) val = U.trim(U.sub(val, 0, imp));
    if (hasVar(name)) return;
    let flat: Str.String;
    let i: int = 0;
    while ((val[i] as int) != 0) {                             // one line per variable: newlines become spaces
        let c: int = val[i] as int;
        if (c == 10 || c == 13) flat.append(" "); else flat.appendChar(c as char);
        i = i + 1;
    }
    gVars = U.cat(gVars, U.cat(name, U.cat(":", U.cat(flat.cstr(), "\n"))));
    flat.free();
}

// replaces var(--x) / var(--x, fallback) everywhere in s (values may themselves contain var())
function subst(s: char*, depth: int) => char* {
    if (depth > 6) return s;
    if (U.indexOf(s, 0, "var(") < 0) return s;
    let o: Str.String;
    let n: int = U.slen(s);
    let i: int = 0;
    while (i < n) {
        let isVar: bool = U.startsAt(s, i, "var(");
        if (isVar && i > 0 && U.isAlnum(s[i - 1] as int)) isVar = false;     // "myvar(" is not var()
        if (isVar) {
            let j: int = i + 4;
            let pd: int = 1;
            while (j < n && pd > 0) {
                let c: int = s[j] as int;
                if (c == 40) pd = pd + 1;
                else if (c == 41) pd = pd - 1;
                if (pd > 0) j = j + 1;
            }
            let inner: char* = U.sub(s, i + 4, j);
            let comma: int = nextComma(inner, 0);
            let name: char* = U.trim(U.sub(inner, 0, comma));
            let fb: char* = "";
            if (comma < U.slen(inner)) fb = U.trim(U.sub(inner, comma + 1, U.slen(inner)));
            let v: char* = fb;
            if (hasVar(name)) v = getVar(name);
            o.append(subst(v, depth + 1));
            i = j + 1;
        }
        else { o.appendChar(s[i]); i = i + 1; }
    }
    let r: char* = U.dup(o.cstr());
    o.free();
    return r;
}

// ---------- nesting ----------

// `&` -> parent selector; a selector without `&` is a descendant of the parent
function combineOne(p: char*, s: char*) => char* {
    if (U.indexOf(s, 0, "&") < 0) {
        if (U.slen(p) == 0) return s;
        return U.cat(p, U.cat(" ", s));
    }
    let o: Str.String;
    let i: int = 0;
    while ((s[i] as int) != 0) {
        if ((s[i] as int) == 38) o.append(p); else o.appendChar(s[i]);
        i = i + 1;
    }
    let r: char* = U.dup(o.cstr());
    o.free();
    return r;
}

// every parent selector x every own selector, comma separated
function crossSelectors(parent: char*, own: char*) => char* {
    let o: Str.String;
    let first: bool = true;
    let pn: int = U.slen(parent);
    let on: int = U.slen(own);
    let pa: int = 0;
    while (pa <= pn) {
        let pe: int = nextComma(parent, pa);
        let ptxt: char* = U.trim(U.sub(parent, pa, pe));
        let sa: int = 0;
        while (sa <= on) {
            let se: int = nextComma(own, sa);
            let stxt: char* = U.trim(U.sub(own, sa, se));
            if (U.slen(stxt) > 0) {
                if (!first) o.append(", ");
                o.append(combineOne(ptxt, stxt));
                first = false;
            }
            sa = se + 1;
        }
        pa = pe + 1;
    }
    let r: char* = U.dup(o.cstr());
    o.free();
    return r;
}

// a nested `prelude { inner }` inside a block whose selector is `parent`
function handleRule(prelude: char*, inner: char*, parent: char*, nest: Str.String*) => void {
    if ((prelude[0] as int) == 64) {                           // @-rule
        if (U.startsAt(prelude, 0, "@media")) {
            if (mediaTrue(U.sub(prelude, 6, U.slen(prelude)))) block(inner, parent, nest);
        }
        else if (U.startsAt(prelude, 0, "@supports") || U.startsAt(prelude, 0, "@layer") || U.startsAt(prelude, 0, "@container")) {
            block(inner, parent, nest);
        }
        // @font-face, @keyframes, @page, ... are dropped
    }
    else block(inner, crossSelectors(parent, prelude), nest);
}

// Processes the content of a rule body (or the whole sheet when parent is ""): declarations of this block become one
// `parent { ... }` rule, nested rules follow it.
function block(s: char*, parent: char*, out: Str.String*) => void {
    let d: Str.String;
    let nest: Str.String;
    let n: int = U.slen(s);
    let i: int = 0;
    while (i < n) {
        let c: int = s[i] as int;
        if (U.isSpace(c) || c == 59) i = i + 1;
        else {
            let j: int = i;
            let pd: int = 0;
            let q: int = 0;
            let stop: bool = false;
            while (j < n && !stop) {
                let c2: int = s[j] as int;
                if (c2 == 92) j = j + 1;                       // skip the escaped character
                else if (q != 0) { if (c2 == q || c2 == 10) q = 0; }
                else if (c2 == 34 || c2 == 39) q = c2;
                else if (c2 == 40) pd = pd + 1;
                else if (c2 == 41) pd = pd - 1;
                else if (pd <= 0 && (c2 == 59 || c2 == 123 || c2 == 125)) stop = true;
                if (!stop) j = j + 1;
            }
            if (j > n) j = n;
            let term: int = 0;
            if (j < n) term = s[j] as int;
            if (term == 123) {                                 // nested rule or at-rule
                let prelude: char* = U.trim(U.sub(s, i, j));
                let e: int = closeIdx(s, j + 1);
                handleRule(prelude, U.sub(s, j + 1, e), parent, &nest);
                i = e + 1;
            }
            else {                                             // declaration (or stray text)
                let decl: char* = U.trim(U.sub(s, i, j));
                if (U.slen(decl) > 0 && (decl[0] as int) != 64) {
                    if (U.startsAt(decl, 0, "--") && !UI_VARS) registerVar(decl);
                    else { d.append(decl); d.append(";"); }
                }
                i = j + 1;
            }
        }
    }
    if (U.slen(parent) > 0 && d.len > 0) {
        out.append(parent);
        out.append(" { ");
        out.append(d.cstr());
        out.append(" }\n");
    }
    out.append(nest.cstr());
    d.free();
    nest.free();
}

// Stage 1: nesting + @media + collect `--vars`. var() is left in place, so a variable defined in a stylesheet that is
// queued LATER than its use can still be found by resolve().
function flattenRaw(css: char*, vw: int, vh: int) => char* {
    gW = vw;
    gH = vh;
    // a UTF-8 byte order mark (EF BB BF) would stick to the first selector of the sheet and stop that rule from matching
    let body: char* = css;
    if (U.slen(css) >= 3) {
        let b0: int = css[0] as int; let b1: int = css[1] as int; let b2: int = css[2] as int;
        if ((b0 == 239 || b0 == 0 - 17) && (b1 == 187 || b1 == 0 - 69) && (b2 == 191 || b2 == 0 - 65)) body = U.sub(css, 3, U.slen(css));
    }
    let src: char* = stripComments(body);
    let out: Str.String;
    block(src, "", &out);
    let flat: char* = U.dup(out.cstr());
    out.free();
    return flat;
}

// Stage 2: replace every var(--x[, fallback]) using all the variables collected so far. Also used for style="" attributes.
function resolve(css: char*) => char* {
    if (UI_VARS) return css;
    return subst(css, 0);
}

// the entry point for a single sheet: raw stylesheet text -> flat CSS the Ui's sheetParse understands
function flatten(css: char*, vw: int, vh: int) => char* {
    return resolve(flattenRaw(css, vw, vh));
}
