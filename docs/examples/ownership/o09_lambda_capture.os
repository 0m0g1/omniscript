// o09_lambda_capture: 9 closure captures a local
module C = import "standard/1/Console.os";
let live: int = 0;
let trace: char* = "";
function t(s: char*) => void { trace = trace + s; }
function check(name: char*, want: char*) => void {
    if (trace == want && live == 0) { C.println(`ok    ${name}`); }
    else { C.println(`FAIL  ${name}: got="${trace}" want="${want}" live=${live}`); }
}
function make() => fn(int) => int {
    let k: int = 10;
    return function(x: int) => int { return x + k; };
}
let f = make();
t(`${f(1)}`);
check("9 closure captures a local", "11");
