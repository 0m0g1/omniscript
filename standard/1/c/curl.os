// standard/1/c/curl.os  (libcurl)
extern "C" fn curl_global_init(flags: i64) => int;
extern "C" fn curl_easy_init() => void*;
extern "C" fn curl_easy_cleanup(c: void*) => void;
extern "C" fn curl_easy_setopt(c: void*, opt: int, ...) => int;
extern "C" fn curl_easy_perform(c: void*) => int;
extern "C" fn curl_easy_getinfo(c: void*, info: int, ...) => int;
extern "C" fn curl_slist_append(l: void*, s: char*) => void*;
extern "C" fn curl_slist_free_all(l: void*) => void;
extern "C" fn curl_easy_strerror(code: int) => char*;
