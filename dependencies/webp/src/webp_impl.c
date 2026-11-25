// dependencies/webp/src/webp_impl.c -- thin WebP decoder DLL for Omniscript (wraps libwebp's decoder).
#include <stdint.h>
#include "webp/decode.h"

#if defined(_WIN32)
#define OM_API __declspec(dllexport)
#else
#define OM_API __attribute__((visibility("default")))
#endif

// 1 if the bytes look like a WebP container ("RIFF....WEBP"), else 0.
OM_API int om_webp_check(const uint8_t* buf, int n) {
    if (!buf || n < 12) return 0;
    return buf[0]=='R' && buf[1]=='I' && buf[2]=='F' && buf[3]=='F' &&
           buf[8]=='W' && buf[9]=='E' && buf[10]=='B' && buf[11]=='P';
}

// Decode to tightly packed RGBA8 (row 0 = top). Returns NULL on failure. Free with om_webp_free.
OM_API uint8_t* om_webp_decode(const uint8_t* buf, int n, int* w, int* h) {
    if (!om_webp_check(buf, n)) return 0;
    int ww = 0, hh = 0;
    uint8_t* px = WebPDecodeRGBA(buf, (size_t)n, &ww, &hh);
    if (!px) return 0;
    *w = ww; *h = hh;
    return px;
}

OM_API void om_webp_free(void* p) { if (p) WebPFree(p); }
