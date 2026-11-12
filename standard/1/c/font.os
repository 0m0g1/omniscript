// standard/1/c/font.os  (stb_truetype)
// DLL built from dependencies/stb/src/stb_impl.c

extern "dependencies/stb/bin/stb.dll" {
    fn stbtt_InitFont(info: void*, data: u8*, offset: int) => int;
    fn stbtt_ScaleForPixelHeight(info: void*, px: float) => float;
    fn stbtt_GetCodepointHMetrics(info: void*, cp: int, adv: int*, lsb: int*) => void;
    fn stbtt_GetFontVMetrics(info: void*, asc: int*, desc: int*, gap: int*) => void;
    fn stbtt_GetCodepointBitmap(info: void*, sx: float, sy: float, cp: int, w: int*, h: int*, xo: int*, yo: int*) => u8*;
    fn stbtt_FreeBitmap(bm: u8*, ud: void*) => void;
}