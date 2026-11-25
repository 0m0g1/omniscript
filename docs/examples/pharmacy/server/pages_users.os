// pharmacy/server/pages_users.os -- admin: list users, add a user, enable/disable.
module Str = import "standard/1/String.os";
module SQ = import "standard/1/Sqlite.os";
module HS = import "standard/1/HttpServer.os";
module CStr = import "standard/1/c/string.os";
module H = import "pharmacy/server/html.os";
module A = import "pharmacy/server/auth.os";
module D = import "pharmacy/server/db.os";
module FM = import "pharmacy/server/forms.os";

function usersPage(req: HS.Request*, res: HS.Response*) => void {
    let c: A.Ctx;
    if (!A.need(req, res, &c, 3)) return;
    let db: SQ.Db* = D.handle();
    let st: SQ.Stmt;
    let out: Str.String;
    H.start(&out, req, "Users", c.rank, c.username.cstr(), c.csrf.cstr());
    out.append("<table><tr><th>Username</th><th>Name</th><th>Role</th><th>Status</th><th></th></tr>");
    db.lock();
    if (db.prepare("SELECT id, username, full_name, role, active FROM users ORDER BY username", &st)) {
        while (st.step() == SQ.ROW) {
            out.append("<tr>"); H.td(&out, st.colText(1)); H.td(&out, st.colText(2)); H.td(&out, st.colText(3));
            if (st.colInt(4) == 1) H.td(&out, "active"); else H.td(&out, "disabled");
            out.append("<td><form method=\"post\" action=\"/users/"); out.appendInt(st.colInt(0)); out.append("/toggle\">");
            H.csrf(&out, c.csrf.cstr());
            out.append("<button type=\"submit\">");
            if (st.colInt(4) == 1) out.append("Disable"); else out.append("Enable");
            out.append("</button></form></td></tr>");
        }
        st.finalize();
    }
    db.unlock();
    out.append("</table><h2>Add user</h2><form class=\"form\" method=\"post\" action=\"/users/new\">");
    H.csrf(&out, c.csrf.cstr());
    out.append("<label>Username<input type=\"text\" name=\"username\"></label><label>Full name<input type=\"text\" name=\"full\"></label>");
    out.append("<label>Password (at least 8 characters)<input type=\"password\" name=\"password\"></label>");
    out.append("<label>Role<select name=\"role\"><option value=\"cashier\">cashier</option><option value=\"pharmacist\">pharmacist</option><option value=\"admin\">admin</option></select></label>");
    out.append("<button type=\"submit\">Create</button></form>");
    H.finish(&out);
    res.html(out.cstr());
    out.free();
    A.ctxFree(&c);
}

function userNewPost(req: HS.Request*, res: HS.Response*) => void {
    let c: A.Ctx;
    let f: FM.Form;
    if (!A.needPost(req, res, &c, &f, 3)) return;
    let db: SQ.Db* = D.handle();
    let err: char* = nullptr;
    if (!H.has(f.get("username")) || CStr.strlen(f.get("username")) > 40) err = "Username is required (40 characters at most).";
    else if (CStr.strlen(f.get("password")) < 8) err = "Password must be at least 8 characters.";
    else if (A.rankOf(f.get("role")) == 0) err = "Unknown role.";
    if (err == nullptr) {
        db.lock();
        let ok: bool = A.addUser(f.get("username"), f.get("full"), f.get("password"), f.get("role"));
        db.unlock();
        if (!ok) err = "Could not create the user (is the username taken?).";
    }
    if (err != nullptr) H.redirectMsg(res, "/users", "err", err);
    else H.redirectMsg(res, "/users", "ok", "User created.");
    A.release(&c, &f);
}

function userTogglePost(req: HS.Request*, res: HS.Response*) => void {
    let c: A.Ctx;
    let f: FM.Form;
    if (!A.needPost(req, res, &c, &f, 3)) return;
    let db: SQ.Db* = D.handle();
    let id: i64 = 0;
    if (FM.idParam(req, "id", &id) && id != c.userId) {
        let st: SQ.Stmt;
        db.lock();
        if (db.prepare("UPDATE users SET active = 1 - active WHERE id = ?", &st)) { st.bindInt(1, id); st.step(); st.finalize(); }
        db.unlock();
        H.redirectMsg(res, "/users", "ok", "User updated.");
    } else {
        H.redirectMsg(res, "/users", "err", "You cannot disable your own account.");
    }
    A.release(&c, &f);
}
