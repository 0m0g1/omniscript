// standard/1/Ui/Css/Parser/StyleSheet.os
// Rule storage (sorted by specificity, stable) and the stylesheet text parser.

function sheetAdd(sheet: Sheet*, sel: Compound*, decls: Attr*) => void { sheetAddSel(sheet, sel, decls, nullptr); }

// selText: the selector as written (kept for the inspector); may be nullptr
function sheetAddSel(sheet: Sheet*, sel: Compound*, decls: Attr*, selText: char*) => void {
    let r: Rule* = CMem.malloc(sizeof(Rule)) as Rule*;
    r.selText = nullptr;
    if (selText != nullptr) r.selText = strDup(selText);
    r.sel = sel; r.decls = decls; r.spec = specOf(sel); r.seq = sheet.seq; r.next = nullptr;
    sheet.seq = sheet.seq + 1;
    if (sheet.head == nullptr || sheet.head.spec > r.spec) { r.next = sheet.head; sheet.head = r; }
    else {
        let p: Rule* = sheet.head;
        while (p.next != nullptr && p.next.spec <= r.spec) p = p.next;     // stable: equal specificity keeps source order
        r.next = p.next; p.next = r;
    }
}

function sheetClear(sheet: Sheet*) => void {
    let r: Rule* = sheet.head;
    while (r != nullptr) { let nx: Rule* = r.next; freeCompound(r.sel); freeAttrs(r.decls); strFree(r.selText); CMem.free(r as void*); r = nx; }
    sheet.head = nullptr; sheet.seq = 0; sheet.usesPseudo = false;
}

// Moves lx.i to the `}` that closes the block whose `{` was just consumed (or to the end of the text).
// Braces inside "strings", 'strings', /* comments */, \-escapes and ( ... ) groups (url(data:...{...})) do not count.
function scanCssBlock(css: char*, lx: Lex*) => void {
    let depth: int = 1;
    let paren: int = 0;
    let go: bool = true;
    while (go) {
        let c: int = css[lx.i] as int;
        if (c == 0) { go = false; }
        else if (c == 34 || c == 39) {
            lx.i = lx.i + 1;
            let inStr: bool = true;
            while (inStr) {
                let d: int = css[lx.i] as int;
                if (d == 0) { inStr = false; }
                else if (d == 92) { lx.i = lx.i + 1; if (css[lx.i] as int != 0) lx.i = lx.i + 1; }
                else if (d == c) { lx.i = lx.i + 1; inStr = false; }
                else { lx.i = lx.i + 1; }
            }
        }
        else if (c == 47 && css[lx.i + 1] as int == 42) {
            lx.i = lx.i + 2;
            let inCom: bool = true;
            while (inCom) {
                let e: int = css[lx.i] as int;
                if (e == 0) { inCom = false; }
                else if (e == 42 && css[lx.i + 1] as int == 47) { lx.i = lx.i + 2; inCom = false; }
                else { lx.i = lx.i + 1; }
            }
        }
        else if (c == 92) { lx.i = lx.i + 1; if (css[lx.i] as int != 0) lx.i = lx.i + 1; }
        else if (c == 40) { paren = paren + 1; lx.i = lx.i + 1; }
        else if (c == 41) { if (paren > 0) paren = paren - 1; lx.i = lx.i + 1; }
        else if (c == 123 && paren == 0) { depth = depth + 1; lx.i = lx.i + 1; }
        else if (c == 125 && paren == 0) { depth = depth - 1; if (depth == 0) go = false; else lx.i = lx.i + 1; }
        else { lx.i = lx.i + 1; }
    }
}

// Adds every rule in `css` to the sheet. Selector lists become one rule per selector.
// Conditional group rules (@media, @supports, @layer) are evaluated NOW and their rules flattened into the sheet;
// call cssSetMedia() first and re-parse when the viewport changes (see Media/MediaQuery.os).
function sheetParse(sheet: Sheet*, css: char*) => void { sheetParseBlock(sheet, css, 0); }

// ---- CSS nesting (a style rule that contains other rules / @media blocks inside its { }) ----
// sheetParseBlock flattens it: the rule's own declarations become one rule; every nested rule is turned into a
// synthetic top-level rule text (parent selector joined with the nested one, `&` = the parent) and parsed again.
// Nested @media / @supports / @layer blocks are entered like top-level ones and keep the parent selector.

struct NestIt { pos: int = 0; ps: int = 0; pe: int = 0; bs: int = 0; be: int = 0; found: bool = false; }

// if s[i] starts a "string" or a /* comment */, the index just after it; otherwise i
function cssSkipTrivia(s: char*, i: int) => int {
    let c: int = s[i] as int;
    let r: int = i;
    if (c == 34 || c == 39) {
        let j: int = i + 1; let inStr: bool = true;
        while (inStr) {
            let d: int = s[j] as int;
            if (d == 0) { inStr = false; }
            else if (d == 92) { j = j + 1; if ((s[j] as int) != 0) j = j + 1; }
            else if (d == c) { j = j + 1; inStr = false; }
            else { j = j + 1; }
        }
        r = j;
    }
    else if (c == 47 && (s[i + 1] as int) == 42) {
        let j2: int = i + 2; let inCom: bool = true;
        while (inCom) {
            let e: int = s[j2] as int;
            if (e == 0) { inCom = false; }
            else if (e == 42 && (s[j2 + 1] as int) == 47) { j2 = j2 + 2; inCom = false; }
            else { j2 = j2 + 1; }
        }
        r = j2;
    }
    return r;
}

// true when a rule body holds a nested block: a `{` outside strings, comments and ( ) groups
function cssHasNested(s: char*) => bool {
    let n: int = strLen(s); let i: int = 0; let paren: int = 0; let found: bool = false;
    while (i < n && !found) {
        let j: int = cssSkipTrivia(s, i);
        if (j != i) { i = j; }
        else {
            let c: int = s[i] as int;
            if (c == 40) { paren = paren + 1; }
            else if (c == 41) { if (paren > 0) paren = paren - 1; }
            else if (c == 123 && paren == 0) { found = true; }
            i = i + 1;
        }
    }
    return found;
}

// the body without its nested blocks (and without their preludes): only the rule's own declarations
function nestDecls(s: char*) => char* {
    let n: int = strLen(s); let out: char* = strDup("");
    let seg: int = 0; let i: int = 0; let paren: int = 0;
    while (i < n) {
        let j: int = cssSkipTrivia(s, i);
        if (j != i) { i = j; }
        else {
            let c: int = s[i] as int;
            if (c == 40) { paren = paren + 1; i = i + 1; }
            else if (c == 41) { if (paren > 0) paren = paren - 1; i = i + 1; }
            else if (c == 59 && paren == 0) {
                let piece: char* = strSub(s, seg, i + 1 - seg);
                let o2: char* = strCat(out, piece); strFree(piece); strFree(out); out = o2;
                i = i + 1; seg = i;
            }
            else if (c == 123 && paren == 0) {
                let lx: Lex; lx.s = s; lx.i = i + 1; lx.ok = false; lx.val = 0.0;
                scanCssBlock(s, &lx);
                i = lx.i; if ((s[i] as int) == 125) i = i + 1;
                seg = i;
            }
            else { i = i + 1; }
        }
    }
    if (seg < n) {
        let tail: char* = strSub(s, seg, n - seg);
        let o3: char* = strCat(out, tail); strFree(tail); strFree(out); out = o3;
    }
    return out;
}

// finds the next nested block at or after it.pos: prelude = [ps, pe), inner text = [bs, be); it.pos moves past the block
function nestNext(s: char*, it: NestIt*) => void {
    let n: int = strLen(s); let i: int = it.pos; let seg: int = it.pos; let paren: int = 0; let go: bool = true;
    it.found = false;
    while (i < n && go) {
        let j: int = cssSkipTrivia(s, i);
        if (j != i) { i = j; }
        else {
            let c: int = s[i] as int;
            if (c == 40) { paren = paren + 1; i = i + 1; }
            else if (c == 41) { if (paren > 0) paren = paren - 1; i = i + 1; }
            else if (c == 59 && paren == 0) { i = i + 1; seg = i; }
            else if (c == 123 && paren == 0) {
                it.ps = seg; it.pe = i; it.bs = i + 1;
                let lx: Lex; lx.s = s; lx.i = i + 1; lx.ok = false; lx.val = 0.0;
                scanCssBlock(s, &lx);
                it.be = lx.i; i = lx.i; if ((s[i] as int) == 125) i = i + 1;
                it.pos = i; it.found = true; go = false;
            }
            else { i = i + 1; }
        }
    }
    if (go) it.pos = n;
}

// item idx of a comma separated selector list (top-level commas only), trimmed; nullptr past the end
function listItem(s: char*, idx: int) => char* {
    let n: int = strLen(s); let a: int = 0; let k: int = 0; let pd: int = 0; let q: int = 0; let cnt: int = 0;
    let res: char* = nullptr; let done: bool = false;
    while (k <= n && !done) {
        let ch: int = 0;
        if (k < n) ch = s[k] as int;
        if (q != 0) { if (ch == q) q = 0; }
        else if (ch == 34 || ch == 39) { q = ch; }
        else if (ch == 40 || ch == 91) { pd = pd + 1; }
        else if (ch == 41 || ch == 93) { if (pd > 0) pd = pd - 1; }
        if (k == n || (ch == 44 && pd == 0 && q == 0)) {
            if (cnt == idx) { res = trimDup(s, a, k); done = true; }
            cnt = cnt + 1; a = k + 1;
        }
        k = k + 1;
    }
    return res;
}

// one nested selector under one parent selector: every `&` becomes the parent, otherwise "parent child"
function nestOne(p: char*, c: char*) => char* {
    let res: char* = strDup(""); let n: int = strLen(c); let i: int = 0; let start: int = 0; let amp: bool = false;
    while (i <= n) {
        if (i == n || (c[i] as int) == 38) {
            let piece: char* = strSub(c, start, i - start);
            let r2: char* = strCat(res, piece); strFree(piece); strFree(res); res = r2;
            if (i < n) { let r3: char* = strCat(res, p); strFree(res); res = r3; amp = true; }
            start = i + 1;
        }
        i = i + 1;
    }
    if (!amp) {
        let sp: char* = strCat(p, " ");
        let r4: char* = strCat(sp, c); strFree(sp); strFree(res); res = r4;
    }
    return res;
}

// parent selector list x nested selector list -> one comma separated selector list
function nestJoin(parent: char*, child: char*) => char* {
    let out: char* = strDup("");
    let pi: int = 0; let more1: bool = true;
    while (more1) {
        let p: char* = listItem(parent, pi);
        if (p == nullptr) { more1 = false; }
        else {
            if (strLen(p) > 0) {
                let ci: int = 0; let more2: bool = true;
                while (more2) {
                    let c: char* = listItem(child, ci);
                    if (c == nullptr) { more2 = false; }
                    else {
                        if (strLen(c) > 0) {
                            let one: char* = nestOne(p, c);
                            if (strLen(out) > 0) { let o1: char* = strCat(out, ","); strFree(out); out = o1; }
                            let o2: char* = strCat(out, one); strFree(one); strFree(out); out = o2;
                        }
                        strFree(c); ci = ci + 1;
                    }
                }
            }
            strFree(p); pi = pi + 1;
        }
    }
    return out;
}

function sheetParseBlock(sheet: Sheet*, css: char*, depth: int) => void {
    let lx: Lex; lx.s = css; lx.i = 0; lx.ok = false; lx.val = 0.0;
    let more: bool = true;
    while (more) {
        skipCssWs(&lx);
        if (lx.peek() == 0) { more = false; }
        else if (lx.peek() == 64) {
            // at-rule: read the prelude, then either a `;` (statement) or a { block }
            let ps: int = lx.i;
            while (lx.peek() != 123 && lx.peek() != 59 && lx.peek() != 0) lx.i = lx.i + 1;
            let prelude: char* = trimDup(css, ps, lx.i);
            if (lx.peek() == 59) { lx.i = lx.i + 1; }                    // @import @charset @layer a, b;  (ignored)
            else if (lx.peek() == 123) {
                lx.i = lx.i + 1;
                let abs: int = lx.i;
                scanCssBlock(css, &lx);
                let abody: char* = strSub(css, abs, lx.i - abs);
                if (lx.peek() == 125) lx.i = lx.i + 1;
                if (depth < 6 && atRuleEnter(prelude)) sheetParseBlock(sheet, abody, depth + 1);   // Parser/AtRules.os
                strFree(abody);
            }
            strFree(prelude);
        } else {
            let ss: int = lx.i;
            while (lx.peek() != 123 && lx.peek() != 0) lx.i = lx.i + 1;
            let selText: char* = strSub(css, ss, lx.i - ss);
            if (lx.peek() == 123) lx.i = lx.i + 1;
            let bs: int = lx.i;
            scanCssBlock(css, &lx);
            let body0: char* = strSub(css, bs, lx.i - bs);
            if (lx.peek() == 125) lx.i = lx.i + 1;
            let isNested: bool = cssHasNested(body0);
            let body: char* = body0;
            if (isNested) body = nestDecls(body0);                 // the rule's own declarations; nested rules are handled after it
            // split the selector list on top-level commas (not inside (...), [...] or quotes)
            let sl: int = strLen(selText); let a: int = 0; let k: int = 0;
            let pd: int = 0; let q: int = 0;
            while (k <= sl) {
                let ch: int = 0;
                if (k < sl) ch = selText[k] as int;
                if (q != 0) { if (ch == q) q = 0; }
                else if (ch == 34 || ch == 39) { q = ch; }
                else if (ch == 40 || ch == 91) { pd = pd + 1; }
                else if (ch == 41 || ch == 93) { if (pd > 0) pd = pd - 1; }
                if (k == sl || (ch == 44 && pd == 0 && q == 0)) {
                    let one: char* = strSub(selText, a, k - a);
                    let sel: Compound* = parseSelector(one, sheet);
                    let oneT: char* = trimDup(one, 0, strLen(one));
                    strFree(one);
                    if (sel != nullptr) sheetAddSel(sheet, sel, parseDecls(body), oneT);
                    strFree(oneT);
                    a = k + 1;
                }
                k = k + 1;
            }
            if (isNested && depth < 6) {
                // nested rules: "parent { child { ... } @media (...) { ... } }" -> "parent child { ... }" / "parent { ... }" under the @media
                let it: NestIt; it.pos = 0; it.ps = 0; it.pe = 0; it.bs = 0; it.be = 0; it.found = false;
                let go2: bool = true;
                while (go2) {
                    nestNext(body0, &it);
                    if (!it.found) { go2 = false; }
                    else {
                        let pre: char* = trimDup(body0, it.ps, it.pe);
                        let inner: char* = strSub(body0, it.bs, it.be - it.bs);
                        if (strLen(pre) > 0) {
                            if ((pre[0] as int) == 64) {
                                if (atRuleEnter(pre)) {
                                    let h1: char* = strCat(selText, " {");
                                    let h2: char* = strCat(h1, inner); strFree(h1);
                                    let h3: char* = strCat(h2, "}"); strFree(h2);
                                    sheetParseBlock(sheet, h3, depth + 1);
                                    strFree(h3);
                                }
                            } else {
                                let comb: char* = nestJoin(selText, pre);
                                let g1: char* = strCat(comb, " {"); strFree(comb);
                                let g2: char* = strCat(g1, inner); strFree(g1);
                                let g3: char* = strCat(g2, "}"); strFree(g2);
                                sheetParseBlock(sheet, g3, depth + 1);
                                strFree(g3);
                            }
                        }
                        strFree(pre); strFree(inner);
                    }
                }
            }
            strFree(selText); strFree(body);
            if (isNested) strFree(body0);
        }
    }
}
