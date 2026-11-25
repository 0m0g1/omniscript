// pharmacy/server/sqlite_smoke.os -- run from the project root: Osengine.exe pharmacy/server/sqlite_smoke.os
module Console = import "standard/1/Console.os";
module Sqlite = import "standard/1/Sqlite.os";

function main() => int {
    let db: Sqlite.Db;
    if (!db.open("docs/examples/pharmacy/data/smoke.db")) { Console.error("cannot open db (does the pharmacy/data folder exist?)"); return 1; }
    db.exec("DROP TABLE IF EXISTS t");
    db.exec("CREATE TABLE t(id INTEGER PRIMARY KEY, name TEXT, price INTEGER)");
    let st: Sqlite.Stmt;
    if (db.prepare("INSERT INTO t(name, price) VALUES(?, ?)", &st)) {
        st.bindText(1, "Paracetamol 500mg"); st.bindInt(2, 1250 as i64); st.step(); st.reset();
        st.bindText(1, "O'Brien \"quoted\"; DROP TABLE t;--"); st.bindInt(2, 99 as i64); st.step();
        st.finalize();
    } else { Console.error(db.error()); return 1; }
    Console.println("rows: "); Console.println(db.scalarInt("SELECT COUNT(*) FROM t", 0 as i64));
    if (db.prepare("SELECT id, name, price FROM t ORDER BY id", &st)) {
        while (st.step() == Sqlite.ROW) {
            Console.println(st.colInt(0)); Console.println(st.colText(1)); Console.println(st.colInt(2));
        }
        st.finalize();
    }
    db.begin(); db.exec("DELETE FROM t"); db.rollback();
    Console.println("after rollback: "); Console.println(db.scalarInt("SELECT COUNT(*) FROM t", 0 as i64));
    db.close();
    return 0;
}
