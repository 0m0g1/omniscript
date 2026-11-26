// browser/js/ast.os  -- syntax tree (V8's "ast/"). One generic node shape; `next` chains lists (args, statements, params...).
module U = import "util.os";

// expressions
let N_NUM: int = 1;      // n
let N_STR: int = 2;      // s
let N_TPL: int = 3;      // a = parts (N_STR nodes and expression nodes, in order)
let N_IDENT: int = 4;    // s
let N_BOOL: int = 5;     // n
let N_NULL: int = 6;
let N_UNDEF: int = 7;
let N_ARRAY: int = 8;    // a = elements
let N_OBJECT: int = 9;   // a = N_PROP list (s = key, a = value)
let N_PROP: int = 10;
let N_MEMBER: int = 11;  // a = object, s = name, op = 1 for ?.
let N_INDEX: int = 12;   // a = object, b = key expression
let N_CALL: int = 13;    // a = callee, b = args
let N_UNARY: int = 14;   // op, a
let N_UPDATE: int = 15;  // op (OP_INC/OP_DEC), a = target, n = 1.0 when prefix
let N_BINARY: int = 16;  // op, a, b
let N_LOGICAL: int = 17; // op (OP_AND/OP_OR/OP_NULLISH), a, b
let N_ASSIGN: int = 18;  // op (OP_ASSIGN or the arithmetic op for +=), a = target, b = value
let N_COND: int = 19;    // a ? b : c
let N_SEQ: int = 20;     // a , b
let N_FUNC: int = 21;    // s = name, a = params (N_IDENT list), b = body, n = 1.0 when b is an expression (arrow)
// statements
let N_VAR: int = 30;     // a = N_DECL list
let N_DECL: int = 31;    // s = name, a = init
let N_EXPR: int = 32;    // a
let N_BLOCK: int = 33;   // a = statements
let N_IF: int = 34;      // a, b, c
let N_WHILE: int = 35;   // a, b
let N_FOR: int = 36;     // a = init, b = test, c = update, d = body
let N_RETURN: int = 37;  // a
let N_BREAK: int = 38;
let N_CONTINUE: int = 39;
let N_FUNCDECL: int = 40;// s = name, a = N_FUNC
let N_EMPTY: int = 41;

// operators
let OP_NONE: int = 0;
let OP_ADD: int = 1;  let OP_SUB: int = 2;  let OP_MUL: int = 3;  let OP_DIV: int = 4;  let OP_MOD: int = 5;
let OP_LT: int = 6;   let OP_GT: int = 7;   let OP_LE: int = 8;   let OP_GE: int = 9;
let OP_EQ: int = 10;  let OP_NE: int = 11;
let OP_AND: int = 12; let OP_OR: int = 13;  let OP_NULLISH: int = 14;
let OP_ASSIGN: int = 15;
let OP_NOT: int = 16; let OP_NEG: int = 17; let OP_POS: int = 18; let OP_TYPEOF: int = 19;
let OP_INC: int = 20; let OP_DEC: int = 21;

struct Node {
    k: int = 0;
    op: int = 0;
    n: double = 0.0;
    s: char* = nullptr;
    a: Node* = nullptr;
    b: Node* = nullptr;
    c: Node* = nullptr;
    d: Node* = nullptr;
    next: Node* = nullptr;
    line: int = 0;
    file: char* = nullptr;
}

let curLine: int = 0;          // set by the parser: the position new nodes are stamped with
let curFile: char* = nullptr;

function mk(k: int) => Node* {
    let nd: Node* = U.alloc(sizeof(Node)) as Node*;
    nd.k = k; nd.op = 0; nd.n = 0.0; nd.s = nullptr;
    nd.a = nullptr; nd.b = nullptr; nd.c = nullptr; nd.d = nullptr; nd.next = nullptr;
    nd.line = curLine; nd.file = curFile;
    return nd;
}

// binary operator text -> code (0 = not a binary operator)
function binOp(s: char*) => int {
    if (U.streq(s, "+")) return 1;   if (U.streq(s, "-")) return 2;   if (U.streq(s, "*")) return 3;
    if (U.streq(s, "/")) return 4;   if (U.streq(s, "%")) return 5;   if (U.streq(s, "<")) return 6;
    if (U.streq(s, ">")) return 7;   if (U.streq(s, "<=")) return 8;  if (U.streq(s, ">=")) return 9;
    if (U.streq(s, "==")) return 10; if (U.streq(s, "!=")) return 11; if (U.streq(s, "&&")) return 12;
    if (U.streq(s, "||")) return 13; if (U.streq(s, "??")) return 14;
    return 0;
}

// precedence for precedence climbing (0 = not binary)
function prec(op: int) => int {
    if (op == 14) return 1;
    if (op == 13) return 2;
    if (op == 12) return 3;
    if (op == 10 || op == 11) return 4;
    if (op >= 6 && op <= 9) return 5;
    if (op == 1 || op == 2) return 6;
    if (op == 3 || op == 4 || op == 5) return 7;
    return 0;
}

function append(head: Node*, item: Node*) => Node* {   // add to the end of a `next` chain, returns the head
    if (head == nullptr) return item;
    let t: Node* = head;
    while (t.next != nullptr) t = t.next;
    t.next = item;
    return head;
}
