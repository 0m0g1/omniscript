// standard/1/Audio.os
// Tier 5: audio playback via miniaudio (needs a C shim/library exporting ma_*). Engine + Sound handles.
module CA = import "standard/1/c/openal.os";
module CMem = import "standard/1/c/mem.os";

// ma_engine / ma_sound are large opaque structs: allocate generously (miniaudio ma_engine ~ 4KB, ma_sound ~ 1KB).
struct Engine {
    mem: void* = nullptr;
    init() => bool {
        this.mem = CMem.calloc(1, 8192);
        if (this.mem == nullptr) return false;
        return CA.ma_engine_init(nullptr, this.mem) == 0;
    }
    free() => void { if (this.mem != nullptr) { CA.ma_engine_uninit(this.mem); CMem.free(this.mem); this.mem = nullptr; } }
    setVolume(v: float) => void { CA.ma_engine_set_volume(this.mem, v); }
    // fire-and-forget
    play(path: char*) => bool { return CA.ma_engine_play_sound(this.mem, path, nullptr) == 0; }
}

struct Sound {
    mem: void* = nullptr;
    load(e: Engine*, path: char*) => bool {
        this.mem = CMem.calloc(1, 2048);
        if (this.mem == nullptr) return false;
        return CA.ma_sound_init_from_file(e.mem, path, 0, nullptr, nullptr, this.mem) == 0;
    }
    free() => void { if (this.mem != nullptr) { CA.ma_sound_uninit(this.mem); CMem.free(this.mem); this.mem = nullptr; } }
    start() => void { CA.ma_sound_start(this.mem); }
    stop() => void { CA.ma_sound_stop(this.mem); }
    setVolume(v: float) => void { CA.ma_sound_set_volume(this.mem, v); }
    setLooping(on: bool) => void { if (on) CA.ma_sound_set_looping(this.mem, 1); else CA.ma_sound_set_looping(this.mem, 0); }
    isPlaying() => bool { return CA.ma_sound_is_playing(this.mem) != 0; }
}
