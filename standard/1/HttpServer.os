// standard/1/HttpServer.os  -- STUB (not implemented yet)
// Tier 7: HTTP/1.1 server on Net + Thread/Channel: route table, request parse, response writer, static files.
// All functions return false / do nothing until implemented.
module Net = import "standard/1/Net.os";
struct Request { method: char* = nullptr; path: char* = nullptr; body: char* = nullptr; }
struct HttpServer {
    listener: Net.Socket;
    listen(port: u16) => bool { return false; }                   // TODO: Net.listenTcp + accept loop + worker pool
    route(method: char*, path: char*, handler: fn(Request*, void*) => void) => bool { return false; }  // TODO
    stop() => void { }
}
