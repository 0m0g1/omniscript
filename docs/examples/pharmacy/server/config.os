// pharmacy/server/config.os -- settings. Run everything from the project root (paths are relative to it).
let PORT: u16 = 8090;
// Paths are relative to the folder you RUN from (the project root). Zip extracted into docs/examples:
let DB_PATH: char* = "docs/examples/pharmacy/data/pharmacy.db";
let STATIC_DIR: char* = "docs/examples/pharmacy/static";
let PAGE_SIZE: int = 20;
let SESSION_SECONDS: i64 = 1800;          // idle timeout
let PW_ITER: int = 100000;                // PBKDF2 iterations for new passwords (verification reads the count from the hash)
let CURRENCY: char* = "KSh ";
