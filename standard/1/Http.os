// standard/1/Http.os
// Tier 5: HTTP client via libcurl. Http.get(url, &resp) / Http.post(url, body, contentType, &resp). resp.free() after.
// v2: response headers, final URL, cookie jar file (Http.cookieFile), explicit body length (binary-safe), timeout.
module C = import "standard/1/c/curl.os";
module Str = import "standard/1/String.os";
module CMem = import "standard/1/c/mem.os";

let CURLOPT_URL: int = 10002;
let CURLOPT_WRITEFUNCTION: int = 20011;
let CURLOPT_WRITEDATA: int = 10001;
let CURLOPT_POSTFIELDS: int = 10015;
let CURLOPT_POSTFIELDSIZE: int = 60;
let CURLOPT_HTTPHEADER: int = 10023;
let CURLOPT_FOLLOWLOCATION: int = 52;
let CURLOPT_TIMEOUT: int = 13;
let CURLOPT_USERAGENT: int = 10018;
let CURLOPT_CUSTOMREQUEST: int = 10036;
let CURLOPT_COOKIEFILE: int = 10031;
let CURLOPT_COOKIEJAR: int = 10082;
let CURLOPT_HEADERDATA: int = 10029;
let CURLOPT_HEADERFUNCTION: int = 20079;
let CURLINFO_RESPONSE_CODE: int = 0x200002;
let CURLINFO_EFFECTIVE_URL: int = 0x100001;

// Set once by the host (e.g. "profile/cookies.txt"). nullptr = no cookies.
// Each request loads and saves this file, so a session cookie set by /api/login is sent on the next request.
let cookieFile: char* = nullptr;
let timeoutSec: int = 30;
function setCookieFile(p: char*) => void { cookieFile = p; }

function sameStr(a: char*, b: char*) => bool {
    let i: u64 = 0;
    let ok: bool = true;
    while (ok && ((a[i] as int) != 0 || (b[i] as int) != 0)) {
        if (a[i] != b[i]) ok = false;
        i = i + 1;
    }
    return ok;
}

struct Response {
    status: i64 = 0;
    body: Str.String;
    headers: Str.String;        // raw header block, "Name: value\r\n" lines (all hops if redirected)
    finalUrl: char* = nullptr;  // URL after redirects (owned by curl until cleanup; copied below)
    error: char* = nullptr;
    free() => void { this.body.free(); this.headers.free(); }
}

// libcurl write callback: (data, size, nmemb, userdata)
function onWrite(p: char*, size: u64, n: u64, user: void*) => u64 {
    let s: Str.String* = user as Str.String*;
    s.appendN(p, size * n);
    return size * n;
}

let inited: bool = false;

// method: "GET","POST","PUT","DELETE"...; headers: nullptr-terminated array or nullptr
// bodyLen < 0 means "body is a NUL-terminated string".
function requestEx(method: char*, url: char*, body: char*, bodyLen: i64, headers: char**, out: Response*) => bool {
    if (!inited) { C.curl_global_init(3); inited = true; }
    let c = C.curl_easy_init();
    if (c == nullptr) return false;
    C.curl_easy_setopt(c, CURLOPT_URL, url);
    C.curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1 as i64);
    C.curl_easy_setopt(c, CURLOPT_TIMEOUT, timeoutSec as i64);
    C.curl_easy_setopt(c, CURLOPT_USERAGENT, "omniscript/1");
    // GET and POST are left to curl (CUSTOMREQUEST would stay "POST" across a 303 redirect and re-POST to the new page)
    let isGet: bool = sameStr(method, "GET");
    let isPost: bool = sameStr(method, "POST");
    if (isPost) C.curl_easy_setopt(c, 47, 1 as i64);                // CURLOPT_POST
    else if (!isGet) C.curl_easy_setopt(c, CURLOPT_CUSTOMREQUEST, method);
    C.curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, onWrite as void*);
    C.curl_easy_setopt(c, CURLOPT_WRITEDATA, &out.body as void*);
    C.curl_easy_setopt(c, CURLOPT_HEADERFUNCTION, onWrite as void*);
    C.curl_easy_setopt(c, CURLOPT_HEADERDATA, &out.headers as void*);
    if (cookieFile != nullptr) {
        C.curl_easy_setopt(c, CURLOPT_COOKIEFILE, cookieFile);
        C.curl_easy_setopt(c, CURLOPT_COOKIEJAR, cookieFile);
    }
    if (body != nullptr) {
        if (bodyLen >= 0) C.curl_easy_setopt(c, CURLOPT_POSTFIELDSIZE, bodyLen);
        C.curl_easy_setopt(c, CURLOPT_POSTFIELDS, body);
    }
    let list: void* = nullptr;
    if (headers != nullptr) {
        let i: u64 = 0;
        while (headers[i] != nullptr) { list = C.curl_slist_append(list, headers[i]); i = i + 1; }
        C.curl_easy_setopt(c, CURLOPT_HTTPHEADER, list);
    }
    let rc: int = C.curl_easy_perform(c);
    if (rc == 0) {
        C.curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &out.status);
        let eff: char* = nullptr;
        C.curl_easy_getinfo(c, CURLINFO_EFFECTIVE_URL, &eff);
        if (eff != nullptr) {
            // copy: curl frees its string at cleanup
            let n: u64 = 0;
            while ((eff[n] as int) != 0) { n = n + 1; }
            let d: char* = CMem.malloc(n + 1) as char*;
            let k: u64 = 0;
            while (k <= n) { d[k] = eff[k]; k = k + 1; }
            out.finalUrl = d;
        }
    } else out.error = C.curl_easy_strerror(rc);
    if (list != nullptr) C.curl_slist_free_all(list);
    C.curl_easy_cleanup(c);
    return rc == 0;
}

function request(method: char*, url: char*, body: char*, headers: char**, out: Response*) => bool {
    return requestEx(method, url, body, 0 - 1, headers, out);
}

function get(url: char*, out: Response*) => bool { return request("GET", url, nullptr, nullptr, out); }
function del(url: char*, out: Response*) => bool { return request("DELETE", url, nullptr, nullptr, out); }

function post(url: char*, body: char*, contentType: char*, out: Response*) => bool {
    let ct: Str.String; ct.append("Content-Type: "); ct.append(contentType);
    let hs: [2]char*; hs[0] = ct.cstr(); hs[1] = nullptr;
    let ok: bool = request("POST", url, body, &hs as char**, out);
    ct.free();
    return ok;
}

function postJson(url: char*, json: char*, out: Response*) => bool { return post(url, json, "application/json", out); }
