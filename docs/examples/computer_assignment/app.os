module Console = import "standard/1/Console.os";
module HS = import "standard/1/HttpServer.os";

function health(req: HS.Request*, res: HS.Response*) => void {
    res.json(`{"ok": true}`);
}

function home(req: HS.Request*, res: HS.Response*) => void {
    let name = "0m0g1";
    res.html(`<h1>Hello, ${name}! </h1>`);
}

function main() => int {
    let srv: HS.HttpServer;
    const port = 8090;
    srv.get("/", home);
    srv.get("/health", health);
    if (!srv.listen(port)) Console.println("bind failed");
    srv.free();
    return 0;
}