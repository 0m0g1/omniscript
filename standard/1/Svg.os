// standard/1/Svg.os: SVG -> RGBA8 via lunasvg (dependencies/lunasvg)
module S = import "standard/1/c/svg.os";
module CMem = import "standard/1/c/mem.os";

// Returns a malloc'd RGBA8 buffer (row 0 = top) or nullptr. Caller frees with CMem.free.
// scale: 2.0 = rasterize at 2x intrinsic size. rgb = 0xRRGGBB, used for currentColor.
// outW/outH receive the pixel size.
rasterize(data: u8*, len: int, scale: float, rgb: u32, outW: int*, outH: int*) => u8* {
    let sw: float = 0.0; let sh: float = 0.0;
    let buf: u8* = nullptr;
    if (S.svg_size(data as char*, len, &sw, &sh) == 1) {
        if (sw < 1.0) sw = 24.0;
        if (sh < 1.0) sh = 24.0;
        let pw: int = (sw * scale) as int;
        let ph: int = (sh * scale) as int;
        if (pw > 0 && ph > 0 && pw <= 4096 && ph <= 4096) {
            buf = CMem.malloc((pw * ph * 4) as u64) as u8*;
            if (buf != nullptr) {
                if (S.svg_rasterize(data as char*, len, pw, ph, rgb, buf) == 1) {
                    *outW = pw; *outH = ph;
                } else {
                    CMem.free(buf as void*);
                    buf = nullptr;
                }
            }
        }
    }
    return buf;
}