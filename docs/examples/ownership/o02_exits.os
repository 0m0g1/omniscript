// o02_exits: 2 return/continue/break
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
function early(x: int) => int {
    let r = Res("r");
    if (x > 0) return 1;
    t("after-if ");
    return 0;
}
function sec() => void {
    early(1); early(0);
    let i: int = 0;
    while (i < 3) {
        let l = Res("l");
        i = i + 1;
        if (i == 2) continue;
        if (i == 3) break;
        t("body ");
    }
}
sec();
check("2 return/continue/break", "+r -r +r after-if -r +l body -l +l -l +l -l ");
