// standard/1/Http.os
// Tier 5: HTTP client via libcurl. Http.get(url, &resp) / Http.post(url, body, contentType, &resp). resp.free() after.
module C = import "standard/1/c/curl.os";
module Str = import "standard/1/String.os";
module CMem = import "standard/1/c/mem.os";

let CURLOPT_URL: int = 10002;
let CURLOPT_WRITEFUNCTION: int = 20011;
let CURLOPT_WRITEDATA: int = 10001;
let CURLOPT_POSTFIELDS: int = 10015;
let CURLOPT_HTTPHEADER: int = 10023;
let CURLOPT_FOLLOWLOCATION: int = 52;
let CURLOPT_TIMEOUT: int = 13;
let CURLOPT_USERAGENT: int = 10018;
let CURLOPT_CUSTOMREQUEST: int = 10036;
let CURLINFO_RESPONSE_CODE: int = 0x200002;

struct Response {
    status: i64 = 0;
    body: Str.String;
    error: char* = nullptr;
    free() => void { this.body.free(); }
}

// libcurl write callback: (data, size, nmemb, userdata)
function onWrite(p: char*, size: u64, n: u64, user: void*) => u64 {
    let s: Str.String* = user as Str.String*;
    s.appendN(p, size * n);
    return size * n;
}

let inited: bool = false;

// method: "GET","POST","PUT","DELETE"...; headers: nullptr-terminated array or nullptr
function request(method: char*, url: char*, body: char*, headers: char**, out: Response*) => bool {
    if (!inited) { C.curl_global_init(3); inited = true; }
    let c = C.curl_easy_init();
    if (c == nullptr) return false;
    C.curl_easy_setopt(c, CURLOPT_URL, url);
    C.curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1 as i64);
    C.curl_easy_setopt(c, CURLOPT_TIMEOUT, 30 as i64);
    C.curl_easy_setopt(c, CURLOPT_USERAGENT, "omniscript/1");
    C.curl_easy_setopt(c, CURLOPT_CUSTOMREQUEST, method);
    C.curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, onWrite as void*);
    C.curl_easy_setopt(c, CURLOPT_WRITEDATA, &out.body as void*);
    if (body != nullptr) C.curl_easy_setopt(c, CURLOPT_POSTFIELDS, body);
    let list: void* = nullptr;
    if (headers != nullptr) {
        let i: u64 = 0;
        while (headers[i] != nullptr) { list = C.curl_slist_append(list, headers[i]); i = i + 1; }
        C.curl_easy_setopt(c, CURLOPT_HTTPHEADER, list);
    }
    let rc: int = C.curl_easy_perform(c);
    if (rc == 0) C.curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &out.status);
    else out.error = C.curl_easy_strerror(rc);
    if (list != nullptr) C.curl_slist_free_all(list);
    C.curl_easy_cleanup(c);
    return rc == 0;
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
