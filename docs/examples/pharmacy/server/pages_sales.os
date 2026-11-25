// pharmacy/server/pages_sales.os -- new sale (cart), checkout, receipt, history.
module Str = import "standard/1/String.os";
module SQ = import "standard/1/Sqlite.os";
module HS = import "standard/1/HttpServer.os";
module H = import "pharmacy/server/html.os";
module A = import "pharmacy/server/auth.os";
module D = import "pharmacy/server/db.os";
module FM = import "pharmacy/server/forms.os";
module F = import "pharmacy/server/fmt.os";

function salePage(req: HS.Request*, res: HS.Response*) => void {
    let c: A.Ctx;
    if (!A.need(req, res, &c, 1)) return;
    let db: SQ.Db* = D.handle();
    let st: SQ.Stmt;
    let out: Str.String;
    let sql: Str.String;
    H.start(&out, req, "New sale", c.rank, c.username.cstr(), c.csrf.cstr());
    db.lock();
    out.append("<h2>Cart</h2><table><tr><th>Drug</th><th class=\"num\">Qty</th><th class=\"num\">Unit price</th><th class=\"num\">Line total</th><th></th></tr>");
    let total: i64 = 0;
    if (db.prepare("SELECT c.drug_id, d.name, d.strength, c.qty, d.price_cents FROM cart c JOIN drugs d ON d.id = c.drug_id WHERE c.user_id = ? ORDER BY d.name", &st)) {
        st.bindInt(1, c.userId);
        while (st.step() == SQ.ROW) {
            let line: i64 = st.colInt(3) * st.colInt(4);
            total = total + line;
            out.append("<tr>");
            out.append("<td>"); H.esc(st.colText(1), &out); out.append(" "); H.esc(st.colText(2), &out); out.append("</td>");
            out.append("<td class=\"num\">"); out.appendInt(st.colInt(3)); out.append("</td>");
            H.tdM(&out, st.colInt(4)); H.tdM(&out, line);
            out.append("<td><form method=\"post\" action=\"/sales/cart/remove\">");
            H.csrf(&out, c.csrf.cstr());
            out.append("<input type=\"hidden\" name=\"drug\" value=\""); out.appendInt(st.colInt(0));
            out.append("\"><button type=\"submit\">Remove</button></form></td></tr>");
        }
        st.finalize();
    }
    out.append("<tr class=\"total\"><td colspan=\"3\">Total</td>");
    H.tdM(&out, total);
    out.append("<td></td></tr></table>");
    out.append("<form class=\"form\" method=\"post\" action=\"/sales/checkout\">");
    H.csrf(&out, c.csrf.cstr());
    out.append("<label>Customer (optional)<input type=\"text\" name=\"customer\"></label><button type=\"submit\">Complete sale</button></form>");
    out.append("<h2>Add item</h2><form class=\"form\" method=\"post\" action=\"/sales/cart/add\">");
    H.csrf(&out, c.csrf.cstr());
    out.append("<label>Drug<select name=\"drug\">");
    sql.append("SELECT d.id, d.name, d.strength, d.price_cents, ");
    sql.append(D.ONHAND);
    sql.append(" AS oh FROM drugs d WHERE d.active = 1 ORDER BY d.name");
    if (db.prepare(sql.cstr(), &st)) {
        while (st.step() == SQ.ROW) {
            out.append("<option value=\""); out.appendInt(st.colInt(0)); out.append("\">");
            H.esc(st.colText(1), &out); out.append(" "); H.esc(st.colText(2), &out); out.append(" - ");
            F.money(st.colInt(3), &out); out.append(" (stock "); out.appendInt(st.colInt(4)); out.append(")</option>");
        }
        st.finalize();
    }
    db.unlock();
    out.append("</select></label><label>Quantity<input type=\"text\" name=\"qty\" value=\"1\"></label><button type=\"submit\">Add to cart</button></form>");
    H.finish(&out);
    res.html(out.cstr());
    sql.free(); out.free();
    A.ctxFree(&c);
}

function cartAddPost(req: HS.Request*, res: HS.Response*) => void {
    let c: A.Ctx;
    let f: FM.Form;
    if (!A.needPost(req, res, &c, &f, 1)) return;
    let db: SQ.Db* = D.handle();
    let drug: i64 = 0; let qty: i64 = 0;
    if (!F.parseInt(f.get("drug"), &drug) || !F.parseInt(f.get("qty"), &qty) || qty < 1 || qty > 10000) {
        H.redirectMsg(res, "/sales/new", "err", "Choose a drug and a whole quantity above zero.");
    } else {
        db.lock();
        let ok: bool = D.cartAdd(c.userId, drug, qty);
        db.unlock();
        if (ok) H.redirectMsg(res, "/sales/new", "ok", "Added to cart.");
        else H.redirectMsg(res, "/sales/new", "err", "Could not add that item.");
    }
    A.release(&c, &f);
}

function cartRemovePost(req: HS.Request*, res: HS.Response*) => void {
    let c: A.Ctx;
    let f: FM.Form;
    if (!A.needPost(req, res, &c, &f, 1)) return;
    let db: SQ.Db* = D.handle();
    let drug: i64 = 0;
    if (F.parseInt(f.get("drug"), &drug)) { db.lock(); D.cartRemove(c.userId, drug); db.unlock(); }
    res.redirect("/sales/new", 303);
    A.release(&c, &f);
}

function checkoutPost(req: HS.Request*, res: HS.Response*) => void {
    let c: A.Ctx;
    let f: FM.Form;
    if (!A.needPost(req, res, &c, &f, 1)) return;
    let db: SQ.Db* = D.handle();
    let err: Str.String;
    db.lock();
    let id: i64 = D.createSale(c.userId, f.get("customer"), &err);
    db.unlock();
    if (id > 0) {
        let u: Str.String;
        u.append("/sales/"); u.appendInt(id);
        res.redirect(u.cstr(), 303);
        u.free();
    } else {
        H.redirectMsg(res, "/sales/new", "err", err.cstr());
    }
    err.free();
    A.release(&c, &f);
}

function receiptPage(req: HS.Request*, res: HS.Response*) => void {
    let c: A.Ctx;
    if (!A.need(req, res, &c, 1)) return;
    let id: i64 = 0;
    let db: SQ.Db* = D.handle();
    let st: SQ.Stmt;
    if (!FM.idParam(req, "id", &id)) { res.status = 404; res.text("404"); A.ctxFree(&c); return; }
    let out: Str.String;
    let found: bool = false;
    db.lock();
    if (db.prepare("SELECT s.created_at, s.customer, s.total_cents, COALESCE(u.username, '') FROM sales s LEFT JOIN users u ON u.id = s.user_id WHERE s.id = ?", &st)) {
        st.bindInt(1, id);
        if (st.step() == SQ.ROW) {
            found = true;
            let title: Str.String;
            title.append("Receipt #"); title.appendInt(id);
            H.start(&out, req, title.cstr(), c.rank, c.username.cstr(), c.csrf.cstr());
            title.free();
            out.append("<p>"); H.esc(st.colText(0), &out); out.append(" | served by "); H.esc(st.colText(3), &out);
            if (H.has(st.colText(1))) { out.append(" | customer: "); H.esc(st.colText(1), &out); }
            out.append("</p>");
        }
        st.finalize();
    }
    if (!found) { db.unlock(); res.status = 404; res.text("404 Sale not found"); }
    else {
        out.append("<table><tr><th>Drug</th><th>Batch</th><th>Expiry</th><th class=\"num\">Qty</th><th class=\"num\">Unit</th><th class=\"num\">Total</th></tr>");
        let total: i64 = 0;
        if (db.prepare("SELECT d.name, b.batch_no, b.expiry_date, i.qty, i.unit_cents FROM sale_items i JOIN drugs d ON d.id = i.drug_id JOIN batches b ON b.id = i.batch_id WHERE i.sale_id = ? ORDER BY i.id", &st)) {
            st.bindInt(1, id);
            while (st.step() == SQ.ROW) {
                let line: i64 = st.colInt(3) * st.colInt(4);
                total = total + line;
                out.append("<tr>"); H.td(&out, st.colText(0)); H.td(&out, st.colText(1)); H.td(&out, st.colText(2));
                out.append("<td class=\"num\">"); out.appendInt(st.colInt(3)); out.append("</td>");
                H.tdM(&out, st.colInt(4)); H.tdM(&out, line); out.append("</tr>");
            }
            st.finalize();
        }
        db.unlock();
        out.append("<tr class=\"total\"><td colspan=\"5\">Total</td>");
        H.tdM(&out, total);
        out.append("</tr></table><p>");
        H.link(&out, "/sales/new", "Start another sale");
        out.append("</p>");
        H.finish(&out);
        res.html(out.cstr());
    }
    out.free();
    A.ctxFree(&c);
}

function salesHistory(req: HS.Request*, res: HS.Response*) => void {
    let c: A.Ctx;
    if (!A.need(req, res, &c, 1)) return;
    let db: SQ.Db* = D.handle();
    let st: SQ.Stmt;
    let out: Str.String;
    H.start(&out, req, "Sales", c.rank, c.username.cstr(), c.csrf.cstr());
    out.append("<table><tr><th>#</th><th>When</th><th>Customer</th><th>By</th><th class=\"num\">Total</th></tr>");
    db.lock();
    if (db.prepare("SELECT s.id, s.created_at, s.customer, COALESCE(u.username, ''), s.total_cents FROM sales s LEFT JOIN users u ON u.id = s.user_id ORDER BY s.id DESC LIMIT 100", &st)) {
        while (st.step() == SQ.ROW) {
            out.append("<tr><td>");
            let h: Str.String; h.append("/sales/"); h.appendInt(st.colInt(0));
            let lbl: Str.String; lbl.appendInt(st.colInt(0));
            H.link(&out, h.cstr(), lbl.cstr());
            h.free(); lbl.free();
            out.append("</td>");
            H.td(&out, st.colText(1)); H.td(&out, st.colText(2)); H.td(&out, st.colText(3)); H.tdM(&out, st.colInt(4));
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
