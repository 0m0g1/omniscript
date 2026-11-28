/// Risky case, kept in its own file: an Rc stored inside a struct, with an explicit constructor that
/// assigns it. It shows whether a struct's destructor releases its fields and whether
/// `this.r = R.Rc(n)` double-releases the temporary. A crash or a wrong count here is the answer.

module Console = import "standard/1/Console.os";
module R = import "Rc.os";

let fails: int = 0;

function check(name: char*, ok: bool) => void {
    if (ok) {
        Console.print("PASS  ");
    } else {
        Console.print("FAIL  ");
        fails = fails + 1;
    }
    Console.println(name);
}

struct Holder {
    r: R.Rc;

    constructor(n: u64) {
        this.r = R.Rc(n);
    }

    ~destructor() {
        this.r.release();
    }

    count() => u64 { return this.r.refs(); }
}

function main() => int {
    let base: int = R.liveBlocks();

    if (true) {
        let h: Holder = Holder(16 as u64);
        check("Holder built: one block live", R.liveBlocks() == base + 1);
        check("Holder's Rc has count 1 (a temporary double-count shows as 2)", h.count() == 1);
    }
    check("Holder gone: block freed", R.liveBlocks() == base);

    if (fails == 0) { Console.println("ALL PASS"); }
    return fails;
}
