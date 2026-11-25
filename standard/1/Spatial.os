// standard/1/Spatial.os
// Tier 2: SpatialHash, a uniform-grid hash for "who is near me?" queries (collisions, neighbours).
// Items are plain ids 0..capacity-1 (typically array indices); you keep the positions, the grid keeps only
// per-bucket linked chains. Rebuild every step: setOrigin, clear, insert every item.
// Owns two small int arrays: call free() when done (no destructors yet). Never copy one with `=`.
//
//   let grid: Spatial.SpatialHash;
//   grid.init(cellSize, 8192, maxItems);            // cellSize should be >= the interaction diameter
//   // each step:
//   grid.setOrigin(minX, minY); grid.clear();
//   for i in 0..n: grid.insert(i, x[i], y[i]);
//   // query around item p:
//   let cx = grid.cellX(x[p]); let cy = grid.cellY(y[p]);
//   for oy in -1..1, ox in -1..1:
//       let q = grid.first(cx + ox, cy + oy);
//       while (q >= 0) { ...test p vs q...; q = grid.following(q); }
//
// Cells are hashed into `buckets` slots, so a chain can hold items from other cells: always test the real
// distance. Keep buckets much larger than the number of occupied cells, otherwise two of the nine
// neighbour cells can share a bucket and a pair gets visited twice.

module Math = import "standard/1/Math.os";
module Mem = import "standard/1/Mem.os";

const ZERO: f32 = 0.0;
const HASH_X: i32 = 7919;
const HASH_Y: i32 = 104729;

struct SpatialHash {
    cellSize: f32 = 0.0;
    invCell: f32 = 0.0;
    originX: f32 = 0.0;
    originY: f32 = 0.0;
    buckets: i32 = 0;
    capacity: i32 = 0;
    heads: i32* = nullptr;      // bucket -> first item id, or -1
    chain: i32* = nullptr;      // item id -> next item id in the same bucket, or -1

    // false if the arguments are bad or memory runs out. Calling init again re-creates the grid.
    init(cell: f32, tableSize: i32, maxItems: i32) => bool {
        if (cell <= ZERO || tableSize <= 0 || maxItems <= 0) return false;
        this.free();
        let h = Mem.alloc((tableSize as u64) * sizeof(i32)) as i32*;
        let c = Mem.alloc((maxItems as u64) * sizeof(i32)) as i32*;
        if (h == nullptr || c == nullptr) {
            Mem.release(h as void*);
            Mem.release(c as void*);
            return false;
        }
        this.heads = h;
        this.chain = c;
        this.cellSize = cell;
        this.invCell = (1.0 as f32) / cell;
        this.buckets = tableSize;
        this.capacity = maxItems;
        this.clear();
        return true;
    }

    free() => void {
        Mem.release(this.heads as void*);
        Mem.release(this.chain as void*);
        this.heads = nullptr;
        this.chain = nullptr;
        this.buckets = 0;
        this.capacity = 0;
    }

    // World position of cell (0, 0)'s corner. Set it to the minimum coordinate you will insert.
    setOrigin(x: f32, y: f32) => void {
        this.originX = x;
        this.originY = y;
    }

    // Empties every bucket (items are not freed, they are just ids).
    clear() => void {
        if (this.heads == nullptr) return;
        let i: i32 = 0;
        while (i < this.buckets) {
            this.heads[i] = -1;
            i = i + 1;
        }
    }

    cellX(x: f32) => i32 { return Math.floor(((x - this.originX) * this.invCell) as f64) as i32; }
    cellY(y: f32) => i32 { return Math.floor(((y - this.originY) * this.invCell) as f64) as i32; }

    // Bucket index for a cell (always 0..buckets-1, also for negative cells).
    slot(cx: i32, cy: i32) => i32 {
        let h: i32 = (cx * HASH_X + cy * HASH_Y) % this.buckets;
        if (h < 0) h = -h;
        return h;
    }

    // false when id is out of range or the grid is not initialised.
    insert(id: i32, x: f32, y: f32) => bool {
        if (this.heads == nullptr || id < 0 || id >= this.capacity) return false;
        let h: i32 = this.slot(this.cellX(x), this.cellY(y));
        this.chain[id] = this.heads[h];
        this.heads[h] = id;
        return true;
    }

    // First item in the bucket of cell (cx, cy), or -1.
    first(cx: i32, cy: i32) => i32 {
        if (this.heads == nullptr) return -1;
        return this.heads[this.slot(cx, cy)];
    }

    // Next item after id in its bucket, or -1.
    following(id: i32) => i32 { return this.chain[id]; }
}
