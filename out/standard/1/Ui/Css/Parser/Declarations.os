// standard/1/Ui/Css/Parser/Declarations.os
// "a: b; c: d !important" -> list of (lowercase name, value).

// ---- declaration lists ----
function freeAttrs(a0: Attr*) => void {
    let a: Attr* = a0;
    while (a != nullptr) { let nx: Attr* = a.next; strFree(a.name); strFree(a.value); CMem.free(a as void*); a = nx; }
}

// "a: b; c: d" -> list of (lowercase name, value). An `!important` declaration keeps a "!" in front of its name.
function parseDecls(text: char*) => Attr* {
    let head: Attr* = nullptr; let tail: Attr* = nullptr;
    let n: int = strLen(text);
    let i: int = 0;
    while (i < n) {
        let start: int = i; let depth: int = 0; let colon: int = 0 - 1; let quote: int = 0;
        let more: bool = true;
        while (i < n && more) {
            let c: int = text[i] as int;
            if (quote != 0) { if (c == quote) quote = 0; }
            else if (c == 34 || c == 39) { quote = c; }
            else if (c == 40) { depth = depth + 1; }
            else if (c == 41) { if (depth > 0) depth = depth - 1; }
            else if (c == 58 && depth == 0 && colon < 0) { colon = i; }
            else if (c == 59 && depth == 0) { more = false; }
            if (more) i = i + 1;
        }
        if (colon > start) {
            let name: char* = trimDup(text, start, colon);
            let value: char* = trimDup(text, colon + 1, i);
            let vl: int = strLen(value);
            let bang: int = 0 - 1; let k: int = 0;
            while (k < vl) { if (value[k] as int == 33 && bang < 0) bang = k; k = k + 1; }
            let important: bool = false;
            if (bang >= 0) {
                let bt: char* = trimDup(value, bang + 1, vl);
                important = strEqI(bt, "important");
                strFree(bt);
                let v2: char* = trimDup(value, 0, bang); strFree(value); value = v2;
            }
            let ln: char* = strLowerDup(name);
            strFree(name);
            if (important) { let ln2: char* = strCat("!", ln); strFree(ln); ln = ln2; gSawImportant = true; }   // "!color": applied in the important pass
            if (isCustomName(ln)) gSheetHasVars = true;
            let a: Attr* = CMem.malloc(sizeof(Attr)) as Attr*;
            a.name = ln; a.value = value; a.next = nullptr;
            if (tail != nullptr) tail.next = a; else head = a;
            tail = a;
        }
        i = i + 1;
    }
    return head;
}
