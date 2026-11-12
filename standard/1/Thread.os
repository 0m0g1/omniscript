// standard/1/Thread.os
// Tier 5: threads, mutex, condition variable. (PLATFORM: Win32)
// Thread entry: function(arg: void*) => u32 ... passed as fn pointer.
module CT = import "standard/1/c/thread.os";
module CMem = import "standard/1/c/mem.os";

struct Thread {
    h: void* = nullptr;
    spawn(entry: fn(void*) => u32, arg: void*) => bool {
        let id: u32 = 0;
        this.h = CT.CreateThread(nullptr, 0, entry as void*, arg, 0, &id);
        return this.h != nullptr;
    }
    join() => bool {
        if (this.h == nullptr) return false;
        CT.WaitForSingleObject(this.h, 0xFFFFFFFF);
        CT.CloseHandle(this.h);
        this.h = nullptr;
        return true;
    }
}

function sleepMs(ms: u32) => void { CT.Sleep(ms); }
function currentId() => u32 { return CT.GetCurrentThreadId(); }

// CRITICAL_SECTION is 40 bytes on x64
struct Mutex {
    cs: [40]u8;
    init() => void { CT.InitializeCriticalSection(&this.cs as void*); }
    lock() => void { CT.EnterCriticalSection(&this.cs as void*); }
    tryLock() => bool { return CT.TryEnterCriticalSection(&this.cs as void*) != 0; }
    unlock() => void { CT.LeaveCriticalSection(&this.cs as void*); }
    free() => void { CT.DeleteCriticalSection(&this.cs as void*); }
}

struct Cond {
    cv: [8]u8;   // CONDITION_VARIABLE is pointer-sized
    init() => void { CT.InitializeConditionVariable(&this.cv as void*); }
    wait(m: Mutex*) => void { CT.SleepConditionVariableCS(&this.cv as void*, &m.cs as void*, 0xFFFFFFFF); }
    waitMs(m: Mutex*, ms: u32) => bool { return CT.SleepConditionVariableCS(&this.cv as void*, &m.cs as void*, ms) != 0; }
    signal() => void { CT.WakeConditionVariable(&this.cv as void*); }
    broadcast() => void { CT.WakeAllConditionVariable(&this.cv as void*); }
}

// Run fn(arg) on a new thread, fire and forget (handle closed on join()).
function spawn(entry: fn(void*) => u32, arg: void*, out: Thread*) => bool { return out.spawn(entry, arg); }

function yieldNow() => void { CT.SwitchToThread(); }
function cpuCount() => u32 { return CT.GetActiveProcessorCount(0xFFFF); }

// 64-bit atomic integer (counters/flags), backed by LLVM atomic instructions (seq_cst).
struct Atomic {
    v: i64 = 0;
    load() => i64 { return CT.atomic_load_i64(&this.v); }
    store(x: i64) => void { CT.atomic_store_i64(&this.v, x); }
    add(x: i64) => i64 { return CT.atomic_add_i64(&this.v, x) + x; }   // returns new value
    compareExchange(expected: i64, desired: i64) => bool {
        return CT.atomic_cmpxchg_i64(&this.v, expected, desired) == expected;   // (p, expected, desired)
    }
}
