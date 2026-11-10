extern "C" fn printf(fmt: char*, ...) => int;
extern "C" fn fflush(f: void*) => int;
extern "C" fn malloc(size: u64) => void*;
extern "C" fn free(p: void*) => void;

function main() => void {
    printf("[07] start\n"); fflush(nullptr);
    let w: int, h: int;
    w = 3; h = 4;
    printf("[07] %d %d\n", w, h); fflush(nullptr);
}
