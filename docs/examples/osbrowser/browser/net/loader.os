// browser/net/loader.os  -- TIER 0: URL resolution + fetching bytes (local file or http/https).
// Returns a heap, NUL-terminated char* or nullptr.
// NEXT: content-type sniffing, redirects, cache, binary bodies (images), file:// edge cases.
module U = import "browser/util.os";
module Fs = import "standard/1/Fs.os";
module Http = import "standard/1/Http.os";
module CFile = import "standard/1/c/file.os";

function isHttp(u: char*) => bool { return U.startsAt(u, 0, "http://") || U.startsAt(u, 0, "https://"); }

function dirOf(u: char*) => char* {
    let i: int = U.lastIndexOfCh(u, 47);
    let j: int = U.lastIndexOfCh(u, 92);
    if (j > i) i = j;
    if (i < 0) return U.dup("");
    return U.sub(u, 0, i + 1);
}

// resolve `rel` against page url `base`

function resolve(base: char*, rel: char*) => char* {
    if (isHttp(rel)) return rel;
    if (U.startsAt(rel, 0, "file://")) return U.sub(rel, 7, U.slen(rel));
    // NEXT: absolute-path ("/x.css") and ../ handling
    return U.cat(dirOf(base), rel);
}

// ---- ADAPT 1: read a local file. Uses Fs.fileSize (known) + raw C stdio (names guessed). ----
function readLocal(path: char*) => char* {
    let size: u64 = 0;
    if (!Fs.fileSize(path, &size)) return nullptr;
    let buf: char* = U.newStr(size as int);
    let f: void* = CFile.fopen(path, "rb");                      // ADAPT: real name/signature
    if (f == nullptr) return nullptr;
    CFile.fread(buf as void*, 1, size, f);                       // ADAPT
    CFile.fclose(f);                                             // ADAPT
    return buf;
}

// ---- ADAPT 2: http body -> char*. Response.body is a Str.String; field name guessed (.data). ----
function readHttp(url: char*) => char* {
    let r: Http.Response;
    if (!Http.get(url, &r)) return nullptr;
    if (r.status != 200) { r.free(); return nullptr; }
    let out: char* = U.dup(r.body.data);                         // ADAPT: Str.String's char* field
    r.free();
    return out;
}

// what the user typed -> a loadable url: keeps http(s)/file://, existing local paths, else assumes https://
function normalize(s: char*) => char* {
    let t: char* = U.trim(s);
    let out: char* = t;
    if (U.slen(t) > 0 && !isHttp(t) && !U.startsAt(t, 0, "file://") && !Fs.exists(t)) out = U.cat("https://", t);
    return out;
}

function load(url: char*) => char* {
    if (isHttp(url)) return readHttp(url);
    if (U.startsAt(url, 0, "file://")) return readLocal(U.sub(url, 7, U.slen(url)));
    return readLocal(url);
}
