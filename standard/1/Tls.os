// standard/1/Tls.os  -- STUB (not implemented yet)
// Tier 7: TLS client/server sockets (OpenSSL SSL_*, or Windows SChannel). Wrap Net.Socket.
// All functions return false / do nothing until implemented.
module Net = import "standard/1/Net.os";
struct TlsSocket { sock: Net.Socket; ctx: void* = nullptr; ssl: void* = nullptr;
    connect(host: char*, port: char*) => bool { return false; }   // TODO: SSL_CTX_new, SSL_set_tlsext_host_name, SSL_connect
    send(p: u8*, n: u64) => i64 { return -1; }                    // TODO: SSL_write
    recv(p: u8*, n: u64) => i64 { return -1; }                    // TODO: SSL_read
    close() => void { }                                           // TODO: SSL_shutdown/SSL_free
}
