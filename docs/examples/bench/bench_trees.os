// bench_trees: binary trees (CLBG style); measures new/delete, struct pointers and recursion.
// Run:  .\bin\Release-windows-x86_64\Osengine.exe bench\bench_trees.os
// Total node count has a closed form, so the check needs no reference run:
//   sum over d = 4, 8, 12, ... <= MAXD of 2^(MAXD - d + 4) * (2^(d+1) - 1).   MAXD = 20 -> 166653680 ; MAXD = 16 -> 8318704.

module Console = import "standard/1/Console.os";

const MAXD: int = 20;

struct Node {
    left: Node*;
    right: Node*;
}

function make(d: int) => Node* {
    if (d == 0) { return new Node(nullptr, nullptr); }
    return new Node(make(d - 1), make(d - 1));
}

function check(n: Node*) => i64 {
    if (n.left == nullptr) { return 1; }
    return 1 + check(n.left) + check(n.right);
}

function release(n: Node*) => void {
    if (n.left != nullptr) {
        release(n.left);
        release(n.right);
    }
    delete n;
}

function trees(maxd: int) => i64 {
    let total: i64 = 0;
    let one: i64 = 1;
    let d: int = 4;
    while (d <= maxd) {
        let shift: i64 = (maxd - d + 4) as i64;
        let iters: i64 = one << shift;
        let i: i64 = 0;
        while (i < iters) {
            let t: Node* = make(d);
            total = total + check(t);
            release(t);
            i = i + 1;
        }
        d = d + 4;
    }
    return total;
}

function expected(maxd: int) => i64 {
    let total: i64 = 0;
    let one: i64 = 1;
    let d: int = 4;
    while (d <= maxd) {
        let iters: i64 = one << ((maxd - d + 4) as i64);
        let nodes: i64 = (one << ((d + 1) as i64)) - 1;
        total = total + iters * nodes;
        d = d + 4;
    }
    return total;
}

let got: i64 = trees(MAXD);
let want: i64 = expected(MAXD);
if (got == want) { Console.print("PASS  trees  nodes="); }
else { Console.print("FAIL  trees  nodes="); }
Console.println(got);
