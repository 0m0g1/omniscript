// standard/1/Path.os
// Tier 3: pure string path helpers. Accepts / and \ ; joins with '/'.
module Str = import "standard/1/String.os";
module CStr = import "standard/1/c/string.os";

function isSep(c: int) => bool { return c == 47 || c == 92; }

function join(a: char*, b: char*, out: Str.String*) => bool {
    out.set(a);
    let n: u64 = out.len;
    if (n > 0 && !isSep(out.data[n - 1] as int)) out.appendChar(47 as char);
    let skip: u64 = 0;
    while (isSep(b[skip] as int)) skip = skip + 1;
    return out.append((b as u64 + skip) as char*);
}

function lastSep(p: char*) => i64 {
    let n: i64 = CStr.strlen(p) as i64;
    let i: i64 = n - 1;
    while (i >= 0) { if (isSep(p[i] as int)) return i; i = i - 1; }
    return -1;
}

// "a/b/c.txt" -> "c.txt"
function fileName(p: char*, out: Str.String*) => bool {
    let i = lastSep(p);
    return out.set((p as u64 + (i + 1) as u64) as char*);
}

// "a/b/c.txt" -> "a/b" ("" if none)
function dirName(p: char*, out: Str.String*) => bool {
    let i = lastSep(p);
    if (i < 0) return out.set("");
    return out.appendN(p, i as u64);
}

// ".txt" or ""
function extension(p: char*, out: Str.String*) => bool {
    let name: Str.String; fileName(p, &name);
    let d: char* = CStr.strrchr(name.cstr(), 46);
    let ok: bool = true;
    if (d == nullptr || d == name.cstr()) ok = out.set(""); else ok = out.set(d);
    name.free();
    return ok;
}

// "c.txt" -> "c"
function stem(p: char*, out: Str.String*) => bool {
    let name: Str.String; fileName(p, &name);
    let d: char* = CStr.strrchr(name.cstr(), 46);
    let ok: bool = true;
    if (d == nullptr || d == name.cstr()) ok = out.set(name.cstr());
    else ok = out.appendN(name.cstr(), (d as u64) - (name.cstr() as u64));
    name.free();
    return ok;
}

function isAbsolute(p: char*) => bool {
    if (isSep(p[0] as int)) return true;
    return (p[0] as int) != 0 && (p[1] as int) == 58;   // C:
}

// Collapses ".", "..", duplicate separators. Output uses '/'.
function normalize(p: char*, out: Str.String*) => bool {
    let abs: bool = isAbsolute(p);
    let segs: [128]char*;
    let lens: [128]u64;
    let n: u64 = 0;
    let i: u64 = 0;
    let prefix: Str.String;
    if (abs && (p[0] as int) != 47 && (p[0] as int) != 92) { prefix.appendN(p, 2); i = 2; }
    while ((p[i] as int) != 0) {
        while (isSep(p[i] as int)) i = i + 1;
        let start: u64 = i;
        while ((p[i] as int) != 0 && !isSep(p[i] as int)) i = i + 1;
        let l: u64 = i - start;
        if (l == 0) continue;
        let s: char* = (p as u64 + start) as char*;
        if (l == 1 && (s[0] as int) == 46) continue;
        if (l == 2 && (s[0] as int) == 46 && (s[1] as int) == 46) {
            if (n > 0) { n = n - 1; continue; }
            if (abs) continue;
        }
        if (n < 128) { segs[n] = s; lens[n] = l; n = n + 1; }
    }
    out.append(prefix.cstr());
    prefix.free();
    if (abs) out.appendChar(47 as char);
    let k: u64 = 0;
    while (k < n) { if (k > 0) out.appendChar(47 as char); out.appendN(segs[k], lens[k]); k = k + 1; }
    if (out.len == 0) out.append(".");
    return true;
}
// NOTE: exists/isFile/isDirectory belong with Fs.stat (your Fs.os); add as thin wrappers there to avoid a Path -> Fs dependency cycle.
