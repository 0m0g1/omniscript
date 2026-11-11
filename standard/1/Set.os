// standard/1/Set.os
// Tier 3: Set<T> built on Map<T,bool>. Call init(hash, eq) then free().
module M = import "standard/1/Map.os";

struct Set<T> {
    m: M.Map<T, bool>;
    init(h: fn(T*) => u64, e: fn(T*, T*) => bool) => void { this.m.init(h, e); }
    free() => void { this.m.free(); }
    length() => u64 { return this.m.len; }
    isEmpty() => bool { return this.m.len == 0; }
    add(v: T) => bool { return this.m.put(v, true); }
    has(v: T) => bool { return this.m.has(v); }
    remove(v: T) => bool { return this.m.remove(v); }
    clear() => void { this.m.clear(); }
    next(it: u64*, k: T**) => bool {
        let vp: bool* = nullptr;
        return this.m.next(it, k, &vp);
    }
    // a |= b
    unionWith(o: Set<T>*) => bool {
        let it: u64 = 0; let k: T* = nullptr;
        while (o.next(&it, &k)) { if (!this.add(*k)) return false; }
        return true;
    }
    // out = this ∩ o (out must be initialised)
    intersect(o: Set<T>*, out: Set<T>*) => bool {
        let it: u64 = 0; let k: T* = nullptr;
        while (this.next(&it, &k)) { if (o.has(*k)) { if (!out.add(*k)) return false; } }
        return true;
    }
}
