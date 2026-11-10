// standard/1/Random.os
// Tier 1: seedable pseudo-random numbers (64-bit LCG, high bits only). Fast and fine for games and tests.
// NOT cryptographically secure; use std/crypto randomBytes (Tier 6) for secrets.
// Same seed -> same sequence, on every platform.
//
// Written with only + * / & so it does not depend on shift/xor operators existing yet.
//
//   let r: Random;
//   r.seed(42);                    // or r.seedFromTime();
//   let n = r.range(1, 7);         // 1..6
//   let f = r.nextFloat();         // [0, 1)

module Core = import "standard/1/Core.os";
module Time = import "standard/1/Time.os";

struct Random {
    state: u64 = 0x853C49E6748FEA9B;

    seed(n: u64) => void {
        this.state = n + 1442695040888963407;
        this.nextU32();
        this.nextU32();
    }

    seedFromTime() => void { this.seed(Time.monotonicNs() + (Time.unixMs() as u64)); }

    // Uniform 0 .. 2^32-1
    nextU32() => uint {
        this.state = this.state * 6364136223846793005 + 1442695040888963407;
        return (this.state / 4294967296) as uint;
    }

    nextU64() => u64 {
        let hi = this.nextU32() as u64;
        let lo = this.nextU32() as u64;
        return hi * 4294967296 + lo;
    }

    // Uniform 0 .. 2^31-1
    nextInt() => i64 { return (this.nextU32() as i64) / 2; }

    // Uniform in [a, b). Returns a if b <= a.
    range(a: i64, b: i64) => i64 {
        if (b <= a) return a;
        let span: u64 = (b - a) as u64;
        let x: u64 = this.nextU64();
        let r: u64 = x - (x / span) * span;      // x % span (tiny modulo bias for huge spans)
        return a + (r as i64);
    }

    // Uniform double in [0, 1), 53 bits of resolution
    nextFloat() => double {
        let bits: u64 = this.nextU64() / 2048;
        return (bits as double) / 9007199254740992.0;
    }

    rangeFloat(a: double, b: double) => double { return a + (b - a) * this.nextFloat(); }

    nextBool() => bool { return this.nextU32() >= 2147483648; }

    // Fisher-Yates over any array: base = pointer to element 0, elemSize = sizeof(element).
    //   r.shuffle(arr as void*, count, sizeof(i32));
    shuffle(base: void*, count: u64, elemSize: u64) => void {
        if (count < 2) return;
        let p = base as char*;
        let i: u64 = count - 1;
        while (i > 0) {
            let j: u64 = this.range(0, (i + 1) as i64) as u64;
            if (j != i) Core.swapMem(&p[i * elemSize] as void*, &p[j * elemSize] as void*, elemSize);
            i = i - 1;
        }
    }
}
