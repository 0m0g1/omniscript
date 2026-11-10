extern "C" fn printf(fmt: char*, ...) => int;
extern "C" fn fflush(f: void*) => int;
extern "C" fn malloc(size: u64) => void*;
extern "C" fn free(p: void*) => void;

struct A { n: u64 = 9; }
let g_size: u64 = 12;
function main() => void {
    printf("[11] start\n"); fflush(nullptr);
    let a: A;
    printf("[11] %u %u\n", g_size as uint, a.n as uint); fflush(nullptr);
}
