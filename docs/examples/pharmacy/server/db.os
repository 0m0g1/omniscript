// pharmacy/server/db.os -- schema, seed data and the multi-step operations (receive, adjust, sale).
// Handlers take db.lock() around their database work; money is integer cents; dates are ISO text.
module Str = import "standard/1/String.os";
module SQ = import "standard/1/Sqlite.os";
module L = import "standard/1/List.os";
module Console = import "standard/1/Console.os";
module CFG = import "pharmacy/server/config.os";

let db: SQ.Db;

// stock a drug can still be sold from: not expired
let ONHAND: char* = "COALESCE((SELECT SUM(b.quantity) FROM batches b WHERE b.drug_id = d.id AND b.expiry_date >= date('now')), 0)";

function handle() => SQ.Db* { return &db; }

function schema() => void {
    db.exec("CREATE TABLE IF NOT EXISTS users(id INTEGER PRIMARY KEY, username TEXT UNIQUE NOT NULL, pass_hash TEXT NOT NULL, full_name TEXT NOT NULL DEFAULT '', role TEXT NOT NULL, active INTEGER NOT NULL DEFAULT 1, created_at TEXT NOT NULL DEFAULT (datetime('now','localtime')))");
    db.exec("CREATE TABLE IF NOT EXISTS suppliers(id INTEGER PRIMARY KEY, name TEXT NOT NULL, phone TEXT NOT NULL DEFAULT '', email TEXT NOT NULL DEFAULT '')");
    db.exec("CREATE TABLE IF NOT EXISTS drugs(id INTEGER PRIMARY KEY, name TEXT NOT NULL, generic_name TEXT NOT NULL DEFAULT '', category TEXT NOT NULL DEFAULT '', form TEXT NOT NULL DEFAULT '', strength TEXT NOT NULL DEFAULT '', price_cents INTEGER NOT NULL DEFAULT 0, reorder_level INTEGER NOT NULL DEFAULT 10, active INTEGER NOT NULL DEFAULT 1)");
    db.exec("CREATE TABLE IF NOT EXISTS batches(id INTEGER PRIMARY KEY, drug_id INTEGER NOT NULL REFERENCES drugs(id), batch_no TEXT NOT NULL, expiry_date TEXT NOT NULL, quantity INTEGER NOT NULL CHECK(quantity >= 0), cost_cents INTEGER NOT NULL DEFAULT 0, supplier_id INTEGER REFERENCES suppliers(id), received_at TEXT NOT NULL DEFAULT (datetime('now','localtime')))");
    db.exec("CREATE TABLE IF NOT EXISTS stock_movements(id INTEGER PRIMARY KEY, batch_id INTEGER REFERENCES batches(id), drug_id INTEGER NOT NULL REFERENCES drugs(id), type TEXT NOT NULL, qty INTEGER NOT NULL, user_id INTEGER REFERENCES users(id), note TEXT NOT NULL DEFAULT '', created_at TEXT NOT NULL DEFAULT (datetime('now','localtime')))");
    db.exec("CREATE TABLE IF NOT EXISTS sales(id INTEGER PRIMARY KEY, user_id INTEGER REFERENCES users(id), customer TEXT NOT NULL DEFAULT '', total_cents INTEGER NOT NULL DEFAULT 0, created_at TEXT NOT NULL DEFAULT (datetime('now','localtime')))");
    db.exec("CREATE TABLE IF NOT EXISTS sale_items(id INTEGER PRIMARY KEY, sale_id INTEGER NOT NULL REFERENCES sales(id), batch_id INTEGER NOT NULL REFERENCES batches(id), drug_id INTEGER NOT NULL REFERENCES drugs(id), qty INTEGER NOT NULL, unit_cents INTEGER NOT NULL)");
    db.exec("CREATE TABLE IF NOT EXISTS cart(user_id INTEGER NOT NULL, drug_id INTEGER NOT NULL, qty INTEGER NOT NULL, PRIMARY KEY(user_id, drug_id))");
    db.exec("CREATE INDEX IF NOT EXISTS ix_drugs_name ON drugs(name)");
    db.exec("CREATE INDEX IF NOT EXISTS ix_batches_drug ON batches(drug_id, expiry_date)");
    db.exec("CREATE INDEX IF NOT EXISTS ix_moves_drug ON stock_movements(drug_id)");
    db.exec("CREATE INDEX IF NOT EXISTS ix_sales_date ON sales(created_at)");
}

// Sample catalogue so every page has something to show. Dates are relative to today, computed by SQLite.
function seedCatalogue() => void {
    db.exec("INSERT INTO suppliers(name, phone, email) VALUES('MedSupply Ltd','0700111222','orders@medsupply.example'),('HealthLine Distributors','0700333444','sales@healthline.example'),('PharmaCo Kenya','0700555666','info@pharmaco.example')");
    db.exec("INSERT INTO drugs(name, generic_name, category, form, strength, price_cents, reorder_level) VALUES('Paracetamol','Acetaminophen','Analgesic','Tablet','500mg',500,50),('Ibuprofen','Ibuprofen','Analgesic','Tablet','400mg',800,40),('Amoxicillin','Amoxicillin','Antibiotic','Capsule','500mg',2500,30),('Azithromycin','Azithromycin','Antibiotic','Tablet','250mg',4500,20),('Ciprofloxacin','Ciprofloxacin','Antibiotic','Tablet','500mg',3500,20),('Metformin','Metformin','Antidiabetic','Tablet','500mg',1200,40),('Glibenclamide','Glibenclamide','Antidiabetic','Tablet','5mg',900,30),('Amlodipine','Amlodipine','Cardiovascular','Tablet','5mg',1500,30)");
    db.exec("INSERT INTO drugs(name, generic_name, category, form, strength, price_cents, reorder_level) VALUES('Atenolol','Atenolol','Cardiovascular','Tablet','50mg',1100,25),('Omeprazole','Omeprazole','Gastrointestinal','Capsule','20mg',1800,30),('Loperamide','Loperamide','Gastrointestinal','Capsule','2mg',700,20),('Cetirizine','Cetirizine','Antihistamine','Tablet','10mg',600,30),('Salbutamol','Salbutamol','Respiratory','Inhaler','100mcg',3200,10),('Oral Rehydration Salts','ORS','Rehydration','Sachet','20.5g',300,60),('Vitamin C','Ascorbic acid','Supplement','Tablet','500mg',400,50),('Artemether/Lumefantrine','Artemether/Lumefantrine','Antimalarial','Tablet','20/120mg',3800,40)");
    db.exec("INSERT INTO batches(drug_id, batch_no, expiry_date, quantity, cost_cents, supplier_id) SELECT id, 'A' || id || '-01', date('now', '+' || (40 + id * 17) || ' days'), 60 + id * 9, price_cents * 6 / 10, 1 + id % 3 FROM drugs");
    db.exec("INSERT INTO batches(drug_id, batch_no, expiry_date, quantity, cost_cents, supplier_id) SELECT id, 'B' || id || '-02', date('now', '+' || (260 + id * 11) || ' days'), 120, price_cents * 6 / 10, 1 + (id + 1) % 3 FROM drugs");
    db.exec("UPDATE batches SET quantity = 4 WHERE batch_no IN ('A2-01','B2-02','A5-01','B5-02')");
    db.exec("UPDATE batches SET expiry_date = date('now', '+12 days') WHERE batch_no = 'A7-01'");
    db.exec("UPDATE batches SET expiry_date = date('now', '-5 days') WHERE batch_no = 'A11-01'");
    db.exec("INSERT INTO stock_movements(batch_id, drug_id, type, qty, user_id, note) SELECT id, drug_id, 'receive', quantity, 1, 'opening stock' FROM batches");
}

function init() => bool {
    if (!db.open(CFG.DB_PATH)) return false;
    schema();
    if (db.scalarInt("SELECT COUNT(*) FROM drugs", 0 as i64) == 0) seedCatalogue();
    return true;
}

// ---- cart ----
function cartAdd(userId: i64, drugId: i64, qty: i64) => bool {
    let st: SQ.Stmt;
    let ok: bool = false;
    if (db.prepare("INSERT INTO cart(user_id, drug_id, qty) VALUES(?, ?, ?) ON CONFLICT(user_id, drug_id) DO UPDATE SET qty = qty + excluded.qty", &st)) {
        st.bindInt(1, userId); st.bindInt(2, drugId); st.bindInt(3, qty);
        ok = st.step() == SQ.DONE;
        st.finalize();
    }
    return ok;
}

function cartRemove(userId: i64, drugId: i64) => void {
    let st: SQ.Stmt;
    if (db.prepare("DELETE FROM cart WHERE user_id = ? AND drug_id = ?", &st)) {
        st.bindInt(1, userId); st.bindInt(2, drugId); st.step(); st.finalize();
    }
}

function cartClear(userId: i64) => void {
    let st: SQ.Stmt;
    if (db.prepare("DELETE FROM cart WHERE user_id = ?", &st)) { st.bindInt(1, userId); st.step(); st.finalize(); }
}

function onHand(drugId: i64) => i64 {
    let st: SQ.Stmt;
    let n: i64 = 0;
    let sql: Str.String;
    sql.append("SELECT ");
    sql.append(ONHAND);
    sql.append(" FROM drugs d WHERE d.id = ?");
    if (db.prepare(sql.cstr(), &st)) {
        st.bindInt(1, drugId);
        if (st.step() == SQ.ROW) n = st.colInt(0);
        st.finalize();
    }
    sql.free();
    return n;
}

// ---- stock ----
function receive(drugId: i64, batchNo: char*, expiry: char*, qty: i64, costCents: i64, supplierId: i64, userId: i64) => bool {
    let st: SQ.Stmt;
    let ok: bool = false;
    db.begin();
    if (db.prepare("INSERT INTO batches(drug_id, batch_no, expiry_date, quantity, cost_cents, supplier_id) VALUES(?, ?, ?, ?, ?, ?)", &st)) {
        st.bindInt(1, drugId); st.bindText(2, batchNo); st.bindText(3, expiry); st.bindInt(4, qty); st.bindInt(5, costCents);
        if (supplierId > 0) st.bindInt(6, supplierId); else st.bindNull(6);
        ok = st.step() == SQ.DONE;
        st.finalize();
    }
    if (ok) {
        let bid: i64 = db.lastId();
        ok = false;
        if (db.prepare("INSERT INTO stock_movements(batch_id, drug_id, type, qty, user_id, note) VALUES(?, ?, 'receive', ?, ?, ?)", &st)) {
            st.bindInt(1, bid); st.bindInt(2, drugId); st.bindInt(3, qty); st.bindInt(4, userId); st.bindText(5, batchNo);
            ok = st.step() == SQ.DONE;
            st.finalize();
        }
    }
    if (ok) db.commit(); else db.rollback();
    return ok;
}

// 0 ok, 1 batch not found, 2 would go below zero
function adjust(batchId: i64, delta: i64, reason: char*, userId: i64) => int {
    let st: SQ.Stmt;
    let drugId: i64 = 0;
    let have: i64 = 0;
    let found: bool = false;
    if (db.prepare("SELECT drug_id, quantity FROM batches WHERE id = ?", &st)) {
        st.bindInt(1, batchId);
        if (st.step() == SQ.ROW) { found = true; drugId = st.colInt(0); have = st.colInt(1); }
        st.finalize();
    }
    if (!found) return 1;
    if (have + delta < 0) return 2;
    let ok: bool = false;
    db.begin();
    if (db.prepare("UPDATE batches SET quantity = quantity + ? WHERE id = ?", &st)) {
        st.bindInt(1, delta); st.bindInt(2, batchId);
        ok = st.step() == SQ.DONE;
        st.finalize();
    }
    if (ok) {
        ok = false;
        if (db.prepare("INSERT INTO stock_movements(batch_id, drug_id, type, qty, user_id, note) VALUES(?, ?, 'adjust', ?, ?, ?)", &st)) {
            st.bindInt(1, batchId); st.bindInt(2, drugId); st.bindInt(3, delta); st.bindInt(4, userId); st.bindText(5, reason);
            ok = st.step() == SQ.DONE;
            st.finalize();
        }
    }
    if (ok) db.commit(); else db.rollback();
    if (ok) return 0;
    return 1;
}

// ---- sale (first-expiry-first-out, one transaction) ----
struct CartLine { drugId: i64 = 0; qty: i64 = 0; price: i64 = 0; }

// Returns the sale id, or 0 with a message in err. Nothing is changed when it fails.
function createSale(userId: i64, customer: char*, err: Str.String*) => i64 {
    let lines: L.List<CartLine>;
    let st: SQ.Stmt;
    if (db.prepare("SELECT c.drug_id, c.qty, d.price_cents FROM cart c JOIN drugs d ON d.id = c.drug_id WHERE c.user_id = ? AND d.active = 1", &st)) {
        st.bindInt(1, userId);
        while (st.step() == SQ.ROW) {
            let ln: CartLine;
            ln.drugId = st.colInt(0); ln.qty = st.colInt(1); ln.price = st.colInt(2);
            lines.push(ln);
        }
        st.finalize();
    }
    if (lines.len == 0) {
        err.set("The cart is empty.");
        lines.free();
        return 0 as i64;
    }
    let ok: bool = true;
    let saleId: i64 = 0;
    let total: i64 = 0;
    db.begin();
    if (db.prepare("INSERT INTO sales(user_id, customer, total_cents) VALUES(?, ?, 0)", &st)) {
        st.bindInt(1, userId); st.bindText(2, customer);
        ok = st.step() == SQ.DONE;
        st.finalize();
    } else { ok = false; }
    if (ok) saleId = db.lastId();

    let li: u64 = 0;
    while (ok && li < lines.len) {
        let ln: CartLine* = lines.ptr(li);
        let remaining: i64 = ln.qty;
        while (ok && remaining > 0) {
            let bid: i64 = 0;
            let bq: i64 = 0;
            let found: bool = false;
            if (db.prepare("SELECT id, quantity FROM batches WHERE drug_id = ? AND quantity > 0 AND expiry_date >= date('now') ORDER BY expiry_date, id LIMIT 1", &st)) {
                st.bindInt(1, ln.drugId);
                if (st.step() == SQ.ROW) { found = true; bid = st.colInt(0); bq = st.colInt(1); }
                st.finalize();
            }
            if (!found) {
                ok = false;
                let nm: char* = "a drug";
                err.set("Not enough stock for ");
                if (db.prepare("SELECT name FROM drugs WHERE id = ?", &st)) {
                    st.bindInt(1, ln.drugId);
                    if (st.step() == SQ.ROW) err.append(st.colText(0));
                    st.finalize();
                }
                err.append(" (short by ");
                err.appendInt(remaining);
                err.append(").");
            } else {
                let take: i64 = remaining;
                if (bq < take) take = bq;
                let step1: bool = false;
                if (db.prepare("UPDATE batches SET quantity = quantity - ? WHERE id = ?", &st)) {
                    st.bindInt(1, take); st.bindInt(2, bid);
                    step1 = st.step() == SQ.DONE;
                    st.finalize();
                }
                let step2: bool = false;
                if (step1 && db.prepare("INSERT INTO sale_items(sale_id, batch_id, drug_id, qty, unit_cents) VALUES(?, ?, ?, ?, ?)", &st)) {
                    st.bindInt(1, saleId); st.bindInt(2, bid); st.bindInt(3, ln.drugId); st.bindInt(4, take); st.bindInt(5, ln.price);
                    step2 = st.step() == SQ.DONE;
                    st.finalize();
                }
                let step3: bool = false;
                if (step2 && db.prepare("INSERT INTO stock_movements(batch_id, drug_id, type, qty, user_id, note) VALUES(?, ?, 'sale', ?, ?, ?)", &st)) {
                    st.bindInt(1, bid); st.bindInt(2, ln.drugId); st.bindInt(3, 0 - take); st.bindInt(4, userId); st.bindText(5, "sale");
                    step3 = st.step() == SQ.DONE;
                    st.finalize();
                }
                if (!step3) { ok = false; err.set("Database error while recording the sale."); }
                else { remaining = remaining - take; total = total + take * ln.price; }
            }
        }
        li = li + 1;
    }
    if (ok) {
        ok = false;
        if (db.prepare("UPDATE sales SET total_cents = ? WHERE id = ?", &st)) {
            st.bindInt(1, total); st.bindInt(2, saleId);
            ok = st.step() == SQ.DONE;
            st.finalize();
        }
        if (!ok) err.set("Database error while totalling the sale.");
    }
    lines.free();
    if (ok) {
        db.commit();
        cartClear(userId);
        return saleId;
    }
    db.rollback();
    return 0 as i64;
}
