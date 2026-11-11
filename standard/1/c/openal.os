// standard/1/c/openal.os  (miniaudio, via a thin C shim exposing ma_* simple API)
extern "C" fn ma_engine_init(cfg: void*, eng: void*) => int;
extern "C" fn ma_engine_uninit(eng: void*) => void;
extern "C" fn ma_engine_play_sound(eng: void*, path: char*, group: void*) => int;
extern "C" fn ma_engine_set_volume(eng: void*, v: float) => int;
extern "C" fn ma_sound_init_from_file(eng: void*, path: char*, flags: u32, grp: void*, fence: void*, snd: void*) => int;
extern "C" fn ma_sound_start(snd: void*) => int;
extern "C" fn ma_sound_stop(snd: void*) => int;
extern "C" fn ma_sound_uninit(snd: void*) => void;
extern "C" fn ma_sound_set_volume(snd: void*, v: float) => void;
extern "C" fn ma_sound_set_looping(snd: void*, l: u32) => void;
extern "C" fn ma_sound_is_playing(snd: void*) => u32;
