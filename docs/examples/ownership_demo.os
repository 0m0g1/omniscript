// docs\examples\ownership_demo.os
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
*/
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


/* NOT YET SUPPORTED BY THE PARSER -- uncomment when implemented
// 8. arrays: new T[n] / delete[]
*/
function sec8() => void {
    let arr: Res* = new Res[2]("z");             // [NEW] constructs 2 elements with the same argument
    t("arr ");
    delete[] arr;                                // destroys in reverse order
}
// want: +z +z arr -z -z


/* NOT YET SUPPORTED: needs the `delete` statement (parse error today) -- uncomment when implemented
// 9. freedom: aliasing is allowed, YOU decide who deletes. No lifetime checking.
*/
function sec9() => void {
    let p: Res* = new Res("shared");
    let alias: Res* = p;                         // two pointers, one object: fine
    t(`${alias.name} `);
    delete p;                                    // using `alias` after this is your problem (like C), not a compile error
}
// want: +shared shared -shared


/* NOT YET SUPPORTED: needs the `delete` statement (parse error today) -- uncomment when implemented
// 10. the one guard: copying a struct that has a destructor needs a copy constructor, otherwise the compiler refuses
*/
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


/* NOT YET SUPPORTED BY THE PARSER -- uncomment when implemented
// 11. arenas: everything allocated inside is freed at once, destructors of arena objects run   [NEW]
*/
function sec11() => void {
    arena a {
        let s: Str.String(a);
        s.append("hello "); s.append("world");
        let n: Res* = new(a) Res("in-arena");
        t(`${s.cstr()} `);
    }
}
// want: +in-arena hello world -in-arena


/* NOT YET SUPPORTED: StrView / view() do not exist (semantic error: Callable 'view' not found)
// 12. string views: slicing and trimming do not allocate   [NEW]
*/
module SV = import "standard/1/StrView.os";

function sec12() => void {
    let line: char* = "key = value";
    let v: SV.StrView = SV.view(line);
    let eq: int = v.indexOf("=");
    let key: SV.StrView = v.slice(0, eq).trim();
    let val: SV.StrView = v.slice(eq + 1, v.len).trim();
    if (key.equalsC("key") && val.equalsC("value")) t("views-ok ");
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

// 17. operator overloading on a user struct: `can <op> (params) => ret { body }` inside the struct   [NEW]
//     Operands are passed as written (no automatic &), so the parameters are pointers.
struct Num {
    v: int = 0;
    can + (o: Num*) => int { return this.v + o.v; }
    can - (o: Num*) => int { return this.v - o.v; }
    can == (o: Num*) => bool { return this.v == o.v; }
    can < (o: Num*) => bool { return this.v < o.v; }
    can += (o: Num*) => void { this.v = this.v + o.v; }
    can ! () => bool { return this.v == 0; }                       // unary: no parameters
    can - () => Num { let r: Num; r.v = 0 - this.v; return r; }    // unary minus (binary minus is above)
    can -- () => int { this.v = this.v - 1; return this.v; }       // serves a-- and --a
}
function sec17() => void {
    let a: Num;
    let b: Num;
    a.v = 2;
    b.v = 3;
    if ((a + &b) == 5) t("plus ");
    if ((b - &a) == 1) t("minus ");
    if (a == &a) t("eq ");
    if (a != &b) t("ne ");          // only == is defined: a != b is !(a == b)
    if (a < &b) t("lt ");
    a += &b;                        // calls operator+= directly (no a = a + b)
    if (a.v == 5) t("pluseq ");
    let n: int = 7;                 // primitives are untouched
    n += 1;
    if (n == 8) t("prim ");
    let p: Num* = &a;               // pointers are untouched: this is a pointer compare, not Num.operator==
    if (p != nullptr) t("ptr ");
}
// want: plus minus eq ne lt pluseq prim ptr

// 18. operators on the library types: String (== and +=) and StrView (==)   [NEW]
function sec18() => void {
    let s: Str.String;
    s += "ab";
    s += "cd";
    if (s == "abcd") t("str-eq ");
    if (s != "xyz") t("str-ne ");
    let v: SV.StrView = SV.view("  key = value ");
    let eq: int = v.indexOf("=");
    let key: SV.StrView = v.slice(0, eq).trim();
    if (key == "key") t("view-eq ");
    if (key != "val") t("view-ne ");
}
// want: str-eq str-ne view-eq view-ne

// 19. unary operators: ! - -- on a user struct (primitives untouched)   [NEW]
function sec19() => void {
    let a: Num;
    a.v = 2;
    if (!a) t("bad ");
    let z: Num;
    if (!z) t("not ");
    let neg: Num = -a;
    if (neg.v == 0 - 2) t("neg ");
    a--;
    if (a.v == 1) t("dec ");
    let k: int = 5;
    k--;
    if (k == 4) t("primdec ");
    let f: bool = false;
    if (!f) t("primnot ");
}
// want: not neg dec primdec primnot

// 20. indexing: `can []` reads, `can []=` writes; `a[i] += v` becomes a[i] = a[i] + v   [NEW]
struct Slots {
    data: int* = nullptr;
    n: int = 0;
    can [] (i: int) => int {
        if (i < 0 || i >= this.n) return 0 - 1;
        return this.data[i];
    }
    can []= (i: int, v: int) => bool {
        if (i < 0 || i >= this.n) return false;
        this.data[i] = v;
        return true;
    }
}
function sec20() => void {
    let raw: [4]int;
    let s: Slots;
    s.data = (&raw) as int*;
    s.n = 4;
    s[1] = 42;
    if (s[1] == 42) t("idx ");
    if (s[9] == 0 - 1) t("oob ");
    s[2] = s[1] + 1;
    if (s[2] == 43) t("idx2 ");
    s[3] = 1;
    s[3] += 5;
    if (s[3] == 6) t("idxadd ");
    let st: Str.String;
    st += "abc";
    if ((st[1] as int) == 98) t("str-idx ");
    st[0] = 65 as char;
    if ((st[0] as int) == 65) t("str-set ");
    let v: SV.StrView = SV.view("xyz");
    if ((v[2] as int) == 122) t("view-idx ");
}
// want: idx oob idx2 idxadd str-idx str-set view-idx

// 21. compound operator on a MEMBER: h.total += x calls Num.operator+= on the member (no a.x = a.x + b copy)   [NEW]
struct Acc { total: Num; }
function sec21() => void {
    let h: Acc;
    let d: Num;
    d.v = 4;
    h.total.v = 1;
    h.total += &d;
    if (h.total.v == 5) t("member-pluseq ");
}
// want: member-pluseq

// 22. operators inside a GENERIC struct: the methods are renamed per instance (Box<int> gets Box_int.operator...)   [NEW]
struct Box<T> {
    v: T;
    can [] (i: int) => T { return this.v; }
    can == (o: Box<T>*) => bool { return this.v == o.v; }
}
function sec22() => void {
    let a: Box<int>;
    let b: Box<int>;
    a.v = 7;
    b.v = 7;
    if (a[0] == 7) t("box-idx ");
    if (a == &b) t("box-eq ");
    b.v = 8;
    if (a != &b) t("box-ne ");      // only == is defined: a != b is !(a == b)
}
// want: box-idx box-eq box-ne

// 23. out-of-struct operator: `Type can <op> (params) => ret { body }` adds an operator to a struct declared elsewhere   [NEW]
//     Num is the struct from section 17; the method gets an implicit `this: Num*`, exactly like an in-struct operator.
Num can * (o: Num*) => int { return this.v * o.v; }
function sec23() => void {
    let a: Num;
    let b: Num;
    a.v = 3;
    b.v = 4;
    if ((a * &b) == 12) t("mul ");
    let n: int = 6;                 // primitives are untouched
    if (n * 7 == 42) t("primmul ");
}
// want: mul primmul

// 24. out-of-struct operators on a GENERIC struct: `Pair<T> can ...` is appended to the template before its first use   [NEW]
//     The type parameter must be spelled the same as in the struct declaration.
struct Pair<T> { a: T; b: T; }
Pair<T> can + (o: Pair<T>*) => T { return this.a + o.a; }
Pair<T> can == (o: Pair<T>*) => bool { return this.a == o.a && this.b == o.b; }
function sec24() => void {
    let x: Pair<int>;
    let y: Pair<int>;
    x.a = 1;
    x.b = 2;
    y.a = 10;
    y.b = 2;
    if ((x + &y) == 11) t("pair-add ");
    if (x == &x) t("pair-eq ");
    if (x != &y) t("pair-ne ");
}
// want: pair-add pair-eq pair-ne

// 25. copying a struct that has a destructor but no copy constructor is a COMPILE ERROR (two owners, double free)   [NEW]
//     Not a runtime check: uncomment to see the error. Passing a pointer, moving a temporary or defining a copy
//     constructor (section 10) are the legal ways.
// function takesRes(r: Res) => void { }
// function sec25() => void {
//     let a = Res("a");
//     takesRes(a);               // error: 'Res' has a destructor ... pass a pointer or define a copy constructor
// }

function main() => int {
    sec1();  check("1  scope exit",                "+a +b inner -b outer -a ");
    sec2();  check("2  return/continue/break",     "+r -r +r after-if -r +l body -l +l -l +l -l ");
    sec3();  check("3  defer order",               "work inner-work inner-d d2 d1 ");
    sec4();  check("4  defer on early return",     "early close(7) ");
    sec5();  check("5  return local = move",       "+m got -m ");
    sec6();  check("6  members + list",            "+f1 +f2 +e filled -e ~Holder -f2 -f1 ");
    sec7();  check("7  new / delete / defaults",   "+heap used -heap 16 ");      // needs `delete`
    sec8();  check("8  new[] / delete[]",          "+z +z arr -z -z ");     // parser: `delete[]` not supported yet
    sec9();  check("9  aliasing is allowed",       "+shared shared -shared ");      // needs `delete`
    sec10(); check("10 copy constructor",          "+buf +copy -buf size=8 -buf ");      // needs `delete`
    sec11(); check("11 arena",                     "+in-arena hello world -in-arena ");   // parser: `arena` / `new(a)` not supported yet
    sec12(); check("12 string views",              "views-ok ");      // needs StrView
    sec13(); check("13 optionals .value",          "found 9 missing default=0 no-res ");
    sec14(); check("14 lambda + interpolation",   "sum=6 twice=12 ");
    sec15(); check("15 raw malloc/free",           "3 ");
    sec16(); check("16 no free() anywhere",        "[a][b]");
    sec17(); check("17 operators on a struct",     "plus minus eq ne lt pluseq prim ptr ");
    sec18(); check("18 String/StrView operators",  "str-eq str-ne view-eq view-ne ");
    sec19(); check("19 unary operators",           "not neg dec primdec primnot ");
    sec20(); check("20 index operators",           "idx oob idx2 idxadd str-idx str-set view-idx ");
    sec21(); check("21 compound on a member",     "member-pluseq ");
    sec22(); check("22 operators in a generic",   "box-idx box-eq box-ne ");
    sec23(); check("23 out-of-struct operator",   "mul primmul ");
    sec24(); check("24 out-of-struct on generic", "pair-add pair-eq pair-ne ");
    C.println(failed == 0 ? "ALL PASS" : `${failed} FAILED`);
    return failed;
}
