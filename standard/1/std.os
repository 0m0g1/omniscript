// standard/1/std.os
module std {
    // Tier 1: core
    // public module Core = import "standard/1/Core.os";
    // public module Mem = import "standard/1/Mem.os";
    // public module String = import "standard/1/String.os";
    // public module Math = import "standard/1/Math.os";
    // public module Random = import "standard/1/Random.os";
    // public module Time = import "standard/1/Time.os";

    // // Tier 4: I/O and system
    public module Console = import "standard/1/Console.os";
    // public module IO = import "standard/1/IO.os";
    // public module Sys = import "standard/1/Sys.os";

    // // Tier 7
    // public module Vk = import "standard/1/vulkan.os";

    // // Tier 0: raw C, the escape hatch
    // public module CMem = import "standard/1/c/mem.os";
    // public module CString = import "standard/1/c/string.os";
    // public module CFile = import "standard/1/c/file.os";
    // public module CConsole = import "standard/1/c/console.os";
    // public module CMath = import "standard/1/c/math.os";
    // public module CTime = import "standard/1/c/time.os";
    // public module COs = import "standard/1/c/os.os";
}
