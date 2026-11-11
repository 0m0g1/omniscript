// standard/1/Map.os
// Tier 3: hash map Map<K,V>, open addressing (linear probe, tombstones). Owns buffers: call free().
// Needs hash/equals fn pointers (no traits yet). Use StrMap for char* keys.
module Core = import "standard/1/Core.os";
module CMem = import "standard/1/c/mem.os";
module CStr = import "standard/1/c/string.os";

struct Map<K, V> {
    keys: K* = nullptr;
    vals: V* = nullptr;
    state: u8* = nullptr;   // 0 empty, 1 used, 2 tombstone
    len: u64 = 0;
    used: u64 = 0;          // used + tombstones
    cap: u64 = 0;
    hashFn: fn(K*) => u64 = nullptr;
    eqFn: fn(K*, K*) => bool = nullptr;

    init(h: fn(K*) => u64, e: fn(K*, K*) => bool) => void { this.hashFn = h; this.eqFn = e; }

    free() => void {
        if (this.keys != nullptr) CMem.free(this.keys as void*);
        if (this.vals != nullptr) CMem.free(this.vals as void*);
        if (this.state != nullptr) CMem.free(this.state as void*);
        this.keys = nullptr; this.vals = nullptr; this.state = nullptr;
        this.len = 0; this.used = 0; this.cap = 0;
    }

    length() => u64 { return this.len; }
    isEmpty() => bool { return this.len == 0; }

    find(k: K*) => i64 {
        if (this.cap == 0) return -1;
        let i: u64 = this.hashFn(k) % this.cap;
        let n: u64 = 0;
        while (n < this.cap) {
            if (this.state[i] == 0) return -1;
            if (this.state[i] == 1 && this.eqFn(&this.keys[i], k)) return i as i64;
            i = (i + 1) % this.cap;
            n = n + 1;
        }
        return -1;
    }

    grow() => bool {
        let ncap: u64 = this.cap * 2;
        if (ncap == 0) ncap = 16;
        let nk = CMem.malloc(ncap * sizeof(K)) as K*;
        let nv = CMem.malloc(ncap * sizeof(V)) as V*;
        let ns = CMem.calloc(ncap, 1) as u8*;
        if (nk == nullptr || nv == nullptr || ns == nullptr) return false;
        let ok = this.keys; let ov = this.vals; let os = this.state; let oc = this.cap;
        this.keys = nk; this.vals = nv; this.state = ns; this.cap = ncap;
        this.len = 0; this.used = 0;
        let i: u64 = 0;
        while (i < oc) {
            if (os[i] == 1) this.put(ok[i], ov[i]);
            i = i + 1;
        }
        if (ok != nullptr) { CMem.free(ok as void*); CMem.free(ov as void*); CMem.free(os as void*); }
        return true;
    }

    put(k: K, v: V) => bool {
        if (this.cap == 0 || (this.used + 1) * 4 > this.cap * 3) { if (!this.grow()) return false; }
        let idx = this.find(&k);
        if (idx >= 0) { this.vals[idx] = v; return true; }
        let i: u64 = this.hashFn(&k) % this.cap;
        while (this.state[i] == 1) i = (i + 1) % this.cap;
        if (this.state[i] == 0) this.used = this.used + 1;
        this.keys[i] = k; this.vals[i] = v; this.state[i] = 1;
        this.len = this.len + 1;
        return true;
    }

    get(k: K, out: V*) => bool {
        let i = this.find(&k);
        if (i < 0) return false;
        *out = this.vals[i];
        return true;
    }

    ptr(k: K) => V* {
        let i = this.find(&k);
        if (i < 0) return nullptr;
        return &this.vals[i];
    }

    has(k: K) => bool { return this.find(&k) >= 0; }

    remove(k: K) => bool {
        let i = this.find(&k);
        if (i < 0) return false;
        this.state[i] = 2;
        this.len = this.len - 1;
        return true;
    }

    clear() => void {
        if (this.state != nullptr) CMem.memset(this.state as void*, 0, this.cap);
        this.len = 0; this.used = 0;
    }

    // Iterate: let it: u64 = 0; while (m.next(&it, &kp, &vp)) { ... }
    next(it: u64*, k: K**, v: V**) => bool {
        while (*it < this.cap) {
            let i = *it;
            *it = *it + 1;
            if (this.state[i] == 1) { *k = &this.keys[i]; *v = &this.vals[i]; return true; }
        }
        return false;
    }
}

// ---- ready-made hash/eq helpers ----
function hashU64(k: u64*) => u64 {
    let x: u64 = *k;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9;
    x = (x ^ (x >> 27)) * 0x94d049bb133111eb;
    return x ^ (x >> 31);
}
function eqU64(a: u64*, b: u64*) => bool { return *a == *b; }
function hashI64(k: i64*) => u64 { let x: u64 = *k as u64; return hashU64(&x); }
function eqI64(a: i64*, b: i64*) => bool { return *a == *b; }

function hashCStr(k: char**) => u64 {
    let s: char* = *k;
    let h: u64 = 14695981039346656037;
    let i: u64 = 0;
    while ((s[i] as int) != 0) { h = (h ^ (s[i] as u64)) * 1099511628211; i = i + 1; }
    return h;
}
function eqCStr(a: char**, b: char**) => bool { return CStr.strcmp(*a, *b) == 0; }

// StrMap<V>: char* keys (NOT copied; keys must outlive the map).
// let m: Map.StrMap<i32>; m.init(); m.put("a", 1);
struct StrMap<V> {
    m: Map<char*, V>;
    init() => void { this.m.init(hashCStr, eqCStr); }
    free() => void { this.m.free(); }
    length() => u64 { return this.m.len; }
    put(k: char*, v: V) => bool { return this.m.put(k, v); }
    get(k: char*, out: V*) => bool { return this.m.get(k, out); }
    has(k: char*) => bool { return this.m.has(k); }
    remove(k: char*) => bool { return this.m.remove(k); }
    ptr(k: char*) => V* { return this.m.ptr(k); }
    clear() => void { this.m.clear(); }
    next(it: u64*, k: char***, v: V**) => bool { return this.m.next(it, k, v); }
}
