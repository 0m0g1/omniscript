// SHOWCASE 3: script-style top-down code, C-level memory control, then a native .exe.
// No main(), no headers, no build file.
//
// Run:  Osengine.exe docs/examples/showcase/03_script_speed.os --execute
// Expected output: sum of squares 0..999 = 332833500
extern "C" {
    fn printf(fmt: char*, ...) => int;
    fn malloc(size: u64) => void*;
    fn free(p: void*) => void;
}

let n: i32 = 1000;
let buf = malloc((n * sizeof(i64)) as u64) as i64*;
if (buf == nullptr) { return 1; }

for (let i: i32 = 0; i < n; i++) {
    buf[i] = (i as i64) * (i as i64);
}

let total: i64 = 0;
for (let i: i32 = 0; i < n; i++) {
    total += buf[i];
}

printf("sum of squares 0..%d = %lld\n", n - 1, total);
free(buf as void*);
