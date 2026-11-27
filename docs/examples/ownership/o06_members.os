// o06_members: 6 members destroyed after owner
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
struct Holder {
    first: Res; second: Res;
    constructor() { this.first = Res("f1"); this.second = Res("f2"); }
    ~destructor() { t("~Holder "); }
}
function sec() => void {
    let h = Holder();
    t("made ");
}
sec();
check("6 members destroyed after owner", "+f1 +f2 made ~Holder -f2 -f1 ");
