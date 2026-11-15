// standard/1/c/tls.os  (libssl, OpenSSL 3)
// Tier 0: raw TLS bindings. Needs libcrypto-3-x64.dll next to libssl-3-x64.dll.
// CHANGE the path if your OpenSSL DLLs live somewhere else (same folder as c/ssl.os uses).
// SSL_set_tlsext_host_name and SSL_CTX_set_min_proto_version are C macros over SSL_ctrl / SSL_CTX_ctrl:
//   SNI:  SSL_ctrl(ssl, 55, 0, hostname)           min version:  SSL_CTX_ctrl(ctx, 123, 0x0303 (TLS 1.2), nullptr)
extern "dependencies/openssl/bin/libssl-3-x64.dll" {
    fn TLS_client_method() => void*;
    fn TLS_server_method() => void*;
    fn SSL_CTX_new(method: void*) => void*;
    fn SSL_CTX_free(ctx: void*) => void;
    fn SSL_CTX_ctrl(ctx: void*, cmd: int, larg: i64, parg: void*) => i64;
    fn SSL_CTX_set_verify(ctx: void*, mode: int, cb: void*) => void;            // 0 = none, 1 = SSL_VERIFY_PEER
    fn SSL_CTX_set_default_verify_paths(ctx: void*) => int;
    fn SSL_CTX_load_verify_locations(ctx: void*, caFile: char*, caPath: char*) => int;
    fn SSL_CTX_load_verify_store(ctx: void*, uri: char*) => int;                // OpenSSL 3.2+: "org.openssl.winstore://"
    fn SSL_CTX_use_certificate_chain_file(ctx: void*, file: char*) => int;
    fn SSL_CTX_use_PrivateKey_file(ctx: void*, file: char*, kind: int) => int;   // 1 = SSL_FILETYPE_PEM
    fn SSL_CTX_check_private_key(ctx: void*) => int;
    fn SSL_new(ctx: void*) => void*;
    fn SSL_free(ssl: void*) => void;
    fn SSL_set_fd(ssl: void*, fd: int) => int;
    fn SSL_ctrl(ssl: void*, cmd: int, larg: i64, parg: void*) => i64;
    fn SSL_set1_host(ssl: void*, host: char*) => int;                           // enables hostname verification
    fn SSL_connect(ssl: void*) => int;
    fn SSL_accept(ssl: void*) => int;
    fn SSL_read(ssl: void*, buf: void*, n: int) => int;
    fn SSL_write(ssl: void*, buf: void*, n: int) => int;
    fn SSL_shutdown(ssl: void*) => int;
    fn SSL_get_error(ssl: void*, ret: int) => int;                              // 6 = SSL_ERROR_ZERO_RETURN
    fn SSL_get_verify_result(ssl: void*) => i64;                                // 0 = X509_V_OK
}
