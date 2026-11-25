// standard/1/c/webp.os  (libwebp decoder, wrapped by dependencies/webp/src/webp_impl.c)
// DLL built by dependencies/webp/build_webp.sh

extern "dependencies/webp/bin/webp.dll" {
    fn om_webp_check(buf: u8*, n: int) => int;                              // 1 = "RIFF....WEBP"
    fn om_webp_decode(buf: u8*, n: int, w: int*, h: int*) => u8*;           // RGBA8, nullptr on failure
    fn om_webp_free(p: void*) => void;                                      // frees om_webp_decode pixels
}
