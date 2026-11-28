/// Smoke test for Rc.os. Run from the folder that holds Rc.os:   os test_rc.os

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

/// Takes an Rc by value: the copy constructor runs on entry, the destructor on exit.
function countInside(r: R.Rc) => u64 {
    return r.refs();
}

function main() => int {
    let a: R.Rc = R.Rc(16);
    check("new Rc is valid", a.isValid());
    check("count starts at 1", a.refs() == 1);
    check("by-value copy raises count to 2", countInside(a) == 2);
    check("count back to 1 after the copy dies", a.refs() == 1);

    let b: R.Box = R.Box(8);
    check("new Box is valid", b.isValid());
    b.release();
    check("released Box is empty", !b.isValid());
    b.release();
    check("double release is safe", !b.isValid());

    a.release();
    check("released Rc is empty", a.refs() == 0);

    if (fails == 0) { Console.println("ALL PASS"); }
    return fails;
}
