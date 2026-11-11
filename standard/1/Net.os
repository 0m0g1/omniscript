// standard/1/Net.os
// Tier 5: blocking TCP client/server via WinSock. (PLATFORM: WinSock2; POSIX: drop startup(), closesocket->close)
module CN = import "standard/1/c/net.os";
module Str = import "standard/1/String.os";
module CMem = import "standard/1/c/mem.os";

let started: bool = false;

function startup() => bool {
    if (started) return true;
    let wsa: [512]u8;
    if (CN.WSAStartup(0x0202, &wsa as void*) != 0) return false;
    started = true;
    return true;
}
function shutdownNet() => void { if (started) { CN.WSACleanup(); started = false; } }

struct Socket {
    s: u64 = 0xFFFFFFFFFFFFFFFF;    // INVALID_SOCKET
    valid() => bool { return this.s != 0xFFFFFFFFFFFFFFFF; }
    close() => void { if (this.valid()) { CN.closesocket(this.s); this.s = 0xFFFFFFFFFFFFFFFF; } }

    // returns bytes sent, -1 on error
    send(p: u8*, n: u64) => i64 { return CN.send(this.s, p as void*, n as int, 0) as i64; }
    sendAll(p: u8*, n: u64) => bool {
        let off: u64 = 0;
        while (off < n) {
            let r: i64 = this.send(&p[off], n - off);
            if (r <= 0) return false;
            off = off + (r as u64);
        }
        return true;
    }
    sendStr(s: char*) => bool { let n: u64 = 0; while ((s[n] as int) != 0) n = n + 1; return this.sendAll(s as u8*, n); }
    // 0 = peer closed, -1 = error
    recv(p: u8*, n: u64) => i64 { return CN.recv(this.s, p as void*, n as int, 0) as i64; }
    recvInto(out: Str.String*, max: u64) => i64 {
        let buf = CMem.malloc(max) as u8*;
        let r: i64 = this.recv(buf, max);
        if (r > 0) out.appendN(buf as char*, r as u64);
        CMem.free(buf as void*);
        return r;
    }
    shutdownBoth() => void { CN.shutdown(this.s, 2); }
}

// addrinfo layout (x64 Windows): flags i32, family i32, socktype i32, protocol i32, addrlen u64, canonname char*, addr void*, next void*
struct AddrInfo { flags: i32; family: i32; socktype: i32; protocol: i32; addrlen: u64; canon: char*; addr: void*; next: void*; }

function connectTcp(host: char*, port: char*, out: Socket*) => bool {
    if (!startup()) return false;
    let hints: AddrInfo;
    CMem.memset(&hints as void*, 0, sizeof(AddrInfo));
    hints.family = 0; hints.socktype = 1; hints.protocol = 6;
    let res: void* = nullptr;
    if (CN.getaddrinfo(host, port, &hints as void*, &res) != 0) return false;
    let ai: AddrInfo* = res as AddrInfo*;
    while (ai != nullptr) {
        let s: u64 = CN.socket(ai.family as int, ai.socktype as int, ai.protocol as int);
        if (s != 0xFFFFFFFFFFFFFFFF) {
            if (CN.connect(s, ai.addr, ai.addrlen as int) == 0) { out.s = s; CN.freeaddrinfo(res); return true; }
            CN.closesocket(s);
        }
        ai = ai.next as AddrInfo*;
    }
    CN.freeaddrinfo(res);
    return false;
}

// Listen on 0.0.0.0:port (IPv4).
function listenTcp(port: u16, backlog: int, out: Socket*) => bool {
    if (!startup()) return false;
    let s: u64 = CN.socket(2, 1, 6);
    if (s == 0xFFFFFFFFFFFFFFFF) return false;
    let sa: [16]u8;
    CMem.memset(&sa as void*, 0, 16);
    sa[0] = 2; sa[1] = 0;                       // AF_INET
    sa[2] = (port >> 8) as u8; sa[3] = (port & 0xFF) as u8;
    if (CN.bind(s, &sa as void*, 16) != 0 || CN.listen(s, backlog) != 0) { CN.closesocket(s); return false; }
    out.s = s;
    return true;
}

function acceptTcp(server: Socket*, out: Socket*) => bool {
    let s: u64 = CN.accept(server.s, nullptr, nullptr);
    if (s == 0xFFFFFFFFFFFFFFFF) return false;
    out.s = s;
    return true;
}

// ---- UDP (IPv4) ----
struct UdpSocket {
    s: u64 = 0xFFFFFFFFFFFFFFFF;
    open() => bool { if (!startup()) return false; this.s = CN.socket(2, 2, 17); return this.s != 0xFFFFFFFFFFFFFFFF; }
    close() => void { if (this.s != 0xFFFFFFFFFFFFFFFF) { CN.closesocket(this.s); this.s = 0xFFFFFFFFFFFFFFFF; } }
    bindPort(port: u16) => bool {
        let sa: [16]u8; CMem.memset(&sa as void*, 0, 16);
        sa[0] = 2; sa[2] = (port >> 8) as u8; sa[3] = (port & 0xFF) as u8;
        return CN.bind(this.s, &sa as void*, 16) == 0;
    }
    sendTo(ip: char*, port: u16, p: u8*, n: u64) => i64 {
        let sa: [16]u8; CMem.memset(&sa as void*, 0, 16);
        sa[0] = 2; sa[2] = (port >> 8) as u8; sa[3] = (port & 0xFF) as u8;
        if (CN.inet_pton(2, ip, &sa[4] as void*) != 1) return -1;
        return CN.sendto(this.s, p as void*, n as int, 0, &sa as void*, 16) as i64;
    }
    // fills fromIp (>= 16 bytes) and fromPort
    recvFrom(p: u8*, n: u64, fromIp: char*, fromPort: u16*) => i64 {
        let sa: [16]u8; let l: int = 16;
        let r: int = CN.recvfrom(this.s, p as void*, n as int, 0, &sa as void*, &l);
        if (r >= 0) {
            CN.inet_ntop(2, &sa[4] as void*, fromIp, 16);
            *fromPort = ((sa[2] as u16) << 8) | (sa[3] as u16);
        }
        return r as i64;
    }
}

// Resolve hostname to a dotted IPv4 string.
function resolve(host: char*, out: Str.String*) => bool {
    if (!startup()) return false;
    let hints: AddrInfo; CMem.memset(&hints as void*, 0, sizeof(AddrInfo));
    hints.family = 2; hints.socktype = 1;
    let res: void* = nullptr;
    if (CN.getaddrinfo(host, nullptr, &hints as void*, &res) != 0) return false;
    let ai: AddrInfo* = res as AddrInfo*;
    let buf: [16]char;
    CN.inet_ntop(2, ((ai.addr as u64) + 4) as void*, &buf as char*, 16);
    CN.freeaddrinfo(res);
    return out.append(&buf as char*);
}
