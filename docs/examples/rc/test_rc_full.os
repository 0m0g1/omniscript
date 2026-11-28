/// Comprehensive Rc/Box test. Every check also looks at liveBlocks(), so a missing free or a
/// double free shows up as a wrong count and not only as a crash.

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

function refsInside(r: R.Rc) => u64 { return r.refs(); }
function innerRefs(r: R.Rc) => u64 { return r.refs(); }
function outerRefs(r: R.Rc) => u64 { return innerRefs(r); }
function addrOf(r: R.Rc) => u64 { return r.get() as u64; }
function liveInsideBoxCopy(b: R.Box) => int { return R.liveBlocks(); }

function mutateBoxCopy(b: R.Box) => void {
    let p: u8* = b.get() as u8*;
    p[0] = 99 as u8;
}

function earlyExit(early: bool) => int {
    let r: R.Rc = R.Rc(8);
    if (early) { return 1; }
    return 2;
}

function make() => R.Rc {
    let r: R.Rc = R.Rc(8);
    return r;
}

function main() => int {
    let base: int = R.liveBlocks();

    // --- Rc basics ---
    let a: R.Rc = R.Rc(16);
    check("new Rc: one live block", R.liveBlocks() == base + 1);
    check("new Rc: count is 1", a.refs() == 1);
    check("copy by value: count 2 inside", refsInside(a) == 2);
    check("copy by value: still one block", R.liveBlocks() == base + 1);
    check("copy dies: count back to 1", a.refs() == 1);
    check("nested by-value copies: count 3 at depth 2", outerRefs(a) == 3);
    check("nested copies die: count 1", a.refs() == 1);

    let addr: u64 = a.get() as u64;
    check("copy shares the same block address", addrOf(a) == addr);

    // --- scope exit frees ---
    if (true) {
        let t: R.Rc = R.Rc(8);
        check("inner scope: second block live", R.liveBlocks() == base + 2);
    }
    check("inner scope exit: destructor freed it", R.liveBlocks() == base + 1);

    // --- loop: one construct/destruct per iteration ---
    let i: int = 0;
    while (i < 1000) {
        let w: R.Rc = R.Rc(8);
        i = i + 1;
    }
    check("1000 loop iterations leak nothing", R.liveBlocks() == base + 1);

    // --- early return runs the destructor ---
    check("early return path", earlyExit(true) == 1);
    check("early return freed its Rc", R.liveBlocks() == base + 1);
    check("normal return path", earlyExit(false) == 2);
    check("normal return freed its Rc", R.liveBlocks() == base + 1);

    // --- returning an Rc by value ---
    let m: R.Rc = make();
    check("returned Rc is valid", m.isValid());
    check("returned Rc: count is 1", m.refs() == 1);
    check("returned Rc: exactly one new block", R.liveBlocks() == base + 2);
    m.release();
    check("release frees the returned Rc", R.liveBlocks() == base + 1);
    m.release();
    check("double release on Rc is safe", R.liveBlocks() == base + 1);

    // --- Box deep copy ---
    let b: R.Box = R.Box(8);
    let bp: u8* = b.get() as u8*;
    bp[0] = 7 as u8;
    check("new Box: one more block", R.liveBlocks() == base + 2);
    check("Box copy allocates its own block", liveInsideBoxCopy(b) == base + 3);
    check("Box copy freed on return", R.liveBlocks() == base + 2);
    mutateBoxCopy(b);
    check("mutating a Box copy leaves the original", bp[0] == 7 as u8);
    b.release();
    b.release();
    check("Box double release is safe", R.liveBlocks() == base + 1);

    // --- release the last Rc ---
    a.release();
    check("last release: everything freed", R.liveBlocks() == base);
    check("released Rc reports 0 refs", a.refs() == 0);

    if (fails == 0) { Console.println("ALL PASS"); }
    return fails;
}
