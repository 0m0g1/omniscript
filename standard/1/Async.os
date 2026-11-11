// standard/1/Async.os  -- STUB (not implemented yet)
// Tier 7: async/await, futures, executor. Needs compiler support (coroutines/state machines).
// All functions return false / do nothing until implemented.
struct Future<T> { done: bool = false; value: T; poll() => bool { return this.done; } }
// TODO: Executor (run queue + Thread pool), spawn<T>(fn) => Future<T>, await, timers (sleep), join_all.
function block_on<T>(f: Future<T>*) => bool { return false; }  // TODO
