extern "C" fn printf(fmt: char*, ...) => int;
extern "C" fn fflush(f: void*) => int;
extern "C" fn malloc(size: u64) => void*;
extern "C" fn free(p: void*) => void;

function main() => void {
    printf("[06] start\n"); fflush(nullptr);
    let d: uint[2] = [1, 2];
    let c: float[4] = [0.0, 0.0, 0.0, 1.0];
    let pd = &d;
    let pc = &c;
    printf("[06] %u %u\n", d[0], d[1]); fflush(nullptr);
}
