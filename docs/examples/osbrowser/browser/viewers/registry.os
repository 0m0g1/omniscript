// browser/viewers/registry.os -- picks a viewer from the file name and renders the file as an HTML page.
//   render(data, name) => complete HTML document, or nullptr when no viewer handles this file (caller falls back).
// To add a viewer: write browser/viewers/<kind>.os with `view(data, name) => char*`, import it here,
// give it a K_ number, and map its extensions in kindOf().
module U = import "browser/util.os";
module T = import "browser/viewers/text.os";
module MD = import "browser/viewers/markdown.os";
module CSV = import "browser/viewers/csv.os";
module J = import "browser/viewers/json.os";

let K_NONE: int = 0;
let K_TEXT: int = 1;
let K_MD: int = 2;
let K_CSV: int = 3;
let K_JSON: int = 4;

// lower-case extension without the dot, "" when the name has none
function extOf(name: char*) => char* {
    let dot: int = U.lastIndexOfCh(name, 46);
    if (dot < 0) return "";
    return U.lower(U.sub(name, dot + 1, U.slen(name)));
}

function isTextExt(e: char*) => bool {
    return U.streq(e, "txt") || U.streq(e, "log") || U.streq(e, "ini") || U.streq(e, "cfg") || U.streq(e, "conf")
        || U.streq(e, "yaml") || U.streq(e, "yml") || U.streq(e, "toml") || U.streq(e, "xml") || U.streq(e, "css")
        || U.streq(e, "sh") || U.streq(e, "bat") || U.streq(e, "os") || U.streq(e, "c") || U.streq(e, "h")
        || U.streq(e, "cpp") || U.streq(e, "hpp") || U.streq(e, "cs") || U.streq(e, "java") || U.streq(e, "js")
        || U.streq(e, "py") || U.streq(e, "go")
        || U.streq(e, "rs") || U.streq(e, "ts") || U.streq(e, "rb") || U.streq(e, "php") || U.streq(e, "lua");
}

function kindOf(name: char*) => int {
    let e: char* = extOf(name);
    if (U.streq(e, "md") || U.streq(e, "markdown")) return K_MD;
    if (U.streq(e, "csv") || U.streq(e, "tsv")) return K_CSV;
    if (U.streq(e, "json")) return K_JSON;
    if (isTextExt(e)) return K_TEXT;
    return K_NONE;
}

// last path segment of a url or local path, without ?query / #fragment ("file:///c:/x/a.md?v=1" -> "a.md")
function nameOfUrl(url: char*) => char* {
    let n: int = U.slen(url);
    let cut: int = n;
    let i: int = 0;
    while (i < n && cut == n) {
        let c: int = url[i] as int;
        if (c == 63 || c == 35) cut = i;
        i = i + 1;
    }
    let path: char* = U.sub(url, 0, cut);
    let sl: int = U.lastIndexOfCh(path, 47);
    let bs: int = U.lastIndexOfCh(path, 92);
    if (bs > sl) sl = bs;
    return U.sub(path, sl + 1, U.slen(path));
}

// Should a response that the loader did not flag as a text attachment still be shown by a viewer?
// Yes when the file name has a known extension and the server did not say it is HTML (ctype is "" for local files).
function sniff(url: char*, ctype: char*) => bool {
    if (kindOf(nameOfUrl(url)) == K_NONE) return false;
    let t: char* = U.lower(ctype);
    if (U.indexOf(t, 0, "html") >= 0) return false;
    return true;
}

function render(data: char*, name: char*) => char* {
    let k: int = kindOf(name);
    if (k == K_MD) return MD.view(data, name);
    if (k == K_CSV) return CSV.view(data, name);
    if (k == K_JSON) return J.view(data, name);
    if (k == K_TEXT) return T.view(data, name);
    return nullptr;
}
