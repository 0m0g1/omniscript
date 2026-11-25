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
                let abs: int = lx.i; let ad: int = 1;
                while (lx.peek() != 0 && ad > 0) {
                    if (lx.peek() == 123) ad = ad + 1;
                    if (lx.peek() == 125) ad = ad - 1;
                    if (ad > 0) lx.i = lx.i + 1;
                }
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
            let bs: int = lx.i; let depth2: int = 1;
            while (lx.peek() != 0 && depth2 > 0) {
                if (lx.peek() == 123) depth2 = depth2 + 1;
                if (lx.peek() == 125) depth2 = depth2 - 1;
                if (depth2 > 0) lx.i = lx.i + 1;
            }
            let body: char* = strSub(css, bs, lx.i - bs);
            if (lx.peek() == 125) lx.i = lx.i + 1;
            // split the selector list on commas
            let sl: int = strLen(selText); let a: int = 0; let k: int = 0;
            while (k <= sl) {
                if (k == sl || selText[k] as int == 44) {
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
