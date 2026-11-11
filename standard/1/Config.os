// standard/1/Config.os
// Tier 4: INI-style config. [section] / key = value, # or ; comments. Keys stored as "section.key".
module Str = import "standard/1/String.os";
module L = import "standard/1/List.os";
module CStr = import "standard/1/c/string.os";
module Text = import "standard/1/Text.os";

struct Entry { key: Str.String; val: Str.String; }

struct Config {
    entries: L.List<Entry>;

    free() => void {
        let i: u64 = 0;
        while (i < this.entries.len) { this.entries.data[i].key.free(); this.entries.data[i].val.free(); i = i + 1; }
        this.entries.free();
    }

    find(key: char*) => i64 {
        let i: u64 = 0;
        while (i < this.entries.len) { if (this.entries.data[i].key.equalsC(key)) return i as i64; i = i + 1; }
        return -1;
    }

    set(key: char*, val: char*) => bool {
        let i = this.find(key);
        if (i >= 0) return this.entries.data[i].val.set(val);
        let e: Entry; e.key.set(key); e.val.set(val);
        return this.entries.push(e);
    }

    get(key: char*, def: char*) => char* {
        let i = this.find(key);
        if (i < 0) return def;
        return this.entries.data[i].val.cstr();
    }
    getInt(key: char*, def: i64) => i64 {
        let i = this.find(key);
        if (i < 0) return def;
        let v: i64 = def;
        if (!this.entries.data[i].val.parseInt(&v)) return def;
        return v;
    }
    getBool(key: char*, def: bool) => bool {
        let s: char* = this.get(key, nullptr);
        if (s == nullptr) return def;
        return CStr.strcmp(s, "true") == 0 || CStr.strcmp(s, "1") == 0 || CStr.strcmp(s, "yes") == 0;
    }

    parse(text: char*) => bool {
        let section: Str.String;
        let lines: L.List<Str.String>;
        Text.split(text, "\n", &lines);
        let i: u64 = 0;
        while (i < lines.len) {
            let ln: Str.String* = &lines.data[i];
            ln.trim();
            i = i + 1;
            if (ln.len == 0) continue;
            let c: int = ln.data[0] as int;
            if (c == 35 || c == 59) continue;
            if (c == 91) {
                section.clear();
                section.appendN((ln.data as u64 + 1) as char*, ln.len - 2);
                continue;
            }
            let eq: i64 = ln.indexOf("=");
            if (eq < 0) continue;
            let k: Str.String; k.appendN(ln.data, eq as u64); k.trim();
            let v: Str.String; v.set((ln.data as u64 + eq as u64 + 1) as char*); v.trim();
            let full: Str.String;
            if (section.len > 0) { full.appendString(&section); full.append("."); }
            full.appendString(&k);
            this.set(full.cstr(), v.cstr());
            k.free(); v.free(); full.free();
        }
        Text.freeParts(&lines);
        section.free();
        return true;
    }

    // Serialises as flat "key = value" lines (section prefixes stay in the key).
    write(out: Str.String*) => void {
        let i: u64 = 0;
        while (i < this.entries.len) {
            out.appendString(&this.entries.data[i].key); out.append(" = ");
            out.appendString(&this.entries.data[i].val); out.append("\n");
            i = i + 1;
        }
    }
}
