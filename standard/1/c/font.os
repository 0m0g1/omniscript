// standard/1/c/font.os  (stb_truetype)
extern "C" fn stbtt_InitFont(info: void*, data: u8*, offset: int) => int;
extern "C" fn stbtt_ScaleForPixelHeight(info: void*, px: float) => float;
extern "C" fn stbtt_GetCodepointHMetrics(info: void*, cp: int, adv: int*, lsb: int*) => void;
extern "C" fn stbtt_GetFontVMetrics(info: void*, asc: int*, desc: int*, gap: int*) => void;
extern "C" fn stbtt_GetCodepointBitmap(info: void*, sx: float, sy: float, cp: int, w: int*, h: int*, xo: int*, yo: int*) => u8*;
extern "C" fn stbtt_FreeBitmap(bm: u8*, ud: void*) => void;
