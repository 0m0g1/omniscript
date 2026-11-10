extern "C" fn printf(fmt: char*, ...) => int;
extern "C" fn fflush(f: void*) => int;
extern "C" fn malloc(size: u64) => void*;
extern "C" fn free(p: void*) => void;

struct In { w: uint = 0; h: uint = 0; }
struct Out { e: In; k: uint = 0; }
function main() => void {
    printf("[09] start\n"); fflush(nullptr);
    let o: Out;
    o.e.w = 3;
    o.e.h = 4;
    printf("[09] %u %u\n", o.e.w, o.e.h); fflush(nullptr);
}
