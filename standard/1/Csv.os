// standard/1/Csv.os
// Tier 4: CSV parse/write (RFC 4180: quotes, "" escapes, CRLF). Rows are List<List<String>>-like: Csv.Table.
module Str = import "standard/1/String.os";
module L = import "standard/1/List.os";

struct Table {
    rows: L.List<L.List<Str.String>>;
    free() => void {
        let i: u64 = 0;
        while (i < this.rows.len) {
            let r: L.List<Str.String>* = &this.rows.data[i];
            let j: u64 = 0;
            while (j < r.len) { r.data[j].free(); j = j + 1; }
            r.free();
            i = i + 1;
        }
        this.rows.free();
    }
    rowCount() => u64 { return this.rows.len; }
    cell(r: u64, c: u64) => char* {
        if (r >= this.rows.len) return nullptr;
        if (c >= this.rows.data[r].len) return nullptr;
        return this.rows.data[r].data[c].cstr();
    }
}

function parse(text: char*, delim: char, out: Table*) => bool {
    let row: L.List<Str.String>;
    let cell: Str.String;
    let inQ: bool = false;
    let i: u64 = 0;
    let hasData: bool = false;
    while (true) {
        let c: int = text[i] as int;
        if (c == 0) break;
        hasData = true;
        if (inQ) {
            if (c == 34) {
                if ((text[i + 1] as int) == 34) { cell.appendChar(34 as char); i = i + 1; } else inQ = false;
            } else cell.appendChar(c as char);
        } else if (c == 34) inQ = true;
        else if (c == (delim as int)) { row.push(cell); cell = Str.String{}; }
        else if (c == 10 || c == 13) {
            if (c == 13 && (text[i + 1] as int) == 10) i = i + 1;
            row.push(cell); cell = Str.String{};
            out.rows.push(row); row = L.List<Str.String>{};
            hasData = false;
        } else cell.appendChar(c as char);
        i = i + 1;
    }
    if (hasData) { row.push(cell); out.rows.push(row); } else { cell.free(); row.free(); }
    return true;
}

function writeCell(s: char*, delim: char, out: Str.String*) => void {
    let needs: bool = false;
    let i: u64 = 0;
    while ((s[i] as int) != 0) {
        let c: int = s[i] as int;
        if (c == 34 || c == 10 || c == 13 || c == (delim as int)) needs = true;
        i = i + 1;
    }
    if (!needs) { out.append(s); return; }
    out.appendChar(34 as char);
    i = 0;
    while ((s[i] as int) != 0) {
        if ((s[i] as int) == 34) out.appendChar(34 as char);
        out.appendChar(s[i]);
        i = i + 1;
    }
    out.appendChar(34 as char);
}

function write(t: Table*, delim: char, out: Str.String*) => void {
    let i: u64 = 0;
    while (i < t.rows.len) {
        let j: u64 = 0;
        while (j < t.rows.data[i].len) {
            if (j > 0) out.appendChar(delim);
            writeCell(t.rows.data[i].data[j].cstr(), delim, out);
            j = j + 1;
        }
        out.append("\r\n");
        i = i + 1;
    }
}
