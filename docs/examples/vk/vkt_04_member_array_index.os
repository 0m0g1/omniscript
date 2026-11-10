extern "C" fn printf(fmt: char*, ...) => int;
extern "C" fn fflush(f: void*) => int;
extern "C" fn malloc(size: u64) => void*;
extern "C" fn free(p: void*) => void;

struct A {
    arr: void** = nullptr;
    go() => int {
        this.arr = malloc(32 as u64) as void**;
        this.arr[1] = nullptr;
        let q: void** = &this.arr[1];
        free(this.arr as void*);
        return 1;
    }
}
let g: A;
function main() => void {
    printf("[04] start\n"); fflush(nullptr);
    printf("[04] go=%d\n", g.go()); fflush(nullptr);
}
