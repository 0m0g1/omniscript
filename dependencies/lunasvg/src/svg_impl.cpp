#include <lunasvg.h>
#include <cstring>

#define EXP extern "C" __declspec(dllexport)

// Intrinsic size of the SVG. Returns 1 on success.
EXP int svg_size(const char* data, int len, float* w, float* h) {
    auto doc = lunasvg::Document::loadFromData(data, (size_t)len);
    if (!doc) return 0;
    *w = doc->width();
    *h = doc->height();
    return 1;
}

// Rasterize to straight (non-premultiplied) RGBA8, row 0 = top.
// out must hold w*h*4 bytes. rgb = 0xRRGGBB, used as the CSS `color` so currentColor works.
EXP int svg_rasterize(const char* data, int len, int w, int h, unsigned int rgb, unsigned char* out) {
    auto doc = lunasvg::Document::loadFromData(data, (size_t)len);
    if (!doc) return 0;
    char css[64];
    snprintf(css, sizeof(css), "svg{color:#%06x}", rgb & 0xFFFFFF);
    doc->applyStyleSheet(css);
    auto bmp = doc->renderToBitmap(w, h, 0x00000000);
    if (bmp.isNull()) return 0;
    bmp.convertToRGBA();
    memcpy(out, bmp.data(), (size_t)w * h * 4);
    return 1;
}