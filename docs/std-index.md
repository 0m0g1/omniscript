# std — documented items

112 items in 4 modules.

## std/Console

Printing to the terminal and reading a line from it.

- `let useColor: bool = true` — Whether info, warn and error color their [info], [warn] and [error] tags
- `function setColor(on: bool) => void` — Turns the colored tags of info, warn and error on or off.
- `function out() => void*` — The C stdout stream, for use with the raw C functions.
- `function err() => void*` — The C stderr stream, for use with the raw C functions.
- `function print(s: char*) => void` — Writes s to stdout without a trailing newline.
- `function println(s: char*) => void` — Writes s to stdout, followed by a newline.
- `function log(s: char*) => void` — Same as println. Provided for people used to console.log.
- `function info(s: char*) => void` — Writes s to stdout as an informational line, starting with an [info] tag.
- `function warn(s: char*) => void` — Writes s to **stderr**, starting with a [warn] tag (yellow unless setColor(false)).
- `function error(s: char*) => void` — Writes s to **stderr**, starting with an [error] tag (red unless setColor(false)).
- `function printString(s: Str.String*) => void` — Like print, but takes a String.
- `function printlnString(s: Str.String*) => void` — Like println, but takes a String.
- `function infoString(s: Str.String*) => void` — Like info, but takes a String.
- `function warnString(s: Str.String*) => void` — Like warn, but takes a String.
- `function errorString(s: Str.String*) => void` — Like error, but takes a String.
- `function flush() => void` — Forces everything written to stdout so far to appear now.
- `function readLine(out: Str.String*) => bool` — Reads one line from stdin into out, without the trailing \n or \r\n.
- `function clear() => void` — Clears the terminal and moves the cursor to the top-left corner.

## std/IO

Files and standard streams on top of C stdio.

- `let MODE_READ: char* = "rb"` — Open an existing file for reading.
- `let MODE_WRITE: char* = "wb"` — Create the file, or empty it if it exists, and open it for writing.
- `let MODE_APPEND: char* = "ab"` — Open the file for writing at the end, creating it if needed.
- `let MODE_READWRITE: char* = "r+b"` — Open an existing file for reading and writing.
- `let SEEK_SET: int = 0` — seek origin: the start of the file.
- `let SEEK_CUR: int = 1` — seek origin: the current position.
- `let SEEK_END: int = 2` — seek origin: the end of the file.
- `struct File` — An open file or standard stream. Call open (or attach), use it, then close.
- `File.handle: void* = nullptr` — The underlying C FILE*.
- `File.isOpen: bool = false` — true while the file is open.
- `File.open(path: char*, mode: char*) => bool` — Opens path with one of the MODE_ strings. Returns false if it could not be opened.
- `File.attach(which: uint) => void` — Attaches a process stream: 0 is stdin, 1 is stdout, 2 is stderr. Do not close() these.
- `File.raw() => void*` — Escape hatch: returns the C FILE*.
- `File.close() => void` — Closes the file. Safe to call on a file that is not open.
- `File.flush() => void` — Writes any buffered output to the file.
- `File.eof() => bool` — true once a read has hit the end of the file.
- `File.hasError() => bool` — true if an I/O error has occurred on this file.
- `File.tell() => i64` — Returns the current position, in bytes from the start.
- `File.seek(offset: i64, origin: int) => bool` — Moves to offset bytes from origin (SEEK_SET, SEEK_CUR or SEEK_END).
- `File.size() => i64` — Returns the file size in bytes. The current position is left unchanged.
- `File.readBytes(buf: void*, n: u64) => u64` — Reads up to n bytes into buf. Returns how many were read (0 at end of file).
- `File.readAll(out: Str.String*) => bool` — Replaces out with the whole file contents, read from the start.
- `File.readLine(out: Str.String*) => bool` — Replaces out with the next line, without the trailing \n or \r\n.
- `File.writeBytes(buf: void*, n: u64) => bool` — Writes n bytes from buf. Returns false if fewer were written.
- `File.write(text: char*) => bool` — Writes the C string text.
- `File.writeString(s: Str.String*) => bool` — Writes the contents of a String.
- `File.writeLine(text: char*) => bool` — Writes the C string text followed by a newline.
- `File.writeLineString(s: Str.String*) => bool` — Writes a String followed by a newline.
- `function readFile(path: char*, out: Str.String*) => bool` — Reads the whole file at path into out. Returns false if it could not be opened or read.
- `function writeFile(path: char*, text: char*) => bool` — Creates or overwrites the file at path with the C string text.
- `function writeFileString(path: char*, s: Str.String*) => bool` — Creates or overwrites the file at path with the contents of a String.
- `function appendFile(path: char*, text: char*) => bool` — Appends the C string text to the file at path, creating it if needed.
- `function deleteFile(path: char*) => bool` — Deletes the file at path. Returns false on failure.
- `function renameFile(src: char*, to: char*) => bool` — Renames or moves the file src to to. Returns false on failure.

## std/String

Owned, growable, always NUL-terminated byte string (UTF-8 safe, byte-indexed).

- `function isSpaceChar(c: int) => bool` — true if c is a space, tab, newline or carriage return.
- `struct String` — An owned, growable, NUL-terminated byte string. See the ownership rules at the top of this page.
- `String.data: char* = nullptr` — The buffer, or nullptr before the first allocation. Always NUL-terminated once allocated.
- `String.len: u64 = 0` — Length in bytes, not counting the terminator.
- `String.cap: u64 = 0` — Allocated size in bytes, including room for the terminator.
- `String.reserve(n: u64) => bool` — Makes room for n bytes plus the terminator, keeping the existing contents.
- `String.free() => void` — Releases the buffer and resets the string to empty. Call this when you are done with it.
- `String.clear() => void` — Empties the string but keeps the allocated buffer for reuse.
- `String.truncate(n: u64) => void` — Shortens the string to n bytes. Does nothing if it is already n bytes or shorter.
- `String.length() => u64` — Returns the length in bytes.
- `String.isEmpty() => bool` — true if the string has no bytes.
- `String.cstr() => char*` — Returns the contents as a C string. Always valid, even for an empty or unallocated String.
- `String.set(src: char*) => bool` — Replaces the contents with the C string src.
- `String.appendN(src: char*, n: u64) => bool` — Appends the first n bytes of src.
- `String.append(src: char*) => bool` — Appends the C string src.
- `String.appendString(other: String*) => bool` — Appends another String.
- `String.appendChar(c: char) => bool` — Appends one byte.
- `String.appendInt(v: i64) => bool` — Appends the decimal text of v.
- `String.clone(out: String*) => bool` — Copies this string into out, replacing its contents. Use this instead of =.
- `String.equals(other: String*) => bool` — true if both strings have the same bytes.
- `String.equalsC(s: char*) => bool` — true if the string equals the C string s.
- `String.startsWith(prefix: char*) => bool` — true if the string begins with prefix.
- `String.endsWith(suffix: char*) => bool` — true if the string ends with suffix.
- `String.indexOf(needle: char*) => i64` — Returns the byte index of the first match of needle, or -1 if there is none.
- `String.contains(needle: char*) => bool` — true if needle occurs anywhere in the string.
- `String.substring(start: u64, count: u64, out: String*) => bool` — Copies the bytes from start to start + count (clamped to the end) into out,
- `String.parseInt(out: i64*) => bool` — Parses the whole string as a base-10 integer into out[0].
- `String.trim() => void` — Removes leading and trailing whitespace, in place.
- `String.toUpper() => void` — Converts every byte to uppercase, in place (ASCII).
- `String.toLower() => void` — Converts every byte to lowercase, in place (ASCII).
- `String.replace(find: char*, with: char*) => bool` — Replaces every occurrence of find with with, in place.

## std/Text

Text helpers over `char*` and `String`: split, join, pad, parse, escape, UTF-8, hex, URL and base64.

- `function split(s: char*, sep: char*, out: L.List<Str.String>*) => bool` — Splits s on every occurrence of sep and pushes each part onto out.
- `function freeParts(parts: L.List<Str.String>*) => void` — Frees every String in parts, then the list itself. Use it on a list filled by split.
- `function join(parts: L.List<Str.String>*, sep: char*, out: Str.String*) => bool` — Appends all parts to out, separated by sep.
- `function repeat(s: char*, n: u64, out: Str.String*) => bool` — Appends s to out, n times.
- `function padLeft(s: char*, width: u64, fill: char, out: Str.String*) => bool` — Appends s to out, left-padded with fill until it is at least width bytes long.
- `function padRight(s: char*, width: u64, fill: char, out: Str.String*) => bool` — Appends s to out, right-padded with fill until it is at least width bytes long.
- `function parseF64(s: char*, out: double*) => bool` — Parses a floating-point number from the start of s into out[0].
- `function parseU64(s: char*, base: int, out: u64*) => bool` — Parses an unsigned integer in the given base (2 to 36) from the start of s into out[0].
- `function isDigit(c: int) => bool` — true if c is an ASCII digit (0 to 9).
- `function isAlpha(c: int) => bool` — true if c is an ASCII letter.
- `function isAlnum(c: int) => bool` — true if c is an ASCII letter or digit.
- `function isSpace(c: int) => bool` — true if c is ASCII whitespace (space, tab, newline, vertical tab, form feed, carriage return).
- `function escape(s: char*, out: Str.String*) => bool` — Appends s to out with ", \ and the control characters newline, carriage return and tab
- `function appendUtf8(cp: u32, out: Str.String*) => bool` — Appends the UTF-8 encoding of the code point cp to out (1 to 4 bytes).
- `function utf8Length(s: char*) => u64` — Returns the number of code points in the UTF-8 string s (not the number of bytes).
- `function hexDigit(n: int, upper: bool) => int` — Returns the hex digit character for n in 0 to 15, lowercase or uppercase.
- `function b64Char(v: int, urlSafe: bool) => int` — Returns the base64 alphabet character for v in 0 to 63.
- `function hexEncode(p: u8*, n: u64, out: Str.String*) => bool` — Appends the lowercase hex encoding of the n bytes at p to out.
- `function hexNibble(c: int) => int` — Returns the value (0 to 15) of the hex digit character c, or -1 if it is not a hex digit.
- `function hexDecode(s: char*, out: L.List<u8>*) => bool` — Decodes the hex string s and pushes the bytes onto out (half as many bytes as characters).
- `function urlEncode(s: char*, out: Str.String*) => bool` — Appends the percent-encoding of s to out.
- `function urlDecode(s: char*, out: Str.String*) => bool` — Appends the percent-decoding of s to out. + becomes a space.
- `function base64EncodeOpts(p: u8*, n: u64, urlSafe: bool, pad: bool, out: Str.String*) => bool` — Appends the base64 encoding of the n bytes at p to out.
- `function base64Encode(p: u8*, n: u64, out: Str.String*) => bool` — Appends the standard, padded base64 encoding of the n bytes at p to out.
- `function base64EncodeStr(s: char*, out: Str.String*) => bool` — Appends the standard, padded base64 encoding of the C string s to out.
- `function b64Val(c: int) => int` — Returns the value (0 to 63) of the base64 character c (standard or URL-safe),
- `function base64Decode(s: char*, out: L.List<u8>*) => bool` — Decodes standard or URL-safe base64 from s and pushes the bytes onto out.
- `function base64DecodeStr(s: char*, out: Str.String*) => bool` — Decodes base64 from s into a String, for text payloads. See base64Decode for the rules.
- `function hexEncodeStr(s: char*, out: Str.String*) => bool` — Appends the lowercase hex encoding of the bytes of the C string s to out.

