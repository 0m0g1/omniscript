// standard/1/c/omvideo.os
// Tier 0: bindings for the omvideo.c shim (FFmpeg decode behind a flat API). Build omvideo.c first (see its header).
// OmVideo is opaque (void*). Frames are RGBA8, tightly packed, top-left origin.
extern "dependencies/ffmpeg/bin/omvideo.dll" {
    fn omv_open(path: char*) => void*;            // nullptr on failure
    fn omv_close(v: void*) => void;
    fn omv_width(v: void*) => int;
    fn omv_height(v: void*) => int;
    fn omv_fps(v: void*) => double;
    fn omv_duration(v: void*) => double;          // seconds, -1 if unknown
    fn omv_time(v: void*) => double;              // time of the current frame, seconds
    fn omv_frame(v: void*) => u8*;                // w*h*4 bytes, valid until the next next/seek/close
    fn omv_next(v: void*) => int;                 // 1 frame, 0 end of stream, -1 error
    fn omv_seek(v: void*, sec: double) => int;    // 1 ok (frame is ready in omv_frame), 0 failed
}
