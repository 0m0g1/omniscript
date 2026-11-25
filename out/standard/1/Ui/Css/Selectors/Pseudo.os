// standard/1/Ui/Css/Selectors/Pseudo.os
// Pseudo-classes that need no extra Compound fields: they are one bit in Compound.pseudo (Core/Constants.os).
//   :only-child :empty :first-of-type :last-of-type :link :any-link
// Not yet (each needs a new Compound field in Types.os): :not() :is() :where() :has() :nth-child() :nth-of-type().
// Pseudo-elements (::before ::after ...) still never match.

function pseudoExtBit(pn: char*) => int {
    let r: int = 0;
    if (strEqI(pn, "only-child")) r = PSE_ONLY;
    else if (strEqI(pn, "empty")) r = PSE_EMPTY;
    else if (strEqI(pn, "first-of-type")) r = PSE_FIRST_T;
    else if (strEqI(pn, "last-of-type")) r = PSE_LAST_T;
    else if (strEqI(pn, "link") || strEqI(pn, "any-link")) r = PSE_LINK;
    return r;
}

// true when a sibling before (prev) / after (!prev) `n` has the same tag
function hasSiblingOfType(n: Node*, prev: bool) => bool {
    let s: Node* = n.nextEl();
    if (prev) s = n.prevEl();
    let found: bool = false;
    while (s != nullptr && !found) {
        if (strEq(s.tag, n.tag)) found = true;
        if (prev) s = s.prevEl(); else s = s.nextEl();
    }
    return found;
}

function matchPseudoExt(n: Node*, bits: int) => bool {
    let ok: bool = true;
    if ((bits & PSE_ONLY) != 0) { if (n.prevEl() != nullptr || n.nextEl() != nullptr) ok = false; }
    if ((bits & PSE_EMPTY) != 0) { if (n.first != nullptr) ok = false; }
    if ((bits & PSE_FIRST_T) != 0) { if (hasSiblingOfType(n, true)) ok = false; }
    if ((bits & PSE_LAST_T) != 0) { if (hasSiblingOfType(n, false)) ok = false; }
    if ((bits & PSE_LINK) != 0) { if (!(strEq(n.tag, "a") && n.hasAttr("href"))) ok = false; }
    return ok;
}
