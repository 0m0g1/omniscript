extern "C" fn printf(fmt: char*, ...) => int;
extern "C" fn fflush(f: void*) => int;
extern "C" fn malloc(size: u64) => void*;
extern "C" fn free(p: void*) => void;

struct A {
    h: void* = nullptr;
    make(path: char*, outModule: void**) => int {
        *outModule = nullptr;
        return 1;
    }
    run() => int {
        let m: void*;
        return this.make("x", &m);
    }
}
let g: A;
function main() => void {
    printf("[02] start\n"); fflush(nullptr);
    printf("[02] run=%d\n", g.run()); fflush(nullptr);
}
