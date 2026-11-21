// browser/html/html.os  -- TIER 1: tokenizer + tree builder fused for the draft.
// Collects <style>/<script> text and <link>/<script src> references into Doc; the shell fetches links.
// NEXT: split tokenizer/tree-builder, entities (&amp; ...), implied <html>/<body>, optional end tags, error recovery.
module U = import "browser/util.os";
module UI = import "standard/1/Ui.os";

struct Doc {
    root: UI.Node* = nullptr;
    css: char* = nullptr;
    js: char* = nullptr;
    cssLinks: [4]char*;
    cssN: int = 0;
    jsLinks: [4]char*;
    jsN: int = 0;
}

function isVoid(t: char*) => bool {
    return U.streq(t, "br") || U.streq(t, "meta") || U.streq(t, "link") || U.streq(t, "img") || U.streq(t, "hr") || U.streq(t, "input");
}

function allSpace(s: char*) => bool {
    let i: int = 0;
    let ok: bool = true;
    while ((s[i] as int) != 0) { if (!U.isSpace(s[i] as int)) ok = false; i = i + 1; }
    return ok;
}

function parseHtml(src: char*, doc: Doc*) => void {
    let n: int = U.slen(src);
    doc.css = U.dup("");
    doc.js = U.dup("");
    doc.root = UI.createByTag("html");
    let stack: UI.Node** = U.alloc(8 * 128) as UI.Node**;
    let names: char** = U.alloc(8 * 128) as char**;
    names[0] = "html";
    let sp: int = 0;
    stack[0] = doc.root;
    let i: int = 0;
    while (i < n) {
        let c: int = src[i] as int;
        if (c == 60) {                                           // '<'
            if (U.startsAt(src, i, "<!--")) {
                let e: int = U.indexOf(src, i + 4, "-->");
                if (e < 0) i = n; else i = e + 3;
            } else if ((src[i + 1] as int) == 33) {              // <!doctype ...>
                let e: int = U.indexOf(src, i, ">");
                if (e < 0) i = n; else i = e + 1;
            } else if ((src[i + 1] as int) == 47) {              // </tag>
                let j: int = i + 2;
                while (U.isAlnum(src[j] as int)) { j = j + 1; }
                let name: char* = U.lower(U.sub(src, i + 2, j));
                let k: int = sp;
                while (k > 0 && !U.streq(names[k], name)) { k = k - 1; }
                if (k > 0) sp = k - 1;
                let e: int = U.indexOf(src, j, ">");
                if (e < 0) i = n; else i = e + 1;
            } else {                                             // <tag attrs>
                let j: int = i + 1;
                while (U.isAlnum(src[j] as int)) { j = j + 1; }
                let name: char* = U.lower(U.sub(src, i + 1, j));
                let el: UI.Node* = UI.createByTag(name);
                let href: char* = "";
                let srcAttr: char* = "";
                let rel: char* = "";
                let selfClose: bool = false;
                let inTag: bool = true;
                while (inTag && j < n) {
                    let ch: int = src[j] as int;
                    if (ch == 62) { inTag = false; j = j + 1; }
                    else if (ch == 47) { selfClose = true; j = j + 1; }
                    else if (U.isSpace(ch)) { j = j + 1; }
                    else {
                        let a: int = j;
                        while (j < n && !U.isSpace(src[j] as int) && (src[j] as int) != 61 && (src[j] as int) != 62 && (src[j] as int) != 47) { j = j + 1; }
                        let an: char* = U.lower(U.sub(src, a, j));
                        let av: char* = "";
                        if ((src[j] as int) == 61) {
                            j = j + 1;
                            let q: int = src[j] as int;
                            if (q == 34 || q == 39) {
                                let b: int = j + 1;
                                j = b;
                                while (j < n && (src[j] as int) != q) { j = j + 1; }
                                av = U.sub(src, b, j);
                                j = j + 1;
                            } else {
                                let b: int = j;
                                while (j < n && !U.isSpace(src[j] as int) && (src[j] as int) != 62) { j = j + 1; }
                                av = U.sub(src, b, j);
                            }
                        }
                        el.setAttr(an, av);                              // Ui caches id/class/style/type/value/... itself
                        if (U.streq(an, "href")) href = av;
                        else if (U.streq(an, "src")) srcAttr = av;
                        else if (U.streq(an, "rel")) rel = av;
                    }
                }
                i = j;
                if (U.streq(name, "style") || U.streq(name, "script")) {
                    let close: char* = U.cat("</", name);
                    let e: int = U.indexOf(src, i, close);
                    if (e < 0) e = n;
                    let body: char* = U.sub(src, i, e);
                    if (U.streq(name, "style")) doc.css = U.cat(doc.css, U.cat(body, "\n"));
                    else if (U.slen(srcAttr) > 0) { if (doc.jsN < 4) { doc.jsLinks[doc.jsN] = srcAttr; doc.jsN = doc.jsN + 1; } }
                    else doc.js = U.cat(doc.js, U.cat(body, "\n"));
                    let g: int = U.indexOf(src, e, ">");
                    if (g < 0) i = n; else i = g + 1;
                } else {
                    stack[sp].append(el);
                    if (U.streq(name, "link") && U.streq(rel, "stylesheet") && doc.cssN < 4) {
                        doc.cssLinks[doc.cssN] = href;
                        doc.cssN = doc.cssN + 1;
                    }
                    if (!isVoid(name) && !selfClose && sp < 126) { sp = sp + 1; stack[sp] = el; names[sp] = name; }
                }
            }
        } else {                                                 // text run
            let e: int = U.indexOf(src, i, "<");
            if (e < 0) e = n;
            let t: char* = U.sub(src, i, e);
            if (!allSpace(t)) stack[sp].append(UI.createText(t));
            i = e;
        }
    }
}
