// standard/1/c/curl.os  (libcurl)
extern "dependencies/libcurl/lib/dll-release-x64/libcurl.dll" {
    fn curl_global_init(flags: i64) => int;
    fn curl_easy_init() => void*;
    fn curl_easy_cleanup(c: void*) => void;
    fn curl_easy_setopt(c: void*, opt: int, ...) => int;
    fn curl_easy_perform(c: void*) => int;
    fn curl_easy_getinfo(c: void*, info: int, ...) => int;
    fn curl_slist_append(l: void*, s: char*) => void*;
    fn curl_slist_free_all(l: void*) => void;
    fn curl_easy_strerror(code: int) => char*;
}
