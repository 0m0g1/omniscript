// standard/1/Video.os
// Tier 7: video decoding (FFmpeg through the omvideo.c shim) to RGBA frames. Video only: no audio track yet.
//
//   let v: Video.Video;
//   if (!v.open("clip.mp4")) { ... }
//   // each tick, advance by playback time:
//   while (v.time() < clock && !v.ended()) { v.nextFrame(); }
//   pen.createImage(v.pixels(), v.width, v.height);     // or upload to a GL texture; RGBA8, top-left origin
//   v.seek(12.5);                                       // frame at/after 12.5 s is ready in pixels()
//   v.close();
//
// pixels() is valid until the next nextFrame / seek / close; copy it if you need it longer.
// width/height are read at open; if the stream changes size mid-way they update on that frame.
// Not compiled/run yet. Needs dependencies/ffmpeg/bin/omvideo.dll (build omvideo.c).
module C = import "standard/1/c/omvideo.os";

struct Video {
    h: void* = nullptr;
    width: int = 0;
    height: int = 0;
    fps: double = 0.0;
    duration: double = 0.0;      // seconds, negative if the container does not say
    eof: bool = false;

    open(path: char*) => bool {
        this.close();
        let p = C.omv_open(path);
        if (p == nullptr) return false;
        this.h = p;
        this.width = C.omv_width(p);
        this.height = C.omv_height(p);
        this.fps = C.omv_fps(p);
        this.duration = C.omv_duration(p);
        this.eof = false;
        return true;
    }

    valid() => bool { return this.h != nullptr; }
    ended() => bool { return this.eof; }

    // Decodes the next frame into pixels(). false at the end of the stream (ended() becomes true) or on error.
    nextFrame() => bool {
        if (this.h == nullptr) return false;
        let r: int = C.omv_next(this.h);
        if (r == 1) {
            this.width = C.omv_width(this.h);
            this.height = C.omv_height(this.h);
            return true;
        }
        if (r == 0) this.eof = true;
        return false;
    }

    // RGBA8 pixels of the current frame (width*height*4 bytes), or nullptr when nothing is open.
    pixels() => u8* {
        if (this.h == nullptr) return nullptr;
        return C.omv_frame(this.h);
    }

    // presentation time of the current frame, seconds
    time() => double {
        if (this.h == nullptr) return 0.0;
        return C.omv_time(this.h);
    }

    // Jumps to `sec`; the first frame at or after it is ready in pixels().
    seek(sec: double) => bool {
        if (this.h == nullptr) return false;
        let ok: int = C.omv_seek(this.h, sec);
        if (ok == 1) { this.eof = false; return true; }
        return false;
    }

    close() => void {
        if (this.h != nullptr) C.omv_close(this.h);
        this.h = nullptr;
        this.width = 0;
        this.height = 0;
        this.eof = false;
    }
}
