// destructor_demo.os -- the small version of ownership_demo.os: constructors + ~destructors, C++ style. (see ownership_demo.os for new/delete, defer, optionals)
//  PROPOSAL: needs compiler support; every line below is the target syntax.
module C = import "standard/1/Console.os";
module Str = import "standard/1/String.os";

struct Buffer {
    data: u8*;
    size: int;
    name: char*;

    Buffer(name: char*, size: int) {               // [NEW] constructor: same name as the struct, no return type
        this.name = name;
        this.size = size;
        this.data = new u8[size];                   // [NEW] new T[n]
        C.println(`  + ${name} (${size} bytes)`);
    }

    ~Buffer() {                                     // [NEW] destructor: runs automatically when the value goes out of scope
        delete this.data;                           // [NEW] delete (frees what `new` made)
        C.println(`  - ${this.name} freed`);
    }

    fill(v: int) => void { /* ... */ }              // ordinary methods are unchanged
}

struct Connection {
    buf: Buffer;                                    // a member with a destructor: dropped automatically after ~Connection() runs
    id: int;

    Connection(id: int) { this.id = id; this.buf = Buffer(`conn${id}`, 4096); }
    ~Connection() { C.println(`  - connection ${this.id} closing`); }   // member `buf` is destroyed after this body, no code needed
}

function work(fail: bool) => int {
    let a = Buffer("a", 16);                        // constructed here
    let b = Buffer("b", 32);
    if (fail) return 1;                             // ~b then ~a run before the return (reverse order of construction)
    C.println("  working");
    return 0;
}                                                   // ~b then ~a run here on the normal path too

function main() => int {
    C.println("normal:");   work(false);
    C.println("early return:"); work(true);
    C.println("member destruction:");
    {
        let c = Connection(7);
    }                                               // ~Connection(), then c.buf's ~Buffer()
    C.println("done");
    return 0;
}

// EXPECTED OUTPUT
// normal:
//   + a (16 bytes)
//   + b (32 bytes)
//   working
//   - b freed
//   - a freed
// early return:
//   + a (16 bytes)
//   + b (32 bytes)
//   - b freed
//   - a freed
// member destruction:
//   + conn7 (4096 bytes)
//   - connection 7 closing
//   - conn7 freed
// done
