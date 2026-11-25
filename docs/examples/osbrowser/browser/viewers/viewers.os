// browser/viewers/viewers.os -- the registry: which viewer opens a file, chosen by Content-Type (http) or extension (local).
//   0 = leave it alone (HTML page)   1 = plain text   2 = Markdown   3 = CSV / TSV   4 = JSON
// render() returns a complete HTML document, or nullptr for "treat it as a normal page".
// ADD A FORMAT: write viewers/<name>.os with a toHtml(src, out) function, add a kind below and a branch in render().
// NEXT: images (needs Image.load + Pen2D.createImage + <img> support), JSON tree, source highlighting, tabs.
module Str = import "standard/1/String.os";
module Json = import "standard/1/Json.os";
module U = import "browser/util.os";
module Net = import "browser/net/loader.os";
module K = import "browser/viewers/common.os";
module MD = import "browser/viewers/markdown.os";
module CSV = import "browser/viewers/csv.os";

let TEXT_MAX: int = 1000000;

// lowercase extension without the dot, "" if none
function extOf(url: char*) => char* {
    let p: char* = Net.stripQuery(url);
    let dot: int = U.lastIndexOfCh(p, 46);
    let sl: int = U.lastIndexOfCh(p, 47);
    let bs: int = U.lastIndexOfCh(p, 92);
    if (bs > sl) sl = bs;
    if (dot < 0 || dot < sl) return "";
    return U.lower(U.sub(p, dot + 1, U.slen(p)));
}

function fileName(url: char*) => char* {
    let p: char* = Net.stripQuery(url);
    let sl: int = U.lastIndexOfCh(p, 47);
    let bs: int = U.lastIndexOfCh(p, 92);
    if (bs > sl) sl = bs;
    let nm: char* = U.sub(p, sl + 1, U.slen(p));
    if (U.slen(nm) == 0) nm = p;
    return nm;
}

function isTextExt(e: char*) => bool {
    return U.streq(e, "txt") || U.streq(e, "log") || U.streq(e, "ini") || U.streq(e, "cfg") || U.streq(e, "conf") || U.streq(e, "toml")
        || U.streq(e, "yml") || U.streq(e, "yaml") || U.streq(e, "xml") || U.streq(e, "os") || U.streq(e, "js") || U.streq(e, "py")
        || U.streq(e, "c") || U.streq(e, "h") || U.streq(e, "cpp") || U.streq(e, "hpp") || U.streq(e, "css") || U.streq(e, "sql")
        || U.streq(e, "sh") || U.streq(e, "bat") || U.streq(e, "ps1") || U.streq(e, "java") || U.streq(e, "cs") || U.streq(e, "go")
        || U.streq(e, "rs") || U.streq(e, "ts") || U.streq(e, "rb") || U.streq(e, "php") || U.streq(e, "lua");
}

function kindByExt(e: char*) => int {
    if (U.streq(e, "md") || U.streq(e, "markdown")) return 2;
    if (U.streq(e, "csv") || U.streq(e, "tsv")) return 3;
    if (U.streq(e, "json")) return 4;
    return isTextExt(e) ? 1 : 0;
}

function kindOf(url: char*, ctype: char*) => int {
    let ct: char* = U.lower(ctype);
    let ext: char* = extOf(url);
    if (U.indexOf(ct, 0, "html") >= 0) return 0;
    if (U.indexOf(ct, 0, "markdown") >= 0) return 2;
    if (U.indexOf(ct, 0, "csv") >= 0) return 3;
    if (U.indexOf(ct, 0, "json") >= 0) return 4;
    if (U.startsAt(ct, 0, "text/")) {
        let k: int = kindByExt(ext);
        return k == 0 ? 1 : k;
    }
    if (U.slen(ct) == 0) return kindByExt(ext);
    return 0;
}

function textBody(text: char*, out: Str.String*) => void {
    let src: char* = text;
    let cut: bool = U.slen(text) > TEXT_MAX;
    if (cut) src = U.sub(text, 0, TEXT_MAX);
    out.append("<pre>");
    K.esc(out, src);
    out.append("</pre>");
    if (cut) out.append("<p class=\"note\">File truncated for display (first 1 MB).</p>");
}

function jsonBody(text: char*, out: Str.String*) => void {
    let v: Json.JsonValue* = Json.parseStrict(text);
    if (v == nullptr) {
        out.append("<p class=\"note\">Not valid JSON, showing the raw text.</p>");
        textBody(text, out);
    } else {
        let pretty: Str.String;
        Json.stringify(v, &pretty, true);
        textBody(pretty.cstr(), out);
        pretty.free();
        v.free();
    }
}

// dlName: non-empty when the server marked the response as an attachment (adds the Download link and forces a text view)
function render(url: char*, ctype: char*, text: char*, dlName: char*) => char* {
    let kind: int = kindOf(url, ctype);
    if (kind == 0 && U.slen(dlName) > 0) kind = 1;
    if (kind == 0) return nullptr;
    let body: Str.String;
    if (kind == 2) MD.toHtml(text, &body);
    else if (kind == 3) {
        let shown: int = CSV.toHtml(text, &body);
        body.append("<p class=\"note\">");
        body.appendInt(shown as i64);
        if (shown >= CSV.CSV_MAX_ROWS) body.append(" rows shown (limit reached).</p>");
        else body.append(" rows.</p>");
    }
    else if (kind == 4) jsonBody(text, &body);
    else textBody(text, &body);
    let r: char* = K.page(fileName(url), &body, dlName);
    body.free();
    return r;
}
