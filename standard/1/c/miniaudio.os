// standard/1/c/miniaudio.os  (miniaudio built as a DLL, 64-bit; see dependencies/miniaudio/miniaudio_dll.c)
// ma_result is an int (0 = MA_SUCCESS); ma_bool32 is a u32. ma_engine / ma_sound are passed as opaque void*.
extern "dependencies/miniaudio/miniaudio.dll" {
    fn ma_engine_init(config: void*, engine: void*) => int;
    fn ma_engine_uninit(engine: void*) => void;
    fn ma_engine_set_volume(engine: void*, volume: float) => int;
    fn ma_engine_play_sound(engine: void*, path: char*, group: void*) => int;

    fn ma_sound_init_from_file(engine: void*, path: char*, flags: u32, group: void*, doneFence: void*, sound: void*) => int;
    fn ma_sound_uninit(sound: void*) => void;
    fn ma_sound_start(sound: void*) => int;
    fn ma_sound_stop(sound: void*) => int;
    fn ma_sound_set_volume(sound: void*, volume: float) => void;
    fn ma_sound_set_looping(sound: void*, looping: u32) => void;
    fn ma_sound_is_playing(sound: void*) => u32;
}
