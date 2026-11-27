// o10_raw_malloc: 10 raw malloc/free still works
module C = import "standard/1/Console.os";
let live: int = 0;
let trace: char* = "";
function t(s: char*) => void { trace = trace + s; }
function check(name: char*, want: char*) => void {
    if (trace == want && live == 0) { C.println(`ok    ${name}`); }
    else { C.println(`FAIL  ${name}: got="${trace}" want="${want}" live=${live}`); }
}
module CMem = import "standard/1/c/mem.os";
struct Pt { x: int; y: int; }
let raw: Pt* = CMem.malloc(sizeof(Pt)) as Pt*;
raw.x = 1; raw.y = 2;
t(`${raw.x + raw.y}`);
CMem.free(raw as void*);
check("10 raw malloc/free still works", "3");
