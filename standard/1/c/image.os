// standard/1/c/image.os  (stb_image / stb_image_write + allocator shims)
// DLL built from dependencies/stb/src/stb_impl.c

extern "dependencies/stb/bin/stb.dll" {
    fn stbi_load(path: char*, w: int*, h: int*, ch: int*, want: int) => u8*;
    fn stbi_load_from_memory(buf: u8*, n: int, w: int*, h: int*, ch: int*, want: int) => u8*;
    fn stbi_failure_reason() => char*;
    fn stbi_write_png(path: char*, w: int, h: int, ch: int, data: void*, stride: int) => int;
    fn stbi_write_jpg(path: char*, w: int, h: int, ch: int, data: void*, q: int) => int;
    fn stbi_write_bmp(path: char*, w: int, h: int, ch: int, data: void*) => int;
    fn stbi_write_tga(path: char*, w: int, h: int, ch: int, data: void*) => int;
    fn om_alloc(n: u64) => void*;
    fn om_free(p: void*) => void;
}
