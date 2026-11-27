// x05_optionals: optionals .value / .valueOr
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
function find(a: int, b: int, want: int) => int? {
    if (a == want) return a;
    if (b == want) return b;
    return null;
}
let hit: int? = find(4, 9, 9);
let miss: int? = find(4, 9, 5);
if (hit != null) t(`found ${hit.value} `);
if (miss == null) t("missing ");
t(`default=${miss.valueOr(0)}`);
check("optionals .value / .valueOr", "found 9 missing default=0");
