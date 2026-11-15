// standard/1/tests/test_concurrency_net.os  (Tier 5 + Db)
// Net/Http tests use localhost only, so no internet is needed.
module H = import "standard/1/tests/Harness.os";
module T = import "standard/1/Thread.os";
module Ch = import "standard/1/Channel.os";
module Net = import "standard/1/Net.os";
module Http = import "standard/1/Http.os";
module Db = import "standard/1/Db.os";

let counter: int = 0;
let mtx = T.Mutex.new();

function bump() => void {
    let i: int = 0;
    while (i < 10000) { mtx.lock(); counter = counter + 1; mtx.unlock(); i = i + 1; }
}

function producer() => void {
    let i: int = 0;
    while (i < 100) { chan.send(i); i = i + 1; }
    chan.close();
}
let chan = Ch.Channel<int>.new();

function echoServer() => void {
    let l = Net.TcpListener.bind("127.0.0.1:47123");
    let c = l.accept();
    let msg = c.read(5);
    c.write(msg);
    c.close(); l.close();
}

function main() => int {
    H.suite("Thread + Mutex");
    let t1 = T.Thread.spawn(bump);
    let t2 = T.Thread.spawn(bump);
    let t3 = T.Thread.spawn(bump);
    t1.join(); t2.join(); t3.join();
    H.eqInt(counter, 30000, "mutex protects counter (3 threads x 10000)");

    H.suite("Atomic");
    let at = T.Atomic<int>.new(0);
    at.add(5); at.add(5);
    H.eqInt(at.load(), 10, "atomic add");
    H.check(at.compareExchange(10, 99), "CAS success");
    H.check(!at.compareExchange(10, 1), "CAS fail");

    H.suite("Channel");
    let p = T.Thread.spawn(producer);
    let sum: int = 0;
    let got: int = 0;
    let going: bool = true;
    while (going) {
        let v = chan.receive();
        if (v == null) { going = false; }
        else { sum = sum + (v ?? 0); got = got + 1; }
    }
    p.join();
    H.eqInt(got, 100, "received all");
    H.eqInt(sum, 4950, "sum 0..99");

    H.suite("ThreadPool");
    let pool = T.ThreadPool.new(4);
    let done = T.Atomic<int>.new(0);
    let i: int = 0;
    while (i < 50) { pool.submit(function() => void { done.add(1); }); i = i + 1; }
    pool.wait();
    H.eqInt(done.load(), 50, "pool ran 50 jobs");

    H.suite("Net (TCP loopback)");
    let srv = T.Thread.spawn(echoServer);
    T.sleep(200);
    let cl = Net.TcpStream.connect("127.0.0.1:47123");
    H.check(cl != null, "connect");
    cl.write("hello");
    H.eqStr(cl.read(5).cstr(), "hello", "echo roundtrip");
    cl.close();
    srv.join();
    H.check(Net.TcpStream.connect("127.0.0.1:1") == null, "refused connection returns null");

    H.suite("Http (loopback server + client)");
    let hs = Http.HttpServer.new();
    hs.route("GET", "/ping", function(req: Http.Request) => Http.Response { return Http.Response.text(200, "pong"); });
    hs.listenAsync(47124);
    T.sleep(200);
    let r = Http.Http.get("http://127.0.0.1:47124/ping");
    H.check(r != null, "GET ok");
    H.eqInt(r.status, 200, "status 200");
    H.eqStr(r.body.cstr(), "pong", "body");
    H.eqInt(Http.Http.get("http://127.0.0.1:47124/nope").status, 404, "unknown route 404");
    hs.stop();

    H.suite("Db (SQLite)");
    let db = Db.Database.open(":memory:");
    H.check(db != null, "open");
    db.execute("CREATE TABLE t(id INTEGER PRIMARY KEY, name TEXT)");
    db.execute("INSERT INTO t(name) VALUES ('a'),('b'),('c')");
    H.eqInt(db.query("SELECT * FROM t", []).length(), 3, "query rows");
    let rows = db.query("SELECT name FROM t WHERE id = ?", [2]);
    H.eqStr(rows.get(0).get("name").cstr(), "b", "parameterised query");
    db.transaction(function() => void { db.execute("INSERT INTO t(name) VALUES ('d')"); });
    H.eqInt(db.query("SELECT * FROM t", []).length(), 4, "transaction commit");
    H.check(db.execute("THIS IS NOT SQL") == false, "bad SQL reports failure, no crash");
    db.close();

    return H.finish();
}
