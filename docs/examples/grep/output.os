/// grep output: buffered stdout, stderr messages, optional ANSI color.
/// One reusable Str.String builds each line, then a single write goes to the C stdio buffer.
/// NOTE: --color emits ANSI escapes; Windows Terminal handles them, the legacy console may need VT enabled.
/// NEXT STEPS: -o (print only matches), -b byte offsets, --null, a bigger explicit output buffer.

module IO = import "standard/1/IO.os";
module Str = import "standard/1/String.os";

struct Out {
    f: IO.File;                      // stdout
    e: IO.File;                      // stderr
    buf: Str.String;                 // the line being built
    color: bool = false;
}

function init(o: Out*, color: bool) => void {
    o.f.attach(1);
    o.e.attach(2);
    o.color = color;
}

function destroy(o: Out*) => void { o.buf.free(); }

function flush(o: Out*) => void { o.f.flush(); }

function colorStart(o: Out*, code: char*) => void {
    if (!o.color) return;
    o.buf.appendChar(27 as char);
    o.buf.append("[");
    o.buf.append(code);
    o.buf.append("m");
}

function colorEnd(o: Out*) => void {
    if (!o.color) return;
    o.buf.appendChar(27 as char);
    o.buf.append("[0m");
}

function sep(o: Out*) => void {
    colorStart(o, "36");
    o.buf.append(":");
    colorEnd(o);
}

// Appends "name:" and/or "lineNo:" to the line being built.
function prefix(o: Out*, name: char*, showName: bool, lineNo: i64, showNum: bool) => void {
    if (showName) { colorStart(o, "35"); o.buf.append(name); colorEnd(o); sep(o); }
    if (showNum) { colorStart(o, "32"); o.buf.appendInt(lineNo); colorEnd(o); sep(o); }
}

// Terminates the line, writes it, and resets the buffer.
function endLine(o: Out*) => void {
    o.buf.append("\n");
    o.f.write(o.buf.cstr());
    o.buf.clear();
}

// "grep: name: message" on stderr.
function error(o: Out*, name: char*, msg: char*) => void {
    o.f.flush();                                         // keep stdout/stderr ordering sane
    o.e.write("grep: ");
    if (name != nullptr) { o.e.write(name); o.e.write(": "); }
    o.e.write(msg);
    o.e.write("\n");
}
