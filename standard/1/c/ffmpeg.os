// standard/1/c/ffmpeg.os  (FFmpeg 8.x: avformat-62 / avcodec-62 / avutil-60 / swscale-9 / swresample-6)
// AVFormatContext, AVCodecContext, AVFrame, AVPacket etc. are opaque void*.
// NOTE: reading width/height/pts/duration needs struct field offsets (version-specific): use a C shim.
// NOTE: functions taking AVRational BY VALUE (av_rescale_q, av_packet_rescale_ts) are omitted on purpose.

// ---------------- libavformat: demux / mux ----------------
extern "dependencies/ffmpeg/bin/avformat-62.dll" {
    fn avformat_alloc_context() => void*;
    fn avformat_free_context(ctx: void*) => void;
    fn avformat_open_input(ctx: void**, url: char*, fmt: void*, opts: void**) => int;
    fn avformat_find_stream_info(ctx: void*, opts: void**) => int;
    fn avformat_close_input(ctx: void**) => void;
    fn avformat_seek_file(ctx: void*, stream: int, min_ts: i64, ts: i64, max_ts: i64, flags: int) => int;
    fn avformat_network_init() => int;
    fn avformat_network_deinit() => int;
    fn avformat_alloc_output_context2(ctx: void**, fmt: void*, format_name: char*, filename: char*) => int;
    fn avformat_new_stream(ctx: void*, codec: void*) => void*;
    fn avformat_write_header(ctx: void*, opts: void**) => int;
    fn avio_open(pb: void**, url: char*, flags: int) => int;
    fn avio_closep(pb: void**) => int;
    fn av_find_best_stream(ctx: void*, kind: int, wanted: int, related: int, dec: void**, flags: int) => int;
    fn av_read_frame(ctx: void*, pkt: void*) => int;
    fn av_seek_frame(ctx: void*, stream: int, ts: i64, flags: int) => int;
    fn av_write_frame(ctx: void*, pkt: void*) => int;
    fn av_interleaved_write_frame(ctx: void*, pkt: void*) => int;
    fn av_write_trailer(ctx: void*) => int;
    fn av_dump_format(ctx: void*, index: int, url: char*, is_output: int) => void;
}

// ---------------- libavcodec: encode / decode ----------------
extern "dependencies/ffmpeg/bin/avcodec-62.dll" {
    fn av_packet_alloc() => void*;
    fn av_packet_clone(pkt: void*) => void*;
    fn av_packet_unref(pkt: void*) => void;
    fn av_packet_free(pkt: void**) => void;
    fn avcodec_find_decoder(id: int) => void*;
    fn avcodec_find_decoder_by_name(name: char*) => void*;
    fn avcodec_find_encoder(id: int) => void*;
    fn avcodec_find_encoder_by_name(name: char*) => void*;
    fn avcodec_get_name(id: int) => char*;
    fn avcodec_alloc_context3(codec: void*) => void*;
    fn avcodec_parameters_to_context(ctx: void*, par: void*) => int;
    fn avcodec_parameters_from_context(par: void*, ctx: void*) => int;
    fn avcodec_open2(ctx: void*, codec: void*, opts: void**) => int;
    fn avcodec_send_packet(ctx: void*, pkt: void*) => int;
    fn avcodec_receive_frame(ctx: void*, frame: void*) => int;
    fn avcodec_send_frame(ctx: void*, frame: void*) => int;
    fn avcodec_receive_packet(ctx: void*, pkt: void*) => int;
    fn avcodec_flush_buffers(ctx: void*) => void;
    fn avcodec_free_context(ctx: void**) => void;
    fn avcodec_version() => u32;
}

// ---------------- libavutil: frames, images, memory, errors ----------------
extern "dependencies/ffmpeg/bin/avutil-60.dll" {
    fn av_frame_alloc() => void*;
    fn av_frame_free(f: void**) => void;
    fn av_frame_unref(f: void*) => void;
    fn av_frame_ref(dst: void*, src: void*) => int;
    fn av_frame_clone(src: void*) => void*;
    fn av_frame_get_buffer(f: void*, align: int) => int;
    fn av_frame_make_writable(f: void*) => int;
    fn av_image_get_buffer_size(fmt: int, w: int, h: int, align: int) => int;
    fn av_image_fill_arrays(data: u8**, lines: int*, buf: u8*, fmt: int, w: int, h: int, align: int) => int;
    fn av_image_alloc(data: u8**, lines: int*, w: int, h: int, fmt: int, align: int) => int;
    fn av_malloc(size: u64) => void*;
    fn av_free(p: void*) => void;
    fn av_freep(pp: void*) => void;
    fn av_strerror(err: int, buf: char*, size: u64) => int;
    fn av_log_set_level(level: int) => void;
    fn av_dict_set(d: void**, key: char*, value: char*, flags: int) => int;
    fn av_dict_free(d: void**) => void;
    fn av_get_pix_fmt(name: char*) => int;
    fn av_get_pix_fmt_name(fmt: int) => char*;
    fn av_get_bytes_per_sample(fmt: int) => int;
    fn av_samples_get_buffer_size(lines: int*, channels: int, samples: int, fmt: int, align: int) => int;
    fn av_gettime() => i64;
    fn avutil_version() => u32;
}

// ---------------- libswscale: pixel format conversion / scaling ----------------
extern "dependencies/ffmpeg/bin/swscale-9.dll" {
    fn sws_getContext(sw: int, sh: int, sf: int, dw: int, dh: int, df: int, flags: int, a: void*, b: void*, c: void*) => void*;
    fn sws_getCachedContext(ctx: void*, sw: int, sh: int, sf: int, dw: int, dh: int, df: int, flags: int, a: void*, b: void*, c: void*) => void*;
    fn sws_scale(ctx: void*, src: u8**, srcStride: int*, y: int, h: int, dst: u8**, dstStride: int*) => int;
    fn sws_freeContext(ctx: void*) => void;
    fn swscale_version() => u32;
}

// ---------------- libswresample: audio resampling ----------------
extern "dependencies/ffmpeg/bin/swresample-6.dll" {
    fn swr_alloc() => void*;
    fn swr_init(ctx: void*) => int;
    fn swr_convert(ctx: void*, out: u8**, out_count: int, in: u8**, in_count: int) => int;
    fn swr_get_delay(ctx: void*, base: i64) => i64;
    fn swr_free(ctx: void**) => void;
    fn swresample_version() => u32;
}
