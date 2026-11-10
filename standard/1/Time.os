// standard/1/Time.os
// Tier 1: clocks, sleep, Stopwatch. (Duration and DateTime come later; DateTime is Tier 6.)
//   monotonicNs()  never goes backwards; use it for measuring. Not related to the calendar.
//   unixMs()       wall clock, ms since 1970-01-01 UTC; use it for timestamps.

module CTime = import "standard/1/c/time.os";

let g_qpcFreq: i64 = 0;

function qpcFreq() => i64 {
    if (g_qpcFreq == 0) {
        let f: i64 = 0;
        CTime.QueryPerformanceFrequency(&f);
        g_qpcFreq = f;
    }
    return g_qpcFreq;
}

// Nanoseconds from an arbitrary fixed starting point. Splits the division so it cannot overflow.
function monotonicNs() => u64 {
    let c: i64 = 0;
    CTime.QueryPerformanceCounter(&c);
    let f = qpcFreq();
    let secs: i64 = c / f;
    let rem: i64 = c - secs * f;
    return (secs * 1000000000 + (rem * 1000000000) / f) as u64;
}

function monotonicMs() => u64 { return monotonicNs() / 1000000; }

function unixSeconds() => i64 { return CTime._time64(nullptr); }

function unixMs() => i64 {
    let t: u64 = 0;
    CTime.GetSystemTimePreciseAsFileTime(&t);
    return ((t - 116444736000000000) / 10000) as i64;     // FILETIME epoch (1601) -> Unix epoch (1970)
}

function sleep(ms: u64) => void { CTime.Sleep(ms as uint); }

struct Stopwatch {
    startedAt: u64 = 0;

    start() => void { this.startedAt = monotonicNs(); }

    elapsedNs() => u64 { return monotonicNs() - this.startedAt; }
    elapsedMs() => double { return (this.elapsedNs() as double) / 1000000.0; }
    elapsedSeconds() => double { return (this.elapsedNs() as double) / 1000000000.0; }

    // Returns the elapsed ms and starts over.
    restart() => double {
        let e = this.elapsedMs();
        this.start();
        return e;
    }
}
