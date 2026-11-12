// standard/1/c/sqlite.os
extern "dependencies/sqlite/sqlite-dll-win-x64-3500000/sqlite3.dll" {
    fn sqlite3_open(path: char*, db: void**) => int;
    fn sqlite3_close(db: void*) => int;
    fn sqlite3_exec(db: void*, sql: char*, cb: void*, arg: void*, err: char**) => int;
    fn sqlite3_prepare_v2(db: void*, sql: char*, n: int, st: void**, tail: char**) => int;
    fn sqlite3_step(st: void*) => int;
    fn sqlite3_finalize(st: void*) => int;
    fn sqlite3_reset(st: void*) => int;
    fn sqlite3_bind_int64(st: void*, i: int, v: i64) => int;
    fn sqlite3_bind_double(st: void*, i: int, v: double) => int;
    fn sqlite3_bind_text(st: void*, i: int, v: char*, n: int, d: void*) => int;
    fn sqlite3_column_count(st: void*) => int;
    fn sqlite3_column_int64(st: void*, i: int) => i64;
    fn sqlite3_column_double(st: void*, i: int) => double;
    fn sqlite3_column_text(st: void*, i: int) => char*;
    fn sqlite3_column_name(st: void*, i: int) => char*;
    fn sqlite3_errmsg(db: void*) => char*;
    fn sqlite3_free(p: void*) => void;
}
