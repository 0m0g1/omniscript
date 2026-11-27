// o07_global_and_temp: 7 temporary destroyed at end of statement
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
function use(r: Res*) => void { t("use "); }
function sec() => void {
    use(Res("tmp"));          // temporary: destroyed at the end of the statement
    t("after ");
}
sec();
check("7 temporary destroyed at end of statement", "+tmp use -tmp after ");
