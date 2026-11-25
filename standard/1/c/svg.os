// standard/1/c/svg.os  (lunasvg via dependencies/lunasvg/src/svg_impl.cpp)
extern "dependencies/lunasvg/bin/lunasvg.dll" {
    fn svg_size(data: char*, len: int, w: float*, h: float*) => int;
    fn svg_rasterize(data: char*, len: int, w: int, h: int, rgb: u32, out: u8*) => int;
}

