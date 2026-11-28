// docs/examples/gb/gbsound_al.os
// PCM stream output for the Game Boy app, on OpenAL Soft (standard/1/c/openal.os).
// Interleaved signed 16-bit stereo (L, R, L, R ...). One "frame" = one L+R pair.
// Same PcmOut API as gbsound.os (miniaudio version) except open() takes only the sample rate.
//
// How it works: samples are collected into 1024-frame chunks, each chunk is copied into one of 8 OpenAL
// buffers and queued on a single source. Finished buffers are reclaimed on every call. Playback
// starts once 2 buffers are queued and restarts by itself after an underrun.
module AL = import "standard/1/c/openal.os";

let AL_FORMAT_STEREO16: int = 0x1103;
let AL_SOURCE_STATE: int = 0x1010;
let AL_PLAYING: int = 0x1012;
let AL_BUFFERS_PROCESSED: int = 0x1016;
let AL_BUFFER: int = 0x1009;

let CHUNK: int = 1024;      // frames per buffer (about 23 ms at 44100 Hz)
let NBUF: int = 8;          // buffers in flight (about 186 ms total)
let STAGE: i16[2048];       // CHUNK * 2 samples
let BUFS: u32[8];           // all buffer ids
let FREE: u32[8];           // buffers not currently queued

struct PcmOut {
    dev: void* = nullptr;
    ctx: void* = nullptr;
    src: u32 = 0;
    rate: int = 44100;
    stageN: int = 0;
    freeN: int = 0;
    isOpen: bool = false;

    open(rate: int) => bool {
        if (this.isOpen) { return true; }
        this.dev = AL.alcOpenDevice(nullptr);
        if (this.dev == nullptr) { return false; }
        this.ctx = AL.alcCreateContext(this.dev, nullptr);
        if (this.ctx == nullptr) { AL.alcCloseDevice(this.dev); this.dev = nullptr; return false; }
        AL.alcMakeContextCurrent(this.ctx);
        let s: u32 = 0;
        AL.alGenSources(1, &s);
        this.src = s;
        AL.alGenBuffers(NBUF, BUFS);
        for (let i: int = 0; i < NBUF; i = i + 1) { FREE[i] = BUFS[i]; }
        this.freeN = NBUF;
        this.stageN = 0;
        this.rate = rate;
        this.isOpen = true;
        return true;
    }

    // Moves buffers OpenAL has finished playing back into the free list.
    reclaim() => void {
        let done: int = 0;
        AL.alGetSourcei(this.src, AL_BUFFERS_PROCESSED, &done);
        for (let i: int = 0; i < done; i = i + 1) {
            let b: u32 = 0;
            AL.alSourceUnqueueBuffers(this.src, 1, &b);
            FREE[this.freeN] = b;
            this.freeN = this.freeN + 1;
        }
    }

    // Sends the full stage chunk to OpenAL. Caller guarantees freeN > 0.
    submit() => void {
        this.freeN = this.freeN - 1;
        let b: u32 = FREE[this.freeN];
        AL.alBufferData(b, AL_FORMAT_STEREO16, STAGE as void*, CHUNK * 4, this.rate);
        AL.alSourceQueueBuffers(this.src, 1, &b);
        this.stageN = 0;
        let st: int = 0;
        AL.alGetSourcei(this.src, AL_SOURCE_STATE, &st);
        if (st != AL_PLAYING && (NBUF - this.freeN) >= 2) { AL.alSourcePlay(this.src); }
    }

    // Queues up to `frames` stereo frames (2 i16 per frame). Returns how many were accepted.
    push(samples: i16*, frames: int) => int {
        if (!this.isOpen) { return 0; }
        this.reclaim();
        let pushed: int = 0;
        let stop: bool = false;
        while (pushed < frames && !stop) {
            let n: int = frames - pushed;
            let room: int = CHUNK - this.stageN;
            if (n > room) { n = room; }
            for (let i: int = 0; i < n * 2; i = i + 1) { STAGE[this.stageN * 2 + i] = samples[pushed * 2 + i]; }
            this.stageN = this.stageN + n;
            pushed = pushed + n;
            if (this.stageN == CHUNK) {
                if (this.freeN > 0) { this.submit(); } else { stop = true; }
            }
        }
        return pushed;
    }

    // Frames waiting to be played (accurate to one chunk) / frames that can still be pushed.
    queued() => int {
        if (!this.isOpen) { return 0; }
        this.reclaim();
        return (NBUF - this.freeN) * CHUNK + this.stageN;
    }
    space() => int {
        if (!this.isOpen) { return 0; }
        this.reclaim();
        return this.freeN * CHUNK + (CHUNK - this.stageN);
    }

    close() => void {
        if (!this.isOpen) { return; }
        AL.alSourceStop(this.src);
        AL.alSourcei(this.src, AL_BUFFER, 0);
        let s: u32 = this.src;
        AL.alDeleteSources(1, &s);
        AL.alDeleteBuffers(NBUF, BUFS);
        AL.alcMakeContextCurrent(nullptr);
        AL.alcDestroyContext(this.ctx);
        AL.alcCloseDevice(this.dev);
        this.ctx = nullptr;
        this.dev = nullptr;
        this.isOpen = false;
    }
}
