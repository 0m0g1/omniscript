// pharmacy/server/pages_reports.os -- low stock (+CSV), expiring, sales summary.
module Str = import "standard/1/String.os";
module SQ = import "standard/1/Sqlite.os";
module HS = import "standard/1/HttpServer.os";
module H = import "pharmacy/server/html.os";
module A = import "pharmacy/server/auth.os";
module D = import "pharmacy/server/db.os";
module F = import "pharmacy/server/fmt.os";
module Text = import "standard/1/Text.os";

function lowSql(sql: Str.String*) => void {
    sql.append("SELECT id, name, strength, category, oh, rl FROM (SELECT d.id AS id, d.name AS name, d.strength AS strength, d.category AS category, ");
    sql.append(D.ONHAND);
    sql.append(" AS oh, d.reorder_level AS rl FROM drugs d WHERE d.active = 1) WHERE oh <= rl ORDER BY oh, name");
}

function reportsIndex(req: HS.Request*, res: HS.Response*) => void {
    let c: A.Ctx;
    if (!A.need(req, res, &c, 2)) return;
    let out: Str.String;
    H.start(&out, req, "Reports", c.rank, c.username.cstr(), c.csrf.cstr());
    out.append("<ul><li>"); H.link(&out, "/reports/low-stock", "Low stock"); out.append(" (");
    H.link(&out, "/reports/low-stock.csv", "download CSV"); out.append(")</li><li>");
    H.link(&out, "/reports/expiring", "Expiring within 30 days"); out.append("</li><li>");
    H.link(&out, "/reports/expiring?days=90", "Expiring within 90 days"); out.append("</li><li>");
    H.link(&out, "/reports/sales", "Sales summary"); out.append("</li></ul>");
    H.finish(&out);
    res.html(out.cstr());
    out.free();
    A.ctxFree(&c);
}

function lowStockPage(req: HS.Request*, res: HS.Response*) => void {
    let c: A.Ctx;
    if (!A.need(req, res, &c, 2)) return;
    let db: SQ.Db* = D.handle();
    let st: SQ.Stmt;
    let sql: Str.String;
    let out: Str.String;
    lowSql(&sql);
    H.start(&out, req, "Low stock", c.rank, c.username.cstr(), c.csrf.cstr());
    out.append("<p>"); H.link(&out, "/reports/low-stock.csv", "Download CSV"); out.append("</p>");
    out.append("<table><tr><th>Drug</th><th>Strength</th><th>Category</th><th class=\"num\">In stock</th><th class=\"num\">Reorder at</th><th></th></tr>");
    db.lock();
    if (db.prepare(sql.cstr(), &st)) {
        while (st.step() == SQ.ROW) {
            out.append("<tr class=\"low\">");
            H.td(&out, st.colText(1)); H.td(&out, st.colText(2)); H.td(&out, st.colText(3));
            out.append("<td class=\"num\">"); out.appendInt(st.colInt(4)); out.append("</td><td class=\"num\">"); out.appendInt(st.colInt(5)); out.append("</td><td>");
            let h: Str.String; h.append("/drugs/"); h.appendInt(st.colInt(0));
            H.link(&out, h.cstr(), "Open"); h.free();
            out.append("</td></tr>");
        }
        st.finalize();
    }
    db.unlock();
    out.append("</table>");
    H.finish(&out);
    res.html(out.cstr());
    sql.free(); out.free();
    A.ctxFree(&c);
}

function csvCell(out: Str.String*, s: char*) => void {
    out.appendChar(34 as char);
    let i: u64 = 0;
    while ((s[i] as int) != 0) {
        if ((s[i] as int) == 34) out.appendChar(34 as char);
        out.appendChar(s[i]);
        i = i + 1;
    }
    out.appendChar(34 as char);
}

function lowStockCsv(req: HS.Request*, res: HS.Response*) => void {
    let c: A.Ctx;
    if (!A.need(req, res, &c, 2)) return;
    let db: SQ.Db* = D.handle();
    let st: SQ.Stmt;
    let sql: Str.String;
    let out: Str.String;
    lowSql(&sql);
    out.append("drug,strength,category,in_stock,reorder_level\r\n");
    db.lock();
    if (db.prepare(sql.cstr(), &st)) {
        while (st.step() == SQ.ROW) {
            csvCell(&out, st.colText(1)); out.append(",");
            csvCell(&out, st.colText(2)); out.append(",");
            csvCell(&out, st.colText(3)); out.append(",");
            out.appendInt(st.colInt(4)); out.append(","); out.appendInt(st.colInt(5)); out.append("\r\n");
        }
        st.finalize();
    }
    db.unlock();
    res.setBody("text/csv; charset=utf-8", out.cstr());
    res.header("Content-Disposition", "attachment; filename=\"low-stock.csv\"");
    sql.free(); out.free();
    A.ctxFree(&c);
}

function expiringPage(req: HS.Request*, res: HS.Response*) => void {
    let c: A.Ctx;
    if (!A.need(req, res, &c, 2)) return;
    let db: SQ.Db* = D.handle();
    let st: SQ.Stmt;
    let ds: Str.String;
    req.queryParam("days", &ds);
    let days: i64 = 30;
    if (!F.parseInt(ds.cstr(), &days) || days < 1 || days > 3650) days = 30;
    let out: Str.String;
    H.start(&out, req, "Expiring stock", c.rank, c.username.cstr(), c.csrf.cstr());
    out.append("<p>Batches that expire within "); out.appendInt(days); out.append(" days, and batches already expired that still hold stock.</p>");
    out.append("<table><tr><th>Drug</th><th>Batch</th><th>Expiry</th><th class=\"num\">Quantity</th><th>Status</th></tr>");
    db.lock();
    if (db.prepare("SELECT d.name, b.batch_no, b.expiry_date, b.quantity, b.expiry_date < date('now') FROM batches b JOIN drugs d ON d.id = b.drug_id WHERE b.quantity > 0 AND b.expiry_date <= date('now', '+' || ? || ' days') ORDER BY b.expiry_date", &st)) {
        st.bindInt(1, days);
        while (st.step() == SQ.ROW) {
            if (st.colInt(4) == 1) out.append("<tr class=\"expired\">"); else out.append("<tr class=\"low\">");
            H.td(&out, st.colText(0)); H.td(&out, st.colText(1)); H.td(&out, st.colText(2));
            out.append("<td class=\"num\">"); out.appendInt(st.colInt(3)); out.append("</td>");
            if (st.colInt(4) == 1) H.td(&out, "EXPIRED - remove from stock"); else H.td(&out, "expiring soon");
            out.append("</tr>");
        }
        st.finalize();
    }
    db.unlock();
    out.append("</table>");
    H.finish(&out);
    res.html(out.cstr());
    ds.free(); out.free();
    A.ctxFree(&c);
}

function salesReport(req: HS.Request*, res: HS.Response*) => void {
    let c: A.Ctx;
    if (!A.need(req, res, &c, 2)) return;
    let db: SQ.Db* = D.handle();
    let st: SQ.Stmt;
    let from: Str.String;
    let to: Str.String;
    req.queryParam("from", &from);
    req.queryParam("to", &to);
    if (from.len != 10) { from.set("1970-01-01"); }
    if (to.len != 10) { to.set("2999-12-31"); }
    let out: Str.String;
    H.start(&out, req, "Sales summary", c.rank, c.username.cstr(), c.csrf.cstr());
    out.append("<form class=\"bar\" method=\"get\" action=\"/reports/sales\">From <input type=\"text\" name=\"from\" placeholder=\"YYYY-MM-DD\"> to <input type=\"text\" name=\"to\" placeholder=\"YYYY-MM-DD\"><button type=\"submit\">Show</button></form>");
    db.lock();
    if (db.prepare("SELECT COUNT(*), COALESCE(SUM(total_cents), 0) FROM sales WHERE date(created_at) >= ? AND date(created_at) <= ?", &st)) {
        st.bindText(1, from.cstr()); st.bindText(2, to.cstr());
        if (st.step() == SQ.ROW) {
            out.append("<div class=\"cards\"><div class=\"card\"><b>"); out.appendInt(st.colInt(0)); out.append("</b><span>sales</span></div><div class=\"card\"><b>");
            F.money(st.colInt(1), &out); out.append("</b><span>revenue</span></div></div>");
        }
        st.finalize();
    }
    out.append("<h2>Top drugs</h2><table><tr><th>Drug</th><th class=\"num\">Units sold</th><th class=\"num\">Revenue</th></tr>");
    if (db.prepare("SELECT d.name, SUM(i.qty), SUM(i.qty * i.unit_cents) FROM sale_items i JOIN sales s ON s.id = i.sale_id JOIN drugs d ON d.id = i.drug_id WHERE date(s.created_at) >= ? AND date(s.created_at) <= ? GROUP BY d.id ORDER BY SUM(i.qty) DESC LIMIT 10", &st)) {
        st.bindText(1, from.cstr()); st.bindText(2, to.cstr());
        while (st.step() == SQ.ROW) {
            out.append("<tr>"); H.td(&out, st.colText(0));
            out.append("<td class=\"num\">"); out.appendInt(st.colInt(1)); out.append("</td>"); H.tdM(&out, st.colInt(2)); out.append("</tr>");
        }
        st.finalize();
    }
    db.unlock();
    out.append("</table>");
    H.finish(&out);
    res.html(out.cstr());
    from.free(); to.free(); out.free();
    A.ctxFree(&c);
}
