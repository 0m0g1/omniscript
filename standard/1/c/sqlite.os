// standard/1/c/sqlite.os
extern "C" fn sqlite3_open(path: char*, db: void**) => int;
extern "C" fn sqlite3_close(db: void*) => int;
extern "C" fn sqlite3_exec(db: void*, sql: char*, cb: void*, arg: void*, err: char**) => int;
extern "C" fn sqlite3_prepare_v2(db: void*, sql: char*, n: int, st: void**, tail: char**) => int;
extern "C" fn sqlite3_step(st: void*) => int;
extern "C" fn sqlite3_finalize(st: void*) => int;
extern "C" fn sqlite3_reset(st: void*) => int;
extern "C" fn sqlite3_bind_int64(st: void*, i: int, v: i64) => int;
extern "C" fn sqlite3_bind_double(st: void*, i: int, v: double) => int;
extern "C" fn sqlite3_bind_text(st: void*, i: int, v: char*, n: int, d: void*) => int;
extern "C" fn sqlite3_column_count(st: void*) => int;
extern "C" fn sqlite3_column_int64(st: void*, i: int) => i64;
extern "C" fn sqlite3_column_double(st: void*, i: int) => double;
extern "C" fn sqlite3_column_text(st: void*, i: int) => char*;
extern "C" fn sqlite3_column_name(st: void*, i: int) => char*;
extern "C" fn sqlite3_errmsg(db: void*) => char*;
extern "C" fn sqlite3_free(p: void*) => void;
