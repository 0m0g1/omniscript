// volatile let n: int32 = 0;

// if (n == 0) {

// }

// extern "kernel32.dll" {
//     fn QueryPerformanceCounter(counter: int64*) => int;
//     fn QueryPerformanceFrequency(freq: int64*) => int;
// }

// extern "C" fn printf(...fmt: char*) => int;

// let freq: int64 = 0;
// QueryPerformanceFrequency(&freq);

// // Warmup run (optional but recommended)
// // let warmup: int64 = 0;
// // for (let i = 0; i < 1000000; i++) {
// //     warmup += i;
// // }

// // Actual test
// let start: int64 = 0;
// let end: int64 = 0;
// volatile let x: int64 = 0;  // Use int64 to prevent overflow

// QueryPerformanceCounter(&start);

// for (let i: int64 = 0; i < 1000000000; i++) {
//     // x += i as int64;
//     // Todo::fix this
//     // if (x == -1) {
//     //     printf("unlikely");
//     // }
// }

// QueryPerformanceCounter(&end);

// // Results
// let elapsedMs = (end - start) as double * 1000.0 / freq;
// printf("Result: %lld\n", x);  // lld for int64
// printf("Elapsed: %.4f ms\n", elapsedMs);
// printf("Ops/ms: %.1f\n", 1000000.0 / elapsedMs);  // Additional metric