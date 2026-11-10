// standard/1/c/time.os
// Tier 0: raw time bindings.
// PLATFORM: Windows (kernel32 + UCRT). POSIX equivalents: clock_gettime, nanosleep, gettimeofday.

extern "C" fn _time64(t: i64*) => i64;                                  // seconds since 1970-01-01 UTC
extern "C" fn Sleep(ms: uint) => void;
extern "C" fn GetTickCount64() => u64;                                  // ms since boot
extern "C" fn QueryPerformanceCounter(out: i64*) => int;
extern "C" fn QueryPerformanceFrequency(out: i64*) => int;
extern "C" fn GetSystemTimePreciseAsFileTime(out: u64*) => void;        // 100 ns ticks since 1601-01-01 UTC
