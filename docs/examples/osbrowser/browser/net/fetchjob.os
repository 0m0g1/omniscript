// browser/net/fetchjob.os -- async HTTP for fetch(). Worker thread per job; finished jobs come back through a Channel
// and are drained on the main thread once per frame (eventloop.os). The window never blocks on the network.
// Needs: Thread.spawn(entry: fn(void*) => u32, arg), Channel<T>.init/send/tryRecv, Http.requestEx.
// ADAPT: if the generic spelling differs, only `done` and the two Channel calls change.
module Th = import "standard/1/Thread.os";
module CHN = import "standard/1/Channel.os";
module Http = import "standard/1/Http.os";
module U = import "browser/util.os";
module Dbg = import "browser/dbg.os";

// state: 0 free, 1 running, 2 finished (not yet drained)
struct FetchJob {
    id: int = 0;
    state: int = 0;
    method: char* = nullptr;
    url: char* = nullptr;
    body: char* = nullptr;
    bodyLen: i64 = 0 - 1;
    headers: [17]char*;          // nullptr-terminated "Name: value"
    ok: bool = false;            // transport ok (status may still be 4xx/5xx)
    cancelled: bool = false;
    startMs: double = 0.0;
    ms: double = 0.0;
    resp: Http.Response;
    user: void* = nullptr;       // the JS Promise handle, owned by the caller
}

let done: CHN.Channel<FetchJob*>;
let ready: bool = false;
let nextId: int = 1;
let inflight: int = 0;

function init() => void {
    if (ready) return;
    done.init(256 as u64);
    ready = true;
}

function worker(p: void*) => u32 {
    let j: FetchJob* = p as FetchJob*;
    j.ok = Http.requestEx(j.method, j.url, j.body, j.bodyLen, &j.headers as char**, &j.resp);
    j.state = 2;
    done.send(j);
    return 0 as u32;
}

// Allocate a job. Caller fills method/url/body/headers then calls submit().
function create(method: char*, url: char*) => FetchJob* {
    init();
    let j: FetchJob* = U.alloc(sizeof(FetchJob)) as FetchJob*;
    j.id = nextId; nextId = nextId + 1;
    j.method = U.dup(method);
    j.url = U.dup(url);
    return j;
}

function addHeader(j: FetchJob*, line: char*) => bool {
    let i: int = 0;
    while (i < 16 && j.headers[i] != nullptr) { i = i + 1; }
    if (i >= 16) return false;
    j.headers[i] = U.dup(line);
    return true;
}

function submit(j: FetchJob*, nowMs: double) => bool {
    j.startMs = nowMs;
    j.state = 1;
    let t: Th.Thread;
    if (!Th.spawnThread(worker, j as void*, &t)) { j.state = 2; j.ok = false; j.resp.error = "cannot start thread"; done.send(j); return false; }
    inflight = inflight + 1;
    Dbg.log(U.cat(U.cat(j.method, " "), j.url));
    return true;
}

// Non-blocking. Returns the next finished job or nullptr. Call every frame.
function poll(nowMs: double) => FetchJob* {
    if (!ready) return nullptr;
    let j: FetchJob* = nullptr;
    if (!done.tryRecv(&j)) return nullptr;
    j.ms = nowMs - j.startMs;
    inflight = inflight - 1;
    return j;
}

function pending() => int { return inflight; }
