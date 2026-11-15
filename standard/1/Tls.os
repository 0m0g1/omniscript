// standard/1/Tls.os
// Tier 5: TLS client and server sockets over Net.Socket (OpenSSL libssl, see c/tls.os).
//
// Client:
//   let t: Tls.TlsSocket;
//   if (t.connect("example.com", "443")) {          // verifies the certificate chain and the host name
//       t.sendStr("GET / HTTP/1.0\r\nHost: example.com\r\n\r\n");
//       let buf: [1024]u8;  let n: i64 = t.recv(&buf as u8*, 1024);   // 0 = peer closed, -1 = error
//   }
//   t.close();
// Certificates: connect() first tries OpenSSL's Windows store (OpenSSL 3.2+), then its default paths. Set
// t.caFile = "cacert.pem" before connect() to use your own bundle. t.insecure = true turns verification OFF (testing only).
//
// Server:
//   let srv: Tls.TlsServer;
//   srv.listen(8443, "cert.pem", "key.pem");        // PEM certificate chain + private key
//   let c: Tls.TlsSocket;
//   while (srv.accept(&c)) { ...c.send/recv... c.close(); }
//   srv.close();
//
// Blocking I/O, one TLS session per TlsSocket (do not share one across threads without a lock).
// No `return` inside loop bodies (compiler quirk noted in Canvas.os).
module Net = import "standard/1/Net.os";
module CT = import "standard/1/c/tls.os";
module CMem = import "standard/1/c/mem.os";
module Str = import "standard/1/String.os";

let TLS_VERIFY_NONE: int = 0;
let TLS_VERIFY_PEER: int = 1;
let SSL_CTRL_SET_TLSEXT_HOSTNAME: int = 55;
let SSL_CTRL_SET_MIN_PROTO_VERSION: int = 123;
let TLS1_2_VERSION: i64 = 0x0303;
let SSL_FILETYPE_PEM: int = 1;
let SSL_ERROR_ZERO_RETURN: int = 6;

function strLen(s: char*) => u64 { let n: u64 = 0; while ((s[n] as int) != 0) n = n + 1; return n; }

struct TlsSocket {
    sock: Net.Socket;
    ctx: void* = nullptr;
    ssl: void* = nullptr;
    ownsCtx: bool = true;           // false for sockets accepted by a TlsServer (the server owns the context)
    caFile: char* = nullptr;        // optional CA bundle (PEM) for client verification
    insecure: bool = false;         // true = skip certificate verification

    valid() => bool { return this.ssl != nullptr; }

    // TCP connect + TLS handshake (SNI and host-name verification included). False on any failure; the socket is cleaned up.
    connect(host: char*, port: char*) => bool {
        if (!Net.connectTcp(host, port, &this.sock)) return false;
        this.ctx = CT.SSL_CTX_new(CT.TLS_client_method());
        if (this.ctx == nullptr) { this.close(); return false; }
        this.ownsCtx = true;
        CT.SSL_CTX_ctrl(this.ctx, SSL_CTRL_SET_MIN_PROTO_VERSION, TLS1_2_VERSION, nullptr);
        if (this.insecure) {
            CT.SSL_CTX_set_verify(this.ctx, TLS_VERIFY_NONE, nullptr);
        } else {
            CT.SSL_CTX_set_verify(this.ctx, TLS_VERIFY_PEER, nullptr);
            let loaded: bool = false;
            if (this.caFile != nullptr) loaded = CT.SSL_CTX_load_verify_locations(this.ctx, this.caFile, nullptr) == 1;
            if (!loaded) loaded = CT.SSL_CTX_load_verify_store(this.ctx, "org.openssl.winstore://") == 1;
            if (!loaded) loaded = CT.SSL_CTX_set_default_verify_paths(this.ctx) == 1;
            if (!loaded) { this.close(); return false; }
        }
        this.ssl = CT.SSL_new(this.ctx);
        if (this.ssl == nullptr) { this.close(); return false; }
        CT.SSL_set_fd(this.ssl, this.sock.s as int);
        CT.SSL_ctrl(this.ssl, SSL_CTRL_SET_TLSEXT_HOSTNAME, 0, host as void*);
        if (!this.insecure) CT.SSL_set1_host(this.ssl, host);
        if (CT.SSL_connect(this.ssl) != 1) { this.close(); return false; }
        return true;
    }

    // Bytes sent, -1 on error.
    send(p: u8*, n: u64) => i64 {
        if (this.ssl == nullptr) return -1;
        let r: int = CT.SSL_write(this.ssl, p as void*, n as int);
        if (r <= 0) return -1;
        return r as i64;
    }

    sendAll(p: u8*, n: u64) => bool {
        let off: u64 = 0;
        let ok: bool = true;
        while (ok && off < n) {
            let r: i64 = this.send(&p[off], n - off);
            if (r <= 0) ok = false;
            else off = off + (r as u64);
        }
        return ok;
    }

    sendStr(s: char*) => bool { return this.sendAll(s as u8*, strLen(s)); }

    // Bytes received; 0 = peer closed the connection, -1 = error.
    recv(p: u8*, n: u64) => i64 {
        if (this.ssl == nullptr) return -1;
        let r: int = CT.SSL_read(this.ssl, p as void*, n as int);
        if (r > 0) return r as i64;
        if (r == 0) return 0;
        let e: int = CT.SSL_get_error(this.ssl, r);
        if (e == SSL_ERROR_ZERO_RETURN) return 0;
        return -1;
    }

    // Appends up to `max` received bytes to out.
    recvInto(out: Str.String*, max: u64) => i64 {
        let buf = CMem.malloc(max) as u8*;
        if (buf == nullptr) return -1;
        let r: i64 = this.recv(buf, max);
        if (r > 0) out.appendN(buf as char*, r as u64);
        CMem.free(buf as void*);
        return r;
    }

    close() => void {
        if (this.ssl != nullptr) {
            CT.SSL_shutdown(this.ssl);
            CT.SSL_free(this.ssl);
            this.ssl = nullptr;
        }
        if (this.ctx != nullptr && this.ownsCtx) CT.SSL_CTX_free(this.ctx);
        this.ctx = nullptr;
        this.sock.close();
    }
}

struct TlsServer {
    listener: Net.Socket;
    ctx: void* = nullptr;

    // Loads the certificate chain + private key (both PEM) and starts listening on 0.0.0.0:port.
    listen(port: u16, certFile: char*, keyFile: char*) => bool {
        this.ctx = CT.SSL_CTX_new(CT.TLS_server_method());
        if (this.ctx == nullptr) return false;
        CT.SSL_CTX_ctrl(this.ctx, SSL_CTRL_SET_MIN_PROTO_VERSION, TLS1_2_VERSION, nullptr);
        let ok: bool = CT.SSL_CTX_use_certificate_chain_file(this.ctx, certFile) == 1;
        if (ok) ok = CT.SSL_CTX_use_PrivateKey_file(this.ctx, keyFile, SSL_FILETYPE_PEM) == 1;
        if (ok) ok = CT.SSL_CTX_check_private_key(this.ctx) == 1;
        if (ok) ok = Net.listenTcp(port, 64, &this.listener);
        if (!ok) { this.close(); return false; }
        return true;
    }

    // Waits for a client and completes the TLS handshake. False if accepting or the handshake failed
    // (call again to keep serving; false from a closed listener means the server was stopped).
    accept(out: TlsSocket*) => bool {
        if (this.ctx == nullptr) return false;
        if (!Net.acceptTcp(&this.listener, &out.sock)) return false;
        out.ctx = this.ctx;
        out.ownsCtx = false;
        out.ssl = CT.SSL_new(this.ctx);
        if (out.ssl == nullptr) { out.close(); return false; }
        CT.SSL_set_fd(out.ssl, out.sock.s as int);
        if (CT.SSL_accept(out.ssl) != 1) { out.close(); return false; }
        return true;
    }

    close() => void {
        this.listener.close();
        if (this.ctx != nullptr) { CT.SSL_CTX_free(this.ctx); this.ctx = nullptr; }
    }
}
