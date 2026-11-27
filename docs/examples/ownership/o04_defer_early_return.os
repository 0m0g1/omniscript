// o04_defer_early_return: 4 defer on early return
module C = import "standard/1/Console.os";
let live: int = 0;
let trace: char* = "";
function t(s: char*) => void { trace = trace + s; }
function check(name: char*, want: char*) => void {
    if (trace == want && live == 0) { C.println(`ok    ${name}`); }
    else { C.println(`FAIL  ${name}: got="${trace}" want="${want}" live=${live}`); }
}
function sec() => void {
    let h: int = 7;
    defer t(`close(${h}) `);
    if (h == 7) { t("early "); return; }
    t("unreachable ");
}
sec();
check("4 defer on early return", "early close(7) ");
