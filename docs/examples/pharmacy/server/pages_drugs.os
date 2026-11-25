// pharmacy/server/pages_drugs.os -- drug catalogue: list/search, add, edit, details, soft delete.
module Str = import "standard/1/String.os";
module SQ = import "standard/1/Sqlite.os";
module HS = import "standard/1/HttpServer.os";
module Text = import "standard/1/Text.os";
module CStr = import "standard/1/c/string.os";
module H = import "pharmacy/server/html.os";
module A = import "pharmacy/server/auth.os";
module D = import "pharmacy/server/db.os";
module FM = import "pharmacy/server/forms.os";
module F = import "pharmacy/server/fmt.os";
module CFG = import "pharmacy/server/config.os";

function drugsList(req: HS.Request*, res: HS.Response*) => void {
    let c: A.Ctx;
    if (!A.need(req, res, &c, 2)) return;
    let db: SQ.Db* = D.handle();
    let q: Str.String;
    let cat: Str.String;
    let pg: Str.String;
    req.queryParam("q", &q);
    req.queryParam("cat", &cat);
    req.queryParam("page", &pg);
    let page: i64 = 1;
    if (!F.parseInt(pg.cstr(), &page) || page < 1) page = 1;
    let pat: Str.String;
    pat.append("%");
    pat.append(q.cstr());
    pat.append("%");

    let out: Str.String;
    H.start(&out, req, "Drugs", c.rank, c.username.cstr(), c.csrf.cstr());
    out.append("<form class=\"bar\" method=\"get\" action=\"/drugs\"><input type=\"text\" name=\"q\" placeholder=\"Search name or generic\" value=\"");
    H.esc(q.cstr(), &out);
    out.append("\"><input type=\"text\" name=\"cat\" placeholder=\"Category\" value=\"");
    H.esc(cat.cstr(), &out);
    out.append("\"><button type=\"submit\">Search</button> ");
    H.link(&out, "/drugs/new", "+ Add drug");
    out.append("</form>");

    let where: char* = " FROM drugs d WHERE d.active = 1 AND (d.name LIKE ?1 OR d.generic_name LIKE ?1) AND (?2 = '' OR d.category = ?2)";
    let total: i64 = 0;
    let st: SQ.Stmt;
    let sql: Str.String;
    db.lock();
    sql.append("SELECT COUNT(*)");
    sql.append(where);
    if (db.prepare(sql.cstr(), &st)) {
        st.bindText(1, pat.cstr()); st.bindText(2, cat.cstr());
        if (st.step() == SQ.ROW) total = st.colInt(0);
        st.finalize();
    }
    sql.clear();
    sql.append("SELECT d.id, d.name, d.generic_name, d.category, d.strength, d.price_cents, d.reorder_level, ");
    sql.append(D.ONHAND);
    sql.append(where);
    sql.append(" ORDER BY d.name LIMIT ?3 OFFSET ?4");
    out.append("<table><tr><th>Name</th><th>Generic</th><th>Category</th><th>Strength</th><th class=\"num\">Price</th><th class=\"num\">In stock</th><th></th></tr>");
    if (db.prepare(sql.cstr(), &st)) {
        st.bindText(1, pat.cstr()); st.bindText(2, cat.cstr());
        st.bindInt(3, CFG.PAGE_SIZE as i64); st.bindInt(4, (page - 1) * (CFG.PAGE_SIZE as i64));
        while (st.step() == SQ.ROW) {
            let onhand: i64 = st.colInt(7);
            let low: bool = onhand <= st.colInt(6);
            if (low) out.append("<tr class=\"low\">"); else out.append("<tr>");
            out.append("<td>");
            let href: Str.String;
            href.append("/drugs/");
            href.appendInt(st.colInt(0));
            H.link(&out, href.cstr(), st.colText(1));
            href.free();
            out.append("</td>");
            H.td(&out, st.colText(2)); H.td(&out, st.colText(3)); H.td(&out, st.colText(4));
            H.tdM(&out, st.colInt(5));
            out.append("<td class=\"num\">"); out.appendInt(onhand); out.append("</td><td>");
            let e: Str.String;
            e.append("/drugs/"); e.appendInt(st.colInt(0)); e.append("/edit");
            H.link(&out, e.cstr(), "Edit");
            e.free();
            out.append("</td></tr>");
        }
        st.finalize();
    }
    db.unlock();
    out.append("</table>");
    let pages: i64 = (total + (CFG.PAGE_SIZE as i64) - 1) / (CFG.PAGE_SIZE as i64);
    out.append("<p class=\"pager\">");
    out.appendInt(total); out.append(" drugs. Page "); out.appendInt(page); out.append(" of "); out.appendInt(pages);
    let base: Str.String;
    base.append("/drugs?q="); Text.urlEncode(q.cstr(), &base); base.append("&cat="); Text.urlEncode(cat.cstr(), &base); base.append("&page=");
    if (page > 1) {
        let p1: Str.String; p1.append(base.cstr()); p1.appendInt(page - 1);
        out.append(" "); H.link(&out, p1.cstr(), "< Previous"); p1.free();
    }
    if (page < pages) {
        let p2: Str.String; p2.append(base.cstr()); p2.appendInt(page + 1);
        out.append(" "); H.link(&out, p2.cstr(), "Next >"); p2.free();
    }
    out.append("</p>");
    H.finish(&out);
    res.html(out.cstr());
    base.free(); sql.free(); pat.free(); q.free(); cat.free(); pg.free(); out.free();
    A.ctxFree(&c);
}

// shared form for add / edit. id = 0 for a new drug.
function drugForm(req: HS.Request*, res: HS.Response*, c: A.Ctx*, id: i64) => void {
    let db: SQ.Db* = D.handle();
    let st: SQ.Stmt;
    let name: Str.String; let generic: Str.String; let category: Str.String; let form: Str.String; let strength: Str.String;
    let price: i64 = 0; let reorder: i64 = 10;
    let found: bool = id == 0;
    if (id > 0) {
        db.lock();
        if (db.prepare("SELECT name, generic_name, category, form, strength, price_cents, reorder_level FROM drugs WHERE id = ? AND active = 1", &st)) {
            st.bindInt(1, id);
            if (st.step() == SQ.ROW) {
                found = true;
                name.set(st.colText(0)); generic.set(st.colText(1)); category.set(st.colText(2)); form.set(st.colText(3)); strength.set(st.colText(4));
                price = st.colInt(5); reorder = st.colInt(6);
            }
            st.finalize();
        }
        db.unlock();
    }
    if (!found) {
        res.status = 404;
        res.text("404 Drug not found");
    } else {
        let out: Str.String;
        let title: char* = "Add drug";
        if (id > 0) title = "Edit drug";
        H.start(&out, req, title, c.rank, c.username.cstr(), c.csrf.cstr());
        out.append("<form class=\"form\" method=\"post\" action=\"");
        if (id > 0) { out.append("/drugs/"); out.appendInt(id); out.append("/edit"); } else { out.append("/drugs/new"); }
        out.append("\">");
        H.csrf(&out, c.csrf.cstr());
        out.append("<label>Name<input type=\"text\" name=\"name\" value=\""); H.esc(name.cstr(), &out);
        out.append("\"></label><label>Generic name<input type=\"text\" name=\"generic\" value=\""); H.esc(generic.cstr(), &out);
        out.append("\"></label><label>Category<input type=\"text\" name=\"category\" value=\""); H.esc(category.cstr(), &out);
        out.append("\"></label><label>Form (tablet, syrup...)<input type=\"text\" name=\"form\" value=\""); H.esc(form.cstr(), &out);
        out.append("\"></label><label>Strength<input type=\"text\" name=\"strength\" value=\""); H.esc(strength.cstr(), &out);
        out.append("\"></label><label>Price (e.g. 12.50)<input type=\"text\" name=\"price\" value=\"");
        if (id > 0) { out.appendInt(price / 100); out.append("."); F.pad2(price % 100, &out); }
        out.append("\"></label><label>Reorder level<input type=\"text\" name=\"reorder\" value=\""); out.appendInt(reorder);
        out.append("\"></label><button type=\"submit\">Save</button> ");
        H.link(&out, "/drugs", "Cancel");
        out.append("</form>");
        H.finish(&out);
        res.html(out.cstr());
        out.free();
    }
    name.free(); generic.free(); category.free(); form.free(); strength.free();
}

function drugNewPage(req: HS.Request*, res: HS.Response*) => void {
    let c: A.Ctx;
    if (!A.need(req, res, &c, 2)) return;
    drugForm(req, res, &c, 0 as i64);
    A.ctxFree(&c);
}

function drugEditPage(req: HS.Request*, res: HS.Response*) => void {
    let c: A.Ctx;
    if (!A.need(req, res, &c, 2)) return;
    let id: i64 = 0;
    if (!FM.idParam(req, "id", &id)) { res.status = 404; res.text("404"); }
    else drugForm(req, res, &c, id);
    A.ctxFree(&c);
}

// POST for both add (id absent) and edit
function drugSave(req: HS.Request*, res: HS.Response*, id: i64) => void {
    let c: A.Ctx;
    let f: FM.Form;
    if (!A.needPost(req, res, &c, &f, 2)) return;
    let db: SQ.Db* = D.handle();
    let price: i64 = 0; let reorder: i64 = 0;
    let err: char* = nullptr;
    let name: char* = f.get("name");
    if ((name[0] as int) == 0) err = "Name is required.";
    else if (CStr.strlen(name) > 120) err = "Name is too long.";
    else if (!F.parseMoney(f.get("price"), &price)) err = "Price must be a number like 12.50.";
    else if (!F.parseInt(f.get("reorder"), &reorder)) err = "Reorder level must be a whole number.";
    if (err != nullptr) {
        let back: Str.String;
        if (id > 0) { back.append("/drugs/"); back.appendInt(id); back.append("/edit"); } else { back.append("/drugs/new"); }
        H.redirectMsg(res, back.cstr(), "err", err);
        back.free();
    } else {
        let st: SQ.Stmt;
        let ok: bool = false;
        db.lock();
        if (id > 0) {
            if (db.prepare("UPDATE drugs SET name = ?, generic_name = ?, category = ?, form = ?, strength = ?, price_cents = ?, reorder_level = ? WHERE id = ?", &st)) {
                st.bindText(1, name); st.bindText(2, f.get("generic")); st.bindText(3, f.get("category")); st.bindText(4, f.get("form")); st.bindText(5, f.get("strength"));
                st.bindInt(6, price); st.bindInt(7, reorder); st.bindInt(8, id);
                ok = st.step() == SQ.DONE;
                st.finalize();
            }
        } else {
            if (db.prepare("INSERT INTO drugs(name, generic_name, category, form, strength, price_cents, reorder_level) VALUES(?, ?, ?, ?, ?, ?, ?)", &st)) {
                st.bindText(1, name); st.bindText(2, f.get("generic")); st.bindText(3, f.get("category")); st.bindText(4, f.get("form")); st.bindText(5, f.get("strength"));
                st.bindInt(6, price); st.bindInt(7, reorder);
                ok = st.step() == SQ.DONE;
                st.finalize();
            }
        }
        db.unlock();
        if (ok) H.redirectMsg(res, "/drugs", "ok", "Drug saved.");
        else H.redirectMsg(res, "/drugs", "err", "Could not save the drug.");
    }
    A.release(&c, &f);
}

function drugNewPost(req: HS.Request*, res: HS.Response*) => void { drugSave(req, res, 0 as i64); }

function drugEditPost(req: HS.Request*, res: HS.Response*) => void {
    let id: i64 = 0;
    if (!FM.idParam(req, "id", &id)) { res.status = 404; res.text("404"); }
    else drugSave(req, res, id);
}

function drugDeletePost(req: HS.Request*, res: HS.Response*) => void {
    let c: A.Ctx;
    let f: FM.Form;
    if (!A.needPost(req, res, &c, &f, 2)) return;
    let id: i64 = 0;
    let db: SQ.Db* = D.handle();
    if (FM.idParam(req, "id", &id)) {
        let st: SQ.Stmt;
        db.lock();
        if (db.prepare("UPDATE drugs SET active = 0 WHERE id = ?", &st)) { st.bindInt(1, id); st.step(); st.finalize(); }
        db.unlock();
    }
    H.redirectMsg(res, "/drugs", "ok", "Drug removed from the catalogue.");
    A.release(&c, &f);
}

function drugDetail(req: HS.Request*, res: HS.Response*) => void {
    let c: A.Ctx;
    if (!A.need(req, res, &c, 2)) return;
    let id: i64 = 0;
    let db: SQ.Db* = D.handle();
    let st: SQ.Stmt;
    if (!FM.idParam(req, "id", &id)) { res.status = 404; res.text("404"); A.ctxFree(&c); return; }
    let out: Str.String;
    let found: bool = false;
    db.lock();
    if (db.prepare("SELECT name, generic_name, category, form, strength, price_cents, reorder_level FROM drugs WHERE id = ? AND active = 1", &st)) {
        st.bindInt(1, id);
        if (st.step() == SQ.ROW) {
            found = true;
            H.start(&out, req, st.colText(0), c.rank, c.username.cstr(), c.csrf.cstr());
            out.append("<p>");
            H.esc(st.colText(1), &out); out.append(" | "); H.esc(st.colText(2), &out); out.append(" | ");
            H.esc(st.colText(3), &out); out.append(" "); H.esc(st.colText(4), &out); out.append(" | ");
            F.money(st.colInt(5), &out); out.append(" | reorder at "); out.appendInt(st.colInt(6));
            out.append("</p>");
        }
        st.finalize();
    }
    if (!found) {
        db.unlock();
        res.status = 404; res.text("404 Drug not found");
    } else {
        out.append("<h2>Batches</h2><table><tr><th>Batch</th><th>Expiry</th><th class=\"num\">Quantity</th><th>Supplier</th><th></th></tr>");
        if (db.prepare("SELECT b.id, b.batch_no, b.expiry_date, b.quantity, COALESCE(s.name, '') FROM batches b LEFT JOIN suppliers s ON s.id = b.supplier_id WHERE b.drug_id = ? ORDER BY b.expiry_date", &st)) {
            st.bindInt(1, id);
            while (st.step() == SQ.ROW) {
                out.append("<tr>");
                H.td(&out, st.colText(1)); H.td(&out, st.colText(2)); out.append("<td class=\"num\">"); out.appendInt(st.colInt(3)); out.append("</td>");
                H.td(&out, st.colText(4));
                out.append("<td>");
                let a: Str.String; a.append("/stock/adjust/"); a.appendInt(st.colInt(0));
                H.link(&out, a.cstr(), "Adjust"); a.free();
                out.append("</td></tr>");
            }
            st.finalize();
        }
        out.append("</table><h2>Recent movements</h2><table><tr><th>When</th><th>Type</th><th class=\"num\">Qty</th><th>Note</th></tr>");
        if (db.prepare("SELECT created_at, type, qty, note FROM stock_movements WHERE drug_id = ? ORDER BY id DESC LIMIT 15", &st)) {
            st.bindInt(1, id);
            while (st.step() == SQ.ROW) {
                out.append("<tr>"); H.td(&out, st.colText(0)); H.td(&out, st.colText(1));
                out.append("<td class=\"num\">"); out.appendInt(st.colInt(2)); out.append("</td>"); H.td(&out, st.colText(3)); out.append("</tr>");
            }
            st.finalize();
        }
        db.unlock();
        out.append("</table><form method=\"post\" action=\"/drugs/");
        out.appendInt(id);
        out.append("/delete\">");
        H.csrf(&out, c.csrf.cstr());
        out.append("<button type=\"submit\" class=\"danger\">Remove from catalogue</button></form>");
        H.finish(&out);
        res.html(out.cstr());
    }
    out.free();
    A.ctxFree(&c);
}
