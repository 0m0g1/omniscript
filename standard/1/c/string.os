// standard/1/c/string.os
// Tier 0: raw C string, number-parsing and ctype bindings.

extern "C" fn strlen(s: char*) => u64;
extern "C" fn strcmp(a: char*, b: char*) => int;
extern "C" fn strncmp(a: char*, b: char*, n: u64) => int;
extern "C" fn strcpy(dst: char*, src: char*) => char*;
extern "C" fn strncpy(dst: char*, src: char*, n: u64) => char*;
extern "C" fn strcat(dst: char*, src: char*) => char*;
extern "C" fn strstr(hay: char*, needle: char*) => char*;
extern "C" fn strchr(s: char*, c: int) => char*;
extern "C" fn strrchr(s: char*, c: int) => char*;

extern "C" fn strtoll(s: char*, end: char**, base: int) => i64;
extern "C" fn strtoull(s: char*, end: char**, base: int) => u64;
extern "C" fn strtod(s: char*, end: char**) => double;

extern "C" fn toupper(c: int) => int;
extern "C" fn tolower(c: int) => int;
extern "C" fn isalpha(c: int) => int;
extern "C" fn isdigit(c: int) => int;
extern "C" fn isalnum(c: int) => int;
extern "C" fn isspace(c: int) => int;
extern "C" fn isupper(c: int) => int;
extern "C" fn islower(c: int) => int;
extern "C" fn ispunct(c: int) => int;
extern "C" fn isxdigit(c: int) => int;
