// standard/1/c/stdio_extra.os  (merge into c/string.os / c/console.os if you prefer)
extern "C" fn sscanf(s: char*, fmt: char*, ...) => int;
extern "C" fn snprintf(buf: char*, n: u64, fmt: char*, ...) => int;
extern "C" fn fopen(path: char*, mode: char*) => void*;
extern "C" fn fclose(f: void*) => int;
extern "C" fn fread(buf: void*, sz: u64, n: u64, f: void*) => u64;
extern "C" fn fwrite(buf: void*, sz: u64, n: u64, f: void*) => u64;
