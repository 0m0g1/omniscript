extern "C" fn printf(fmt: char*, ...) => int;
extern "C" fn fflush(f: void*) => int;
extern "C" fn malloc(size: u64) => void*;
extern "C" fn free(p: void*) => void;

function main() => void {
    printf("[10] start\n"); fflush(nullptr);
    let found: int = 0;
    let i: uint = 0;
    while (i < 5 && !found) {
        if (i == 3) { found = 1; }
        i = i + 1;
    }
    let r: int = 2;
    if (r == 1) { r = 5; } else if (r != 2) { r = 6; } else { r = 7; }
    let neg: int = -1;
    printf("[10] i=%u r=%d neg=%d\n", i, r, neg); fflush(nullptr);
}
