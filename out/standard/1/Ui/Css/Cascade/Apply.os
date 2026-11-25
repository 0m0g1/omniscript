// standard/1/Ui/Css/Cascade/Apply.os
// Declaration lists -> Style. var() substitution and the important filter live in applyDeclsRec.

function applyDecls(st: Style*, decls: Attr*, ctx: CssCtx*, pass: int) => void {
    applyDeclsRec(st, decls, ctx, pass, nullptr, SRC_AUTHOR, nullptr);
}

// parse + apply a style="..." string (font-size first, then the rest)
function applyDeclText(st: Style*, text: char*, ctx: CssCtx*, parentPx: float) => void {
    applyDeclTextRec(st, text, ctx, parentPx, nullptr, SRC_INLINE, nullptr);
}

// Like applyDecls, and when `n` is non-null records one StyleNote per declaration.
// Normal pass skips "!name" (important) declarations; the important pass (gImportantPass) applies only those.
// var(...) is substituted from the current scope chain (gScope) just before applyProp sees the value.
function applyDeclsRec(st: Style*, decls: Attr*, ctx: CssCtx*, pass: int, n: Node*, source: int, selector: char*) => void {
    let a: Attr* = decls;
    while (a != nullptr) {
        let imp: bool = (a.name[0] as int) == 33;
        if (imp == gImportantPass) {
            let nm: char* = a.name;
            let ownName: bool = false;
            if (imp) { nm = strSub(a.name, 1, strLen(a.name) - 1); ownName = true; }
            let val: char* = a.value;
            let ownVal: bool = false;
            if (gSheetHasVars && containsStr(a.value, "var(")) { val = varSubstDepth(a.value, 0); ownVal = true; }
            let res: int = AP_SKIP;
            if (val != nullptr) res = applyProp(st, nm, val, ctx, pass);
            else if (pass == 1) res = AP_INVALID;                 // var() with no value and no fallback: invalid at computed-value time
            if (n != nullptr && res != AP_SKIP) noteAdd(n, nm, a.value, source, res, selector);
            if (ownVal) strFree(val);
            if (ownName) strFree(nm);
        }
        a = a.next;
    }
}

function applyDeclTextRec(st: Style*, text: char*, ctx: CssCtx*, parentPx: float, n: Node*, source: int, selector: char*) => void {
    let d: Attr* = parseDecls(text);
    ctx.curPx = parentPx; applyDeclsRec(st, d, ctx, 0, n, source, selector);
    ctx.curPx = st.fontPx; applyDeclsRec(st, d, ctx, 1, n, source, selector);
    freeAttrs(d);
}
