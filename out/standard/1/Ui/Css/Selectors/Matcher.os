// standard/1/Ui/Css/Selectors/Matcher.os
// Does a node match a compound / a combinator chain? (Blink: SelectorChecker)

// ---- matching ----
function matchCompound(n: Node*, c: Compound*) => bool {
    if (c.never) return false;
    if (n.kind == NK_TEXT) return false;
    let ok: bool = true;
    if (c.tag != nullptr) { if (!strEq(c.tag, n.tag)) ok = false; }
    if (ok && c.id != nullptr) { if (n.id == nullptr || !strEq(n.id, c.id)) ok = false; }
    if (ok && c.cls != nullptr) { if (n.cls == nullptr || !hasAllTokens(n.cls, c.cls)) ok = false; }
    if (ok && c.attr != nullptr) {
        let av: char* = n.getAttr(c.attr);
        if (av == nullptr) { ok = false; }
        else if (c.attrOp == AOP_EQ) { if (!strEq(av, c.attrVal)) ok = false; }
        else if (c.attrOp == AOP_WORD) { if (!hasToken(av, c.attrVal, 0, strLen(c.attrVal))) ok = false; }
        else if (c.attrOp == AOP_PREFIX) { if (!startsWith(av, c.attrVal)) ok = false; }
        else if (c.attrOp == AOP_SUFFIX) {
            let al: int = strLen(av); let vl: int = strLen(c.attrVal);
            if (vl > al || !eqAtI(av, al - vl, c.attrVal)) ok = false;
        }
        else if (c.attrOp == AOP_SUB) { if (!containsStr(av, c.attrVal)) ok = false; }
    }
    if (ok && c.pseudo != 0) {
        if ((c.pseudo & PSE_HOVER) != 0 && !n.hover) ok = false;
        if ((c.pseudo & PSE_FOCUS) != 0 && !n.focus) ok = false;
        if ((c.pseudo & PSE_ACTIVE) != 0 && !n.active) ok = false;
        if ((c.pseudo & PSE_CHECKED) != 0 && !n.checked) ok = false;
        if ((c.pseudo & PSE_DISABLED) != 0 && !n.disabled) ok = false;
        if ((c.pseudo & PSE_ENABLED) != 0 && n.disabled) ok = false;
        if ((c.pseudo & PSE_FIRST) != 0 && n.prevEl() != nullptr) ok = false;
        if ((c.pseudo & PSE_LAST) != 0 && n.nextEl() != nullptr) ok = false;
        if ((c.pseudo & PSE_ROOT) != 0 && n.parent != nullptr && !strEq(n.tag, "html")) ok = false;
        if (ok && (c.pseudo & PSE_EXT_MASK) != 0) ok = matchPseudoExt(n, c.pseudo);
    }
    return ok;
}

function matchChain(n: Node*, c: Compound*) => bool {
    if (!matchCompound(n, c)) return false;
    if (c.left == nullptr) return true;
    let ok: bool = false;
    if (c.comb == COMB_CHILD) { if (n.parent != nullptr) ok = matchChain(n.parent, c.left); }
    else if (c.comb == COMB_DESC) {
        let p: Node* = n.parent;
        while (p != nullptr && !ok) { ok = matchChain(p, c.left); p = p.parent; }
    }
    else if (c.comb == COMB_ADJ) { let s: Node* = n.prevEl(); if (s != nullptr) ok = matchChain(s, c.left); }
    else if (c.comb == COMB_SIB) {
        let s2: Node* = n.prevEl();
        while (s2 != nullptr && !ok) { ok = matchChain(s2, c.left); s2 = s2.prevEl(); }
    }
    return ok;
}
