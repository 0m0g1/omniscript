// standard/1/Iter.os  -- STUB (not implemented yet)
// Tier 7: lazy iterators (map/filter/take/zip/chain/collect). Needs closures or fn-pointer generics + generic traits.
// All functions return false / do nothing until implemented.
struct Iter<T> { next: fn(void*, T*) => bool = nullptr; state: void* = nullptr; }
// TODO: fromList(List<T>*), map<T,U>, filter<T>, take, skip, zip, enumerate, chain, collect(List<T>*), fold, count.
function fromArray<T>(a: T*, n: u64) => Iter<T> { let it: Iter<T>; return it; }  // TODO
