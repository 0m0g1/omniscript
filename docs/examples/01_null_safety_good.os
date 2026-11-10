// SHOWCASE 1b: same program, with the check the compiler demanded.
//
// Run:  Osengine.exe docs/examples/showcase/01_null_safety_good.os --execute
// Expected output: could not open file
extern "C" {
    fn printf(fmt: char*, ...) => int;
    fn fopen(path: char*, mode: char*) => void*;
    fn fclose(f: void*) => int;
    fn fgetc(f: void*) => int;
}

let f = fopen("does_not_exist.txt", "r");
if (f == nullptr) {
    printf("could not open file\n");
} else {
    printf("first byte: %d\n", fgetc(f));   // f is usable here
    fclose(f);
}
