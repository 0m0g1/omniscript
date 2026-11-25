// browser/viewers/json.os -- JSON shown pretty-printed; invalid JSON falls back to the raw text with a note.
// NEXT: collapsible tree view.
module Str = import "standard/1/String.os";
module Json = import "standard/1/Json.os";
module U = import "browser/util.os";
module K = import "browser/viewers/common.os";
module T = import "browser/viewers/text.os";

function view(data: char*, name: char*) => char* {
    let body: Str.String;
    let v: Json.JsonValue* = Json.parseStrict(data);
    if (v == nullptr) {
        K.note(&body, "Not valid JSON, showing the raw text.");
        T.textBody(data, &body);
    } else {
        let pretty: Str.String;
        Json.stringify(v, &pretty, true);
        T.textBody(pretty.cstr(), &body);
        pretty.free();
        v.free();
    }
    let r: char* = K.page(name, &body, name);
    body.free();
    return r;
}
