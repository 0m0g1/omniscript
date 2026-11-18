// standard/1/c/shaderc.os  (GLSL -> SPIR-V compiler; shaderc_shared.dll ships with the Vulkan SDK Bin folder)
// Copy shaderc_shared.dll next to Osengine.exe (or put it on PATH).
// shader kinds: 0 = vertex, 1 = fragment, 2 = compute. status 0 = success.
extern "shaderc_shared.dll" {
    fn shaderc_compiler_initialize() => void*;
    fn shaderc_compiler_release(c: void*) => void;
    fn shaderc_compile_into_spv(c: void*, src: char*, srcSize: u64, kind: int, fileName: char*, entry: char*, opts: void*) => void*;
    fn shaderc_result_release(r: void*) => void;
    fn shaderc_result_get_length(r: void*) => u64;
    fn shaderc_result_get_bytes(r: void*) => void*;
    fn shaderc_result_get_compilation_status(r: void*) => int;
    fn shaderc_result_get_error_message(r: void*) => char*;
}
