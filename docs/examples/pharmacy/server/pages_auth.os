// pharmacy/server/pages_auth.os -- /login, /logout, dashboard.
module Str = import "standard/1/String.os";
module SQ = import "standard/1/Sqlite.os";
module HS = import "standard/1/HttpServer.os";
module CR = import "standard/1/Crypto.os";
module CStr = import "standard/1/c/string.os";
module H = import "pharmacy/server/html.os";
module A = import "pharmacy/server/auth.os";
module D = import "pharmacy/server/db.os";
module FM = import "pharmacy/server/forms.os";
module F = import "pharmacy/server/fmt.os";

function loginPage(req: HS.Request*, res: HS.Response*) => void {
    let c: A.Ctx;
    if (A.loadCtx(req, &c)) { A.ctxFree(&c); res.redirect("/", 303); return; }
    A.ctxFree(&c);
    let out: Str.String;
    out.append("<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Log in - Pharmacy</title><link rel=\"stylesheet\" href=\"/static/app.css\"></head><body><div class=\"login\"><h1>Pharmacy Manager</h1>");
    H.flashOf(&out, req);
    out.append("<form method=\"post\" action=\"/login\"><label>Username<input type=\"text\" name=\"username\"></label><label>Password<input type=\"password\" name=\"password\"></label><button type=\"submit\">Log in</button></form></div></body></html>");
    res.html(out.cstr());
    out.free();
}

function loginPost(req: HS.Request*, res: HS.Response*) => void {
    let f: FM.Form;
    FM.parse(req.body.cstr(), &f);
    let db: SQ.Db* = D.handle();
    let st: SQ.Stmt;
    let good: bool = false;
    let uid: i64 = 0;
    let rank: int = 0;
    let uname: Str.String;
    db.lock();
    if (db.prepare("SELECT id, pass_hash, role, username FROM users WHERE username = ? AND active = 1", &st)) {
        st.bindText(1, f.get("username"));
        if (st.step() == SQ.ROW) {
            if (CR.verifyPassword(f.get("password"), st.colText(1))) {
                good = true;
                uid = st.colInt(0);
                rank = A.rankOf(st.colText(2));
                uname.set(st.colText(3));
            }
        }
        st.finalize();
    }
    db.unlock();
    if (good && rank > 0 && A.startSession(res, uid, rank, uname.cstr())) {
        res.redirect("/", 303);
    } else {
        H.redirectMsg(res, "/login", "err", "Invalid username or password.");
    }
    uname.free();
    f.free();
}

function logoutPost(req: HS.Request*, res: HS.Response*) => void {
    let c: A.Ctx;
    let f: FM.Form;
    if (!A.needPost(req, res, &c, &f, 1)) return;
    A.endSession(req, res);
    A.release(&c, &f);
    res.redirect("/login", 303);
}

function dashboard(req: HS.Request*, res: HS.Response*) => void {
    let c: A.Ctx;
    if (!A.need(req, res, &c, 1)) return;
    let db: SQ.Db* = D.handle();
    let out: Str.String;
    H.start(&out, req, "Dashboard", c.rank, c.username.cstr(), c.csrf.cstr());
    db.lock();
    let nDrugs: i64 = db.scalarInt("SELECT COUNT(*) FROM drugs WHERE active = 1", 0 as i64);
    let lowSql: Str.String;
    lowSql.append("SELECT COUNT(*) FROM (SELECT d.id, ");
    lowSql.append(D.ONHAND);
    lowSql.append(" AS oh, d.reorder_level AS rl FROM drugs d WHERE d.active = 1) WHERE oh <= rl");
    let nLow: i64 = db.scalarInt(lowSql.cstr(), 0 as i64);
    lowSql.free();
    let nExp: i64 = db.scalarInt("SELECT COUNT(*) FROM batches WHERE quantity > 0 AND expiry_date >= date('now') AND expiry_date <= date('now', '+30 days')", 0 as i64);
    let nExpired: i64 = db.scalarInt("SELECT COUNT(*) FROM batches WHERE quantity > 0 AND expiry_date < date('now')", 0 as i64);
    let salesToday: i64 = db.scalarInt("SELECT COALESCE(SUM(total_cents), 0) FROM sales WHERE date(created_at) = date('now', 'localtime')", 0 as i64);
    let nSalesToday: i64 = db.scalarInt("SELECT COUNT(*) FROM sales WHERE date(created_at) = date('now', 'localtime')", 0 as i64);
    db.unlock();
    out.append("<div class=\"cards\"><div class=\"card\"><b>");
    out.appendInt(nDrugs);
    out.append("</b><span>drugs in catalogue</span></div><div class=\"card warn\"><b>");
    out.appendInt(nLow);
    out.append("</b><span>low stock</span></div><div class=\"card warn\"><b>");
    out.appendInt(nExp);
    out.append("</b><span>batches expiring in 30 days</span></div><div class=\"card bad\"><b>");
    out.appendInt(nExpired);
    out.append("</b><span>expired batches with stock</span></div><div class=\"card\"><b>");
    F.money(salesToday, &out);
    out.append("</b><span>sales today (");
    out.appendInt(nSalesToday);
    out.append(")</span></div></div>");
    if (c.rank >= 2) {
        out.append("<p>");
        H.link(&out, "/reports/low-stock", "Low-stock report");
        out.append(" | ");
        H.link(&out, "/reports/expiring", "Expiring soon");
        out.append("</p>");
    }
    H.finish(&out);
    res.html(out.cstr());
    out.free();
    A.ctxFree(&c);
}
