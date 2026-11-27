// o03_defer: 3 defer order
module C = import "standard/1/Console.os";
let live: int = 0;
let trace: char* = "";
function t(s: char*) => void { trace = trace + s; }
function check(name: char*, want: char*) => void {
    if (trace == want && live == 0) { C.println(`ok    ${name}`); }
    else { C.println(`FAIL  ${name}: got="${trace}" want="${want}" live=${live}`); }
}
function sec() => void {
    defer t("d1 ");
    defer t("d2 ");
    t("work ");
    if (true) { defer t("inner-d "); t("inner-work "); }
}
sec();
check("3 defer order", "work inner-work inner-d d2 d1 ");
