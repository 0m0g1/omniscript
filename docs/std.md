# std

## Index

- [std/Audio](#stdaudio)
- [std/Audio/CA](#stdaudioca)
- [std/Bench](#stdbench)
- [std/Binary](#stdbinary)
- [std/Canvas](#stdcanvas)
- [std/Canvas/F](#stdcanvasf)
- [std/Canvas/LF](#stdcanvaslf)
- [std/Canvas/LU](#stdcanvaslu)
- [std/CConsole](#stdcconsole)
- [std/CCurl](#stdccurl)
- [std/CDl](#stdcdl)
- [std/CFfmpeg](#stdcffmpeg)
- [std/CFile](#stdcfile)
- [std/CFont](#stdcfont)
- [std/CGlfw](#stdcglfw)
- [std/Channel](#stdchannel)
- [std/CImage](#stdcimage)
- [std/Cli](#stdcli)
- [std/CMath](#stdcmath)
- [std/CMem](#stdcmem)
- [std/CNet](#stdcnet)
- [std/Collections](#stdcollections)
- [std/Collections/Algo](#stdcollectionsalgo)
- [std/Collections/Map](#stdcollectionsmap)
- [std/Collections/PriorityQueue](#stdcollectionspriorityqueue)
- [std/Collections/Queue](#stdcollectionsqueue)
- [std/Collections/Queue/D](#stdcollectionsqueued)
- [std/Collections/Set](#stdcollectionsset)
- [std/Collections/Stack](#stdcollectionsstack)
- [std/Compress](#stdcompress)
- [std/Config](#stdconfig)
- [std/Console](#stdconsole) — Printing to the terminal and reading a line from it.
- [std/COpenAL](#stdcopenal)
- [std/COpenGL](#stdcopengl)
- [std/Core](#stdcore)
- [std/COs](#stdcos)
- [std/Crypto](#stdcrypto)
- [std/CSqlite](#stdcsqlite)
- [std/CSsl](#stdcssl)
- [std/CStdioExtra](#stdcstdioextra)
- [std/CString](#stdcstring)
- [std/Csv](#stdcsv)
- [std/CThread](#stdcthread)
- [std/CTime](#stdctime)
- [std/CVulkan](#stdcvulkan)
- [std/CZlib](#stdczlib)
- [std/DateTime](#stddatetime)
- [std/Db](#stddb)
- [std/Debug](#stddebug)
- [std/Ffi](#stdffi)
- [std/Font](#stdfont)
- [std/Fs](#stdfs)
- [std/Fs/CF](#stdfscf)
- [std/Gui](#stdgui)
- [std/Http](#stdhttp)
- [std/HttpServer](#stdhttpserver)
- [std/Image](#stdimage)
- [std/Input](#stdinput)
- [std/IO](#stdio) — Files and standard streams on top of C stdio.
- [std/Iter](#stditer)
- [std/Json](#stdjson)
- [std/List](#stdlist)
- [std/Log](#stdlog)
- [std/Math](#stdmath)
- [std/Mem](#stdmem)
- [std/Net](#stdnet)
- [std/OpenGL](#stdopengl)
- [std/Path](#stdpath)
- [std/Process](#stdprocess)
- [std/Random](#stdrandom)
- [std/Random/Time](#stdrandomtime)
- [std/Reflect](#stdreflect)
- [std/Regex](#stdregex)
- [std/String](#stdstring) — Owned, growable, always NUL-terminated byte string (UTF-8 safe, byte-indexed).
- [std/Sys](#stdsys)
- [std/Tasks](#stdtasks)
- [std/Test](#stdtest)
- [std/Text](#stdtext) — Text helpers over `char*` and `String`: split, join, pad, parse, escape, UTF-8, hex, URL and base64.
- [std/Thread](#stdthread)
- [std/Tls](#stdtls)
- [std/Tls/CT](#stdtlsct)
- [std/Toml](#stdtoml)
- [std/Uuid](#stduuid)
- [std/Video](#stdvideo)
- [std/Video/C](#stdvideoc)
- [std/Vk](#stdvk)
- [std/Vk/S](#stdvks)
- [std/Window](#stdwindow)
- [std/Ws](#stdws)
- [std/Xml](#stdxml)
- [std/Yaml](#stdyaml)

## std/Audio

_No documentation yet._

## std/Audio/CA

_No documentation yet._

## std/Bench

_No documentation yet._

## std/Binary

_No documentation yet._

## std/Canvas

_No documentation yet._

## std/Canvas/F

_No documentation yet._

## std/Canvas/LF

_No documentation yet._

## std/Canvas/LU

_No documentation yet._

## std/CConsole

_No documentation yet._

## std/CCurl

_No documentation yet._

## std/CDl

_No documentation yet._

## std/CFfmpeg

_No documentation yet._

## std/CFile

_No documentation yet._

## std/CFont

_No documentation yet._

## std/CGlfw

_No documentation yet._

## std/Channel

_No documentation yet._

## std/CImage

_No documentation yet._

## std/Cli

_No documentation yet._

## std/CMath

_No documentation yet._

## std/CMem

_No documentation yet._

## std/CNet

_No documentation yet._

## std/Collections

_No documentation yet._

## std/Collections/Algo

_No documentation yet._

## std/Collections/Map

_No documentation yet._

## std/Collections/PriorityQueue

_No documentation yet._

## std/Collections/Queue

_No documentation yet._

## std/Collections/Queue/D

_No documentation yet._

## std/Collections/Set

_No documentation yet._

## std/Collections/Stack

_No documentation yet._

## std/Compress

_No documentation yet._

## std/Config

_No documentation yet._

## std/Console

Printing to the terminal and reading a line from it.

Output goes to stdout (`print`, `println`, `log`, `info`) or stderr (`warn`, `error`).
Every function also has a `String` version (`printString`, `printlnString`, ...)
that takes a `Str.String*` instead of a `char*`.

```
import "std/Console";

Console.println("hello");
Console.warn("disk is almost full");
```

Formatted output: variadic forwarding is not possible yet. Either call the extern `printf`
directly, or build the text with `String.append` / `appendInt` and print it with `printlnString`.

### `let useColor: bool = true`

Whether `info`, `warn` and `error` color their `[info]`, `[warn]` and `[error]` tags
with ANSI escape codes. On by default; change it with `setColor`.

### `function setColor(on: bool) => void`

Turns the colored tags of `info`, `warn` and `error` on or off.
Turn them off when the output goes to a file or a terminal without ANSI support.

### `function out() => void*`

The C `stdout` stream, for use with the raw C functions.
Windows only for now, because it calls `__acrt_iob_func`.

### `function err() => void*`

The C `stderr` stream, for use with the raw C functions.
Windows only for now, because it calls `__acrt_iob_func`.

### `function print(s: char*) => void`

Writes `s` to stdout without a trailing newline.

### `function println(s: char*) => void`

Writes `s` to stdout, followed by a newline.

```
Console.println("hello");   // hello
```

### `function log(s: char*) => void`

Same as `println`. Provided for people used to `console.log`.

### `function info(s: char*) => void`

Writes `s` to stdout as an informational line, starting with an `[info]` tag.
The tag is colored cyan unless `setColor(false)` was called.

### `function warn(s: char*) => void`

Writes `s` to **stderr**, starting with a `[warn]` tag (yellow unless `setColor(false)`).
stdout is flushed first, so output from `println` and `warn` appears in the right order.

### `function error(s: char*) => void`

Writes `s` to **stderr**, starting with an `[error]` tag (red unless `setColor(false)`).
stdout is flushed first, so output from `println` and `error` appears in the right order.

### `function printString(s: Str.String*) => void`

Like `print`, but takes a `String`.

### `function printlnString(s: Str.String*) => void`

Like `println`, but takes a `String`.

### `function infoString(s: Str.String*) => void`

Like `info`, but takes a `String`.

### `function warnString(s: Str.String*) => void`

Like `warn`, but takes a `String`.

### `function errorString(s: Str.String*) => void`

Like `error`, but takes a `String`.

### `function flush() => void`

Forces everything written to stdout so far to appear now.
Useful after `print` when you want a prompt to show before waiting for input.

### `function readLine(out: Str.String*) => bool`

Reads one line from stdin into `out`, without the trailing `\n` or `\r\n`.
Anything already in `out` is cleared first.
Returns `false` when stdin is at end of file and nothing was read.

```
let name: Str.String;
Console.print("name? ");
Console.flush();
if (Console.readLine(&name)) Console.println(name.cstr());
name.free();
```

### `function clear() => void`

Clears the terminal and moves the cursor to the top-left corner.
Uses ANSI escape codes, so Windows 10 or later needs virtual terminal mode enabled.

## std/COpenAL

_No documentation yet._

## std/COpenGL

_No documentation yet._

## std/Core

_No documentation yet._

## std/COs

_No documentation yet._

## std/Crypto

_No documentation yet._

## std/CSqlite

_No documentation yet._

## std/CSsl

_No documentation yet._

## std/CStdioExtra

_No documentation yet._

## std/CString

_No documentation yet._

## std/Csv

_No documentation yet._

## std/CThread

_No documentation yet._

## std/CTime

_No documentation yet._

## std/CVulkan

_No documentation yet._

## std/CZlib

_No documentation yet._

## std/DateTime

_No documentation yet._

## std/Db

_No documentation yet._

## std/Debug

_No documentation yet._

## std/Ffi

_No documentation yet._

## std/Font

_No documentation yet._

## std/Fs

_No documentation yet._

## std/Fs/CF

_No documentation yet._

## std/Gui

_No documentation yet._

## std/Http

_No documentation yet._

## std/HttpServer

_No documentation yet._

## std/Image

_No documentation yet._

## std/Input

_No documentation yet._

## std/IO

Files and standard streams on top of C stdio.

Text goes in as `char*` or `String*`; text comes out into a `String*` you own (see `String.os`).
Failure is reported with a `bool` return (`false` means failed, or end of file).
There is no `defer` yet, so `close()` your files.

## Example

```
let f: File;
if (f.open("notes.txt", MODE_READ)) {
    let line: Str.String;
    while (f.readLine(&line)) { printf("%s\n", line.cstr()); }
    line.free();
    f.close();
}
```

### `let MODE_READ: char* = "rb"`

Open an existing file for reading.

### `let MODE_WRITE: char* = "wb"`

Create the file, or empty it if it exists, and open it for writing.

### `let MODE_APPEND: char* = "ab"`

Open the file for writing at the end, creating it if needed.

### `let MODE_READWRITE: char* = "r+b"`

Open an existing file for reading and writing.

### `let SEEK_SET: int = 0`

`seek` origin: the start of the file.

### `let SEEK_CUR: int = 1`

`seek` origin: the current position.

### `let SEEK_END: int = 2`

`seek` origin: the end of the file.

### `struct File`

An open file or standard stream. Call `open` (or `attach`), use it, then `close`.

#### `handle: void* = nullptr`

*Member of `File`.*

The underlying C `FILE*`.

#### `isOpen: bool = false`

*Member of `File`.*

`true` while the file is open.

#### `open(path: char*, mode: char*) => bool`

*Member of `File`.*

Opens `path` with one of the `MODE_` strings. Returns `false` if it could not be opened.

#### `attach(which: uint) => void`

*Member of `File`.*

Attaches a process stream: 0 is stdin, 1 is stdout, 2 is stderr. Do not `close()` these.

#### `raw() => void*`

*Member of `File`.*

Escape hatch: returns the C `FILE*`.

#### `close() => void`

*Member of `File`.*

Closes the file. Safe to call on a file that is not open.

#### `flush() => void`

*Member of `File`.*

Writes any buffered output to the file.

#### `eof() => bool`

*Member of `File`.*

`true` once a read has hit the end of the file.

#### `hasError() => bool`

*Member of `File`.*

`true` if an I/O error has occurred on this file.

#### `tell() => i64`

*Member of `File`.*

Returns the current position, in bytes from the start.

#### `seek(offset: i64, origin: int) => bool`

*Member of `File`.*

Moves to `offset` bytes from `origin` (`SEEK_SET`, `SEEK_CUR` or `SEEK_END`).
Returns `false` on failure.

#### `size() => i64`

*Member of `File`.*

Returns the file size in bytes. The current position is left unchanged.

#### `readBytes(buf: void*, n: u64) => u64`

*Member of `File`.*

Reads up to `n` bytes into `buf`. Returns how many were read (0 at end of file).

#### `readAll(out: Str.String*) => bool`

*Member of `File`.*

Replaces `out` with the whole file contents, read from the start.

#### `readLine(out: Str.String*) => bool`

*Member of `File`.*

Replaces `out` with the next line, without the trailing `\n` or `\r\n`.
Returns `false` at end of file.

#### `writeBytes(buf: void*, n: u64) => bool`

*Member of `File`.*

Writes `n` bytes from `buf`. Returns `false` if fewer were written.

#### `write(text: char*) => bool`

*Member of `File`.*

Writes the C string `text`.

#### `writeString(s: Str.String*) => bool`

*Member of `File`.*

Writes the contents of a `String`.

#### `writeLine(text: char*) => bool`

*Member of `File`.*

Writes the C string `text` followed by a newline.

#### `writeLineString(s: Str.String*) => bool`

*Member of `File`.*

Writes a `String` followed by a newline.

### `function readFile(path: char*, out: Str.String*) => bool`

Reads the whole file at `path` into `out`. Returns `false` if it could not be opened or read.

### `function writeFile(path: char*, text: char*) => bool`

Creates or overwrites the file at `path` with the C string `text`.

### `function writeFileString(path: char*, s: Str.String*) => bool`

Creates or overwrites the file at `path` with the contents of a `String`.

### `function appendFile(path: char*, text: char*) => bool`

Appends the C string `text` to the file at `path`, creating it if needed.

### `function deleteFile(path: char*) => bool`

Deletes the file at `path`. Returns `false` on failure.

### `function renameFile(src: char*, to: char*) => bool`

Renames or moves the file `src` to `to`. Returns `false` on failure.

## std/Iter

_No documentation yet._

## std/Json

_No documentation yet._

## std/List

_No documentation yet._

## std/Log

_No documentation yet._

## std/Math

_No documentation yet._

## std/Mem

_No documentation yet._

## std/Net

_No documentation yet._

## std/OpenGL

_No documentation yet._

## std/Path

_No documentation yet._

## std/Process

_No documentation yet._

## std/Random

_No documentation yet._

## std/Random/Time

_No documentation yet._

## std/Reflect

_No documentation yet._

## std/Regex

_No documentation yet._

## std/String

Owned, growable, always NUL-terminated byte string (UTF-8 safe, byte-indexed).

## Ownership rules

- A `String` owns its buffer. Call `free()` when you are done (there is no destructor or `defer` yet).
- Never copy one `String` into another with `=`: that copies the pointer and double-frees.
  Use `a.clone(&b)` instead.
- Methods that can fail return `bool`.

## Usage

```
let s: String;
s.set("hello");
s.append(", world");
printf("%s (%llu bytes)\n", s.cstr(), s.length());
s.free();
```

### `function isSpaceChar(c: int) => bool`

`true` if `c` is a space, tab, newline or carriage return.

### `struct String`

An owned, growable, NUL-terminated byte string. See the ownership rules at the top of this page.

#### `data: char* = nullptr`

*Member of `String`.*

The buffer, or `nullptr` before the first allocation. Always NUL-terminated once allocated.

#### `len: u64 = 0`

*Member of `String`.*

Length in bytes, not counting the terminator.

#### `cap: u64 = 0`

*Member of `String`.*

Allocated size in bytes, including room for the terminator.

#### `reserve(n: u64) => bool`

*Member of `String`.*

Makes room for `n` bytes plus the terminator, keeping the existing contents.
Returns `false` if the allocation fails.

#### `free() => void`

*Member of `String`.*

Releases the buffer and resets the string to empty. Call this when you are done with it.

#### `clear() => void`

*Member of `String`.*

Empties the string but keeps the allocated buffer for reuse.

#### `truncate(n: u64) => void`

*Member of `String`.*

Shortens the string to `n` bytes. Does nothing if it is already `n` bytes or shorter.

#### `length() => u64`

*Member of `String`.*

Returns the length in bytes.

#### `isEmpty() => bool`

*Member of `String`.*

`true` if the string has no bytes.

#### `cstr() => char*`

*Member of `String`.*

Returns the contents as a C string. Always valid, even for an empty or unallocated `String`.

#### `set(src: char*) => bool`

*Member of `String`.*

Replaces the contents with the C string `src`.

#### `appendN(src: char*, n: u64) => bool`

*Member of `String`.*

Appends the first `n` bytes of `src`.

#### `append(src: char*) => bool`

*Member of `String`.*

Appends the C string `src`.

#### `appendString(other: String*) => bool`

*Member of `String`.*

Appends another `String`.

#### `appendChar(c: char) => bool`

*Member of `String`.*

Appends one byte.

#### `appendInt(v: i64) => bool`

*Member of `String`.*

Appends the decimal text of `v`.

#### `clone(out: String*) => bool`

*Member of `String`.*

Copies this string into `out`, replacing its contents. Use this instead of `=`.

#### `equals(other: String*) => bool`

*Member of `String`.*

`true` if both strings have the same bytes.

#### `equalsC(s: char*) => bool`

*Member of `String`.*

`true` if the string equals the C string `s`.

#### `startsWith(prefix: char*) => bool`

*Member of `String`.*

`true` if the string begins with `prefix`.

#### `endsWith(suffix: char*) => bool`

*Member of `String`.*

`true` if the string ends with `suffix`.

#### `indexOf(needle: char*) => i64`

*Member of `String`.*

Returns the byte index of the first match of `needle`, or `-1` if there is none.

#### `contains(needle: char*) => bool`

*Member of `String`.*

`true` if `needle` occurs anywhere in the string.

#### `substring(start: u64, count: u64, out: String*) => bool`

*Member of `String`.*

Copies the bytes from `start` to `start + count` (clamped to the end) into `out`,
replacing its contents. Returns `false` if `start` is past the end.

#### `parseInt(out: i64*) => bool`

*Member of `String`.*

Parses the whole string as a base-10 integer into `out[0]`.
Returns `false` if the string is empty or has trailing characters.

#### `trim() => void`

*Member of `String`.*

Removes leading and trailing whitespace, in place.

#### `toUpper() => void`

*Member of `String`.*

Converts every byte to uppercase, in place (ASCII).

#### `toLower() => void`

*Member of `String`.*

Converts every byte to lowercase, in place (ASCII).

#### `replace(find: char*, with: char*) => bool`

*Member of `String`.*

Replaces every occurrence of `find` with `with`, in place.
Returns `false` if `find` is empty or memory runs out (the string is left unchanged).

## std/Sys

_No documentation yet._

## std/Tasks

_No documentation yet._

## std/Test

_No documentation yet._

## std/Text

Text helpers over `char*` and `String`: split, join, pad, parse, escape, UTF-8, hex, URL and base64.

Tier 3. Functions that build text append into a `String` you own (see `String.os`), so call
`free()` on it when you are done. Functions that can fail return `bool`.

### `function split(s: char*, sep: char*, out: L.List<Str.String>*) => bool`

Splits `s` on every occurrence of `sep` and pushes each part onto `out`.

Each part is a newly allocated `String`: free each one, then the list (or call `freeParts`).
Returns `false` if `sep` is empty or an allocation fails.

### `function freeParts(parts: L.List<Str.String>*) => void`

Frees every `String` in `parts`, then the list itself. Use it on a list filled by `split`.

### `function join(parts: L.List<Str.String>*, sep: char*, out: Str.String*) => bool`

Appends all `parts` to `out`, separated by `sep`.

### `function repeat(s: char*, n: u64, out: Str.String*) => bool`

Appends `s` to `out`, `n` times.

### `function padLeft(s: char*, width: u64, fill: char, out: Str.String*) => bool`

Appends `s` to `out`, left-padded with `fill` until it is at least `width` bytes long.

### `function padRight(s: char*, width: u64, fill: char, out: Str.String*) => bool`

Appends `s` to `out`, right-padded with `fill` until it is at least `width` bytes long.

### `function parseF64(s: char*, out: double*) => bool`

Parses a floating-point number from the start of `s` into `out[0]`.
Returns `false` if no number could be read.

### `function parseU64(s: char*, base: int, out: u64*) => bool`

Parses an unsigned integer in the given `base` (2 to 36) from the start of `s` into `out[0]`.
Returns `false` if no digits could be read.

### `function isDigit(c: int) => bool`

`true` if `c` is an ASCII digit (`0` to `9`).

### `function isAlpha(c: int) => bool`

`true` if `c` is an ASCII letter.

### `function isAlnum(c: int) => bool`

`true` if `c` is an ASCII letter or digit.

### `function isSpace(c: int) => bool`

`true` if `c` is ASCII whitespace (space, tab, newline, vertical tab, form feed, carriage return).

### `function escape(s: char*, out: Str.String*) => bool`

Appends `s` to `out` with `"`, `\` and the control characters newline, carriage return and tab
escaped, JSON-style.

### `function appendUtf8(cp: u32, out: Str.String*) => bool`

Appends the UTF-8 encoding of the code point `cp` to `out` (1 to 4 bytes).

### `function utf8Length(s: char*) => u64`

Returns the number of code points in the UTF-8 string `s` (not the number of bytes).

### `function hexDigit(n: int, upper: bool) => int`

Returns the hex digit character for `n` in 0 to 15, lowercase or uppercase.

### `function b64Char(v: int, urlSafe: bool) => int`

Returns the base64 alphabet character for `v` in 0 to 63.
Standard alphabet uses `+` and `/`; with `urlSafe` it uses `-` and `_`.

### `function hexEncode(p: u8*, n: u64, out: Str.String*) => bool`

Appends the lowercase hex encoding of the `n` bytes at `p` to `out`.

### `function hexNibble(c: int) => int`

Returns the value (0 to 15) of the hex digit character `c`, or `-1` if it is not a hex digit.

### `function hexDecode(s: char*, out: L.List<u8>*) => bool`

Decodes the hex string `s` and pushes the bytes onto `out` (half as many bytes as characters).
Returns `false` on an odd length or a character that is not a hex digit.

### `function urlEncode(s: char*, out: Str.String*) => bool`

Appends the percent-encoding of `s` to `out`.
Letters, digits and `-` `_` `.` `~` are kept; every other byte becomes `%XX`.

### `function urlDecode(s: char*, out: Str.String*) => bool`

Appends the percent-decoding of `s` to `out`. `+` becomes a space.
Returns `false` on a malformed `%XX` sequence.

### `function base64EncodeOpts(p: u8*, n: u64, urlSafe: bool, pad: bool, out: Str.String*) => bool`

Appends the base64 encoding of the `n` bytes at `p` to `out`.

With `urlSafe` the alphabet uses `-` and `_` instead of `+` and `/`.
With `pad` the output is padded with `=` to a multiple of 4 characters.

### `function base64Encode(p: u8*, n: u64, out: Str.String*) => bool`

Appends the standard, padded base64 encoding of the `n` bytes at `p` to `out`.

### `function base64EncodeStr(s: char*, out: Str.String*) => bool`

Appends the standard, padded base64 encoding of the C string `s` to `out`.

### `function b64Val(c: int) => int`

Returns the value (0 to 63) of the base64 character `c` (standard or URL-safe),
or `-1` if it is not a base64 character.

### `function base64Decode(s: char*, out: L.List<u8>*) => bool`

Decodes standard or URL-safe base64 from `s` and pushes the bytes onto `out`.

Padding is optional and ASCII whitespace is ignored.
Returns `false` on an invalid character, data after `=`, or an impossible length.

### `function base64DecodeStr(s: char*, out: Str.String*) => bool`

Decodes base64 from `s` into a `String`, for text payloads. See `base64Decode` for the rules.

### `function hexEncodeStr(s: char*, out: Str.String*) => bool`

Appends the lowercase hex encoding of the bytes of the C string `s` to `out`.

## std/Thread

_No documentation yet._

## std/Tls

_No documentation yet._

## std/Tls/CT

_No documentation yet._

## std/Toml

_No documentation yet._

## std/Uuid

_No documentation yet._

## std/Video

_No documentation yet._

## std/Video/C

_No documentation yet._

## std/Vk

_No documentation yet._

## std/Vk/S

_No documentation yet._

## std/Window

_No documentation yet._

## std/Ws

_No documentation yet._

## std/Xml

_No documentation yet._

## std/Yaml

_No documentation yet._

