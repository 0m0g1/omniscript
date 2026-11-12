// standard/1/c/openal.os  (OpenAL Soft, 64-bit)
extern "dependencies/openal-soft-1.24.3-bin/bin/Win64/soft_oal.dll" {
    fn alcOpenDevice(name: char*) => void*;
    fn alcCloseDevice(dev: void*) => int;
    fn alcCreateContext(dev: void*, attrs: int*) => void*;
    fn alcMakeContextCurrent(ctx: void*) => int;
    fn alcDestroyContext(ctx: void*) => void;
    fn alGetError() => int;
    fn alGenBuffers(n: int, bufs: u32*) => void;
    fn alDeleteBuffers(n: int, bufs: u32*) => void;
    fn alBufferData(buf: u32, fmt: int, data: void*, size: int, freq: int) => void;
    fn alGenSources(n: int, srcs: u32*) => void;
    fn alDeleteSources(n: int, srcs: u32*) => void;
    fn alSourcei(src: u32, param: int, v: int) => void;
    fn alSourcef(src: u32, param: int, v: float) => void;
    fn alGetSourcei(src: u32, param: int, v: int*) => void;
    fn alSourcePlay(src: u32) => void;
    fn alSourceStop(src: u32) => void;
    fn alSourcePause(src: u32) => void;
    fn alListenerf(param: int, v: float) => void;
}