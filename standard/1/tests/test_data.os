// standard/1/tests/test_data.os  (Tier 3 + 6: Text, Regex, Json, Csv, Binary, Crypto, Uuid, Compress, DateTime)
module H = import "standard/1/tests/Harness.os";
module Text = import "standard/1/Text.os";
module Rx = import "standard/1/Regex.os";
module Json = import "standard/1/Json.os";
module Csv = import "standard/1/Csv.os";
module Bin = import "standard/1/Binary.os";
module Crypto = import "standard/1/Crypto.os";
module Uuid = import "standard/1/Uuid.os";
module Z = import "standard/1/Compress.os";
module DT = import "standard/1/DateTime.os";
module Str = import "standard/1/String.os";

function main() => int {
    H.suite("Text");
    H.eqStr(Text.base64Encode("hello").cstr(), "aGVsbG8=", "base64 encode");
    H.eqStr(Text.base64Decode("aGVsbG8=").cstr(), "hello", "base64 decode");
    H.eqStr(Text.hexEncode("AB").cstr(), "4142", "hex encode");
    H.eqStr(Text.urlEncode("a b&c").cstr(), "a%20b%26c", "url encode");
    H.eqStr(Text.urlDecode("a%20b%26c").cstr(), "a b&c", "url decode");

    H.suite("Regex");
    let re = Rx.Regex.compile("[0-9]+");
    H.check(re != null, "compile ok");
    H.check(Rx.Regex.compile("([unclosed") == null, "bad pattern returns null");
    H.check(re.matches("12345"), "matches");
    H.check(!re.matches("abc"), "no match");
    H.eqStr(re.find("abc 42 def").cstr(), "42", "find");
    H.eqInt(re.findAll("1 22 333").length(), 3, "findAll");
    H.eqStr(re.replace("a1b22", "#").cstr(), "a#b#", "replace");

    H.suite("Json");
    let j = Json.Json.parse("{\"name\":\"omni\",\"n\":42,\"ok\":true,\"list\":[1,2,3]}");
    H.check(j != null, "parse ok");
    H.eqStr(j.get("name").asString(), "omni", "string field");
    H.eqInt(j.get("n").asInt(), 42, "int field");
    H.check(j.get("ok").asBool(), "bool field");
    H.eqInt(j.get("list").at(2).asInt(), 3, "array index");
    H.check(Json.Json.parse("{bad json") == null, "bad json returns null");
    let o = Json.Json.object();
    o.set("a", 1);
    o.set("b", "two");
    let round = Json.Json.parse(o.stringify());
    H.eqInt(round.get("a").asInt(), 1, "roundtrip int");
    H.eqStr(round.get("b").asString(), "two", "roundtrip string");
    let esc = Json.Json.parse("{\"s\":\"line\\nbreak \\\"q\\\"\"}");
    H.eqStr(esc.get("s").asString(), "line\nbreak \"q\"", "escape sequences");

    H.suite("Csv");
    let c = Csv.Csv.parse("name,age\nann,30\nbob,25\n");
    H.eqInt(c.rows().length(), 2, "row count");
    H.eqStr(c.header().get(1).cstr(), "age", "header");
    let q = Csv.Csv.parse("a,b\n\"x,y\",z\n");
    H.eqStr(q.rows().get(0).get(0).cstr(), "x,y", "quoted comma");

    H.suite("Binary");
    let w = Bin.ByteWriter.new();
    w.writeU8(7); w.writeU32(0xDEADBEEF); w.writeU16(513);
    let r = Bin.ByteReader.from(w.bytes(), w.length());
    H.eqInt(r.readU8() as int, 7, "u8");
    H.eqInt((r.readU32() == 0xDEADBEEF) as int, 1, "u32");
    H.eqInt(r.readU16() as int, 513, "u16");

    H.suite("Crypto");
    H.eqStr(Crypto.sha256("abc").cstr(),
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "sha256 abc");
    H.eqStr(Crypto.md5("abc").cstr(), "900150983cd24fb0d6963f7d28e17f72", "md5 abc");
    H.eqStr(Crypto.sha1("abc").cstr(), "a9993e364706816aba3e25717850c26c9cd0d89d", "sha1 abc");
    H.eqInt(Crypto.randomBytes(16).length(), 16, "randomBytes length");
    H.check(Crypto.constantTimeEquals("same", "same"), "ct equal");
    H.check(!Crypto.constantTimeEquals("same", "diff"), "ct differ");

    H.suite("Uuid");
    let u = Uuid.uuid4().toString();
    H.eqInt(u.length(), 36, "uuid length");
    H.check(u.cstr()[14] == '4', "version nibble is 4");
    H.check(Uuid.uuid4().toString().cstr()[0] != 0, "second uuid generated");
    H.check(u.cstr()[0] != Uuid.uuid4().toString().cstr()[0] || u.cstr()[1] != Uuid.uuid4().toString().cstr()[1], "uuids differ (probabilistic)");

    H.suite("Compress");
    let input = Str.from("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    let packed = Z.gzip(input);
    H.check(packed.length() < input.length(), "gzip shrinks repetitive data");
    H.eqStr(Z.gunzip(packed).cstr(), input.cstr(), "gzip roundtrip");

    H.suite("DateTime");
    let d = DT.DateTime.parse("2024-02-28 12:00:00");
    H.check(d != null, "parse");
    let d2 = d.addDays(2);
    H.eqStr(d2.format("%Y-%m-%d"), "2024-03-01", "leap year addDays");
    H.eqInt(d2.diff(d), 2 * 86400, "diff seconds");
    H.check(DT.DateTime.now().unixSeconds() > 1700000000, "now is sane");

    return H.finish();
}
