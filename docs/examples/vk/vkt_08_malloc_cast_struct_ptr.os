extern "C" fn printf(fmt: char*, ...) => int;
extern "C" fn fflush(f: void*) => int;
extern "C" fn malloc(size: u64) => void*;
extern "C" fn free(p: void*) => void;

struct P {
    flags: uint = 0;
    count: uint = 0;
    ext: uint[3];
}
function main() => void {
    printf("[08] start\n"); fflush(nullptr);
    let props = malloc(96 as u64) as P*;
    props[1].flags = 5;
    printf("[08] %u\n", props[1].flags); fflush(nullptr);
    free(props as void*);
}
