// browser/viewers/markdown.os -- Markdown -> HTML body.
// Blocks: # headings, paragraphs, ``` code fences, - * + and 1. lists (flat), > quotes, --- rules, | tables |.
// Inline: **bold**, *italic*, `code`, [text](url), ![alt](src), backslash escapes. Raw HTML in the source is shown as text.
module Str = import "standard/1/String.os";
module U = import "browser/util.os";
module K = import "browser/viewers/common.os";

function inline(s: char*, out: Str.String*) => void {
    let n: int = U.slen(s);
    let i: int = 0;
    while (i < n) {
        let c: int = s[i] as int;
        if (c == 92 && i + 1 < n) {
            K.escCh(out, s[i + 1] as int);
            i = i + 2;
        }
        else if (c == 96) {
            let j: int = U.indexOf(s, i + 1, "`");
            if (j >= 0) { out.append("<code>"); K.escRange(out, s, i + 1, j); out.append("</code>"); i = j + 1; }
            else { out.append("`"); i = i + 1; }
        }
        else if (c == 42 && i + 1 < n && (s[i + 1] as int) == 42) {
            let j2: int = U.indexOf(s, i + 2, "**");
            if (j2 >= 0) { out.append("<b>"); inline(U.sub(s, i + 2, j2), out); out.append("</b>"); i = j2 + 2; }
            else { out.append("**"); i = i + 2; }
        }
        else if (c == 42) {
            let j3: int = U.indexOf(s, i + 1, "*");
            if (j3 > i + 1) { out.append("<i>"); inline(U.sub(s, i + 1, j3), out); out.append("</i>"); i = j3 + 1; }
            else { out.append("*"); i = i + 1; }
        }
        else if ((c == 33 && i + 1 < n && (s[i + 1] as int) == 91) || c == 91) {
            let img: bool = c == 33;
            let st: int = img ? i + 2 : i + 1;
            let a: int = U.indexOf(s, st, "]");
            let ok: bool = a >= 0 && a + 1 < n && (s[a + 1] as int) == 40;
            let b: int = -1;
            if (ok) { b = U.indexOf(s, a + 2, ")"); ok = b >= 0; }
            if (ok) {
                if (img) {
                    out.append("<img alt=\"");
                    K.escRange(out, s, st, a);
                    out.append("\" src=\"");
                    K.escRange(out, s, a + 2, b);
                    out.append("\">");
                } else {
                    out.append("<a href=\"");
                    K.escRange(out, s, a + 2, b);
                    out.append("\">");
                    inline(U.sub(s, st, a), out);
                    out.append("</a>");
                }
                i = b + 1;
            } else { K.escCh(out, c); i = i + 1; }
        }
        else { K.escCh(out, c); i = i + 1; }
    }
}

function isBlank(s: char*) => bool {
    let i: int = 0;
    while ((s[i] as int) != 0) {
        if (!U.isSpace(s[i] as int)) return false;
        i = i + 1;
    }
    return true;
}

// first non-space index
function indentOf(s: char*) => int {
    let i: int = 0;
    while ((s[i] as int) == 32 || (s[i] as int) == 9) i = i + 1;
    return i;
}

// "---", "***", "___" (3 or more, spaces allowed)
function isRule(s: char*) => bool {
    let ch: int = 0;
    let cnt: int = 0;
    let i: int = 0;
    while ((s[i] as int) != 0) {
        let c: int = s[i] as int;
        if (c != 32) {
            if ((c != 45 && c != 42 && c != 95) || (ch != 0 && ch != c)) return false;
            ch = c;
            cnt = cnt + 1;
        }
        i = i + 1;
    }
    return cnt >= 3;
}

// table separator row: only | - : and spaces
function isSepRow(s: char*) => bool {
    let dashes: int = 0;
    let i: int = 0;
    while ((s[i] as int) != 0) {
        let c: int = s[i] as int;
        if (c == 45) dashes = dashes + 1;
        else if (c != 124 && c != 58 && c != 32) return false;
        i = i + 1;
    }
    return dashes > 0;
}

function tableRow(line: char*, tag: char*, out: Str.String*) => void {
    let n: int = U.slen(line);
    let i: int = indentOf(line);
    if (i < n && (line[i] as int) == 124) i = i + 1;
    out.append("<tr>");
    let more: bool = i < n;
    while (more) {
        let e: int = i;
        while (e < n && (line[e] as int) != 124) e = e + 1;
        if (e >= n && U.slen(U.trim(U.sub(line, i, e))) == 0) { more = false; }
        else {
            out.append("<"); out.append(tag); out.append(">");
            inline(U.trim(U.sub(line, i, e)), out);
            out.append("</"); out.append(tag); out.append(">");
            i = e + 1;
            if (e >= n) more = false;
        }
    }
    out.append("</tr>");
}

function flushPara(para: Str.String*, out: Str.String*) => void {
    if (para.len > 0) {
        out.append("<p>");
        inline(para.cstr(), out);
        out.append("</p>");
        para.clear();
    }
}

function closeList(list: int*, out: Str.String*) => void {
    if (list[0] == 1) out.append("</ul>");
    else if (list[0] == 2) out.append("</ol>");
    list[0] = 0;
}

function closeTable(tbl: int*, out: Str.String*) => void {
    if (tbl[0] != 0) out.append("</table>");
    tbl[0] = 0;
}

function toHtml(src: char*, out: Str.String*) => void {
    let n: int = U.slen(src);
    let i: int = 0;
    let para: Str.String;
    let inCode: bool = false;
    let list: int = 0;
    let tbl: int = 0;
    while (i < n) {
        let e: int = i;
        while (e < n && (src[e] as int) != 10) e = e + 1;
        let line: char* = U.sub(src, i, e);
        if (e > i && (line[e - i - 1] as int) == 13) line = U.sub(line, 0, e - i - 1);
        i = e + 1;
        let ind: int = indentOf(line);
        let t: char* = &line[ind];
        let c0: int = t[0] as int;

        if (inCode) {
            if (U.startsAt(t, 0, "```")) { out.append("</pre>"); inCode = false; }
            else { K.esc(out, line); out.append("\n"); }
        }
        else if (U.startsAt(t, 0, "```")) {
            flushPara(&para, out); closeList(&list, out); closeTable(&tbl, out);
            out.append("<pre>");
            inCode = true;
        }
        else if (isBlank(line)) {
            flushPara(&para, out); closeList(&list, out); closeTable(&tbl, out);
        }
        else if (c0 == 35) {
            let lv: int = 0;
            while ((t[lv] as int) == 35 && lv < 6) lv = lv + 1;
            if ((t[lv] as int) == 32) {
                flushPara(&para, out); closeList(&list, out); closeTable(&tbl, out);
                out.append("<h"); out.appendInt(lv as i64); out.append(">");
                inline(U.trim(&t[lv]), out);
                out.append("</h"); out.appendInt(lv as i64); out.append(">");
            } else { if (para.len > 0) para.append(" "); para.append(t); }
        }
        else if (isRule(t)) {
            flushPara(&para, out); closeList(&list, out); closeTable(&tbl, out);
            out.append("<hr>");
        }
        else if (c0 == 62) {
            flushPara(&para, out); closeList(&list, out); closeTable(&tbl, out);
            let q: int = (t[1] as int) == 32 ? 2 : 1;
            out.append("<blockquote>"); inline(&t[q], out); out.append("</blockquote>");
        }
        else if ((c0 == 45 || c0 == 42 || c0 == 43) && (t[1] as int) == 32) {
            flushPara(&para, out); closeTable(&tbl, out);
            if (list != 1) { closeList(&list, out); out.append("<ul>"); list = 1; }
            out.append("<li>"); inline(&t[2], out); out.append("</li>");
        }
        else if (U.isDigit(c0)) {
            let d: int = 0;
            while (U.isDigit(t[d] as int)) d = d + 1;
            if ((t[d] as int) == 46 && (t[d + 1] as int) == 32) {
                flushPara(&para, out); closeTable(&tbl, out);
                if (list != 2) { closeList(&list, out); out.append("<ol>"); list = 2; }
                out.append("<li>"); inline(&t[d + 2], out); out.append("</li>");
            } else { if (para.len > 0) para.append(" "); para.append(t); }
        }
        else if (c0 == 124) {
            flushPara(&para, out); closeList(&list, out);
            if (tbl == 0) { out.append("<table>"); tableRow(t, "th", out); tbl = 1; }
            else if (tbl == 1 && isSepRow(t)) { tbl = 2; }
            else { tableRow(t, "td", out); tbl = 2; }
        }
        else {
            closeList(&list, out); closeTable(&tbl, out);
            if (para.len > 0) para.append(" ");
            para.append(t);
        }
    }
    if (inCode) out.append("</pre>");
    flushPara(&para, out); closeList(&list, out); closeTable(&tbl, out);
    para.free();
}

// registry entry: complete HTML page for a Markdown file; the Download link is added because the file is shown as an attachment
function view(data: char*, name: char*) => char* {
    let body: Str.String;
    toHtml(data, &body);
    let r: char* = K.page(name, &body, name);
    body.free();
    return r;
}
