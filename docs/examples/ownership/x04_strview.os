// x04_strview: StrView
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
    let line: char* = "key = value";
    let v: StrView = view(line);
    let eq: int = v.indexOf("=");
    let key: StrView = v.slice(0, eq).trim();
    if (key == "key") t("views-ok ");
}
sec();
check("StrView", "views-ok ");
