// pharmacy/server/pages_stock.os -- receive stock, adjust a batch, movement history.
module Str = import "standard/1/String.os";
module SQ = import "standard/1/Sqlite.os";
module HS = import "standard/1/HttpServer.os";
module CStr = import "standard/1/c/string.os";
module H = import "pharmacy/server/html.os";
module A = import "pharmacy/server/auth.os";
module D = import "pharmacy/server/db.os";
module FM = import "pharmacy/server/forms.os";
module F = import "pharmacy/server/fmt.os";

// true if text is a real calendar date YYYY-MM-DD (checked by SQLite)
function validDate(s: char*) => bool {
    let db: SQ.Db* = D.handle();
    let st: SQ.Stmt;
    let ok: bool = false;
    if (CStr.strlen(s) == 10) {
        if (db.prepare("SELECT date(?1) = ?1", &st)) {
            st.bindText(1, s);
            if (st.step() == SQ.ROW) ok = st.colInt(0) == 1;
            st.finalize();
        }
    }
    return ok;
}

function receivePage(req: HS.Request*, res: HS.Response*) => void {
    let c: A.Ctx;
    if (!A.need(req, res, &c, 2)) return;
    let db: SQ.Db* = D.handle();
    let st: SQ.Stmt;
    let out: Str.String;
    H.start(&out, req, "Receive stock", c.rank, c.username.cstr(), c.csrf.cstr());
    out.append("<form class=\"form\" method=\"post\" action=\"/stock/receive\">");
    H.csrf(&out, c.csrf.cstr());
    out.append("<label>Drug<select name=\"drug\">");
    db.lock();
    if (db.prepare("SELECT id, name, strength FROM drugs WHERE active = 1 ORDER BY name", &st)) {
        while (st.step() == SQ.ROW) {
            out.append("<option value=\""); out.appendInt(st.colInt(0)); out.append("\">");
            H.esc(st.colText(1), &out); out.append(" "); H.esc(st.colText(2), &out); out.append("</option>");
        }
        st.finalize();
    }
    out.append("</select></label><label>Supplier<select name=\"supplier\"><option value=\"0\">(none)</option>");
    if (db.prepare("SELECT id, name FROM suppliers ORDER BY name", &st)) {
        while (st.step() == SQ.ROW) {
            out.append("<option value=\""); out.appendInt(st.colInt(0)); out.append("\">"); H.esc(st.colText(1), &out); out.append("</option>");
        }
        st.finalize();
    }
    db.unlock();
    out.append("</select></label><label>Batch number<input type=\"text\" name=\"batch\"></label>");
    out.append("<label>Expiry date (YYYY-MM-DD)<input type=\"text\" name=\"expiry\" placeholder=\"2027-06-30\"></label>");
    out.append("<label>Quantity<input type=\"text\" name=\"qty\"></label><label>Cost per unit (e.g. 7.50)<input type=\"text\" name=\"cost\"></label>");
    out.append("<button type=\"submit\">Receive</button></form>");
    H.finish(&out);
    res.html(out.cstr());
    out.free();
    A.ctxFree(&c);
}

function receivePost(req: HS.Request*, res: HS.Response*) => void {
    let c: A.Ctx;
    let f: FM.Form;
    if (!A.needPost(req, res, &c, &f, 2)) return;
    let db: SQ.Db* = D.handle();
    let drug: i64 = 0; let sup: i64 = 0; let qty: i64 = 0; let cost: i64 = 0;
    let err: char* = nullptr;
    db.lock();
    if (!F.parseInt(f.get("drug"), &drug) || drug < 1) err = "Choose a drug.";
    else if (!H.has(f.get("batch"))) err = "Batch number is required.";
    else if (!validDate(f.get("expiry"))) err = "Expiry must be a real date written YYYY-MM-DD.";
    else if (!F.parseInt(f.get("qty"), &qty) || qty < 1) err = "Quantity must be a whole number above zero.";
    else if (!F.parseMoney(f.get("cost"), &cost)) err = "Cost must be a number like 7.50.";
    if (err == nullptr) {
        if (!F.parseInt(f.get("supplier"), &sup)) sup = 0;
        if (!D.receive(drug, f.get("batch"), f.get("expiry"), qty, cost, sup, c.userId)) err = "Could not record the delivery.";
    }
    db.unlock();
    if (err != nullptr) H.redirectMsg(res, "/stock/receive", "err", err);
    else H.redirectMsg(res, "/stock/movements", "ok", "Stock received.");
    A.release(&c, &f);
}

function adjustPage(req: HS.Request*, res: HS.Response*) => void {
    let c: A.Ctx;
    if (!A.need(req, res, &c, 2)) return;
    let id: i64 = 0;
    let db: SQ.Db* = D.handle();
    let st: SQ.Stmt;
    if (!FM.idParam(req, "id", &id)) { res.status = 404; res.text("404"); A.ctxFree(&c); return; }
    let out: Str.String;
    let found: bool = false;
    db.lock();
    if (db.prepare("SELECT d.name, b.batch_no, b.expiry_date, b.quantity FROM batches b JOIN drugs d ON d.id = b.drug_id WHERE b.id = ?", &st)) {
        st.bindInt(1, id);
        if (st.step() == SQ.ROW) {
            found = true;
            H.start(&out, req, "Adjust stock", c.rank, c.username.cstr(), c.csrf.cstr());
            out.append("<p>"); H.esc(st.colText(0), &out); out.append(", batch "); H.esc(st.colText(1), &out);
            out.append(", expires "); H.esc(st.colText(2), &out); out.append(", currently "); out.appendInt(st.colInt(3)); out.append(" units.</p>");
        }
        st.finalize();
    }
    db.unlock();
    if (!found) { res.status = 404; res.text("404 Batch not found"); }
    else {
        out.append("<form class=\"form\" method=\"post\" action=\"/stock/adjust/"); out.appendInt(id); out.append("\">");
        H.csrf(&out, c.csrf.cstr());
        out.append("<label>Change (use a minus sign to remove, e.g. -5)<input type=\"text\" name=\"delta\"></label>");
        out.append("<label>Reason (damaged, count correction...)<input type=\"text\" name=\"reason\"></label><button type=\"submit\">Apply</button></form>");
        H.finish(&out);
        res.html(out.cstr());
    }
    out.free();
    A.ctxFree(&c);
}

function adjustPost(req: HS.Request*, res: HS.Response*) => void {
    let c: A.Ctx;
    let f: FM.Form;
    if (!A.needPost(req, res, &c, &f, 2)) return;
    let id: i64 = 0;
    let delta: i64 = 0;
    let db: SQ.Db* = D.handle();
    let msg: char* = nullptr;
    if (!FM.idParam(req, "id", &id)) msg = "Bad batch.";
    else if (!F.parseSigned(f.get("delta"), &delta) || delta == 0) msg = "Enter a whole number change, not zero.";
    else if (!H.has(f.get("reason"))) msg = "Give a reason.";
    let back: Str.String;
    back.append("/stock/adjust/"); back.appendInt(id);
    if (msg == nullptr) {
        db.lock();
        let r: int = D.adjust(id, delta, f.get("reason"), c.userId);
        db.unlock();
        if (r == 0) H.redirectMsg(res, "/stock/movements", "ok", "Stock adjusted.");
        else if (r == 2) H.redirectMsg(res, back.cstr(), "err", "That would take the batch below zero.");
        else H.redirectMsg(res, back.cstr(), "err", "Batch not found.");
    } else {
        H.redirectMsg(res, back.cstr(), "err", msg);
    }
    back.free();
    A.release(&c, &f);
}

function movementsPage(req: HS.Request*, res: HS.Response*) => void {
    let c: A.Ctx;
    if (!A.need(req, res, &c, 2)) return;
    let db: SQ.Db* = D.handle();
    let st: SQ.Stmt;
    let out: Str.String;
    H.start(&out, req, "Stock movements", c.rank, c.username.cstr(), c.csrf.cstr());
    out.append("<table><tr><th>When</th><th>Drug</th><th>Batch</th><th>Type</th><th class=\"num\">Qty</th><th>By</th><th>Note</th></tr>");
    db.lock();
    if (db.prepare("SELECT m.created_at, d.name, COALESCE(b.batch_no, ''), m.type, m.qty, COALESCE(u.username, ''), m.note FROM stock_movements m JOIN drugs d ON d.id = m.drug_id LEFT JOIN batches b ON b.id = m.batch_id LEFT JOIN users u ON u.id = m.user_id ORDER BY m.id DESC LIMIT 100", &st)) {
        while (st.step() == SQ.ROW) {
            out.append("<tr>");
            H.td(&out, st.colText(0)); H.td(&out, st.colText(1)); H.td(&out, st.colText(2)); H.td(&out, st.colText(3));
            out.append("<td class=\"num\">"); out.appendInt(st.colInt(4)); out.append("</td>");
            H.td(&out, st.colText(5)); H.td(&out, st.colText(6));
            out.append("</tr>");
        }
        st.finalize();
    }
    db.unlock();
    out.append("</table>");
    H.finish(&out);
    res.html(out.cstr());
    out.free();
    A.ctxFree(&c);
}
