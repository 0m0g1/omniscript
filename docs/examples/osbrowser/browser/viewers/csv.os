// browser/viewers/csv.os -- CSV / TSV -> HTML table. First row is the header. Handles "quoted, fields", "" escapes, CRLF.
// The delimiter (comma, semicolon or tab) is guessed from the first line. Shows at most CSV_MAX_ROWS rows.
module Str = import "standard/1/String.os";
module U = import "browser/util.os";
module K = import "browser/viewers/common.os";

let CSV_MAX_ROWS: int = 5000;

function guessDelim(src: char*) => int {
    let commas: int = 0; let semis: int = 0; let tabs: int = 0;
    let q: bool = false;
    let i: int = 0;
    while ((src[i] as int) != 0 && (src[i] as int) != 10) {
        let c: int = src[i] as int;
        if (c == 34) q = !q;
        else if (!q) {
            if (c == 44) commas = commas + 1;
            else if (c == 59) semis = semis + 1;
            else if (c == 9) tabs = tabs + 1;
        }
        i = i + 1;
    }
    if (tabs > commas && tabs >= semis) return 9;
    if (semis > commas) return 59;
    return 44;
}

function cell(out: Str.String*, f: Str.String*, header: bool, rowOpen: bool*) => void {
    if (!rowOpen[0]) { out.append("<tr>"); rowOpen[0] = true; }
    if (header) out.append("<th>"); else out.append("<td>");
    K.esc(out, f.cstr());
    if (header) out.append("</th>"); else out.append("</td>");
    f.clear();
}

// returns the number of data rows shown (header excluded)
function toHtml(src: char*, out: Str.String*) => int {
    let delim: int = guessDelim(src);
    let n: int = U.slen(src);
    let i: int = 0;
    let q: bool = false;
    let row: int = 0;
    let rowOpen: bool = false;
    let f: Str.String;
    out.append("<table>");
    while (i < n && row <= CSV_MAX_ROWS) {
        let c: int = src[i] as int;
        if (q) {
            if (c == 34) {
                if (i + 1 < n && (src[i + 1] as int) == 34) { f.appendChar(34 as char); i = i + 2; }
                else { q = false; i = i + 1; }
            } else { f.appendChar(c as char); i = i + 1; }
        }
        else if (c == 34 && f.len == 0) { q = true; i = i + 1; }
        else if (c == delim) { cell(out, &f, row == 0, &rowOpen); i = i + 1; }
        else if (c == 10 || c == 13) {
            if (rowOpen || f.len > 0) {
                cell(out, &f, row == 0, &rowOpen);
                out.append("</tr>");
                rowOpen = false;
                row = row + 1;
            }
            if (c == 13 && i + 1 < n && (src[i + 1] as int) == 10) i = i + 1;
            i = i + 1;
        }
        else { f.appendChar(c as char); i = i + 1; }
    }
    if (rowOpen || f.len > 0) {
        cell(out, &f, row == 0, &rowOpen);
        out.append("</tr>");
        row = row + 1;
    }
    out.append("</table>");
    f.free();
    return row > 0 ? row - 1 : 0;
}

function view(data: char*, name: char*) => char* {
    let body: Str.String;
    let shown: int = toHtml(data, &body);
    body.append("<p class=\"note\">");
    body.appendInt(shown as i64);
    body.append(shown >= CSV_MAX_ROWS ? " rows shown (limit reached)." : " rows.");
    body.append("</p>");
    let r: char* = K.page(name, &body, name);
    body.free();
    return r;
}
