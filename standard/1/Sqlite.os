// standard/1/Sqlite.os
// SQLite wrapper over standard/1/c/sqlite.os.
//
//   let db: Sqlite.Db;
//   db.open("data/app.db");
//   db.exec("CREATE TABLE IF NOT EXISTS t(id INTEGER PRIMARY KEY, name TEXT)");
//   let st: Sqlite.Stmt;
//   if (db.prepare("INSERT INTO t(name) VALUES(?)", &st)) { st.bindText(1, "x"); st.step(); st.finalize(); }
//   if (db.prepare("SELECT id, name FROM t WHERE id > ?", &st)) {
//       st.bindInt(1, 0);
//       while (st.step() == Sqlite.ROW) { Console.println(st.colText(1)); }
//       st.finalize();
//   }
//
// Parameter indexes are 1-based, column indexes 0-based. colText never returns nullptr ("" for NULL) and the pointer is
// valid until the next step/reset/finalize: copy it if you keep it. Bound text is copied by SQLite (TRANSIENT).
// Threads: db.lock() / db.unlock() is a Mutex you take around a request's whole database work (and around begin..commit).
// Money: keep it as integer cents (INTEGER columns), never floating point.
module C = import "standard/1/c/sqlite.os";
module Th = import "standard/1/Thread.os";

let ROW: int = 100;
let DONE: int = 101;
let T_INT: int = 1;
let T_FLOAT: int = 2;
let T_TEXT: int = 3;
let T_NULL: int = 5;

// SQLITE_TRANSIENT is (void*)-1: SQLite copies the text. ADAPT: if the int -> void* cast is refused, change this one
// function (nullptr = SQLITE_STATIC works too, but then the string must outlive the step).
function transient() => void* {
    let m: i64 = 0 - 1;
    return m as void*;
}

struct Stmt {
    st: void* = nullptr;
    db: void* = nullptr;

    bindInt(i: int, v: i64) => bool { return C.sqlite3_bind_int64(this.st, i, v) == 0; }
    bindNum(i: int, v: double) => bool { return C.sqlite3_bind_double(this.st, i, v) == 0; }
    bindText(i: int, s: char*) => bool {
        if (s == nullptr) return C.sqlite3_bind_null(this.st, i) == 0;
        return C.sqlite3_bind_text(this.st, i, s, 0 - 1, transient()) == 0;
    }
    bindNull(i: int) => bool { return C.sqlite3_bind_null(this.st, i) == 0; }

    // 100 (Sqlite.ROW) = a row is ready, 101 (Sqlite.DONE) = finished, anything else = error
    step() => int { return C.sqlite3_step(this.st); }
    reset() => void { C.sqlite3_reset(this.st); C.sqlite3_clear_bindings(this.st); }

    colCount() => int { return C.sqlite3_column_count(this.st); }
    colType(i: int) => int { return C.sqlite3_column_type(this.st, i); }
    isNull(i: int) => bool { return C.sqlite3_column_type(this.st, i) == 5; }
    colInt(i: int) => i64 { return C.sqlite3_column_int64(this.st, i); }
    colNum(i: int) => double { return C.sqlite3_column_double(this.st, i); }
    colText(i: int) => char* {
        let t: char* = C.sqlite3_column_text(this.st, i);
        if (t == nullptr) return "";
        return t;
    }
    colName(i: int) => char* { return C.sqlite3_column_name(this.st, i); }

    finalize() => void {
        if (this.st != nullptr) C.sqlite3_finalize(this.st);
        this.st = nullptr;
    }
}

struct Db {
    h: void* = nullptr;
    m: Th.Mutex;
    isOpen: bool = false;

    open(path: char*) => bool {
        let rc: int = C.sqlite3_open(path, &this.h);
        if (rc != 0) return false;
        this.m.init();
        C.sqlite3_busy_timeout(this.h, 5000);
        this.isOpen = true;
        this.exec("PRAGMA foreign_keys = ON;");
        this.exec("PRAGMA journal_mode = WAL;");
        return true;
    }

    close() => void {
        if (this.isOpen) { C.sqlite3_close(this.h); this.m.free(); }
        this.isOpen = false;
        this.h = nullptr;
    }

    // last error text of this connection (valid until the next call)
    error() => char* { return C.sqlite3_errmsg(this.h); }

    exec(sql: char*) => bool {
        let e: char* = nullptr;
        let rc: int = C.sqlite3_exec(this.h, sql, nullptr, nullptr, &e);
        if (e != nullptr) C.sqlite3_free(e as void*);
        return rc == 0;
    }

    prepare(sql: char*, out: Stmt*) => bool {
        out.st = nullptr; out.db = this.h;
        let rc: int = C.sqlite3_prepare_v2(this.h, sql, 0 - 1, &out.st, nullptr);
        return rc == 0 && out.st != nullptr;
    }

    lastId() => i64 { return C.sqlite3_last_insert_rowid(this.h); }
    changes() => int { return C.sqlite3_changes(this.h); }

    begin() => bool { return this.exec("BEGIN IMMEDIATE;"); }
    commit() => bool { return this.exec("COMMIT;"); }
    rollback() => bool { return this.exec("ROLLBACK;"); }

    lock() => void { this.m.lock(); }
    unlock() => void { this.m.unlock(); }

    // first column of the first row as an integer, or `def` (count queries, existence checks, etc.)
    scalarInt(sql: char*, def: i64) => i64 {
        let st: Stmt;
        let v: i64 = def;
        if (this.prepare(sql, &st)) {
            if (st.step() == 100) v = st.colInt(0);
            st.finalize();
        }
        return v;
    }
}
