// standard/1/Video.os  -- STUB (not implemented yet)
// Tier 7: video decode/playback (ffmpeg libav* or Media Foundation) feeding Image/OpenGL + Audio.
// All functions return false / do nothing until implemented.
struct Video {
    open(path: char*) => bool { return false; }                   // TODO: avformat_open_input, find streams
    nextFrame() => bool { return false; }                         // TODO: av_read_frame/avcodec_receive_frame/sws_scale -> RGBA
    seek(sec: double) => bool { return false; }
    close() => void { }
}
