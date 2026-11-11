// standard/1/std.os
// Ordered by tier. Uncommented = exists and compiles. Uncomment one at a time as you implement/verify each.
module std {
    // ---------------- Tier 0: raw C, the escape hatch ----------------
    public module CMem = import "standard/1/c/mem.os";
    public module CString = import "standard/1/c/string.os";
    public module CFile = import "standard/1/c/file.os";
    public module CConsole = import "standard/1/c/console.os";
    public module CMath = import "standard/1/c/math.os";
    public module CTime = import "standard/1/c/time.os";
    public module COs = import "standard/1/c/os.os";
    // public module CStdioExtra = import "standard/1/c/stdio_extra.os";
    // public module CThread = import "standard/1/c/thread.os";
    // public module CNet = import "standard/1/c/net.os";
    // public module CDl = import "standard/1/c/dl.os";
    // public module CGlfw = import "standard/1/c/glfw.os";
    // public module CVulkan = import "standard/1/c/vulkan.os";
    // public module COpenGL = import "standard/1/c/opengl.os";
    // public module COpenAL = import "standard/1/c/openal.os";
    // public module CCurl = import "standard/1/c/curl.os";
    // public module CSqlite = import "standard/1/c/sqlite.os";
    // public module CSsl = import "standard/1/c/ssl.os";
    // public module CZlib = import "standard/1/c/zlib.os";
    // public module CFfmpeg = import "standard/1/c/ffmpeg.os";
    // public module CImage = import "standard/1/c/image.os";
    // public module CFont = import "standard/1/c/font.os";

    // ---------------- Tier 1: core ----------------
    public module Core = import "standard/1/Core.os";
    public module Mem = import "standard/1/Mem.os";
    public module String = import "standard/1/String.os";
    public module Math = import "standard/1/Math.os";
    public module Random = import "standard/1/Random.os";
    public module Time = import "standard/1/Time.os";

    // ---------------- Tier 2: collections and iteration ----------------
    public module List = import "standard/1/List.os";
    // public module Collections = import "standard/1/Collections.os";
    // public module Map = import "standard/1/Map.os";
    // public module Set = import "standard/1/Set.os";
    // public module Queue = import "standard/1/Queue.os";
    // public module Stack = import "standard/1/Stack.os";
    // public module Deque = import "standard/1/Deque.os";
    // public module PriorityQueue = import "standard/1/PriorityQueue.os";
    // public module Algo = import "standard/1/Algo.os";
    // public module Iter = import "standard/1/Iter.os";            // stub

    // ---------------- Tier 3: text and data formats ----------------
    // public module Text = import "standard/1/Text.os";
    // public module Regex = import "standard/1/Regex.os";
    // public module Json = import "standard/1/Json.os";
    // public module Csv = import "standard/1/Csv.os";
    // public module Toml = import "standard/1/Toml.os";            // stub
    // public module Yaml = import "standard/1/Yaml.os";            // stub
    // public module Xml = import "standard/1/Xml.os";
    // public module Binary = import "standard/1/Binary.os";

    // ---------------- Tier 4: input/output, filesystem, process ----------------
    public module Console = import "standard/1/Console.os";
    public module IO = import "standard/1/IO.os";
    public module Sys = import "standard/1/Sys.os";
    // public module Path = import "standard/1/Path.os";
    // public module Fs = import "standard/1/Fs.os";
    // public module Process = import "standard/1/Process.os";
    // public module Log = import "standard/1/Log.os";
    // public module Cli = import "standard/1/Cli.os";

    // ---------------- Tier 5: concurrency and networking ----------------
    // public module Thread = import "standard/1/Thread.os";
    // public module Channel = import "standard/1/Channel.os";
    // public module Async = import "standard/1/Async.os";          // stub
    // public module Net = import "standard/1/Net.os";
    // public module Http = import "standard/1/Http.os";
    // public module HttpServer = import "standard/1/HttpServer.os"; // stub
    // public module Ws = import "standard/1/Ws.os";                // stub
    // public module Tls = import "standard/1/Tls.os";              // stub

    // ---------------- Tier 6: security, storage, compression ----------------
    // public module Crypto = import "standard/1/Crypto.os";
    // public module Compress = import "standard/1/Compress.os";
    // public module Db = import "standard/1/Db.os";
    // public module Uuid = import "standard/1/Uuid.os";
    // public module DateTime = import "standard/1/DateTime.os";
    // public module Config = import "standard/1/Config.os";

    // ---------------- Tier 7: graphics, windowing, audio, media ----------------
    // public module Window = import "standard/1/Window.os";
    // public module Input = import "standard/1/Input.os";
    // public module OpenGL = import "standard/1/OpenGL.os";
    // public module Vk = import "standard/1/Vulkan.os";
    // public module Canvas = import "standard/1/Canvas.os";        // stub
    // public module Image = import "standard/1/Image.os";
    // public module Font = import "standard/1/Font.os";
    // public module Audio = import "standard/1/Audio.os";
    // public module Video = import "standard/1/Video.os";          // stub
    // public module Gui = import "standard/1/Gui.os";              // stub

    // ---------------- Tier 8: language tooling and interop ----------------
    // public module Ffi = import "standard/1/Ffi.os";
    // public module Reflect = import "standard/1/Reflect.os";      // stub
    // public module Test = import "standard/1/Test.os";            // yours; enable when ready
    // public module Bench = import "standard/1/Bench.os";
    // public module Debug = import "standard/1/Debug.os";
}
