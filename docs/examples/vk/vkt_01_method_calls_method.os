extern "C" fn printf(fmt: char*, ...) => int;
extern "C" fn fflush(f: void*) => int;
extern "C" fn malloc(size: u64) => void*;
extern "C" fn free(p: void*) => void;

struct A {
    v: uint = 7;
    a() => int { return 1; }
    b() => int {
        if (!this.a()) return 0;
        return this.a();
    }
}
let g: A;
function main() => void {
    printf("[01] start\n"); fflush(nullptr);
    printf("[01] b=%d\n", g.b()); fflush(nullptr);
}
