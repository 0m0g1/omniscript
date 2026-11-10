// SHOWCASE 1a: the bug C lets you ship.
// In C this compiles and crashes at runtime if the file is missing.
// In OmniScript, fopen returns a nullable pointer, so this should NOT compile.
//
// Run:  Osengine.exe docs/examples/showcase/01_null_safety_bad.os --execute
// Expected: compile error saying the result of fopen must be checked before use.
extern "C" {
    fn printf(fmt: char*, ...) => int;
    fn fopen(path: char*, mode: char*) => void*;
    fn fgetc(f: void*) => int;
}

let f = fopen("does_not_exist.txt", "r");
let c = fgetc(f);                  // <- error: f may be null
printf("first byte: %d\n", c);
