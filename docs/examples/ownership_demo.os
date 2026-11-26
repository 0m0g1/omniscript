// ownership_demo.os -- target ergonomics for OmniScript, C++ style (constructors, ~destructors, new/delete), self-checking.
//
// STATUS: PROPOSAL. Compiles only once the features exist. [NEW] marks syntax that does not exist yet.
// Every section is independent: comment out the ones you have not implemented. Each prints "ok" or "FAIL" with the trace it got.
// Philosophy: free like C. Aliasing, raw pointers and use-after-delete are allowed. The only guard is copying a struct that has
// a destructor (section 9), because that silently double-frees.
module C = import "standard/1/Console.os";
module Str = import "standard/1/String.os";
module L = import "standard/1/List.os";
module CMem = import "standard/1/c/mem.os";

// ------------------------------------------------------------ test plumbing
let live: int = 0;                 // objects alive (constructor +1, destructor -1); must be 0 after every section
let trace: Str.String;             // order of events
let failed: int = 0;

function t(s: char*) => void { trace.append(s); }
function same(a: char*, b: char*) => bool {
    let i: int = 0;
    while ((a[i] as int) == (b[i] as int)) { if ((a[i] as int) == 0) return true; i = i + 1; }
    return false;
}
function check(name: char*, want: char*) => void {
    if (same(trace.cstr(), want) && live == 0) C.println(`ok    ${name}`);
    else { failed = failed + 1; C.println(`FAIL  ${name}: got="${trace.cstr()}" want="${want}" live=${live}`); }
    trace.clear();
    live = 0;
}

// ------------------------------------------------------------ the resource used in most sections
struct Res {
    name: char*;
    constructor(n: char*) { this.name = n; live = live + 1; t(`+${n} `); }              // [NEW] constructor
    ~destructor() { live = live - 1; t(`-${this.name} `); }                            // [NEW] destructor
}

// NOTE: from the parser source: struct/class constructors are methods named `constructor(...)`, destructors `~destructor()`.
// (Whether `Res("a")` then calls `constructor` is decided in ConstructStructPrototype / StructConstructorStatement, not seen yet.)
// NOTE: destructors are written ~destructor() (the parser already requires it). Bare { } blocks are not statements: use if (true) { }.
// 1. stack objects are destroyed at scope exit, reverse order, inner block first
function sec1() => void {
    let a = Res("a");
    if (true) { let b = Res("b"); t("inner "); }
    t("outer ");
}
// want: +a +b inner -b outer -a

// 2. every exit path runs destructors: return, continue, break
function early(x: int) => int {
    let r = Res("r");
    if (x > 0) return 1;
    t("after-if ");
    return 0;
}
function sec2() => void {
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
// want: +r -r +r after-if -r +l body -l +l -l +l -l

// 3. defer: runs at scope exit, last in first out   [NEW]
function sec3() => void {
    defer t("d1 ");
    defer t("d2 ");
    t("work ");
    if (true) { defer t("inner-d "); t("inner-work "); }
}
// want: work inner-work inner-d d2 d1

// 4. defer still runs on early return (for raw handles that have no destructor)
function sec4() => void {
    let h: int = 7;
    defer t(`close(${h}) `);
    if (h == 7) { t("early "); return; }
    t("unreachable ");
}
// want: early close(7)

// 5. returning a local by value is a move: no copy, no extra destructor
function make(n: char*) => Res { let r = Res(n); return r; }
function sec5() => void { let x = make("m"); t("got "); }
// want: +m got -m

// 6. members destroy automatically after the owner's destructor; containers destroy their elements
struct Holder {
    first: Res; second: Res;
    constructor() { this.first = Res("f1"); this.second = Res("f2"); }
    ~destructor() { t("~Holder "); }
}
function sec6() => void {
    let h = Holder();
    let list: L.List<Res>;
    list.push(Res("e"));
    t("filled ");
}
// want: +f1 +f2 +e filled -e ~Holder -f2 -f1

/* NOT YET SUPPORTED: needs the `delete` statement (parse error today) -- uncomment when implemented
// 7. new / delete: constructor runs on new, destructor on delete, struct defaults run on the heap
struct Pt { x: int = 7; y: int = 9; }
function sec7() => void {
    let p: Res* = new Res("heap");               // [NEW]
    t("used ");
    delete p;                                    // [NEW] runs ~destructor(), then frees
    let q: Pt* = new Pt;                         // defaults run (today malloc gives 0)
    t(`${q.x + q.y} `);
    delete q;
}
// want: +heap used -heap 16

*/

/* NOT YET SUPPORTED BY THE PARSER -- uncomment when implemented
// 8. arrays: new T[n] / delete[]
function sec8() => void {
    let arr: Res* = new Res[2]("z");             // [NEW] constructs 2 elements with the same argument
    t("arr ");
    delete[] arr;                                // destroys in reverse order
}
// want: +z +z arr -z -z

*/

/* NOT YET SUPPORTED: needs the `delete` statement (parse error today) -- uncomment when implemented
// 9. freedom: aliasing is allowed, YOU decide who deletes. No lifetime checking.
function sec9() => void {
    let p: Res* = new Res("shared");
    let alias: Res* = p;                         // two pointers, one object: fine
    t(`${alias.name} `);
    delete p;                                    // using `alias` after this is your problem (like C), not a compile error
}
// want: +shared shared -shared

*/

/* NOT YET SUPPORTED: needs the `delete` statement (parse error today) -- uncomment when implemented
// 10. the one guard: copying a struct that has a destructor needs a copy constructor, otherwise the compiler refuses
struct Buf {
    data: u8*; size: int;
    constructor(n: int) { this.size = n; this.data = new u8[n]; live = live + 1; t("+buf "); }
    constructor(o: Buf&) { this.size = o.size; this.data = new u8[o.size]; live = live + 1; t("+copy "); }   // [NEW] copy constructor
    ~destructor() { delete this.data; live = live - 1; t("-buf "); }
}
function takes(b: Buf) => int { return b.size; }                                // by-value parameter = a copy
function sec10() => void {
    let a = Buf(8);
    let n: int = takes(a);                       // calls the copy constructor, destroys the copy after the call
    t(`size=${n} `);
}
// want: +buf +copy -buf size=8 -buf
// COMPILE-ERROR TEST (uncomment to see the message): `struct NoCopy { ~NoCopy() {} }  let x = NoCopy(); let y = x;`
//   expected: "NoCopy has a destructor: define a copy constructor, pass a pointer, or mark it [trivialCopy]"

*/

/* NOT YET SUPPORTED BY THE PARSER -- uncomment when implemented
// 11. arenas: everything allocated inside is freed at once, destructors of arena objects run   [NEW]
function sec11() => void {
    arena a {
        let s: Str.String(a);
        s.append("hello "); s.append("world");
        let n: Res* = new(a) Res("in-arena");
        t(`${s.cstr()} `);
    }
}
// want: +in-arena hello world -in-arena

*/

// 12. string views: slicing and trimming do not allocate   [NEW]
function sec12() => void {
    let line: char* = "key = value";
    let v: StrView = view(line);
    let eq: int = v.indexOf("=");
    let key: StrView = v.slice(0, eq).trim();
    let val: StrView = v.slice(eq + 1, v.len).trim();
    if (key == "key" && val == "value") t("views-ok ");
}
// want: views-ok

// 13. optionals: T? holds a value or nothing. Check, then .value   [NEW]
function find(a: int, b: int, want: int) => int? {
    if (a == want) return a;
    if (b == want) return b;
    return null;                                 // "no value"
}
function sec13() => void {
    let hit: int? = find(4, 9, 9);
    let miss: int? = find(4, 9, 5);
    if (hit != null) t(`found ${hit.value} `);
    if (miss == null) t("missing ");
    t(`default=${miss.valueOr(0)} `);            // .valueOr(d): the value, or d when empty
    let r: Res*? = null;                         // optional pointer
    if (r == null) t("no-res ");
}
// want: found 9 missing default=0 no-res

// 14. lambdas + interpolation (for-in and ranges are NOT supported yet: `for (n in x)` and `0..3` are parse errors)
function sec14() => void {
    let sum: int = 0;
    for (let i: int = 1; i < 4; i = i + 1) { sum = sum + i; }
    let twice = function(x: int) => int { return x * 2; };
    t(`sum=${sum} twice=${twice(sum)} `);
}
// want: sum=6 twice=12
// TODO (language): for (n in list) { }   and   for (i in 0..3) { }

// 15. raw memory stays available, no keyword required (C style)
function sec15() => void {
    let raw: Pt* = CMem.malloc(sizeof(Pt)) as Pt*;
    raw.x = 1; raw.y = 2;
    t(`${raw.x + raw.y} `);
    CMem.free(raw as void*);
}
// want: 3

// 16. a normal function: no free() anywhere, string returned and cleaned up by its destructor
function describe(a: char*, b: char*) => Str.String {
    let out: Str.String;
    out.append(`[${a}]`);
    out.append(`[${b}]`);
    return out;
}
function sec16() => void {
    let text = describe("a", "b");
    t(text.cstr());
}
// want: [a][b]

function main() => int {
    sec1();  check("1  scope exit",                "+a +b inner -b outer -a ");
    sec2();  check("2  return/continue/break",     "+r -r +r after-if -r +l body -l +l -l +l -l ");
    sec3();  check("3  defer order",               "work inner-work inner-d d2 d1 ");
    sec4();  check("4  defer on early return",     "early close(7) ");
    sec5();  check("5  return local = move",       "+m got -m ");
    sec6();  check("6  members + list",            "+f1 +f2 +e filled -e ~Holder -f2 -f1 ");
    // sec7();  check("7  new / delete / defaults",   "+heap used -heap 16 ");      // needs `delete`
    // sec8();  check("8  new[] / delete[]",          "+z +z arr -z -z ");     // parser: `delete[]` not supported yet
    // sec9();  check("9  aliasing is allowed",       "+shared shared -shared ");      // needs `delete`
    // sec10(); check("10 copy constructor",          "+buf +copy -buf size=8 -buf ");      // needs `delete`
    // sec11(); check("11 arena",                     "+in-arena hello world -in-arena ");   // parser: `arena` / `new(a)` not supported yet
    sec12(); check("12 string views",              "views-ok ");
    sec13(); check("13 optionals .value",          "found 9 missing default=0 no-res ");
    sec14(); check("14 lambda + interpolation",   "sum=6 twice=12 ");
    sec15(); check("15 raw malloc/free",           "3 ");
    sec16(); check("16 no free() anywhere",        "[a][b]");
    C.println(failed == 0 ? "ALL PASS" : `${failed} FAILED`);
    return failed;
}
