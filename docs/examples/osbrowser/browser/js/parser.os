// browser/js/parser.os  -- tokens -> AST (V8's "parsing/parser"). Recursive descent + precedence climbing.
// Supported: var/let/const, function decls/exprs, arrow functions, if/else, while, for(;;), return/break/continue,
// object/array/template literals, member/index/call chains, ?. (member only), ?:, ??, ++/--, comma operator.
// NEXT: for-of/for-in, try/catch/throw, switch, new, spread, destructuring, classes, regex literals.
module U = import "util.os";
module Dbg = import "dbg.os";
module SC = import "js/scanner.os";
module AST = import "js/ast.os";

let toks: SC.Tok* = nullptr;
let ntok: int = 0;
let pos: int = 0;
let failed: bool = false;
let errPos: int = 0;
let errCount: int = 0;
let mainToks: SC.Tok* = nullptr;
let mainSrc: char* = nullptr;

function flat(t: char*) => char* {                   // newlines/tabs -> spaces so the Console line stays on one row
    let n: int = U.slen(t);
    let o: char* = U.newStr(n + 1);
    let i: int = 0;
    while (i < n) { let c: int = t[i] as int; if (c < 32) c = 32; o[i] = c as char; i = i + 1; }
    o[n] = 0 as char;
    return o;
}

function where() => char* {                          // " [file:line] ...source around the token..."
    if (toks != mainToks || mainSrc == nullptr) return "";
    let off: int = toks[pos].off;
    let a: int = off - 40;
    let b: int = off + 30;
    let n: int = U.slen(mainSrc);
    if (a < 0) a = 0;
    if (b > n) b = n;
    let loc: char* = U.cat(" [", U.cat(toks[pos].file, U.cat(":", U.cat(U.fromInt(toks[pos].line), "]"))));
    return U.cat(loc, U.cat(" ...", U.cat(flat(U.sub(mainSrc, a, b)), "...")));
}

function fail(msg: char*) => void {
    if (!failed) {
        if (errCount < 8) Dbg.error(U.cat("js parse: ", U.cat(msg, U.cat(" near '", U.cat(toks[pos].s, U.cat("'", where()))))));
        errCount = errCount + 1;
        errPos = 0;
        if (toks == mainToks) errPos = pos;           // an error inside a template's inner source has no main-stream position
    }
    failed = true;
    pos = ntok - 1;
}

function isP(s: char*) => bool { return toks[pos].t == SC.T_PUNCT && U.streq(toks[pos].s, s); }
function isKw(s: char*) => bool { return toks[pos].t == SC.T_ID && U.streq(toks[pos].s, s); }
function eatP(s: char*) => bool { if (isP(s)) { pos = pos + 1; return true; } return false; }
function expectP(s: char*) => void { if (!eatP(s)) fail(U.cat("expected ", s)); }
function atEnd() => bool { return toks[pos].t == SC.T_EOF; }

// ---------- template literals: `a${expr}b` -> N_TPL(parts) ----------
function parseTemplate(raw: char*) => AST.Node* {
    let tpl: AST.Node* = AST.mk(AST.N_TPL);
    let n: int = U.slen(raw);
    let i: int = 0;
    let textStart: int = 0;
    while (i <= n) {
        let atExpr: bool = i + 1 < n && (raw[i] as int) == 36 && (raw[i + 1] as int) == 123;
        if (i == n || atExpr) {
            if (i > textStart) {
                let lit: AST.Node* = AST.mk(AST.N_STR);
                lit.s = SC.unescape(raw, textStart, i);
                tpl.a = AST.append(tpl.a, lit);
            }
            if (atExpr) {
                let depth: int = 1;
                let j: int = i + 2;
                while (j < n && depth > 0) {
                    let c: int = raw[j] as int;
                    if (c == 123) depth = depth + 1;
                    else if (c == 125) depth = depth - 1;
                    if (depth > 0) j = j + 1;
                }
                let inner: char* = U.sub(raw, i + 2, j);
                // re-enter the parser on the inner source, then restore our token state
                let sToks: SC.Tok* = toks; let sN: int = ntok; let sPos: int = pos;
                toks = SC.scan(inner); ntok = SC.count(); pos = 0;
                let e: AST.Node* = parseExpression();
                toks = sToks; ntok = sN; pos = sPos;
                tpl.a = AST.append(tpl.a, e);
                i = j + 1;
                textStart = i;
            }
            else { i = i + 1; }
        }
        else { i = i + 1; }
    }
    return tpl;
}

// ---------- expressions ----------
function parseParams() => AST.Node* {                  // after '(' ... consumes ')'. `a = expr` keeps the default in p.a
    let head: AST.Node* = nullptr;
    while (!isP(")") && !atEnd()) {
        let p: AST.Node* = AST.mk(AST.N_IDENT);
        if (toks[pos].t != SC.T_ID) fail("unsupported parameter (destructuring/rest)");
        p.s = toks[pos].s;
        pos = pos + 1;
        if (eatP("=")) p.a = parseAssign();
        head = AST.append(head, p);
        if (!eatP(",")) { if (!isP(")")) fail("bad parameter list"); }
    }
    expectP(")");
    return head;
}

function parseArrowBody(fn: AST.Node*) => void {
    if (isP("{")) { fn.b = parseBlock(); fn.n = 0.0; }
    else { fn.b = parseAssign(); fn.n = 1.0; }
}

function isArrowAhead() => bool {                      // at '(' : does the matching ')' continue with '=>' ?
    let depth: int = 0;
    let i: int = pos;
    let r: bool = false;
    let go: bool = true;
    while (go && toks[i].t != SC.T_EOF) {
        if (toks[i].t == SC.T_PUNCT) {
            if (U.streq(toks[i].s, "(")) depth = depth + 1;
            else if (U.streq(toks[i].s, ")")) {
                depth = depth - 1;
                if (depth == 0) {
                    go = false;
                    r = toks[i + 1].t == SC.T_PUNCT && U.streq(toks[i + 1].s, "=>");
                }
            }
        }
        i = i + 1;
    }
    return r;
}

function parseFunctionRest(name: char*) => AST.Node* {   // after the optional name: (params) { body }
    let fn: AST.Node* = AST.mk(AST.N_FUNC);
    fn.s = name;
    expectP("(");
    fn.a = parseParams();
    fn.b = parseBlock();
    fn.n = 0.0;
    return fn;
}

function parseObject() => AST.Node* {                  // after '{'
    let o: AST.Node* = AST.mk(AST.N_OBJECT);
    while (!isP("}") && !atEnd()) {
        let pr: AST.Node* = AST.mk(AST.N_PROP);
        let t: SC.Tok* = &toks[pos];
        if (t.t == SC.T_ID || t.t == SC.T_STR) pr.s = t.s;
        else if (t.t == SC.T_NUM) pr.s = U.fromNum(t.n);
        else fail("bad object key");
        pos = pos + 1;
        if (eatP(":")) pr.a = parseAssign();
        else { let id: AST.Node* = AST.mk(AST.N_IDENT); id.s = pr.s; pr.a = id; }   // shorthand {a}
        o.a = AST.append(o.a, pr);
        if (!eatP(",")) { if (!isP("}")) fail("expected , or }"); }
    }
    expectP("}");
    return o;
}

function stamp() => void { if (toks == mainToks) { AST.curLine = toks[pos].line; AST.curFile = toks[pos].file; } }

function parsePrimary() => AST.Node* {
    stamp();
    let t: SC.Tok* = &toks[pos];
    let r: AST.Node* = nullptr;
    if (t.t == SC.T_NUM) { r = AST.mk(AST.N_NUM); r.n = t.n; pos = pos + 1; }
    else if (t.t == SC.T_STR) { r = AST.mk(AST.N_STR); r.s = t.s; pos = pos + 1; }
    else if (t.t == SC.T_TPL) { pos = pos + 1; r = parseTemplate(t.s); }
    else if (t.t == SC.T_ID) {
        let name: char* = t.s;
        pos = pos + 1;
        if (U.streq(name, "true")) { r = AST.mk(AST.N_BOOL); r.n = 1.0; }
        else if (U.streq(name, "false")) { r = AST.mk(AST.N_BOOL); r.n = 0.0; }
        else if (U.streq(name, "null")) r = AST.mk(AST.N_NULL);
        else if (U.streq(name, "undefined")) r = AST.mk(AST.N_UNDEF);
        else if (U.streq(name, "function")) {
            let fname: char* = "";
            if (toks[pos].t == SC.T_ID) { fname = toks[pos].s; pos = pos + 1; }
            r = parseFunctionRest(fname);
        }
        else { r = AST.mk(AST.N_IDENT); r.s = name; }
    }
    else if (isP("(")) { pos = pos + 1; r = parseExpression(); expectP(")"); }
    else if (isP("[")) {
        pos = pos + 1;
        r = AST.mk(AST.N_ARRAY);
        while (!isP("]") && !atEnd()) {
            r.a = AST.append(r.a, parseAssign());
            if (!eatP(",")) { if (!isP("]")) fail("expected , or ]"); }
        }
        expectP("]");
    }
    else if (isP("{")) { pos = pos + 1; r = parseObject(); }
    else { fail("unexpected token"); r = AST.mk(AST.N_UNDEF); }
    return r;
}

function parseCallTail(base: AST.Node*) => AST.Node* {   // member / index / call chain
    let e: AST.Node* = base;
    let more: bool = true;
    while (more && !failed) {
        if (isP(".") || isP("?.")) {
            let opt: bool = isP("?.");
            pos = pos + 1;
            let m: AST.Node* = AST.mk(AST.N_MEMBER);
            m.a = e;
            m.s = toks[pos].s;
            if (opt) m.op = 1;
            pos = pos + 1;
            e = m;
        }
        else if (isP("[")) {
            pos = pos + 1;
            let ix: AST.Node* = AST.mk(AST.N_INDEX);
            ix.a = e;
            ix.b = parseExpression();
            expectP("]");
            e = ix;
        }
        else if (isP("(")) {
            pos = pos + 1;
            let c: AST.Node* = AST.mk(AST.N_CALL);
            c.a = e;
            while (!isP(")") && !atEnd()) {
                c.b = AST.append(c.b, parseAssign());
                if (!eatP(",")) { if (!isP(")")) fail("expected , or )"); }
            }
            expectP(")");
            e = c;
        }
        else more = false;
    }
    return e;
}

function parseUnary() => AST.Node* {
    let r: AST.Node* = nullptr;
    if (isP("!") || isP("-") || isP("+")) {
        let u: AST.Node* = AST.mk(AST.N_UNARY);
        if (isP("!")) u.op = AST.OP_NOT; else if (isP("-")) u.op = AST.OP_NEG; else u.op = AST.OP_POS;
        pos = pos + 1;
        u.a = parseUnary();
        r = u;
    }
    else if (isKw("typeof")) {
        pos = pos + 1;
        let u: AST.Node* = AST.mk(AST.N_UNARY);
        u.op = AST.OP_TYPEOF;
        u.a = parseUnary();
        r = u;
    }
    else if (isP("++") || isP("--")) {
        let u: AST.Node* = AST.mk(AST.N_UPDATE);
        if (isP("++")) u.op = AST.OP_INC; else u.op = AST.OP_DEC;
        u.n = 1.0;
        pos = pos + 1;
        u.a = parseUnary();
        r = u;
    }
    else {
        r = parseCallTail(parsePrimary());
        if (isP("++") || isP("--")) {
            let u: AST.Node* = AST.mk(AST.N_UPDATE);
            if (isP("++")) u.op = AST.OP_INC; else u.op = AST.OP_DEC;
            u.n = 0.0;
            pos = pos + 1;
            u.a = r;
            r = u;
        }
    }
    return r;
}

function parseBinary(minPrec: int) => AST.Node* {
    let left: AST.Node* = parseUnary();
    let go: bool = true;
    while (go && !failed) {
        let op: int = 0;
        if (toks[pos].t == SC.T_PUNCT) op = AST.binOp(toks[pos].s);
        let p: int = AST.prec(op);
        if (p == 0 || p < minPrec) go = false;
        else {
            pos = pos + 1;
            let right: AST.Node* = parseBinary(p + 1);
            let nd: AST.Node* = AST.mk(AST.N_BINARY);
            if (op == AST.OP_AND || op == AST.OP_OR || op == AST.OP_NULLISH) nd.k = AST.N_LOGICAL;
            nd.op = op; nd.a = left; nd.b = right;
            left = nd;
        }
    }
    return left;
}

function parseCond() => AST.Node* {
    let c: AST.Node* = parseBinary(1);
    if (isP("?")) {
        pos = pos + 1;
        let nd: AST.Node* = AST.mk(AST.N_COND);
        nd.a = c;
        nd.b = parseAssign();
        expectP(":");
        nd.c = parseAssign();
        c = nd;
    }
    return c;
}

function assignOp() => int {                           // current token as an assignment operator, else -1
    if (toks[pos].t != SC.T_PUNCT) return 0 - 1;
    let s: char* = toks[pos].s;
    if (U.streq(s, "=")) return AST.OP_ASSIGN;
    if (U.streq(s, "+=")) return AST.OP_ADD;
    if (U.streq(s, "-=")) return AST.OP_SUB;
    if (U.streq(s, "*=")) return AST.OP_MUL;
    if (U.streq(s, "/=")) return AST.OP_DIV;
    return 0 - 1;
}

function parseAssign() => AST.Node* {
    // arrow functions: `x => ...`, `() => ...`, `(a, b) => ...`
    if (toks[pos].t == SC.T_ID && toks[pos + 1].t == SC.T_PUNCT && U.streq(toks[pos + 1].s, "=>")) {
        let fn: AST.Node* = AST.mk(AST.N_FUNC);
        fn.s = "";
        let p: AST.Node* = AST.mk(AST.N_IDENT);
        p.s = toks[pos].s;
        fn.a = p;
        pos = pos + 2;
        parseArrowBody(fn);
        return fn;
    }
    if (isP("(") && isArrowAhead()) {
        let fn: AST.Node* = AST.mk(AST.N_FUNC);
        fn.s = "";
        pos = pos + 1;
        fn.a = parseParams();
        expectP("=>");
        parseArrowBody(fn);
        return fn;
    }
    let left: AST.Node* = parseCond();
    let op: int = assignOp();
    if (op >= 0) {
        if (left.k != AST.N_IDENT && left.k != AST.N_MEMBER && left.k != AST.N_INDEX) { fail("invalid assignment target"); return left; }
        pos = pos + 1;
        let nd: AST.Node* = AST.mk(AST.N_ASSIGN);
        nd.op = op; nd.a = left; nd.b = parseAssign();
        return nd;
    }
    return left;
}

function parseExpression() => AST.Node* {              // includes the comma operator
    let e: AST.Node* = parseAssign();
    while (isP(",") && !failed) {
        pos = pos + 1;
        let s: AST.Node* = AST.mk(AST.N_SEQ);
        s.a = e;
        s.b = parseAssign();
        e = s;
    }
    return e;
}

// ---------- statements ----------
function endStmt() => void { eatP(";"); }

function parseBlock() => AST.Node* {
    let b: AST.Node* = AST.mk(AST.N_BLOCK);
    expectP("{");
    while (!isP("}") && !atEnd() && !failed) b.a = AST.append(b.a, parseStatement());
    expectP("}");
    return b;
}

function parseVarDecls() => AST.Node* {                // at the keyword; leaves the terminator to the caller
    let v: AST.Node* = AST.mk(AST.N_VAR);
    pos = pos + 1;
    let more: bool = true;
    while (more && !failed) {
        let d: AST.Node* = AST.mk(AST.N_DECL);
        if (toks[pos].t != SC.T_ID) fail("expected a variable name");
        d.s = toks[pos].s;
        pos = pos + 1;
        if (eatP("=")) d.a = parseAssign();
        v.a = AST.append(v.a, d);
        more = eatP(",");
    }
    return v;
}

function parseStatement() => AST.Node* {
    stamp();
    let r: AST.Node* = nullptr;
    if (isP("{")) r = parseBlock();
    else if (isP(";")) { pos = pos + 1; r = AST.mk(AST.N_EMPTY); }
    else if (isKw("let") || isKw("const") || isKw("var")) { r = parseVarDecls(); endStmt(); }
    else if (isKw("function") && toks[pos + 1].t == SC.T_ID) {
        pos = pos + 1;
        let name: char* = toks[pos].s;
        pos = pos + 1;
        r = AST.mk(AST.N_FUNCDECL);
        r.s = name;
        r.a = parseFunctionRest(name);
    }
    else if (isKw("if")) {
        pos = pos + 1;
        r = AST.mk(AST.N_IF);
        expectP("(");
        r.a = parseExpression();
        expectP(")");
        r.b = parseStatement();
        if (isKw("else")) { pos = pos + 1; r.c = parseStatement(); }
    }
    else if (isKw("while")) {
        pos = pos + 1;
        r = AST.mk(AST.N_WHILE);
        expectP("(");
        r.a = parseExpression();
        expectP(")");
        r.b = parseStatement();
    }
    else if (isKw("for")) {
        pos = pos + 1;
        r = AST.mk(AST.N_FOR);
        expectP("(");
        if (isKw("let") || isKw("const") || isKw("var")) r.a = parseVarDecls();
        else if (!isP(";")) { let ex: AST.Node* = AST.mk(AST.N_EXPR); ex.a = parseExpression(); r.a = ex; }
        expectP(";");
        if (!isP(";")) r.b = parseExpression();
        expectP(";");
        if (!isP(")")) r.c = parseExpression();
        expectP(")");
        r.d = parseStatement();
    }
    else if (isKw("return")) {
        pos = pos + 1;
        r = AST.mk(AST.N_RETURN);
        if (!isP(";") && !isP("}") && !atEnd()) r.a = parseExpression();
        endStmt();
    }
    else if (isKw("break")) { pos = pos + 1; r = AST.mk(AST.N_BREAK); endStmt(); }
    else if (isKw("continue")) { pos = pos + 1; r = AST.mk(AST.N_CONTINUE); endStmt(); }
    else {
        r = AST.mk(AST.N_EXPR);
        r.a = parseExpression();
        endStmt();
    }
    return r;
}

// After a syntax error: resume after the end of the broken top-level statement (a `;` or `}` back at depth 0).
function recover(start: int) => void {
    let depth: int = 0;
    let i: int = start;
    let resume: int = 0 - 1;
    while (resume < 0 && toks[i].t != SC.T_EOF) {
        if (toks[i].t == SC.T_PUNCT) {
            let t: char* = toks[i].s;
            if (U.streq(t, "{") || U.streq(t, "(") || U.streq(t, "[")) depth = depth + 1;
            else if (U.streq(t, "}") || U.streq(t, ")") || U.streq(t, "]")) {
                depth = depth - 1;
                if (depth <= 0 && U.streq(t, "}") && i >= errPos) resume = i + 1;
            }
            else if (U.streq(t, ";") && depth <= 0 && i >= errPos) resume = i + 1;
        }
        i = i + 1;
    }
    if (resume < 0) resume = ntok - 1;
    pos = resume;
    failed = false;
}

// Whole program -> N_BLOCK. Broken statements are reported and skipped; everything else still runs.
function parse(src: char*) => AST.Node* {
    mainSrc = src;
    toks = SC.scan(src);
    mainToks = toks;
    ntok = SC.count();
    pos = 0;
    failed = false;
    errCount = 0;
    let prog: AST.Node* = AST.mk(AST.N_BLOCK);
    let guard: int = 0;
    while (!atEnd() && guard < 100000) {
        let start: int = pos;
        let st: AST.Node* = parseStatement();
        if (failed) { recover(start); if (pos <= start) pos = start + 1; }
        else prog.a = AST.append(prog.a, st);
        guard = guard + 1;
    }
    return prog;
}

function hadError() => bool { return failed; }
