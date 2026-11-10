extern "C" fn printf(fmt: char*, ...) => int;
extern "C" fn fflush(f: void*) => int;
extern "C" fn malloc(size: u64) => void*;
extern "C" fn free(p: void*) => void;

struct S {
    a: uint = 0;
    m: void* = nullptr;
}
function main() => void {
    printf("[05] start\n"); fflush(nullptr);
    let s: S[2];
    s[0].a = 4;
    s[1].a = 5;
    s[1].m = nullptr;
    let p = &s;
    printf("[05] %u %u\n", s[0].a, s[1].a); fflush(nullptr);
}
