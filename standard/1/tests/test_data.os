// standard/1/tests/test_data.os  (Tier 3 + 6: Text, Regex, Json, Csv, Binary, Crypto, Uuid, Compress, DateTime)
// Written against the real out-parameter API of each module (no static methods, no null wrappers).
module H = import "standard/1/tests/Harness.os";
module Text = import "standard/1/Text.os";
module Rx = import "standard/1/Regex.os";
module Json = import "standard/1/Json.os";
module Csv = import "standard/1/Csv.os";
module Bin = import "standard/1/Binary.os";
module Crypto = import "standard/1/Crypto.os";
module U = import "standard/1/Uuid.os";   // alias is U (not Uuid): the module and its struct are both called Uuid
module Z = import "standard/1/Compress.os";
module DT = import "standard/1/DateTime.os";
module Str = import "standard/1/String.os";
module CMem = import "standard/1/c/mem.os";

// Compares a binary digest to its expected lowercase hex string.
function eqHex(d: u8*, n: u64, want: char*, name: char*) => void {
    let hex: [129]char;
    Bin.toHex(d, n, &hex as char*);
    H.eqStr(&hex as char*, want, name);
}

function main() => int {
    H.suite("Text");
    let b: Str.String;
    Text.base64EncodeStr("hello", &b);
    H.eqStr(b.cstr(), "aGVsbG8=", "base64 encode");
    let d: Str.String;
    Text.base64DecodeStr("aGVsbG8=", &d);
    H.eqStr(d.cstr(), "hello", "base64 decode");
    let hx: Str.String;
    Text.hexEncodeStr("AB", &hx);
    H.eqStr(hx.cstr(), "4142", "hex encode");
    let ue: Str.String;
    Text.urlEncode("a b&c", &ue);
    H.eqStr(ue.cstr(), "a%20b%26c", "url encode");
    let ud: Str.String;
    Text.urlDecode("a%20b%26c", &ud);
    H.eqStr(ud.cstr(), "a b&c", "url decode");
    b.free(); d.free(); hx.free(); ue.free(); ud.free();

    H.suite("Regex");
    H.check(Rx.validate("[0-9]+"), "valid pattern accepted");
    H.check(!Rx.validate("([unclosed"), "unclosed bracket rejected");
    H.check(Rx.matchAll("[0-9]+", "12345"), "matches");
    H.check(!Rx.matchAll("[0-9]+", "abc"), "no match");
    let fs: u64 = 0;
    let fl: u64 = 0;
    H.check(Rx.find("[0-9]+", "abc 42 def", &fs, &fl), "find hit");
    H.eqInt(fs as int, 4, "find start");
    H.eqInt(fl as int, 2, "find length");
    H.eqInt(Rx.countMatches("[0-9]+", "1 22 333") as int, 3, "findAll count");
    let rp: Str.String;
    Rx.replaceAll("[0-9]+", "a1b22", "#", &rp);
    H.eqStr(rp.cstr(), "a#b#", "replace");
    rp.free();

    H.suite("Json");
    let j = Json.parse("{\"name\":\"omni\",\"n\":42,\"ok\":true,\"list\":[1,2,3]}");
    H.check(j != nullptr, "parse ok");
    H.eqStr(j.get("name").asStr(""), "omni", "string field");
    H.eqInt(j.get("n").asNum(0.0) as int, 42, "int field");
    H.check(j.get("ok").asBool(false), "bool field");
    H.eqInt(j.get("list").at(2).asNum(0.0) as int, 3, "array index");
    H.eqInt(Json.getInt(j, "n", 0) as int, 42, "getInt");
    H.eqInt(j.get("list").length() as int, 3, "array length");
    H.check(j.get("missing") == nullptr, "missing key is nullptr");
    j.free(); CMem.free(j as void*);

    H.check(Json.parse("{bad json") == nullptr, "bad json returns nullptr");

    let o = Json.newObject();
    Json.setNum(o, "a", 1.0);
    Json.setStr(o, "b", "two");
    let js: Str.String;
    Json.stringify(o, &js, false);
    let round = Json.parse(js.cstr());
    H.check(round != nullptr, "roundtrip parse");
    H.eqInt(round.get("a").asNum(0.0) as int, 1, "roundtrip int");
    H.eqStr(round.get("b").asStr(""), "two", "roundtrip string");
    round.free(); CMem.free(round as void*);
    js.free();
    o.free(); CMem.free(o as void*);

    let esc = Json.parse("{\"s\":\"line\\nbreak \\\"q\\\"\"}");
    H.check(esc != nullptr, "escape parse");
    H.eqStr(esc.get("s").asStr(""), "line\nbreak \"q\"", "escape sequences");
    esc.free(); CMem.free(esc as void*);

    H.suite("Csv");
    let c: Csv.Table;
    Csv.parse("name,age\nann,30\nbob,25\n", ',', &c);
    H.eqInt(c.rowCount() as int, 3, "row count (header + 2)");
    H.eqStr(c.cell(0, 1), "age", "header");
    H.eqStr(c.cell(2, 0), "bob", "cell");
    c.free();
    let q: Csv.Table;
    Csv.parse("a,b\n\"x,y\",z\n", ',', &q);
    H.eqStr(q.cell(1, 0), "x,y", "quoted comma");
    q.free();

    H.suite("Binary");
    let w: Bin.ByteWriter;
    w.u8(7); w.u32(0xDEADBEEF); w.u16(513);
    let r: Bin.ByteReader;
    r.init(w.bytes(), w.length());
    let v8: u8 = 0;
    let v32: u32 = 0;
    let v16: u16 = 0;
    H.check(r.u8(&v8), "read u8 ok");
    H.check(r.u32(&v32), "read u32 ok");
    H.check(r.u16(&v16), "read u16 ok");
    H.eqInt(v8 as int, 7, "u8");
    H.check(v32 == 0xDEADBEEF, "u32");
    H.eqInt(v16 as int, 513, "u16");
    H.check(r.eof(), "reader at end");
    w.free();

    H.suite("Crypto");
    let abc: char* = "abc";
    let same: char* = "same";
    let diff: char* = "diff";
    let dg: [32]u8;
    Crypto.sha256(abc as u8*, 3, &dg as u8*);
    eqHex(&dg as u8*, 32, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "sha256 abc");
    Crypto.md5(abc as u8*, 3, &dg as u8*);
    eqHex(&dg as u8*, 16, "900150983cd24fb0d6963f7d28e17f72", "md5 abc");
    Crypto.sha1(abc as u8*, 3, &dg as u8*);
    eqHex(&dg as u8*, 20, "a9993e364706816aba3e25717850c26c9cd0d89d", "sha1 abc");
    let rnd: [16]u8;
    H.check(Crypto.randomBytes(&rnd as u8*, 16), "randomBytes ok");
    H.check(Crypto.constantTimeEquals(same as u8*, same as u8*, 4), "ct equal");
    H.check(!Crypto.constantTimeEquals(same as u8*, diff as u8*, 4), "ct differ");

    H.suite("Uuid");
    let u1: U.Uuid;
    let u2: U.Uuid;
    H.check(U.v4(&u1), "v4 generated");
    H.check(U.v4(&u2), "second v4 generated");
    let s1: Str.String;
    let s2: Str.String;
    U.toString(&u1, &s1);
    U.toString(&u2, &s2);
    H.eqInt(s1.length() as int, 36, "uuid length");
    H.check((s1.cstr()[14] as int) == 52, "version nibble is 4");
    H.check(!s1.equals(&s2), "uuids differ (probabilistic)");
    let u3: U.Uuid;
    H.check(U.parse(s1.cstr(), &u3), "parse roundtrip ok");
    let s3: Str.String;
    U.toString(&u3, &s3);
    H.check(s1.equals(&s3), "parse roundtrip equal");
    s1.free(); s2.free(); s3.free();

    H.suite("Compress");
    let input: Str.String;
    Text.repeat("a", 80, &input);
    let packedLen: u64 = 0;
    let packed = Z.gzip(input.cstr() as u8*, input.length(), 6, &packedLen);
    H.check(packed != nullptr, "gzip ok");
    H.check(packedLen < input.length() + 18, "gzip output sane");
    let outLen: u64 = 0;
    let back = Z.gunzip(packed, packedLen, 1024, &outLen);
    H.check(back != nullptr, "gunzip ok");
    H.eqInt(outLen as int, 80, "gunzip length");
    H.eqStr(back as char*, input.cstr(), "gzip roundtrip");
    CMem.free(back as void*);
    CMem.free(packed as void*);
    input.free();

    H.suite("DateTime");
    let dt: DT.DateTime;
    H.check(dt.parseIso("2024-02-28T12:00:00"), "parse");
    let dt2: DT.DateTime;
    dt2.fromUnix(dt.toUnix());
    dt2.addDays(2);
    let iso: Str.String;
    dt2.iso(&iso);
    H.eqStr(iso.cstr(), "2024-03-01T12:00:00Z", "leap year addDays");
    iso.free();
    H.eqInt(DT.diffSeconds(&dt2, &dt) as int, 2 * 86400, "diff seconds");
    let nowDt = DT.now();
    H.check(nowDt.toUnix() > 1700000000, "now is sane");

    return H.finish();
}
