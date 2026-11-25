// pharmacy/server/forms.os -- application/x-www-form-urlencoded bodies.
module Str = import "standard/1/String.os";
module HS = import "standard/1/HttpServer.os";
module L = import "standard/1/List.os";
module Text = import "standard/1/Text.os";

struct Form {
    items: L.List<HS.Param>;

    // value of a field, "" when absent
    get(name: char*) => char* {
        let found: char* = "";
        let got: bool = false;
        let i: u64 = 0;
        while (!got && i < this.items.len) {
            if (this.items.data[i].name.equalsC(name)) { found = this.items.data[i].value.cstr(); got = true; }
            i = i + 1;
        }
        return found;
    }

    free() => void {
        let i: u64 = 0;
        while (i < this.items.len) { this.items.data[i].name.free(); this.items.data[i].value.free(); i = i + 1; }
        this.items.free();
    }
}

function parse(body: char*, f: Form*) => void {
    let i: u64 = 0;
    let more: bool = (body[0] as int) != 0;
    while (more) {
        let start: u64 = i;
        while ((body[i] as int) != 0 && (body[i] as int) != 38) i = i + 1;
        let end: u64 = i;
        let eq: u64 = start;
        while (eq < end && (body[eq] as int) != 61) eq = eq + 1;
        let rawN: Str.String;
        let rawV: Str.String;
        rawN.appendN(&body[start], eq - start);
        if (eq < end) rawV.appendN(&body[eq + 1], end - eq - 1);
        let p: HS.Param;
        Text.urlDecode(rawN.cstr(), &p.name);
        Text.urlDecode(rawV.cstr(), &p.value);
        f.items.push(p);
        rawN.free();
        rawV.free();
        if ((body[i] as int) == 0) more = false; else i = i + 1;
    }
}

// path parameter as an integer id; false if missing or not a number
function idParam(req: HS.Request*, name: char*, out: i64*) => bool {
    let s: char* = req.param(name);
    let ok: bool = s != nullptr && (s[0] as int) != 0;
    let v: i64 = 0;
    let i: u64 = 0;
    while (ok && (s[i] as int) != 0) {
        let c: int = s[i] as int;
        if (c < 48 || c > 57) ok = false; else v = v * 10 + ((c - 48) as i64);
        if (v > 999999999) ok = false;
        i = i + 1;
    }
    out[0] = v;
    return ok;
}
