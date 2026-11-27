// x06_for_in_range: for-in and ranges
module C = import "standard/1/Console.os";
let live: int = 0;
let trace: char* = "";
function t(s: char*) => void { trace = trace + s; }
function check(name: char*, want: char*) => void {
    if (trace == want && live == 0) { C.println(`ok    ${name}`); }
    else { C.println(`FAIL  ${name}: got="${trace}" want="${want}" live=${live}`); }
}
struct Res {
    name: char*;
    constructor(n: char*) { this.name = n; live = live + 1; t(`+${n} `); }
    ~destructor() { live = live - 1; t(`-${this.name} `); }
}
function sec() => void {
    let sum: int = 0;
    let nums: int[3];
    for (n in nums) { sum = sum + 1; }
    for (i in 0..3) { sum = sum + 1; }
    t(`${sum}`);
}
sec();
check("for-in and ranges", "6");
