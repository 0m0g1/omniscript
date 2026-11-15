// standard/1/tests/test_io.os  (Tier 4: IO, Sys)
// API usage is taken from foundation.os (known to work). The Path / Fs / Process / Cli sections
// from the previous version are not here yet: their real signatures aren't known (see notes).
module H = import "standard/1/tests/Harness.os";
module IO = import "standard/1/IO.os";
module Sys = import "standard/1/Sys.os";
module Str = import "standard/1/String.os";

function main() => int {
    H.suite("IO");
    H.check(IO.writeFile("os_test_tmp.txt", "line one\nline two\n"), "writeFile");
    let text: Str.String;
    H.check(IO.readFile("os_test_tmp.txt", &text), "readFile");
    H.check(text.length() == 18, "file length");

    let f: IO.File;
    H.check(f.open("os_test_tmp.txt", IO.MODE_READ), "open");
    let line: Str.String;
    H.check(f.readLine(&line) && line.equalsC("line one"), "readLine 1");
    H.check(f.readLine(&line) && line.equalsC("line two"), "readLine 2");
    H.check(!f.readLine(&line), "readLine at EOF");
    f.close();
    H.check(IO.deleteFile("os_test_tmp.txt"), "deleteFile");
    H.check(!IO.readFile("os_test_tmp.txt", &text), "readFile of a missing file fails");
    text.free();
    line.free();

    H.suite("Sys");
    let v: Str.String;
    H.check(Sys.setEnv("OS_STD_TEST", "abc"), "setEnv");
    H.check(Sys.env("OS_STD_TEST", &v) && v.equalsC("abc"), "env roundtrip");
    H.check(!Sys.hasEnv("OS_STD_DOES_NOT_EXIST"), "hasEnv false");
    H.check(Sys.cwd(&v) && v.length() > 0, "cwd");
    H.check(Sys.cpuCount() >= 1, "cpuCount");
    v.free();

    return H.finish();
}
