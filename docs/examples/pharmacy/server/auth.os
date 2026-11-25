// pharmacy/server/auth.os -- password hashing, sessions (in memory), CSRF tokens, role checks.
module Str = import "standard/1/String.os";
module HS = import "standard/1/HttpServer.os";
module L = import "standard/1/List.os";
module Th = import "standard/1/Thread.os";
module Time = import "standard/1/Time.os";
module Text = import "standard/1/Text.os";
module CR = import "standard/1/Crypto.os";
module CStr = import "standard/1/c/string.os";
module CFG = import "pharmacy/server/config.os";
module D = import "pharmacy/server/db.os";
module FM = import "pharmacy/server/forms.os";
module SQ = import "standard/1/Sqlite.os";

// ---- passwords: same format as Crypto.hashPassword, but with our own iteration count ----
function hashPw(password: char*, out: Str.String*) => bool {
    let salt: [16]u8;
    let dk: [32]u8;
    if (!CR.randomBytes(&salt as u8*, 16)) return false;
    if (!CR.pbkdf2Sha256(password, &salt as u8*, 16, CFG.PW_ITER, &dk as u8*, 32)) return false;
    let ok: bool = out.append("pbkdf2-sha256$");
    if (ok) ok = out.appendInt(CFG.PW_ITER as i64);
    if (ok) ok = out.appendChar(36 as char);
    if (ok) ok = CR.base64Encode(&salt as u8*, 16, out);
    if (ok) ok = out.appendChar(36 as char);
    if (ok) ok = CR.base64Encode(&dk as u8*, 32, out);
    return ok;
}

function addUser(username: char*, fullName: char*, password: char*, role: char*) => bool {
    let h: Str.String;
    let ok: bool = hashPw(password, &h);
    let st: SQ.Stmt;
    let db: SQ.Db* = D.handle();
    if (ok) {
        ok = false;
        if (db.prepare("INSERT INTO users(username, pass_hash, full_name, role) VALUES(?, ?, ?, ?)", &st)) {
            st.bindText(1, username); st.bindText(2, h.cstr()); st.bindText(3, fullName); st.bindText(4, role);
            ok = st.step() == SQ.DONE;
            st.finalize();
        }
    }
    h.free();
    return ok;
}

// first run: demo accounts (change these passwords!)
function seedUsers() => void {
    let db: SQ.Db* = D.handle();
    if (db.scalarInt("SELECT COUNT(*) FROM users", 0 as i64) == 0) {
        addUser("admin", "Administrator", "admin123", "admin");
        addUser("pharmacist", "Demo Pharmacist", "pharm123", "pharmacist");
        addUser("cashier", "Demo Cashier", "cashier123", "cashier");
    }
}

function rankOf(role: char*) => int {
    let r: int = 0;
    if (CStr.strcmp(role, "admin") == 0) r = 3;
    else if (CStr.strcmp(role, "pharmacist") == 0) r = 2;
    else if (CStr.strcmp(role, "cashier") == 0) r = 1;
    return r;
}

// ---- sessions ----
struct Session {
    token: Str.String; username: Str.String; csrf: Str.String;
    userId: i64 = 0; rank: int = 0; expires: i64 = 0;
}

let sessions: L.List<Session>;
let smu: Th.Mutex;

function init() => void { smu.init(); }

function randomHex(nbytes: u64, out: Str.String*) => bool {
    let b: [32]u8;
    if (nbytes > 32) return false;
    if (!CR.randomBytes(&b as u8*, nbytes)) return false;
    return Text.hexEncode(&b as u8*, nbytes, out);
}

// per-request view of the logged-in user (copied out of the session table so no pointer into it is kept)
struct Ctx {
    ok: bool = false;
    userId: i64 = 0; rank: int = 0;
    username: Str.String; csrf: Str.String; token: Str.String;
}

function ctxFree(c: Ctx*) => void { c.username.free(); c.csrf.free(); c.token.free(); }

function cookieToken(req: HS.Request*, out: Str.String*) => bool {
    let h: char* = req.header("cookie");
    if (h == nullptr) return false;
    let p: char* = CStr.strstr(h, "sid=");
    let good: bool = p != nullptr;
    if (good && (p as u64) > (h as u64)) {
        let prev: int = p[0 - 1] as int;
        if (prev != 32 && prev != 59) good = false;
    }
    if (!good) return false;
    let i: u64 = 4;
    while ((p[i] as int) != 0 && (p[i] as int) != 59 && (p[i] as int) != 32) i = i + 1;
    out.appendN(&p[4], i - 4);
    return out.len > 0;
}

function purgeExpired(now: i64) => void {
    let i: u64 = 0;
    while (i < sessions.len) {
        if (sessions.data[i].expires < now) {
            sessions.data[i].token.free(); sessions.data[i].username.free(); sessions.data[i].csrf.free();
            sessions.removeAt(i);
        } else { i = i + 1; }
    }
}

function loadCtx(req: HS.Request*, c: Ctx*) => bool {
    if (!cookieToken(req, &c.token)) return false;
    let now: i64 = Time.unixSeconds();
    smu.lock();
    let i: u64 = 0;
    while (i < sessions.len && !c.ok) {
        if (sessions.data[i].token.equals(&c.token) && sessions.data[i].expires >= now) {
            c.ok = true;
            c.userId = sessions.data[i].userId;
            c.rank = sessions.data[i].rank;
            c.username.set(sessions.data[i].username.cstr());
            c.csrf.set(sessions.data[i].csrf.cstr());
            sessions.data[i].expires = now + CFG.SESSION_SECONDS;
        }
        i = i + 1;
    }
    smu.unlock();
    return c.ok;
}

function startSession(res: HS.Response*, userId: i64, rank: int, username: char*) => bool {
    let s: Session;
    if (!randomHex(24, &s.token)) return false;
    if (!randomHex(16, &s.csrf)) return false;
    s.username.set(username);
    s.userId = userId; s.rank = rank;
    let now: i64 = Time.unixSeconds();
    s.expires = now + CFG.SESSION_SECONDS;
    let cookie: Str.String;
    cookie.append("sid=");
    cookie.append(s.token.cstr());
    cookie.append("; Path=/; HttpOnly; SameSite=Lax");
    smu.lock();
    purgeExpired(now);
    sessions.push(s);
    smu.unlock();
    res.header("Set-Cookie", cookie.cstr());
    cookie.free();
    return true;
}

function endSession(req: HS.Request*, res: HS.Response*) => void {
    let tok: Str.String;
    if (cookieToken(req, &tok)) {
        smu.lock();
        let i: u64 = 0;
        let gone: bool = false;
        while (i < sessions.len && !gone) {
            if (sessions.data[i].token.equals(&tok)) {
                sessions.data[i].token.free(); sessions.data[i].username.free(); sessions.data[i].csrf.free();
                sessions.removeAt(i);
                gone = true;
            }
            i = i + 1;
        }
        smu.unlock();
    }
    tok.free();
    res.header("Set-Cookie", "sid=; Path=/; HttpOnly; Max-Age=0");
}

function forbidden(res: HS.Response*) => void {
    res.status = 403;
    res.html("<!DOCTYPE html><html><body style=\"font-family:sans-serif\"><h1>403 Forbidden</h1><p>Your account may not do this.</p><p><a href=\"/\">Back</a></p></body></html>");
}

// Logged in with at least minRank? Otherwise answers (redirect to /login, or 403), frees c and returns false.
function need(req: HS.Request*, res: HS.Response*, c: Ctx*, minRank: int) => bool {
    if (!loadCtx(req, c)) {
        res.redirect("/login", 303);
        ctxFree(c);
        return false;
    }
    if (c.rank < minRank) {
        forbidden(res);
        ctxFree(c);
        return false;
    }
    return true;
}

// POST version: also parses the body into f and checks the CSRF token. Frees c and f on failure.
function needPost(req: HS.Request*, res: HS.Response*, c: Ctx*, f: FM.Form*, minRank: int) => bool {
    if (!need(req, res, c, minRank)) return false;
    FM.parse(req.body.cstr(), f);
    if (CStr.strcmp(f.get("csrf"), c.csrf.cstr()) != 0 || c.csrf.len == 0) {
        res.status = 403;
        res.text("403 Forbidden: bad or missing form token. Go back, reload the page and try again.");
        f.free();
        ctxFree(c);
        return false;
    }
    return true;
}

function release(c: Ctx*, f: FM.Form*) => void { f.free(); ctxFree(c); }
