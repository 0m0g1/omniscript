// browser/net/loader.os  -- TIER 0: URL resolution + fetching bytes (local file or http/https).
// Returns a heap, NUL-terminated char* or nullptr.
// loadBytes() is the binary-safe path (images). NEXT: content-type sniffing, cache, file:// edge cases.
module U = import "util.os";
module Fs = import "standard/1/Fs.os";
module Http = import "standard/1/Http.os";
module CFile = import "standard/1/c/file.os";
module Str = import "standard/1/String.os";

// ---- request log (read by the inspector's Network tab) ----
// Ring of the last LOGMAX requests: url, a "status  bytes" label and ok flag. Written under the browser's net mutex.
let LOGMAX: int = 128;
let logTotal: int = 0;
let logUrls: void* = nullptr;
let logLbls: void* = nullptr;
let logOks: void* = nullptr;

function logNote(url: char*, ok: bool, status: int, bytes: int) => void {
    if (logUrls == nullptr) { logUrls = U.alloc(8 * LOGMAX); logLbls = U.alloc(8 * LOGMAX); logOks = U.alloc(8 * LOGMAX); }
    let us: char** = logUrls as char**;
    let ls: char** = logLbls as char**;
    let os: int* = logOks as int*;
    let e: int = logTotal % LOGMAX;
    us[e] = U.dup(url);
    if (status == 0) ls[e] = U.dup("failed");
    else ls[e] = U.cat(U.fromInt(status), U.cat("  ", U.cat(U.fromInt(bytes), " B")));
    if (ok) os[e] = 1; else os[e] = 0;
    logTotal = logTotal + 1;
}
function logCount() => int { if (logTotal < LOGMAX) return logTotal; return LOGMAX; }
function logIdx(k: int) => int { return (logTotal - 1 - k) % LOGMAX; }           // k = 0 is the newest request
function logUrl(k: int) => char* { let a: char** = logUrls as char**; return a[logIdx(k)]; }
function logLabel(k: int) => char* { let a: char** = logLbls as char**; return a[logIdx(k)]; }
function logOk(k: int) => bool { let a: int* = logOks as int*; return a[logIdx(k)] == 1; }

function isHttp(u: char*) => bool { return U.startsAt(u, 0, "http://") || U.startsAt(u, 0, "https://"); }

function dirOf(u: char*) => char* {
    let i: int = U.lastIndexOfCh(u, 47);
    let j: int = U.lastIndexOfCh(u, 92);
    if (j > i) i = j;
    if (i < 0) return U.dup("");
    return U.sub(u, 0, i + 1);
}

// ---- URL helpers ----
// index of the first c1 or c2 at/after `from`, or -1
function firstOf(s: char*, from: int, c1: int, c2: int) => int {
    let n: int = U.slen(s);
    let i: int = from;
    let at: int = 0 - 1;
    while (i < n && at < 0) {
        let c: int = s[i] as int;
        if (c == c1 || c == c2) at = i;
        i = i + 1;
    }
    return at;
}

// scheme://host[:port]
function originOf(u: char*) => char* {
    let n: int = U.slen(u);
    let s: int = U.indexOf(u, 0, "//");
    let j: int = s + 2;
    while (j < n && (u[j] as int) != 47 && (u[j] as int) != 63 && (u[j] as int) != 35) j = j + 1;
    return U.sub(u, 0, j);
}

// u without "#fragment"
function stripFrag(u: char*) => char* {
    let i: int = firstOf(u, 0, 35, 35);
    if (i < 0) return u;
    return U.sub(u, 0, i);
}

// u without "?query" and "#fragment"
function stripQuery(u: char*) => char* {
    let i: int = firstOf(u, 0, 63, 35);
    if (i < 0) return u;
    return U.sub(u, 0, i);
}

// "/a/./b/../c" -> "/a/c"  (path must start with '/')
function collapse(path: char*) => char* {
    let n: int = U.slen(path);
    let out: char* = U.newStr(n + 1);
    let o: int = 0;
    let i: int = 0;
    while (i < n) {
        let s: int = i + 1;
        let e: int = s;
        while (e < n && (path[e] as int) != 47) e = e + 1;
        let len: int = e - s;
        if (len == 1 && (path[s] as int) == 46) { }
        else if (len == 2 && (path[s] as int) == 46 && (path[s + 1] as int) == 46) {
            while (o > 0 && (out[o - 1] as int) != 47) o = o - 1;
            if (o > 0) o = o - 1;
        }
        else if (len == 0 && e < n) { }
        else {
            out[o] = 47 as char; o = o + 1;
            let k: int = 0;
            while (k < len) { out[o] = path[s + k]; o = o + 1; k = k + 1; }
        }
        i = e;
    }
    if (o == 0) { out[0] = 47 as char; }
    return out;
}

// resolve `rel` against page url `base` (http(s): absolute paths, ../, ?query, #frag, //host; local: relative to the folder)
function resolve(base: char*, rel: char*) => char* {
    if (isHttp(rel)) return rel;
    if (U.startsAt(rel, 0, "file://")) return U.sub(rel, 7, U.slen(rel));
    if (!isHttp(base)) return U.cat(dirOf(base), rel);
    let origin: char* = originOf(base);
    if (U.startsAt(rel, 0, "//")) {
        let colon: int = firstOf(origin, 0, 58, 58);
        return U.cat(U.sub(origin, 0, colon + 1), rel);
    }
    let bp: char* = U.sub(base, U.slen(origin), U.slen(base));
    bp = stripQuery(bp);
    if (U.slen(bp) == 0) bp = "/";
    let p: char* = rel;
    if ((rel[0] as int) == 63 || (rel[0] as int) == 35) p = U.cat(bp, rel);
    else if ((rel[0] as int) != 47) {
        let slash: int = U.lastIndexOfCh(bp, 47);
        p = U.cat(U.sub(bp, 0, slash + 1), rel);
    }
    let cut: int = firstOf(p, 0, 63, 35);
    let path: char* = p;
    let tail: char* = "";
    if (cut >= 0) { path = U.sub(p, 0, cut); tail = U.sub(p, cut, U.slen(p)); }
    if ((path[0] as int) != 47) path = U.cat("/", path);
    return U.cat(origin, U.cat(collapse(path), tail));
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

// ---- binary bodies (images) ----
// Fetches url as raw bytes into `out` (out.data / out.len; the bytes are never cut at a zero byte). false when the
// file is missing, the request fails or the HTTP status is not 2xx. The caller frees `out`.
function loadBytes(url: char*, out: Str.String*) => bool {
    let ok: bool = false;
    if (isHttp(url)) {
        Http.setCookieFile("browser_cookies.txt");
        let r: Http.Response;
        let got: bool = Http.get(url, &r);
        let code: int = r.status as int;
        if (got && code >= 200 && code < 300 && r.body.len > 0) ok = r.body.clone(out);
        logNote(url, ok, code, r.body.len as int);
        r.free();
    } else {
        let path: char* = url;
        if (U.startsAt(url, 0, "file://")) path = U.sub(url, 7, U.slen(url));
        let size: u64 = 0;
        if (Fs.fileSize(path, &size) && size > 0) {
            let f: void* = CFile.fopen(path, "rb");
            if (f != nullptr) {
                if (out.reserve(size)) {
                    CFile.fread(out.data as void*, 1, size, f);
                    out.len = size;
                    out.data[size] = 0 as char;
                    ok = true;
                }
                CFile.fclose(f);
            }
        }
        if (ok) logNote(url, true, 200, out.len as int); else logNote(url, false, 404, 0);
    }
    return ok;
}

// ---- http ----
// Final URL of the last http request (after redirects), "" for local files. Read it right after load()/post().
let lastFinal: char* = "";                 // "" = none (a module-level char* = nullptr is typed void*)
let lastStatus: int = 0;

// Set by finish() for the last http response: should it be saved as a file instead of shown?
let lastDownload: bool = false;
let lastText: bool = false;                // the download is text, so it can also be shown in the page
let lastName: char* = "";                  // suggested file name (from Content-Disposition, else the URL)
let lastType: char* = "";                  // Content-Type of the last http response, "" for local files

// value of the LAST header called `name` in a raw header block ("" if absent)
function headerValue(raw: char*, name: char*) => char* {
    let n: int = U.slen(raw);
    let nl: int = U.slen(name);
    let i: int = 0;
    let found: char* = "";
    while (i < n) {
        let e: int = i;
        while (e < n && (raw[e] as int) != 10) e = e + 1;
        let ok: bool = (e - i) > nl;
        let k: int = 0;
        while (ok && k < nl) {
            if (U.lowerCh(raw[i + k] as int) != U.lowerCh(name[k] as int)) ok = false;
            k = k + 1;
        }
        if (ok && (raw[i + nl] as int) == 58) {
            let a: int = i + nl + 1;
            let b: int = e;
            while (b > a && ((raw[b - 1] as int) == 13 || (raw[b - 1] as int) == 32)) b = b - 1;
            while (a < b && (raw[a] as int) == 32) a = a + 1;
            found = U.sub(raw, a, b);
        }
        i = e + 1;
    }
    return found;
}

// file name for a download: filename="x" from Content-Disposition, else the last path segment of the URL; unsafe characters become _
function downloadName(disp: char*, url: char*) => char* {
    let raw: char* = "";
    let at: int = U.indexOf(U.lower(disp), 0, "filename=");
    if (at >= 0) {
        let a: int = at + 9;
        let n: int = U.slen(disp);
        if (a < n && (disp[a] as int) == 34) a = a + 1;
        let b: int = a;
        while (b < n && (disp[b] as int) != 34 && (disp[b] as int) != 59) b = b + 1;
        raw = U.sub(disp, a, b);
    } else {
        let path: char* = stripQuery(url);
        let sl: int = U.lastIndexOfCh(path, 47);
        raw = U.sub(path, sl + 1, U.slen(path));
    }
    if (U.slen(raw) == 0) raw = "download";
    let out: char* = U.dup(raw);
    let i: int = 0;
    while ((out[i] as int) != 0) {
        let c: int = out[i] as int;
        if (c == 92 || c == 47 || c == 58 || c == 42 || c == 63 || c == 34 || c == 60 || c == 62 || c == 124 || c < 32) out[i] = 95 as char;
        i = i + 1;
    }
    return out;
}

// anything that is not text, JSON or XML cannot be shown here, so it is saved
function isDownloadType(ct: char*) => bool {
    let t: char* = U.lower(ct);
    let dl: bool = U.slen(t) > 0;
    if (U.startsAt(t, 0, "text/") || U.startsAt(t, 0, "application/json") || U.indexOf(t, 0, "xml") >= 0) dl = false;
    return dl;
}

// Any HTTP status gives a page (a 403/404 error page is shown like a browser does); only transport failures return nullptr.
function finish(r: Http.Response*, ok: bool) => char* {
    lastFinal = "";
    lastStatus = 0;
    lastDownload = false;
    lastText = false;
    lastName = "";
    lastType = "";
    if (!ok) { r.free(); return nullptr; }
    lastStatus = r.status as int;
    if (r.finalUrl != nullptr) lastFinal = U.dup(r.finalUrl);
    let disp: char* = headerValue(r.headers.cstr(), "content-disposition");
    let ctype: char* = headerValue(r.headers.cstr(), "content-type");
    lastType = U.dup(ctype);
    if (U.startsAt(U.lower(disp), 0, "attachment") || isDownloadType(ctype)) {
        lastDownload = true;
        lastText = !isDownloadType(ctype);
        lastName = downloadName(disp, lastFinal);
    }
    let out: char* = U.dup(r.body.cstr());
    r.free();
    return out;
}

function readHttp(url: char*) => char* {
    Http.setCookieFile("browser_cookies.txt");
    let r: Http.Response;
    let ok: bool = Http.get(url, &r);
    let o: char* = finish(&r, ok);
    let nb: int = 0;
    if (o != nullptr) nb = U.slen(o);
    logNote(url, o != nullptr, lastStatus, nb);
    return o;
}

// POST application/x-www-form-urlencoded (what <form method=post> sends)
function post(url: char*, body: char*) => char* {
    Http.setCookieFile("browser_cookies.txt");
    let r: Http.Response;
    let ok: bool = Http.post(url, body, "application/x-www-form-urlencoded", &r);
    let o: char* = finish(&r, ok);
    let nb: int = 0;
    if (o != nullptr) nb = U.slen(o);
    logNote(url, o != nullptr, lastStatus, nb);
    return o;
}

// what the user typed -> a loadable url: keeps http(s)/file://, existing local paths, else assumes https://
function normalize(s: char*) => char* {
    let t: char* = U.trim(s);
    let out: char* = t;
    if (U.slen(t) > 0 && !isHttp(t) && !U.startsAt(t, 0, "file://") && !Fs.exists(t)) out = U.cat("https://", t);
    return out;
}

function load(url: char*) => char* {
    lastFinal = "";
    lastDownload = false;
    lastType = "";
    if (isHttp(url)) return readHttp(url);
    let p: char* = url;
    if (U.startsAt(url, 0, "file://")) p = U.sub(url, 7, U.slen(url));
    let o: char* = readLocal(p);
    if (o != nullptr) logNote(url, true, 200, U.slen(o)); else logNote(url, false, 404, 0);
    return o;
}

// "name.csv" -> "name.csv", or "name (1).csv" when it already exists
function uniqueName(name: char*) => char* {
    let out: char* = name;
    let i: int = 1;
    let dot: int = U.lastIndexOfCh(name, 46);
    while (Fs.exists(out) && i < 100) {
        if (dot < 0) out = U.cat(name, U.cat(" (", U.cat(U.fromInt(i), ")")));
        else out = U.cat(U.sub(name, 0, dot), U.cat(" (", U.cat(U.fromInt(i), U.cat(")", U.sub(name, dot, U.slen(name))))));
        i = i + 1;
    }
    return out;
}

// Saves a downloaded body into the folder the browser runs from. Returns the file name used, or nullptr on failure.
// (Bodies are NUL-terminated text here, so a binary file would be cut at its first zero byte.)
function saveDownload(data: char*, name: char*) => char* {
    let path: char* = uniqueName(name);
    let f: void* = CFile.fopen(path, "wb");
    if (f == nullptr) return nullptr;
    let n: u64 = U.slen(data) as u64;
    if (n > 0) CFile.fwrite(data as void*, 1, n, f);             // ADAPT: fwrite name/signature
    CFile.fclose(f);
    return path;
}
