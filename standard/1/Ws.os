// standard/1/Ws.os  -- STUB (not implemented yet)
// Tier 7: WebSocket client/server (RFC 6455): handshake via Http/Net, framing, ping/pong, masking. Uses Crypto.sha1+base64.
// All functions return false / do nothing until implemented.
module Net = import "standard/1/Net.os";
struct WebSocket { sock: Net.Socket;
    connect(url: char*) => bool { return false; }                 // TODO: parse ws://, Upgrade handshake, verify Sec-WebSocket-Accept
    sendText(s: char*) => bool { return false; }                  // TODO: frame + mask
    recvText(out: void*) => bool { return false; }                // TODO: unmask, fragments, control frames
    close() => void { }
}
