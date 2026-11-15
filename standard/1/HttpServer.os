// standard/1/HttpServer.os
// Tier 5: HTTP/1.1 server on Net + Thread. One thread per connection, route table, request parser, response writer,
// static files. (PLATFORM: Win32 threads via Thread.os)
//
//   function hello(req: HttpServer.Request*, res: HttpServer.Response*) => void {
//       res.text("hello, world");
//   }
//   function user(req: HttpServer.Request*, res: HttpServer.Response*) => void {
//       res.json("{\"id\":\"...\"}");                            // req.param("id"), req.header("accept"), req.body ...
//   }
//   let srv: HttpServer.HttpServer;
//   srv.get("/", hello);
//   srv.get("/users/:id", user);                                  // :name segments are captured -> req.param("name")
//   srv.post("/echo", echo);                                      // "*" as method matches every method
//   srv.serveStatic("/assets", "public");                         // GET /assets/* -> public/*  (no ".." escapes)
//   srv.listen(8080);                                             // = bind(8080) + serve(); blocks until stop()
//
// Routing: first registered route that matches wins. Patterns are exact segments, ":name" captures, and a trailing
// "/*" wildcard (the remainder is req.rest). HEAD uses the GET route and drops the body. A path that matches only
// under another method gets 405, otherwise 404 (or srv.notFound if set).
//
// Limits and behaviour: every response is "Connection: close" (no keep-alive, no pipelining); request bodies need
// Content-Length (chunked uploads get 501); headers are capped at srv.maxHeader (64 KiB), bodies at srv.maxBody (8 MiB),
// at most srv.maxConnections (512) live connections, and a socket read/write timeout of srv.timeoutMs (15 s).
// Plain HTTP only: put it behind a TLS terminator, or build on Tls.TlsServer yourself, for https.
// No `return` inside loop bodies (compiler quirk noted in Canvas.os).
module Net = import "standard/1/Net.os";
module CN = import "standard/1/c/net.os";
module Th = import "standard/1/Thread.os";
module CT = import "standard/1/c/thread.os";
module Str = import "standard/1/String.os";
module L = import "standard/1/List.os";
module Text = import "standard/1/Text.os";
module CMem = import "standard/1/c/mem.os";
module CStr = import "standard/1/c/string.os";
module CFile = import "standard/1/c/file.os";

extern "C" fn _stricmp(a: char*, b: char*) => int;

// ---------------------------------------------------------------------------------------------------------------
// Request / Response
// ---------------------------------------------------------------------------------------------------------------
struct Header { name: Str.String; value: Str.String; }
struct Param { name: Str.String; value: Str.String; }

function freeHeaders(l: L.List<Header>*) => void {
    let i: u64 = 0;
    while (i < l.len) { l.data[i].name.free(); l.data[i].value.free(); i = i + 1; }
    l.free();
}

function freeParams(l: L.List<Param>*) => void {
    let i: u64 = 0;
    while (i < l.len) { l.data[i].name.free(); l.data[i].value.free(); i = i + 1; }
    l.free();
}

function isWs(c: int) => bool { return c == 32 || c == 9; }

struct Request {
    method: Str.String;       // "GET", "POST", ...
    target: Str.String;       // raw request target, e.g. "/a/b?x=1"
    path: Str.String;         // "/a/b" (raw, not percent-decoded)
    query: Str.String;       // "x=1" (without '?')
    version: Str.String;      // "HTTP/1.1"
    body: Str.String;         // request body (bytes, NUL-terminated)
    rest: Str.String;         // remainder matched by a "/*" wildcard route
    headers: L.List<Header>;
    params: L.List<Param>;    // ":name" captures of the matched route
    user: void* = nullptr;    // the pointer given to routeUser()

    // Header value by name (case-insensitive), nullptr if absent.
    header(name: char*) => char* {
        let found: char* = nullptr;
        let i: u64 = 0;
        while (found == nullptr && i < this.headers.len) {
            if (_stricmp(this.headers.data[i].name.cstr(), name) == 0) found = this.headers.data[i].value.cstr();
            i = i + 1;
        }
        return found;
    }

    // Path parameter captured by a ":name" segment, nullptr if absent. Value is raw (not percent-decoded).
    param(name: char*) => char* {
        let found: char* = nullptr;
        let i: u64 = 0;
        while (found == nullptr && i < this.params.len) {
            if (CStr.strcmp(this.params.data[i].name.cstr(), name) == 0) found = this.params.data[i].value.cstr();
            i = i + 1;
        }
        return found;
    }

    // Percent-decoded query-string value for name (the name is matched as written) appended to out. False if absent.
    queryParam(name: char*, out: Str.String*) => bool {
        let q: char* = this.query.cstr();
        let nlen: u64 = CStr.strlen(name);
        let found: bool = false;
        let more: bool = (q[0] as int) != 0;
        let i: u64 = 0;
        while (more && !found) {
            let start: u64 = i;
            while ((q[i] as int) != 0 && (q[i] as int) != 38) i = i + 1;
            let end: u64 = i;
            let eq: u64 = start;
            while (eq < end && (q[eq] as int) != 61) eq = eq + 1;
            if ((eq - start) == nlen && CMem.memcmp(&q[start] as void*, name as void*, nlen) == 0) {
                found = true;
                let raw: Str.String;
                if (eq < end) raw.appendN(&q[eq + 1], end - eq - 1);
                out.clear();
                Text.urlDecode(raw.cstr(), out);
                raw.free();
            }
            if ((q[i] as int) == 0) more = false;
            else i = i + 1;
        }
        return found;
    }

    free() => void {
        this.method.free(); this.target.free(); this.path.free(); this.query.free();
        this.version.free(); this.body.free(); this.rest.free();
        freeHeaders(&this.headers);
        freeParams(&this.params);
    }
}

function hasCrlf(s: char*) => bool {
    let bad: bool = false;
    let i: u64 = 0;
    while (!bad && (s[i] as int) != 0) {
        let c: int = s[i] as int;
        if (c == 13 || c == 10) bad = true;
        i = i + 1;
    }
    return bad;
}

function reason(code: int) => char* {
    if (code == 100) return "Continue";
    if (code == 200) return "OK";
    if (code == 201) return "Created";
    if (code == 202) return "Accepted";
    if (code == 204) return "No Content";
    if (code == 206) return "Partial Content";
    if (code == 301) return "Moved Permanently";
    if (code == 302) return "Found";
    if (code == 303) return "See Other";
    if (code == 304) return "Not Modified";
    if (code == 307) return "Temporary Redirect";
    if (code == 308) return "Permanent Redirect";
    if (code == 400) return "Bad Request";
    if (code == 401) return "Unauthorized";
    if (code == 403) return "Forbidden";
    if (code == 404) return "Not Found";
    if (code == 405) return "Method Not Allowed";
    if (code == 408) return "Request Timeout";
    if (code == 409) return "Conflict";
    if (code == 411) return "Length Required";
    if (code == 413) return "Payload Too Large";
    if (code == 415) return "Unsupported Media Type";
    if (code == 422) return "Unprocessable Entity";
    if (code == 429) return "Too Many Requests";
    if (code == 431) return "Request Header Fields Too Large";
    if (code == 500) return "Internal Server Error";
    if (code == 501) return "Not Implemented";
    if (code == 503) return "Service Unavailable";
    return "Status";
}

// Content-Type for a file name, by extension.
function mimeType(path: char*) => char* {
    let dot: char* = CStr.strrchr(path, 46);
    if (dot == nullptr) return "application/octet-stream";
    let n: u64 = CStr.strlen(dot);
    if (n > 15) return "application/octet-stream";
    let ext: [16]char;
    let i: u64 = 0;
    while (i <= n) { ext[i] = CStr.tolower(dot[i] as int) as char; i = i + 1; }
    let e: char* = &ext as char*;
    if (CStr.strcmp(e, ".html") == 0 || CStr.strcmp(e, ".htm") == 0) return "text/html; charset=utf-8";
    if (CStr.strcmp(e, ".css") == 0) return "text/css; charset=utf-8";
    if (CStr.strcmp(e, ".js") == 0 || CStr.strcmp(e, ".mjs") == 0) return "text/javascript; charset=utf-8";
    if (CStr.strcmp(e, ".json") == 0) return "application/json";
    if (CStr.strcmp(e, ".txt") == 0) return "text/plain; charset=utf-8";
    if (CStr.strcmp(e, ".csv") == 0) return "text/csv; charset=utf-8";
    if (CStr.strcmp(e, ".xml") == 0) return "application/xml";
    if (CStr.strcmp(e, ".svg") == 0) return "image/svg+xml";
    if (CStr.strcmp(e, ".png") == 0) return "image/png";
    if (CStr.strcmp(e, ".jpg") == 0 || CStr.strcmp(e, ".jpeg") == 0) return "image/jpeg";
    if (CStr.strcmp(e, ".gif") == 0) return "image/gif";
    if (CStr.strcmp(e, ".webp") == 0) return "image/webp";
    if (CStr.strcmp(e, ".ico") == 0) return "image/x-icon";
    if (CStr.strcmp(e, ".woff") == 0) return "font/woff";
    if (CStr.strcmp(e, ".woff2") == 0) return "font/woff2";
    if (CStr.strcmp(e, ".wasm") == 0) return "application/wasm";
    if (CStr.strcmp(e, ".pdf") == 0) return "application/pdf";
    if (CStr.strcmp(e, ".mp4") == 0) return "video/mp4";
    if (CStr.strcmp(e, ".webm") == 0) return "video/webm";
    if (CStr.strcmp(e, ".mp3") == 0) return "audio/mpeg";
    return "application/octet-stream";
}

struct Response {
    status: int = 200;
    headers: L.List<Header>;
    body: Str.String;

    // Adds a header, or replaces it if already set (case-insensitive). False if name/value contain CR or LF.
    header(name: char*, value: char*) => bool {
        if (hasCrlf(name) || hasCrlf(value)) return false;
        let i: u64 = 0;
        let replaced: bool = false;
        while (!replaced && i < this.headers.len) {
            if (_stricmp(this.headers.data[i].name.cstr(), name) == 0) {
                this.headers.data[i].value.set(value);
                replaced = true;
            }
            i = i + 1;
        }
        if (replaced) return true;
        let h: Header;
        h.name.set(name);
        h.value.set(value);
        return this.headers.push(h);
    }

    setBody(contentType: char*, s: char*) => bool {
        this.header("Content-Type", contentType);
        return this.body.set(s);
    }
    text(s: char*) => bool { return this.setBody("text/plain; charset=utf-8", s); }
    html(s: char*) => bool { return this.setBody("text/html; charset=utf-8", s); }
    json(s: char*) => bool { return this.setBody("application/json", s); }

    redirect(url: char*, code: int) => bool {
        this.status = code;
        return this.header("Location", url);
    }

    // Replaces the body with the file's bytes and sets Content-Type from the extension. False if it cannot be read
    // (missing, a directory, or larger than 128 MiB).
    sendFile(path: char*) => bool {
        let f: void* = CFile.fopen(path, "rb");
        if (f == nullptr) return false;
        CFile._fseeki64(f, 0, 2);
        let size: i64 = CFile._ftelli64(f);
        CFile._fseeki64(f, 0, 0);
        let ok: bool = size >= 0 && size <= 134217728;
        let got: u64 = 0;
        if (ok) ok = this.body.reserve(size as u64);
        if (ok && size > 0) {
            got = CFile.fread(this.body.data as void*, 1, size as u64, f);
            ok = got == (size as u64);
        }
        CFile.fclose(f);
        if (!ok) return false;
        this.body.len = got;
        this.body.data[got] = 0 as char;
        this.header("Content-Type", mimeType(path));
        return true;
    }

    free() => void {
        freeHeaders(&this.headers);
        this.body.free();
    }
}

// ---------------------------------------------------------------------------------------------------------------
// Routing
// ---------------------------------------------------------------------------------------------------------------
struct Route {
    method: Str.String;       // "GET", "POST", ... or "*"
    pattern: Str.String;
    handler: fn(Request*, Response*) => void = nullptr;
    user: void* = nullptr;
    isStatic: bool = false;
    dir: Str.String;          // static routes: directory to serve
}

function clearMatch(req: Request*) => void {
    let i: u64 = 0;
    while (i < req.params.len) { req.params.data[i].name.free(); req.params.data[i].value.free(); i = i + 1; }
    req.params.clear();
    req.rest.clear();
}

// Matches pattern ("/users/:id/*") against path segment by segment. Fills req.params / req.rest.
function matchPath(pat: char*, path: char*, req: Request*) => bool {
    let pi: u64 = 0;
    let ti: u64 = 0;
    let ok: bool = true;
    let done: bool = false;
    while (!done) {
        let pc: int = pat[pi] as int;
        let tc: int = path[ti] as int;
        if (pc == 0 && tc == 0) {
            done = true;
        } else if (pc == 0 || tc == 0 || pc != 47 || tc != 47) {
            ok = false;
            done = true;
        } else {
            pi = pi + 1;
            ti = ti + 1;
            let ps: u64 = pi;
            let ts: u64 = ti;
            while ((pat[pi] as int) != 0 && (pat[pi] as int) != 47) pi = pi + 1;
            while ((path[ti] as int) != 0 && (path[ti] as int) != 47) ti = ti + 1;
            let pl: u64 = pi - ps;
            let tl: u64 = ti - ts;
            if (pl == 1 && (pat[ps] as int) == 42) {
                req.rest.set(&path[ts]);                       // wildcard swallows the remainder
                done = true;
            } else if (pl > 1 && (pat[ps] as int) == 58) {
                if (tl == 0) {
                    ok = false;
                    done = true;
                } else {
                    let p: Param;
                    p.name.appendN(&pat[ps + 1], pl - 1);
                    p.value.appendN(&path[ts], tl);
                    req.params.push(p);
                }
            } else {
                if (pl != tl || CMem.memcmp(&pat[ps] as void*, &path[ts] as void*, pl) != 0) {
                    ok = false;
                    done = true;
                }
            }
        }
    }
    return ok;
}

function staticHandler(req: Request*, res: Response*) => void {
    let dir: Str.String* = req.user as Str.String*;
    let dec: Str.String;
    let bad: int = 0;
    if (!Text.urlDecode(req.rest.cstr(), &dec)) bad = 400;
    if (bad == 0 && CStr.strlen(dec.cstr()) != dec.len) bad = 400;              // embedded NUL
    if (bad == 0 && (CStr.strstr(dec.cstr(), "..") != nullptr || CStr.strchr(dec.cstr(), 92) != nullptr || CStr.strchr(dec.cstr(), 58) != nullptr)) bad = 403;
    if (bad != 0) {
        res.status = bad;
        res.text(reason(bad));
    } else {
        let full: Str.String;
        full.set(dir.cstr());
        if (full.len > 0 && !dir.endsWith("/")) full.appendChar(47 as char);
        full.append(dec.cstr());
        if (dec.len == 0 || dec.endsWith("/")) full.append("index.html");
        if (!res.sendFile(full.cstr())) {
            res.status = 404;
            res.text("404 Not Found");
        }
        full.free();
    }
    dec.free();
}

// ---------------------------------------------------------------------------------------------------------------
// One connection: read request, dispatch, write response
// ---------------------------------------------------------------------------------------------------------------
function writeResponse(sock: Net.Socket*, res: Response*, isHead: bool) => void {
    let noBody: bool = res.status < 200 || res.status == 204 || res.status == 304;
    let out: Str.String;
    out.append("HTTP/1.1 ");
    out.appendInt(res.status as i64);
    out.append(" ");
    out.append(reason(res.status));
    out.append("\r\n");
    let hasType: bool = false;
    let i: u64 = 0;
    while (i < res.headers.len) {
        let nm: char* = res.headers.data[i].name.cstr();
        if (_stricmp(nm, "Content-Length") != 0 && _stricmp(nm, "Connection") != 0) {
            out.append(nm);
            out.append(": ");
            out.append(res.headers.data[i].value.cstr());
            out.append("\r\n");
        }
        if (_stricmp(nm, "Content-Type") == 0) hasType = true;
        i = i + 1;
    }
    if (!hasType && res.body.len > 0 && !noBody) out.append("Content-Type: text/plain; charset=utf-8\r\n");
    if (!noBody) {
        out.append("Content-Length: ");
        out.appendInt(res.body.len as i64);
        out.append("\r\n");
    }
    out.append("Connection: close\r\n\r\n");
    if (!isHead && !noBody) out.appendString(&res.body);
    sock.sendAll(out.cstr() as u8*, out.len);
    out.free();
}

function sendError(sock: Net.Socket*, code: int) => void {
    let res: Response;
    res.status = code;
    res.text(reason(code));
    writeResponse(sock, &res, false);
    res.free();
}

// Reads and parses one request. On failure returns false with *status = HTTP status to answer with (0 = just hang up).
function parseRequest(srvp: void*, sock: Net.Socket*, req: Request*, status: int*) => bool {
    let srv: HttpServer* = srvp as HttpServer*;
    status[0] = 400;
    let raw: Str.String;
    let buf: [4096]u8;
    let term: char* = nullptr;
    let alive: bool = true;
    while (term == nullptr && alive) {
        term = CStr.strstr(raw.cstr(), "\r\n\r\n");
        if (term == nullptr) {
            if (raw.len > srv.maxHeader) {
                status[0] = 431;
                alive = false;
            } else {
                let n: i64 = sock.recv(&buf as u8*, 4096);
                if (n <= 0) { *status = 0; alive = false; }
                else raw.appendN(&buf as char*, n as u64);
            }
        }
    }
    if (term == nullptr) {
        raw.free();
        return false;
    }

    let idx: u64 = (term as u64) - (raw.data as u64);
    let h: char* = raw.data;
    let lineEnd: char* = CStr.strstr(h, "\r\n");

    // request line: METHOD SP target SP HTTP/1.x
    let sp1: char* = CStr.strchr(h, 32);
    let ok: bool = sp1 != nullptr && (sp1 as u64) < (lineEnd as u64);
    let sp2: char* = nullptr;
    if (ok) {
        sp2 = CStr.strchr(&sp1[1], 32);
        ok = sp2 != nullptr && (sp2 as u64) < (lineEnd as u64);
    }
    if (ok) {
        req.method.appendN(h, (sp1 as u64) - (h as u64));
        req.target.appendN(&sp1[1], (sp2 as u64) - (sp1 as u64) - 1);
        req.version.appendN(&sp2[1], (lineEnd as u64) - (sp2 as u64) - 1);
        if (req.method.len == 0 || req.target.len == 0 || !req.version.startsWith("HTTP/1.")) ok = false;
    }

    // header lines
    if (ok) {
        let line: char* = &lineEnd[2];
        let more: bool = true;
        while (more && ok) {
            let e: char* = CStr.strstr(line, "\r\n");
            if (e == nullptr || e == line) {
                more = false;
            } else {
                let colon: char* = CStr.strchr(line, 58);
                if (colon == nullptr || (colon as u64) >= (e as u64) || colon == line) {
                    ok = false;
                } else if (req.headers.len >= 100) {
                    ok = false;
                    status[0] = 431;
                } else {
                    let hd: Header;
                    hd.name.appendN(line, (colon as u64) - (line as u64));
                    let vs: char* = &colon[1];
                    while (isWs(vs[0] as int)) vs = &vs[1];
                    let vlen: u64 = (e as u64) - (vs as u64);
                    while (vlen > 0 && isWs(vs[vlen - 1] as int)) vlen = vlen - 1;
                    hd.value.appendN(vs, vlen);
                    req.headers.push(hd);
                    line = &e[2];
                }
            }
        }
    }

    // body framing
    let clen: u64 = 0;
    if (ok && req.header("transfer-encoding") != nullptr) {
        ok = false;
        status[0] = 501;
    }
    if (ok) {
        let cl: char* = req.header("content-length");
        if (cl != nullptr) {
            let c0: int = cl[0] as int;
            if (c0 < 48 || c0 > 57) {
                ok = false;
            } else {
                let endp: char* = nullptr;
                clen = CStr.strtoull(cl, &endp, 10);
                if ((endp[0] as int) != 0) {
                    ok = false;
                } else if (clen > srv.maxBody) {
                    ok = false;
                    status[0] = 413;
                }
            }
        }
    }
    if (ok && clen > 0) {
        let start: u64 = idx + 4;
        let have: u64 = raw.len - start;
        if (have > clen) have = clen;
        req.body.appendN(&raw.data[start], have);
        if (req.body.len < clen) {
            let ex: char* = req.header("expect");
            if (ex != nullptr && _stricmp(ex, "100-continue") == 0) sock.sendStr("HTTP/1.1 100 Continue\r\n\r\n");
        }
        while (ok && req.body.len < clen) {
            let want: u64 = clen - req.body.len;
            if (want > 4096) want = 4096;
            let n: i64 = sock.recv(&buf as u8*, want);
            if (n <= 0) { ok = false; *status = 0; }
            else req.body.appendN(&buf as char*, n as u64);
        }
    }

    // split target into path and query (origin-form targets only)
    if (ok) {
        let t: char* = req.target.cstr();
        let q: char* = CStr.strchr(t, 63);
        if (q == nullptr) {
            req.path.set(t);
        } else {
            req.path.appendN(t, (q as u64) - (t as u64));
            req.query.set(&q[1]);
        }
        if (req.path.len == 0 || (req.path.data[0] as int) != 47) { ok = false; *status = 400; }
    }
    raw.free();
    return ok;
}

function dispatch(srvp: void*, req: Request*, res: Response*) => void {
    let srv: HttpServer* = srvp as HttpServer*;
    let isHead: bool = req.method.equalsC("HEAD");
    let handled: bool = false;
    let pathSeen: bool = false;
    let i: u64 = 0;
    while (!handled && i < srv.routes.len) {
        let r: Route* = &srv.routes.data[i];
        clearMatch(req);
        if (matchPath(r.pattern.cstr(), req.path.cstr(), req)) {
            let methodOk: bool = r.method.equalsC("*") || r.method.equalsC(req.method.cstr()) || (isHead && r.method.equalsC("GET"));
            if (methodOk) {
                req.user = r.user;
                if (r.isStatic) req.user = &r.dir as void*;
                r.handler(req, res);
                handled = true;
            } else {
                pathSeen = true;
            }
        }
        i = i + 1;
    }
    if (!handled) {
        clearMatch(req);
        if (srv.notFound != nullptr) {
            srv.notFound(req, res);
        } else if (pathSeen) {
            res.status = 405;
            res.text("405 Method Not Allowed");
        } else {
            res.status = 404;
            res.text("404 Not Found");
        }
    }
}

function handleConn(srvp: void*, sock: Net.Socket*) => void {
    let srv: HttpServer* = srvp as HttpServer*;
    let tmo: u32 = srv.timeoutMs;
    CN.setsockopt(sock.s, 0xFFFF, 0x1006, &tmo as void*, 4);      // SO_RCVTIMEO
    CN.setsockopt(sock.s, 0xFFFF, 0x1005, &tmo as void*, 4);      // SO_SNDTIMEO
    let req: Request;
    let res: Response;
    let status: int = 400;
    if (parseRequest(srvp, sock, &req, &status)) {
        let isHead: bool = req.method.equalsC("HEAD");
        dispatch(srvp, &req, &res);
        writeResponse(sock, &res, isHead);
    } else if (status != 0) {
        sendError(sock, status);
    }
    req.free();
    res.free();
}

struct Conn { server: void*; sock: Net.Socket; }   // server is an HttpServer*

function connMain(arg: void*) => u32 {
    let c: Conn* = arg as Conn*;
    let srv: HttpServer* = c.server as HttpServer*;
    handleConn(c.server, &c.sock);
    CN.shutdown(c.sock.s, 1);
    c.sock.close();
    CMem.free(c as void*);
    let one: i64 = 1;
    srv.active.add(0 - one);
    return 0;
}

// ---------------------------------------------------------------------------------------------------------------
// Server
// ---------------------------------------------------------------------------------------------------------------
struct HttpServer {
    listener: Net.Socket;
    routes: L.List<Route>;
    stopFlag: Th.Atomic;
    active: Th.Atomic;
    maxHeader: u64 = 65536;
    maxBody: u64 = 8388608;
    maxConnections: u64 = 512;
    timeoutMs: u32 = 15000;
    notFound: fn(Request*, Response*) => void = nullptr;     // optional custom 404 handler

    routeUser(method: char*, pattern: char*, handler: fn(Request*, Response*) => void, user: void*) => bool {
        let r: Route;
        r.method.set(method);
        r.pattern.set(pattern);
        r.handler = handler;
        r.user = user;
        return this.routes.push(r);
    }
    route(method: char*, pattern: char*, handler: fn(Request*, Response*) => void) => bool {
        return this.routeUser(method, pattern, handler, nullptr);
    }
    get(pattern: char*, handler: fn(Request*, Response*) => void) => bool { return this.routeUser("GET", pattern, handler, nullptr); }
    post(pattern: char*, handler: fn(Request*, Response*) => void) => bool { return this.routeUser("POST", pattern, handler, nullptr); }
    put(pattern: char*, handler: fn(Request*, Response*) => void) => bool { return this.routeUser("PUT", pattern, handler, nullptr); }
    del(pattern: char*, handler: fn(Request*, Response*) => void) => bool { return this.routeUser("DELETE", pattern, handler, nullptr); }

    // GET <urlPrefix>/* serves files from dir (e.g. serveStatic("/assets", "public")).
    serveStatic(urlPrefix: char*, dir: char*) => bool {
        let r: Route;
        let pat: Str.String;
        pat.set(urlPrefix);
        while (pat.len > 0 && (pat.data[pat.len - 1] as int) == 47) pat.truncate(pat.len - 1);
        pat.append("/*");
        r.method.set("GET");
        r.pattern.set(pat.cstr());
        pat.free();
        r.handler = staticHandler;
        r.isStatic = true;
        r.dir.set(dir);
        return this.routes.push(r);
    }

    // Starts listening on 0.0.0.0:port without accepting yet.
    bind(port: u16) => bool {
        this.stopFlag.store(0);
        return Net.listenTcp(port, 128, &this.listener);
    }

    // Accept loop: blocks until stop() is called, then waits (up to 5 s) for running requests to finish.
    serve() => void {
        while (this.stopFlag.load() == 0) {
            let cs: Net.Socket;
            if (Net.acceptTcp(&this.listener, &cs)) {
                if (this.active.load() >= (this.maxConnections as i64)) {
                    sendError(&cs, 503);
                    cs.close();
                } else {
                    let c = CMem.malloc(sizeof(Conn)) as Conn*;
                    if (c == nullptr) {
                        cs.close();
                    } else {
                        c.server = this as void*;
                        c.sock = cs;
                        this.active.add(1);
                        let t: Th.Thread;
                        if (t.spawn(connMain, c as void*)) {
                            CT.CloseHandle(t.h);               // detached: the connection thread cleans up after itself
                        } else {
                            let one: i64 = 1;
                            this.active.add(0 - one);
                            cs.close();
                            CMem.free(c as void*);
                        }
                    }
                }
            } else {
                if (this.stopFlag.load() == 0) Th.sleepMs(5);   // avoid a busy loop on a persistent accept error
            }
        }
        let waited: u32 = 0;
        while (this.active.load() > 0 && waited < 5000) {
            Th.sleepMs(10);
            waited = waited + 10;
        }
    }

    // bind + serve in one call (blocks). False if the port could not be bound.
    listen(port: u16) => bool {
        if (!this.bind(port)) return false;
        this.serve();
        return true;
    }

    // Safe to call from a handler or another thread: ends serve().
    stop() => void {
        this.stopFlag.store(1);
        this.listener.close();
    }

    free() => void {
        let i: u64 = 0;
        while (i < this.routes.len) {
            this.routes.data[i].method.free();
            this.routes.data[i].pattern.free();
            this.routes.data[i].dir.free();
            i = i + 1;
        }
        this.routes.free();
        this.listener.close();
    }
}
