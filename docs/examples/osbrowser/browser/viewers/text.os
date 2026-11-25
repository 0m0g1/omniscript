// browser/viewers/text.os -- plain text / source code shown in a <pre>. Used for every "text" kind and by json.os as a fallback.
module Str = import "standard/1/String.os";
module U = import "browser/util.os";
module K = import "browser/viewers/common.os";

let TEXT_MAX: int = 1000000;

function textBody(text: char*, out: Str.String*) => void {
    let cut: bool = U.slen(text) > TEXT_MAX;
    out.append("<pre>");
    K.esc(out, cut ? U.sub(text, 0, TEXT_MAX) : text);
    out.append("</pre>");
    if (cut) K.note(out, "File truncated for display (first 1 MB).");
}

function view(data: char*, name: char*) => char* {
    let body: Str.String;
    textBody(data, &body);
    let r: char* = K.page(name, &body, name);
    body.free();
    return r;
}
