// standard/1/c/thread.os  (PLATFORM: Win32)
extern "C" fn CreateThread(attr: void*, stack: u64, entry: void*, arg: void*, flags: u32, id: u32*) => void*;
extern "C" fn WaitForSingleObject(h: void*, ms: u32) => u32;
extern "C" fn CloseHandle(h: void*) => int;
extern "C" fn Sleep(ms: u32) => void;
extern "C" fn GetCurrentThreadId() => u32;
extern "C" fn InitializeCriticalSection(cs: void*) => void;
extern "C" fn EnterCriticalSection(cs: void*) => void;
extern "C" fn TryEnterCriticalSection(cs: void*) => int;
extern "C" fn LeaveCriticalSection(cs: void*) => void;
extern "C" fn DeleteCriticalSection(cs: void*) => void;
extern "C" fn InitializeConditionVariable(cv: void*) => void;
extern "C" fn SleepConditionVariableCS(cv: void*, cs: void*, ms: u32) => int;
extern "C" fn WakeConditionVariable(cv: void*) => void;
extern "C" fn WakeAllConditionVariable(cv: void*) => void;
extern "C" fn SwitchToThread() => int;
extern "C" fn GetActiveProcessorCount(group: u16) => u32;

// Atomics are LLVM instructions, not kernel32 exports (Interlocked*64 are MSVC intrinsics).
// Codegen defines these inline (seq_cst). add/xchg/cmpxchg return the OLD value.
intrinsic fn atomic_add_i64(p: i64*, v: i64) => i64;
intrinsic fn atomic_xchg_i64(p: i64*, v: i64) => i64;
intrinsic fn atomic_cmpxchg_i64(p: i64*, expected: i64, desired: i64) => i64;
intrinsic fn atomic_load_i64(p: i64*) => i64;
intrinsic fn atomic_store_i64(p: i64*, v: i64) => void;
