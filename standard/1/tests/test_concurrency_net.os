// standard/1/tests/test_concurrency_net.os  (Tier 5 + Db)
// Net/Http tests use localhost only, so no internet is needed. Written against the real struct/out-parameter API.
module H = import "standard/1/tests/Harness.os";
module Th = import "standard/1/Thread.os";
module Ch = import "standard/1/Channel.os";
module Net = import "standard/1/Net.os";
module Http = import "standard/1/Http.os";
module Hs = import "standard/1/HttpServer.os";
module Text = import "standard/1/Text.os";
module D = import "standard/1/Db.os";   // alias is D (not Db): the module and its struct are both called Db
module Str = import "standard/1/String.os";

// ---------- Thread + Mutex ----------
struct BumpCtx { m: Th.Mutex; n: int = 0; }

function bump(p: void*) => u32 {
    let c: BumpCtx* = p as BumpCtx*;
    let i: int = 0;
    while (i < 10000) { c.m.lock(); c.n = c.n + 1; c.m.unlock(); i = i + 1; }
    return 0;
}

// ---------- Channel ----------
struct ProdCtx { ch: Ch.Channel<int>*; count: int = 0; }

function producer(p: void*) => u32 {
    let c: ProdCtx* = p as ProdCtx*;
    let i: int = 0;
    while (i < c.count) { c.ch.send(i); i = i + 1; }
    c.ch.close();
    return 0;
}

// ---------- ThreadPool ----------
function bumpAtomic(p: void*) => void {
    let a: Th.Atomic* = p as Th.Atomic*;
    a.add(1);
}

// ---------- Net ----------
function echoOnce(p: void*) => u32 {
    let l: Net.Socket* = p as Net.Socket*;
    let c: Net.Socket;
    if (Net.acceptTcp(l, &c)) {
        let buf: [16]u8;
        let got: u64 = 0;
        let going: bool = true;
        while (going && got < 5) {
            let r: i64 = c.recv(&buf[got], 5 - got);
            if (r <= 0) going = false; else got = got + (r as u64);
        }
        if (got == 5) c.sendAll(&buf as u8*, 5);
        c.close();
    }
    return 0;
}

// ---------- Http (server API: Hs.HttpServer, Hs.Request, Hs.Response) ----------
function pingHandler(rq: Hs.Request*, rp: Hs.Response*) => void { rp.text("pong"); }
function echoHandler(rq: Hs.Request*, rp: Hs.Response*) => void { rp.text(rq.body.cstr()); }
function userHandler(rq: Hs.Request*, rp: Hs.Response*) => void { rp.text(rq.param("id")); }
function filesHandler(rq: Hs.Request*, rp: Hs.Response*) => void { rp.text(rq.rest.cstr()); }
function nameHandler(rq: Hs.Request*, rp: Hs.Response*) => void {
    let v: Str.String;
    if (rq.queryParam("name", &v)) rp.text(v.cstr());
    else { rp.status = 400; rp.text("missing name"); }
    v.free();
}
function serveThread(p: void*) => u32 {
    let s: Hs.HttpServer* = p as Hs.HttpServer*;
    s.serve();
    return 0;
}

// ---------- Db ----------
function insertD(p: void*) => bool {
    let d: D.Db* = p as D.Db*;
    return d.exec("INSERT INTO t(name) VALUES ('d')");
}
function insertThenFail(p: void*) => bool {
    let d: D.Db* = p as D.Db*;
    d.exec("INSERT INTO t(name) VALUES ('e')");
    return false;
}

function rowCount(d: D.Db*) => int {
    let n: i64 = 0;
    if (!d.queryInt("SELECT COUNT(*) FROM t", &n)) return 0 - 1;
    return n as int;
}

// ---------- raw HTTP over a socket (tests the server without libcurl) ----------
// Sends req, reads until the server closes the connection. false if connect/send failed.
function rawHttp(port: char*, req: char*, out: Str.String*) => bool {
    let sock: Net.Socket;
    if (!Net.connectTcp("127.0.0.1", port, &sock)) return false;
    let ok: bool = sock.sendStr(req);
    let going: bool = ok;
    while (going) {
        let r: i64 = sock.recvInto(out, 4096);
        if (r <= 0) going = false;
    }
    sock.close();
    return ok;
}

// "HTTP/1.1 200 OK": the three status digits start at offset 9.
function statusOf(raw: Str.String*) => int {
    if (raw.len < 12) return 0;
    let n: int = 0;
    let i: u64 = 9;
    while (i < 12) { n = n * 10 + ((raw.data[i] as int) - 48); i = i + 1; }
    return n;
}

function bodyOf(raw: Str.String*, out: Str.String*) => bool {
    let p: i64 = raw.indexOf("\r\n\r\n");
    if (p < 0) return false;
    return raw.substring((p + 4) as u64, raw.len, out);
}

// One request against the loopback server: checks the status, and the body when wantBody is not nullptr.
function httpCase(req: char*, wantStatus: int, wantBody: char*, name: char*) => void {
    let raw: Str.String;
    let body: Str.String;
    if (!rawHttp("47124", req, &raw)) {
        H.check(false, name);
    } else {
        H.eqInt(statusOf(&raw), wantStatus, name);
        if (wantBody != nullptr) {
            bodyOf(&raw, &body);
            H.eqStr(body.cstr(), wantBody, name);
        }
    }
    raw.free();
    body.free();
}

function main() => int {
    H.suite("Thread + Mutex");
    let ctx: BumpCtx;
    ctx.m.init();
    let t1: Th.Thread; let t2: Th.Thread; let t3: Th.Thread;
    H.check(t1.spawn(bump, &ctx as void*), "spawn 1");
    H.check(t2.spawn(bump, &ctx as void*), "spawn 2");
    H.check(t3.spawn(bump, &ctx as void*), "spawn 3");
    H.check(t1.join(), "join 1");
    H.check(t2.join(), "join 2");
    H.check(t3.join(), "join 3");
    H.eqInt(ctx.n, 30000, "mutex protects counter (3 threads x 10000)");
    H.check(ctx.m.tryLock(), "tryLock free mutex");
    ctx.m.unlock();
    ctx.m.free();

    H.suite("Atomic");
    let at: Th.Atomic;
    at.add(5); at.add(5);
    H.eqInt(at.load() as int, 10, "atomic add");
    let three: i64 = 3;
    H.eqInt(at.add(0 - three) as int, 7, "add returns new value");
    at.store(10);
    H.check(at.compareExchange(10, 99), "CAS success");
    H.eqInt(at.load() as int, 99, "CAS stored value");
    H.check(!at.compareExchange(10, 1), "CAS fail");
    H.eqInt(at.load() as int, 99, "failed CAS leaves value");

    H.suite("Channel");
    let ch: Ch.Channel<int>;
    ch.init(0);
    let pc: ProdCtx;
    pc.ch = &ch; pc.count = 100;
    let p: Th.Thread;
    H.check(p.spawn(producer, &pc as void*), "spawn producer");
    let sum: int = 0;
    let got: int = 0;
    let v: int = 0;
    while (ch.recv(&v)) { sum = sum + v; got = got + 1; }
    p.join();
    H.eqInt(got, 100, "received all");
    H.eqInt(sum, 4950, "sum 0..99");
    H.check(!ch.recv(&v), "recv on closed+drained returns false");
    H.check(!ch.send(1), "send on closed returns false");
    ch.free();

    H.suite("Channel (bounded, backpressure)");
    let bch: Ch.Channel<int>;
    bch.init(4);
    let bpc: ProdCtx;
    bpc.ch = &bch; bpc.count = 1000;
    let bp: Th.Thread;
    bp.spawn(producer, &bpc as void*);
    let bsum: int = 0;
    let bgot: int = 0;
    while (bch.recv(&v)) { bsum = bsum + v; bgot = bgot + 1; }
    bp.join();
    H.eqInt(bgot, 1000, "bounded: received all");
    H.eqInt(bsum, 499500, "bounded: sum 0..999");
    bch.free();

    H.suite("Channel (tryRecv)");
    let tch: Ch.Channel<int>;
    tch.init(0);
    H.check(!tch.tryRecv(&v), "tryRecv on empty returns false");
    tch.send(42);
    H.check(tch.tryRecv(&v) && v == 42, "tryRecv gets queued value");
    tch.close();
    tch.free();

    H.suite("ThreadPool");
    let pool: Ch.ThreadPool;
    H.check(pool.start(4), "pool start");
    let done: Th.Atomic;
    let i: int = 0;
    while (i < 50) { pool.submit(bumpAtomic, &done as void*); i = i + 1; }
    pool.wait();
    H.eqInt(done.load() as int, 50, "pool ran 50 jobs");
    pool.submit(bumpAtomic, &done as void*);
    pool.shutdown();
    H.eqInt(done.load() as int, 51, "shutdown drains queued jobs");

    H.suite("Net (TCP loopback)");
    let lis: Net.Socket;
    H.check(Net.listenTcp(47123 as u16, 4, &lis), "listen");
    let srv: Th.Thread;
    srv.spawn(echoOnce, &lis as void*);
    let cl: Net.Socket;
    H.check(Net.connectTcp("127.0.0.1", "47123", &cl), "connect");
    H.check(cl.sendStr("hello"), "send");
    let echoed: Str.String;
    let spins: int = 0;
    while (echoed.len < 5 && spins < 100) {
        let r: i64 = cl.recvInto(&echoed, 16);
        if (r <= 0) spins = 100;
        spins = spins + 1;
    }
    H.eqStr(echoed.cstr(), "hello", "echo roundtrip");
    echoed.free();
    cl.close();
    srv.join();
    lis.close();
    let bad: Net.Socket;
    H.check(!Net.connectTcp("127.0.0.1", "1", &bad), "refused connection returns false");

    H.suite("Net (UDP loopback)");
    let ua: Net.UdpSocket; let ub: Net.UdpSocket;
    H.check(ua.open(), "udp open a");
    H.check(ua.bindPort(47125 as u16), "udp bind");
    H.check(ub.open(), "udp open b");
    let ping: char* = "ping";
    H.eqInt(ub.sendTo("127.0.0.1", 47125 as u16, ping as u8*, 4) as int, 4, "udp send");
    let ubuf: [16]u8;
    let fromIp: [16]char;
    let fromPort: u16 = 0;
    H.eqInt(ua.recvFrom(&ubuf as u8*, 16, &fromIp as char*, &fromPort) as int, 4, "udp recv length");
    H.check(ubuf[0] == 112 && ubuf[3] == 103, "udp payload");
    H.eqStr(&fromIp as char*, "127.0.0.1", "udp source ip");
    ua.close(); ub.close();

    H.suite("HttpServer (raw sockets)");
    let hs: Hs.HttpServer;
    hs.get("/ping", pingHandler);
    hs.post("/echo", echoHandler);
    hs.get("/users/:id", userHandler);
    hs.get("/files/*", filesHandler);
    hs.get("/hello", nameHandler);
    H.check(hs.bind(47124 as u16), "server bound");
    let hst: Th.Thread;
    H.check(hst.spawn(serveThread, &hs as void*), "server thread");

    httpCase("GET /ping HTTP/1.1\r\nHost: t\r\nConnection: close\r\n\r\n", 200, "pong", "GET /ping -> 200 pong");
    httpCase("GET /nope HTTP/1.1\r\nHost: t\r\n\r\n", 404, nullptr, "unknown route -> 404");
    httpCase("POST /echo HTTP/1.1\r\nHost: t\r\nContent-Length: 12\r\n\r\nsome payload", 200, "some payload", "POST body echoed");
    httpCase("POST /echo HTTP/1.1\r\nHost: t\r\ncontent-length: 2\r\n\r\nok", 200, "ok", "header names are case-insensitive");
    httpCase("GET /echo HTTP/1.1\r\nHost: t\r\n\r\n", 405, nullptr, "known path, wrong method -> 405");
    httpCase("GET /users/42 HTTP/1.1\r\nHost: t\r\n\r\n", 200, "42", "path parameter :id");
    httpCase("GET /files/a/b.txt HTTP/1.1\r\nHost: t\r\n\r\n", 200, "a/b.txt", "wildcard remainder");
    httpCase("GET /hello?name=a%20b%26c HTTP/1.1\r\nHost: t\r\n\r\n", 200, "a b&c", "query param percent-decoded");
    httpCase("GET /hello HTTP/1.1\r\nHost: t\r\n\r\n", 400, "missing name", "handler can answer 400");
    httpCase("GET /ping?x=1 HTTP/1.1\r\nHost: t\r\n\r\n", 200, "pong", "query string ignored for routing");
    httpCase("HEAD /ping HTTP/1.1\r\nHost: t\r\n\r\n", 200, "", "HEAD uses the GET route and drops the body");
    httpCase("BADREQUEST\r\n\r\n", 400, nullptr, "malformed request line -> 400");
    httpCase("POST /echo HTTP/1.1\r\nHost: t\r\nTransfer-Encoding: chunked\r\n\r\n", 501, nullptr, "chunked upload -> 501");

    let big: Str.String;
    Text.repeat("a", 100000, &big);
    let bigReq: Str.String;
    bigReq.append("POST /echo HTTP/1.1\r\nHost: t\r\nContent-Length: 100000\r\n\r\n");
    bigReq.appendString(&big);
    let bigRaw: Str.String;
    let bigBody: Str.String;
    H.check(rawHttp("47124", bigReq.cstr(), &bigRaw), "large POST sent");
    H.eqInt(statusOf(&bigRaw), 200, "large POST status");
    bodyOf(&bigRaw, &bigBody);
    H.eqInt(bigBody.length() as int, 100000, "large body echoed in full (body framing)");
    big.free(); bigReq.free(); bigRaw.free(); bigBody.free();

    H.suite("Http client (libcurl -> loopback server)");
    let r1: Http.Response;
    H.check(Http.get("http://127.0.0.1:47124/ping", &r1), "GET ok");
    H.eqInt(r1.status as int, 200, "client: status 200");
    H.eqStr(r1.body.cstr(), "pong", "client: body");
    r1.free();
    let r2: Http.Response;
    H.check(Http.get("http://127.0.0.1:47124/nope", &r2), "GET unknown route completes");
    H.eqInt(r2.status as int, 404, "client: unknown route 404");
    r2.free();
    let r3: Http.Response;
    H.check(Http.post("http://127.0.0.1:47124/echo", "some payload", "text/plain", &r3), "POST ok");
    H.eqInt(r3.status as int, 200, "client: POST status");
    H.eqStr(r3.body.cstr(), "some payload", "client: POST body echoed");
    r3.free();

    hs.stop();
    hst.join();
    hs.free();

    H.suite("Db (SQLite)");
    let db: D.Db;
    H.check(db.open(":memory:"), "open");
    H.check(db.exec("CREATE TABLE t(id INTEGER PRIMARY KEY, name TEXT)"), "create table");
    H.check(db.exec("INSERT INTO t(name) VALUES ('a'),('b'),('c')"), "insert rows");
    H.eqInt(rowCount(&db), 3, "query rows");

    let st: D.Stmt;
    H.check(db.prepare("SELECT name FROM t WHERE id = ?", &st), "prepare");
    H.check(st.bindInt(1, 2), "bind");
    H.check(st.step(), "step row");
    H.eqStr(st.colText(0), "b", "parameterised query");
    H.check(!st.step(), "no second row");
    H.check(st.reset(), "reset");
    H.check(st.bindInt(1, 3), "rebind");
    H.check(st.step(), "step again");
    H.eqStr(st.colText(0), "c", "reused statement");
    st.finalize();

    let nm: Str.String;
    H.check(db.queryText("SELECT name FROM t WHERE id = 1", &nm), "queryText");
    H.eqStr(nm.cstr(), "a", "queryText value");
    nm.free();

    H.check(db.transaction(insertD, &db as void*), "transaction commit");
    H.eqInt(rowCount(&db), 4, "committed row visible");
    H.check(!db.transaction(insertThenFail, &db as void*), "failing transaction reports false");
    H.eqInt(rowCount(&db), 4, "rolled back row not visible");
    H.check(!db.exec("THIS IS NOT SQL"), "bad SQL reports failure, no crash");
    H.check(db.exec("SELECT 1"), "db usable after bad SQL");
    db.close();

    return H.finish();
}
