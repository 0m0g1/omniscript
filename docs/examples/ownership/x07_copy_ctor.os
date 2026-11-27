// x07: copy constructor + operator= (needs `delete` and by-value copy rules)
module C = import "standard/1/Console.os";
let live: int = 0;
let trace: char* = "";
function t(s: char*) => void { trace = trace + s; }
function check(name: char*, want: char*) => void {
    if (trace == want && live == 0) { C.println(`ok    ${name}`); }
    else { C.println(`FAIL  ${name}: got="${trace}" want="${want}" live=${live}`); }
}
struct Buf {
    size: int;
    constructor(n: int) { this.size = n; live = live + 1; t("+buf "); }
    constructor(o: Buf&) { this.size = o.size; live = live + 1; t("+copy "); }
    ~destructor() { live = live - 1; t("-buf "); }
}
function takes(b: Buf) => int { return b.size; }
function sec() => void {
    let a = Buf(8);
    let n: int = takes(a);
    t(`size=${n} `);
}
sec();
check("copy constructor on by-value parameter", "+buf +copy -buf size=8 -buf ");
