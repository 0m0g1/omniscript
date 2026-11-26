// browser/html/html.os  -- TIER 1: tokenizer + tree builder fused for the draft.
// Collects <style>/<script> text and <link>/<script src> references into Doc; the shell fetches links.
// Entities (&amp; &#8209; &#x2011; ...) are decoded in text runs and attribute values (decode() below).
// NEXT: split tokenizer/tree-builder, implied <html>/<body>, optional end tags, error recovery.
module U = import "util.os";
module UI = import "standard/1/Ui.os";
module Str = import "standard/1/String.os";

struct Doc {
    root: UI.Node* = nullptr;
    css: char* = nullptr;
    js: char* = nullptr;
    cssLinks: [16]char*;
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

// ---- entities ----
// The Ui font may lack typographic glyphs, so these common ones are turned into plain ASCII. Set to false to keep them as UTF-8.
let SIMPLE_PUNCT: bool = true;

function namedCode(n: char*) => int {
    if (U.streq(n, "amp")) return 38;
    if (U.streq(n, "lt")) return 60;
    if (U.streq(n, "gt")) return 62;
    if (U.streq(n, "quot")) return 34;
    if (U.streq(n, "apos")) return 39;
    if (U.streq(n, "nbsp")) return 160;
    if (U.streq(n, "shy")) return 173;
    if (U.streq(n, "copy")) return 169;
    if (U.streq(n, "reg")) return 174;
    if (U.streq(n, "laquo")) return 171;
    if (U.streq(n, "raquo")) return 187;
    if (U.streq(n, "middot")) return 183;
    if (U.streq(n, "times")) return 215;
    if (U.streq(n, "ndash")) return 8211;
    if (U.streq(n, "mdash")) return 8212;
    if (U.streq(n, "lsquo")) return 8216;
    if (U.streq(n, "rsquo")) return 8217;
    if (U.streq(n, "ldquo")) return 8220;
    if (U.streq(n, "rdquo")) return 8221;
    if (U.streq(n, "bull")) return 8226;
    if (U.streq(n, "hellip")) return 8230;
    if (U.streq(n, "euro")) return 8364;
    if (U.streq(n, "trade")) return 8482;
    if (U.streq(n, "larr")) return 8592;
    if (U.streq(n, "rarr")) return 8594;
    return 0 - 1;
}

// code point -> UTF-8 bytes (arithmetic only: no shifts)
function putUtf8(out: Str.String*, cp: int) => void {
    if (cp < 128) out.appendChar(cp as char);
    else if (cp < 2048) {
        out.appendChar((192 + cp / 64) as char);
        out.appendChar((128 + cp % 64) as char);
    }
    else if (cp < 65536) {
        out.appendChar((224 + cp / 4096) as char);
        out.appendChar((128 + (cp / 64) % 64) as char);
        out.appendChar((128 + cp % 64) as char);
    }
    else {
        out.appendChar((240 + cp / 262144) as char);
        out.appendChar((128 + (cp / 4096) % 64) as char);
        out.appendChar((128 + (cp / 64) % 64) as char);
        out.appendChar((128 + cp % 64) as char);
    }
}

function putCode(out: Str.String*, cp: int) => void {
    if (cp <= 0 || cp > 1114111 || (cp >= 55296 && cp <= 57343)) { out.append("?"); return; }
    if (SIMPLE_PUNCT) {
        if (cp == 173) return;                                                   // soft hyphen: invisible
        if (cp == 160 || cp == 8194 || cp == 8195 || cp == 8201 || cp == 8239) { out.append(" "); return; }
        if (cp >= 8208 && cp <= 8211) { out.append("-"); return; }               // hyphens, non-breaking hyphen, en dash
        if (cp == 8212) { out.append("--"); return; }
        if (cp == 8216 || cp == 8217) { out.append("'"); return; }
        if (cp == 8220 || cp == 8221) { out.append("\""); return; }
        if (cp == 8226 || cp == 183) { out.append("*"); return; }
        if (cp == 8230) { out.append("..."); return; }
    }
    putUtf8(out, cp);
}

// &name; &#123; &#x7B;  ->  characters. Unknown or malformed references stay as typed.
function decode(s: char*) => char* {
    if (U.indexOf(s, 0, "&") < 0) return s;
    let out: Str.String;
    let n: int = U.slen(s);
    let i: int = 0;
    while (i < n) {
        let c: int = s[i] as int;
        let used: int = 0;
        if (c == 38) {
            let j: int = i + 1;
            let cp: int = 0 - 1;
            if ((s[j] as int) == 35) {                                   // numeric
                j = j + 1;
                let hex: bool = false;
                if ((s[j] as int) == 120 || (s[j] as int) == 88) { hex = true; j = j + 1; }
                let v: int = 0;
                let nd: int = 0;
                let more: bool = true;
                while (more && nd < 8) {
                    let ch: int = s[j] as int;
                    let d: int = 0 - 1;
                    if (hex) d = U.hexv(ch);
                    else if (U.isDigit(ch)) d = ch - 48;
                    if (d < 0) more = false;
                    else {
                        if (hex) v = v * 16 + d; else v = v * 10 + d;
                        nd = nd + 1;
                        j = j + 1;
                    }
                }
                if (nd > 0) { cp = v; if ((s[j] as int) == 59) j = j + 1; }
            }
            else {                                                       // named: needs the closing ';'
                let k: int = j;
                while (k < n && k - j < 10 && U.isAlnum(s[k] as int)) { k = k + 1; }
                if (k > j && (s[k] as int) == 59) {
                    cp = namedCode(U.sub(s, j, k));
                    if (cp >= 0) j = k + 1;
                }
            }
            if (cp >= 0) { putCode(&out, cp); used = j - i; }
        }
        if (used > 0) i = i + used;
        else { out.appendChar(c as char); i = i + 1; }
    }
    let r: char* = U.dup(out.cstr());
    out.free();
    return r;
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
                let tagStart: int = i;
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
                                av = decode(U.sub(src, b, j));
                                j = j + 1;
                            } else {
                                let b: int = j;
                                while (j < n && !U.isSpace(src[j] as int) && (src[j] as int) != 62) { j = j + 1; }
                                av = decode(U.sub(src, b, j));
                            }
                        }
                        el.setAttr(an, av);                              // Ui caches id/class/style/type/value/... itself
                        if (U.streq(an, "href")) href = av;
                        else if (U.streq(an, "src")) srcAttr = av;
                        else if (U.streq(an, "rel")) rel = av;
                    }
                }
                i = j;
                if (U.streq(name, "svg") && !selfClose) {
                    // inline <svg>: keep its whole source text (nested <svg> counted) for the rasterizer, skip the children
                    let depth: int = 1;
                    let p: int = i;
                    let endAt: int = n;
                    while (depth > 0 && p < n) {
                        let lt: int = U.indexOf(src, p, "<");
                        if (lt < 0) p = n;
                        else if (U.startsAt(src, lt, "</svg")) {
                            depth = depth - 1;
                            let g: int = U.indexOf(src, lt, ">");
                            if (g < 0) p = n;
                            else { p = g + 1; if (depth == 0) endAt = p; }
                        }
                        else if (U.startsAt(src, lt, "<svg")) { depth = depth + 1; p = lt + 4; }
                        else p = lt + 1;
                    }
                    let raw: char* = U.sub(src, tagStart, endAt);
                    let head: char* = "<svg";
                    if (U.indexOf(raw, 0, "xmlns=") < 0) head = U.cat(head, " xmlns=\"http://www.w3.org/2000/svg\"");
                    if (U.indexOf(raw, 0, "fill=") < 0) head = U.cat(head, " fill=\"currentColor\"");
                    raw = U.cat(head, U.sub(raw, 4, U.slen(raw)));
                    el.setAttr("data-osb-svg", raw);
                    stack[sp].append(el);
                    i = endAt;
                }
                else if (U.streq(name, "style") || U.streq(name, "script")) {
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
                    if (U.streq(name, "link") && U.streq(rel, "stylesheet") && doc.cssN < 16) {
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
            if (!allSpace(t)) stack[sp].append(UI.createText(decode(t)));
            i = e;
        }
    }
}