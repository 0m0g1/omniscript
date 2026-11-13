// standard/1/Iter.os
// Tier 2: lazy iterators over T* slices.
//
// Design:
//   Iter<T> holds data + position + an ordered op chain. Each op is either a
//   predicate (filter) or a transform (map). `next()` runs the ops in the order
//   they were added, so `xs.map(f).filter(p)` filters on `f(x)` as expected.
//
//   filter/map/take/skip all return Iter<T> sharing the same data pointer.
//   Cross-type (map to a different type), zip, chain, and enumerate are free
//   functions returning specialized iterator structs (MapIter, Zip, Chain,
//   Enumerate) that expose `next` and consume methods but don't chain further.
//
// No allocations, no closures, no stubs.

include "standard/1/List.os";

// ===========================================================================
// Iter<T>
// ===========================================================================
struct Iter<T extends i32 | i64 | u32 | u64 | f32 | f64> {
    data: T* = nullptr;
    len: u64 = 0;
    pos: u64 = 0;

    transforms: [8]fn(T) => T;
    nTransforms: u64 = 0;

    predicates: [8]fn(T) => bool;
    nPredicates: u64 = 0;

    // Interleaved op order so map/filter chains run in source order.
    //   opKind[j] == 0 → predicates[opIndex[j]]
    //   opKind[j] == 1 → transforms[opIndex[j]]
    opKind: [16]u8;
    opIndex: [16]u8;
    nOps: u64 = 0;

    // ---- inspection ----
    hasNext() => bool { return this.pos < this.len; }
    count() => u64 { return this.len - this.pos; }
    reset() => void { this.pos = 0; }

    // ---- internal copy of the chain (used by map/filter/take/skip) ----
    copyFrom(src: Iter<T>*) => void {
        this.data = src.data;
        this.len = src.len;
        this.pos = src.pos;
        this.nTransforms = src.nTransforms;
        this.nPredicates = src.nPredicates;
        this.nOps = src.nOps;

        let j: u64 = 0;
        while (j < src.nTransforms) {
            this.transforms[j] = src.transforms[j];
            j = j + 1;
        }
        j = 0;
        while (j < src.nPredicates) {
            this.predicates[j] = src.predicates[j];
            j = j + 1;
        }
        j = 0;
        while (j < src.nOps) {
            this.opKind[j] = src.opKind[j];
            this.opIndex[j] = src.opIndex[j];
            j = j + 1;
        }
    }

    // ---- iteration ----
    next(out: T*) => bool {
        while (this.pos < this.len) {
            let v: T = this.data[this.pos];
            this.pos = this.pos + 1;

            let keep: bool = true;
            let j: u64 = 0;
            while (j < this.nOps && keep) {
                let kind: u8 = this.opKind[j];
                let idx: u8 = this.opIndex[j];
                if (kind == 0) {
                    if (!this.predicates[idx](v)) keep = false;
                } else {
                    v = this.transforms[idx](v);
                }
                j = j + 1;
            }
            if (!keep) continue;

            *out = v;
            return true;
        }
        return false;
    }

    // ---- type-preserving combinators ----
    map(f: fn(T) => T) => Iter<T> {
        let it: Iter<T>;
        it.copyFrom(this);
        if (it.nTransforms < 8 && it.nOps < 16) {
            it.transforms[it.nTransforms] = f;
            it.opKind[it.nOps] = 1;
            it.opIndex[it.nOps] = it.nTransforms;
            it.nTransforms = it.nTransforms + 1;
            it.nOps = it.nOps + 1;
        }
        return it;
    }

    filter(pred: fn(T) => bool) => Iter<T> {
        let it: Iter<T>;
        it.copyFrom(this);
        if (it.nPredicates < 8 && it.nOps < 16) {
            it.predicates[it.nPredicates] = pred;
            it.opKind[it.nOps] = 0;
            it.opIndex[it.nOps] = it.nPredicates;
            it.nPredicates = it.nPredicates + 1;
            it.nOps = it.nOps + 1;
        }
        return it;
    }

    take(n: u64) => Iter<T> {
        let remaining: u64 = this.len - this.pos;
        let actual: u64 = n < remaining ? n : remaining;
        let it: Iter<T>;
        it.copyFrom(this);
        it.len = this.pos + actual;
        return it;
    }

    skip(n: u64) => Iter<T> {
        let remaining: u64 = this.len - this.pos;
        let actual: u64 = n < remaining ? n : remaining;
        let it: Iter<T>;
        it.copyFrom(this);
        it.pos = this.pos + actual;
        return it;
    }

    // ---- terminal combinators ----
    forEach(f: fn(T) => void) => void {
        let v: T = 0 as T;
        while (this.next(&v)) f(v);
    }

    fold(init: T, f: fn(T, T) => T) => T {
        let acc: T = init;
        let v: T = 0 as T;
        while (this.next(&v)) acc = f(acc, v);
        return acc;
    }

    sum() => T {
        let acc: T = 0 as T;
        let v: T = 0 as T;
        while (this.next(&v)) acc = acc + v;
        return acc;
    }

    min(cmp: fn(T, T) => int) => T {
        let result: T = 0 as T;
        let v: T = 0 as T;
        if (!this.next(&result)) return result;
        while (this.next(&v)) {
            if (cmp(v, result) < 0) result = v;
        }
        return result;
    }

    max(cmp: fn(T, T) => int) => T {
        let result: T = 0 as T;
        let v: T = 0 as T;
        if (!this.next(&result)) return result;
        while (this.next(&v)) {
            if (cmp(v, result) > 0) result = v;
        }
        return result;
    }

    collect(out: List<T>*) => void {
        let v: T = 0 as T;
        while (this.next(&v)) { out.push(v); }
    }
}

// ===========================================================================
// MapIter<T, U> — terminal iterator for cross-type map
// ===========================================================================
struct MapIter<T extends i32 | i64 | u32 | u64 | f32 | f64,
               U extends i32 | i64 | u32 | u64 | f32 | f64> {
    src: Iter<T>;
    transform: fn(T) => U = nullptr;

    hasNext() => bool { return this.src.hasNext(); }
    count() => u64 { return this.src.count(); }

    next(out: U*) => bool {
        let v: T = 0 as T;
        if (!this.src.next(&v)) return false;
        *out = this.transform(v);
        return true;
    }

    forEach(f: fn(U) => void) => void {
        let v: U = 0 as U;
        while (this.next(&v)) f(v);
    }

    fold(init: U, f: fn(U, U) => U) => U {
        let acc: U = init;
        let v: U = 0 as U;
        while (this.next(&v)) acc = f(acc, v);
        return acc;
    }

    sum() => U {
        let acc: U = 0 as U;
        let v: U = 0 as U;
        while (this.next(&v)) acc = acc + v;
        return acc;
    }

    collect(out: List<U>*) => void {
        let v: U = 0 as U;
        while (this.next(&v)) { out.push(v); }
    }
}

// ===========================================================================
// Zip<T, U> — terminal iterator that pairs elements from two iterators
// ===========================================================================
struct Zip<T extends i32 | i64 | u32 | u64 | f32 | f64,
           U extends i32 | i64 | u32 | u64 | f32 | f64> {
    a: Iter<T>;
    b: Iter<U>;

    hasNext() => bool { return this.a.hasNext() && this.b.hasNext(); }
    count() => u64 {
        let ca: u64 = this.a.count();
        let cb: u64 = this.b.count();
        return ca < cb ? ca : cb;
    }

    next(outA: T*, outB: U*) => bool {
        if (!this.a.hasNext() || !this.b.hasNext()) return false;
        return this.a.next(outA) && this.b.next(outB);
    }
}

// ===========================================================================
// Enumerate<T> — terminal iterator that yields (index, value)
// ===========================================================================
struct Enumerate<T extends i32 | i64 | u32 | u64 | f32 | f64> {
    src: Iter<T>;
    idx: u64 = 0;

    hasNext() => bool { return this.src.hasNext(); }
    count() => u64 { return this.src.count(); }

    next(outIdx: u64*, outVal: T*) => bool {
        if (!this.src.next(outVal)) return false;
        *outIdx = this.idx;
        this.idx = this.idx + 1;
        return true;
    }
}

// ===========================================================================
// Chain<T> — terminal iterator that yields from `a` then from `b`
// ===========================================================================
struct Chain<T extends i32 | i64 | u32 | u64 | f32 | f64> {
    a: Iter<T>;
    b: Iter<T>;
    onB: bool = false;

    hasNext() => bool {
        if (!this.onB) return this.a.hasNext() || this.b.hasNext();
        return this.b.hasNext();
    }

    next(out: T*) => bool {
        if (!this.onB) {
            if (this.a.next(out)) return true;
            this.onB = true;
        }
        return this.b.next(out);
    }
}

// ===========================================================================
// Free-function constructors and adapters
// ===========================================================================
function iter<T extends i32 | i64 | u32 | u64 | f32 | f64>(a: T*, n: u64) => Iter<T> {
    let it: Iter<T>;
    it.data = a;
    it.len = n;
    it.pos = 0;
    it.nTransforms = 0;
    it.nPredicates = 0;
    it.nOps = 0;
    return it;
}

function mapIter<T extends i32 | i64 | u32 | u64 | f32 | f64,
                 U extends i32 | i64 | u32 | u64 | f32 | f64>(it: Iter<T>, f: fn(T) => U) => MapIter<T, U> {
    let m: MapIter<T, U>;
    m.src = it;
    m.transform = f;
    return m;
}

function zipIter<T extends i32 | i64 | u32 | u64 | f32 | f64,
                 U extends i32 | i64 | u32 | u64 | f32 | f64>(a: Iter<T>, b: Iter<U>) => Zip<T, U> {
    let z: Zip<T, U>;
    z.a = a;
    z.b = b;
    return z;
}

function enumerateIter<T extends i32 | i64 | u32 | u64 | f32 | f64>(it: Iter<T>) => Enumerate<T> {
    let e: Enumerate<T>;
    e.src = it;
    e.idx = 0;
    return e;
}

function chainIter<T extends i32 | i64 | u32 | u64 | f32 | f64>(a: Iter<T>, b: Iter<T>) => Chain<T> {
    let c: Chain<T>;
    c.a = a;
    c.b = b;
    c.onB = false;
    return c;
}