extern "C" fn printf(fmt: char*, ...) => int;
extern "C" fn fflush(f: void*) => int;
extern "C" fn malloc(size: u64) => void*;
extern "C" fn free(p: void*) => void;

struct A {
    p: void* = nullptr;
    n: uint = 3;
    go() => int {
        let q: void** = &this.p;
        let r: uint* = &this.n;
        return 1;
    }
}
let g: A;
function main() => void {
    printf("[03] start\n"); fflush(nullptr);
    printf("[03] go=%d\n", g.go()); fflush(nullptr);
}
