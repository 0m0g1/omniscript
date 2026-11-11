// standard/1/c/image.os  (stb_image / stb_image_write)
extern "C" fn stbi_load(path: char*, w: int*, h: int*, ch: int*, want: int) => u8*;
extern "C" fn stbi_load_from_memory(buf: u8*, n: int, w: int*, h: int*, ch: int*, want: int) => u8*;
extern "C" fn stbi_image_free(p: void*) => void;
extern "C" fn stbi_write_png(path: char*, w: int, h: int, ch: int, data: void*, stride: int) => int;
extern "C" fn stbi_write_jpg(path: char*, w: int, h: int, ch: int, data: void*, q: int) => int;
extern "C" fn stbi_write_bmp(path: char*, w: int, h: int, ch: int, data: void*) => int;
