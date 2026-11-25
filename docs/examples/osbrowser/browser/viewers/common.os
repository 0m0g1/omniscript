// browser/viewers/common.os -- helpers shared by the file viewers: HTML escaping and the page wrapper.
// Every viewer turns its file into an HTML body; page() wraps it with a style sheet (and a Download link for attachments).
module Str = import "standard/1/String.os";
module U = import "browser/util.os";

let CSS: char* = "body{background:#ffffff;color:#1d2733;padding:16px;} h1{font-size:28px;margin:12px 0;} h2{font-size:22px;margin:10px 0;} h3{font-size:18px;margin:8px 0;} pre{background:#f0f1f5;padding:10px;} code{background:#eceef3;} blockquote{border-left:4px solid #b8bdc9;padding-left:12px;color:#555a66;} table{border-collapse:collapse;margin:8px 0;} th{background:#e3e6ec;border:1px solid #c5c9d3;padding:4px 8px;text-align:left;} td{border:1px solid #d5d8e0;padding:4px 8px;} a{color:#0b5cad;} .bar{padding:4px 0 10px 0;} .note{color:#777b88;}";

function escCh(out: Str.String*, c: int) => void {
    if (c == 38) out.append("&amp;");
    else if (c == 60) out.append("&lt;");
    else if (c == 62) out.append("&gt;");
    else if (c == 34) out.append("&quot;");
    else out.appendChar(c as char);
}

function esc(out: Str.String*, s: char*) => void {
    let i: int = 0;
    while ((s[i] as int) != 0) { escCh(out, s[i] as int); i = i + 1; }
}

function escRange(out: Str.String*, s: char*, a: int, b: int) => void {
    let i: int = a;
    while (i < b) { escCh(out, s[i] as int); i = i + 1; }
}

// complete document; dlName non-empty adds a "Download" link (browser.os saves the file when it is clicked)
function page(title: char*, body: Str.String*, dlName: char*) => char* {
    let o: Str.String;
    o.append("<html><head><meta charset=\"utf-8\"><title>");
    esc(&o, title);
    o.append("</title><style>");
    o.append(CSS);
    o.append("</style></head><body>");
    if (U.slen(dlName) > 0) {
        o.append("<p class=\"bar\"><a href=\"#save-download\">Download ");
        esc(&o, dlName);
        o.append("</a></p>");
    }
    o.appendString(body);
    o.append("</body></html>");
    let r: char* = U.dup(o.cstr());
    o.free();
    return r;
}

// a grey one-line note (appended to a body)
function note(out: Str.String*, msg: char*) => void {
    out.append("<p class=\"note\">");
    esc(out, msg);
    out.append("</p>");
}
