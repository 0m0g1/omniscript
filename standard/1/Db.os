// standard/1/Db.os
// Tier 5: SQLite wrapper. Db.open/exec/prepare; Stmt.bind*/step/col*. Call close()/finalize().
module C = import "standard/1/c/sqlite.os";
module Str = import "standard/1/String.os";

let SQLITE_OK: int = 0;
let SQLITE_ROW: int = 100;
let SQLITE_DONE: int = 101;
let SQLITE_TRANSIENT: i64 = -1;

struct Stmt {
    st: void* = nullptr;
    finalize() => void { if (this.st != nullptr) { C.sqlite3_finalize(this.st); this.st = nullptr; } }
    reset() => bool { return C.sqlite3_reset(this.st) == SQLITE_OK; }
    // 1-based indices, like SQLite
    bindInt(i: int, v: i64) => bool { return C.sqlite3_bind_int64(this.st, i, v) == SQLITE_OK; }
    bindF64(i: int, v: double) => bool { return C.sqlite3_bind_double(this.st, i, v) == SQLITE_OK; }
    bindText(i: int, v: char*) => bool { return C.sqlite3_bind_text(this.st, i, v, -1, SQLITE_TRANSIENT as void*) == SQLITE_OK; }
    // true = row available, false = done/error
    step() => bool { return C.sqlite3_step(this.st) == SQLITE_ROW; }
    columns() => int { return C.sqlite3_column_count(this.st); }
    colName(i: int) => char* { return C.sqlite3_column_name(this.st, i); }
    colInt(i: int) => i64 { return C.sqlite3_column_int64(this.st, i); }
    colF64(i: int) => double { return C.sqlite3_column_double(this.st, i); }
    colText(i: int) => char* { return C.sqlite3_column_text(this.st, i); }
}

struct Db {
    h: void* = nullptr;
    open(path: char*) => bool { return C.sqlite3_open(path, &this.h) == SQLITE_OK; }   // ":memory:" for in-memory
    close() => void { if (this.h != nullptr) { C.sqlite3_close(this.h); this.h = nullptr; } }
    exec(sql: char*) => bool { return C.sqlite3_exec(this.h, sql, nullptr, nullptr, nullptr) == SQLITE_OK; }
    prepare(sql: char*, out: Stmt*) => bool { return C.sqlite3_prepare_v2(this.h, sql, -1, &out.st, nullptr) == SQLITE_OK; }
    error() => char* { return C.sqlite3_errmsg(this.h); }
    begin() => bool { return this.exec("BEGIN"); }
    commit() => bool { return this.exec("COMMIT"); }
    rollback() => bool { return this.exec("ROLLBACK"); }

    // Runs body(arg) inside BEGIN/COMMIT; ROLLBACK (and false) if body returns false or COMMIT fails.
    transaction(body: fn(void*) => bool, arg: void*) => bool {
        if (!this.begin()) return false;
        if (body(arg) && this.commit()) return true;
        this.rollback();
        return false;
    }

    // First column of the first row as an integer. false if the statement fails or returns no row.
    queryInt(sql: char*, out: i64*) => bool {
        let st: Stmt;
        if (!this.prepare(sql, &st)) return false;
        let ok: bool = st.step();
        if (ok) out[0] = st.colInt(0);
        st.finalize();
        return ok;
    }

    // First column of the first row as text, appended to out.
    queryText(sql: char*, out: Str.String*) => bool {
        let st: Stmt;
        if (!this.prepare(sql, &st)) return false;
        let ok: bool = st.step();
        if (ok) {
            let t: char* = st.colText(0);
            if (t != nullptr) ok = out.append(t); else ok = false;
        }
        st.finalize();
        return ok;
    }
}
