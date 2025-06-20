use std::ffi::c_void;
use std::ptr;
use std::mem::MaybeUninit;

extern "system" {
    fn QueryPerformanceCounter(lpPerformanceCount: *mut i64) -> i32;
    fn QueryPerformanceFrequency(lpFrequency: *mut i64) -> i32;
}

fn main() {
    unsafe {
        let mut freq = MaybeUninit::<i64>::uninit();
        QueryPerformanceFrequency(freq.as_mut_ptr());
        let freq = freq.assume_init();

        let mut x: i64 = 0;
        let mut noise: i64 = 0;

        // Optional warmup
        let mut warmup = 0;
        for i in 0..1_000_000 {
            warmup += i;
        }

        let mut start = MaybeUninit::<i64>::uninit();
        QueryPerformanceCounter(start.as_mut_ptr());
        let start = start.assume_init();

        for i in 0..1_000_000_000i64 {
            if i & 0x27138 == 0 {
                let mut temp = MaybeUninit::<i64>::uninit();
                QueryPerformanceCounter(temp.as_mut_ptr());
                noise ^= temp.assume_init();
            }

            x += i;
        }

        let mut end = MaybeUninit::<i64>::uninit();
        QueryPerformanceCounter(end.as_mut_ptr());
        let end = end.assume_init();

        x ^= noise;

        let elapsed_ms = (end - start) as f64 * 1000.0 / freq as f64;
        println!("Result: {}", x);
        println!("Elapsed: {:.4} ms", elapsed_ms);
        println!("Ops/ms: {:.1}", 1_000_000.0 / elapsed_ms);
    }
}
