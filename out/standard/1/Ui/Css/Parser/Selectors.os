// standard/1/Ui/Css/Parser/Selectors.os
// Selector text -> right-most Compound with a `left` chain; specificity.

// one compound selector; nullptr when nothing could be read
function parseCompound(lx: Lex*, sheet: Sheet*) => Compound* {
    let c: Compound* = CMem.malloc(sizeof(Compound)) as Compound*;
    CMem.memset(c as void*, 0, sizeof(Compound));
    c.comb = COMB_NONE;
    let read: int = 0; let more: bool = true;
    while (more) {
        let ch: int = lx.peek();
        if (ch == 42) { lx.i = lx.i + 1; read = read + 1; }
        else if (ch == 35) { lx.i = lx.i + 1; c.id = readIdent(lx); read = read + 1; }
        else if (ch == 46) {
            lx.i = lx.i + 1;
            let nm: char* = readIdent(lx);
            if (c.cls == nullptr) { c.cls = nm; }
            else { let joined: char* = strCat(c.cls, " "); let j2: char* = strCat(joined, nm); strFree(joined); strFree(nm); strFree(c.cls); c.cls = j2; }
            read = read + 1;
        }
        else if (ch == 91) {
            lx.i = lx.i + 1; lx.skipWs();
            c.attr = strLowerDup("");
            strFree(c.attr);
            let an: char* = readIdent(lx);
            c.attr = strLowerDup(an); strFree(an);
            lx.skipWs();
            c.attrOp = AOP_EXISTS;
            let o: int = lx.peek();
            if (o == 61) { c.attrOp = AOP_EQ; lx.i = lx.i + 1; }
            else if (o == 126) { c.attrOp = AOP_WORD; lx.i = lx.i + 2; }
            else if (o == 94) { c.attrOp = AOP_PREFIX; lx.i = lx.i + 2; }
            else if (o == 36) { c.attrOp = AOP_SUFFIX; lx.i = lx.i + 2; }
            else if (o == 42) { c.attrOp = AOP_SUB; lx.i = lx.i + 2; }
            if (c.attrOp != AOP_EXISTS) {
                lx.skipWs();
                let q: int = lx.peek();
                if (q == 34 || q == 39) {
                    lx.i = lx.i + 1;
                    let vs: int = lx.i;
                    while (lx.peek() != q && lx.peek() != 0) lx.i = lx.i + 1;
                    c.attrVal = strSub(lx.s, vs, lx.i - vs);
                    if (lx.peek() == q) lx.i = lx.i + 1;
                } else {
                    let vs2: int = lx.i;
                    while (lx.peek() != 93 && lx.peek() != 0 && !isSpaceC(lx.peek())) lx.i = lx.i + 1;
                    c.attrVal = strSub(lx.s, vs2, lx.i - vs2);
                }
            }
            lx.skipWs();
            if (lx.peek() == 93) lx.i = lx.i + 1;
            read = read + 1;
        }
        else if (ch == 58) {
            lx.i = lx.i + 1;
            if (lx.peek() == 58) { lx.i = lx.i + 1; c.never = true; }                 // ::before etc.  NEXT
            let pn: char* = readIdent(lx);
            if (lx.peek() == 40) { skipBalanced(lx); c.never = true; }                 // :not() :nth-child()  NEXT
            else if (strEqI(pn, "hover")) { c.pseudo = c.pseudo | PSE_HOVER; sheet.usesPseudo = true; }
            else if (strEqI(pn, "focus") || strEqI(pn, "focus-visible")) { c.pseudo = c.pseudo | PSE_FOCUS; sheet.usesPseudo = true; }
            else if (strEqI(pn, "active")) { c.pseudo = c.pseudo | PSE_ACTIVE; sheet.usesPseudo = true; }
            else if (strEqI(pn, "checked")) { c.pseudo = c.pseudo | PSE_CHECKED; sheet.usesPseudo = true; }
            else if (strEqI(pn, "disabled")) { c.pseudo = c.pseudo | PSE_DISABLED; }
            else if (strEqI(pn, "enabled")) { c.pseudo = c.pseudo | PSE_ENABLED; }
            else if (strEqI(pn, "first-child")) { c.pseudo = c.pseudo | PSE_FIRST; }
            else if (strEqI(pn, "last-child")) { c.pseudo = c.pseudo | PSE_LAST; }
            else if (strEqI(pn, "root")) { c.pseudo = c.pseudo | PSE_ROOT; }
            else { let xb: int = pseudoExtBit(pn); if (xb != 0) c.pseudo = c.pseudo | xb; else c.never = true; }   // see Selectors/Pseudo.os
            strFree(pn);
            read = read + 1;
        }
        else if (isNameC(ch)) {
            let tn: char* = readIdent(lx);
            c.tag = strLowerDup(tn); strFree(tn);
            read = read + 1;
        }
        else { more = false; }
    }
    if (read == 0) { CMem.free(c as void*); return nullptr; }
    return c;
}

// right-most compound of the parsed selector (nullptr if it does not parse)
function parseSelector(text: char*, sheet: Sheet*) => Compound* {
    let lx: Lex; lx.s = text; lx.i = 0; lx.ok = false; lx.val = 0.0;
    let last: Compound* = nullptr; let bad: bool = false; let more: bool = true;
    while (more) {
        let sawSpace: bool = false;
        while (isSpaceC(lx.peek())) { lx.i = lx.i + 1; sawSpace = true; }
        let ch: int = lx.peek();
        if (ch == 0) { more = false; }
        else {
            let comb: int = COMB_DESC;
            if (ch == 62) { comb = COMB_CHILD; lx.i = lx.i + 1; lx.skipWs(); }
            else if (ch == 43) { comb = COMB_ADJ; lx.i = lx.i + 1; lx.skipWs(); }
            else if (ch == 126) { comb = COMB_SIB; lx.i = lx.i + 1; lx.skipWs(); }
            let c: Compound* = parseCompound(&lx, sheet);
            if (c == nullptr) { bad = true; more = false; }
            else {
                c.left = last;
                if (last == nullptr) c.comb = COMB_NONE; else c.comb = comb;
                last = c;
            }
        }
    }
    if (bad) return nullptr;
    return last;
}

function specOf(c0: Compound*) => int {
    let s: int = 0; let c: Compound* = c0;
    while (c != nullptr) {
        if (c.id != nullptr) s = s + 10000;
        if (c.cls != nullptr) s = s + 100 * (1 + (strLen(c.cls) - strLen(c.cls)));
        if (c.attr != nullptr) s = s + 100;
        if (c.pseudo != 0) s = s + 100;
        if (c.tag != nullptr) s = s + 1;
        c = c.left;
    }
    return s;
}

// ---- sheet ----
function freeCompound(c0: Compound*) => void {
    let c: Compound* = c0;
    while (c != nullptr) {
        let nx: Compound* = c.left;
        strFree(c.tag); strFree(c.id); strFree(c.cls); strFree(c.attr); strFree(c.attrVal);
        CMem.free(c as void*);
        c = nx;
    }
}
