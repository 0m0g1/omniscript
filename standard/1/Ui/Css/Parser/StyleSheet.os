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
            let body: char* = strSub(css, bs, lx.i - bs);
            if (lx.peek() == 125) lx.i = lx.i + 1;
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
            strFree(selText); strFree(body);
        }
    }
}
