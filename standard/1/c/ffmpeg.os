// standard/1/c/ffmpeg.os  (avformat / avcodec / avutil / swscale; AVFormatContext etc. are opaque void*)
extern "C" fn avformat_open_input(ctx: void**, url: char*, fmt: void*, opts: void**) => int;
extern "C" fn avformat_find_stream_info(ctx: void*, opts: void**) => int;
extern "C" fn avformat_close_input(ctx: void**) => void;
extern "C" fn av_find_best_stream(ctx: void*, kind: int, wanted: int, related: int, dec: void**, flags: int) => int;
extern "C" fn av_read_frame(ctx: void*, pkt: void*) => int;
extern "C" fn av_seek_frame(ctx: void*, stream: int, ts: i64, flags: int) => int;
extern "C" fn av_packet_alloc() => void*;
extern "C" fn av_packet_unref(pkt: void*) => void;
extern "C" fn av_packet_free(pkt: void**) => void;
extern "C" fn av_frame_alloc() => void*;
extern "C" fn av_frame_free(f: void**) => void;
extern "C" fn avcodec_alloc_context3(codec: void*) => void*;
extern "C" fn avcodec_parameters_to_context(ctx: void*, par: void*) => int;
extern "C" fn avcodec_open2(ctx: void*, codec: void*, opts: void**) => int;
extern "C" fn avcodec_send_packet(ctx: void*, pkt: void*) => int;
extern "C" fn avcodec_receive_frame(ctx: void*, frame: void*) => int;
extern "C" fn avcodec_free_context(ctx: void**) => void;
extern "C" fn sws_getContext(sw: int, sh: int, sf: int, dw: int, dh: int, df: int, flags: int, a: void*, b: void*, c: void*) => void*;
extern "C" fn sws_scale(ctx: void*, src: u8**, srcStride: int*, y: int, h: int, dst: u8**, dstStride: int*) => int;
extern "C" fn sws_freeContext(ctx: void*) => void;
extern "C" fn av_image_get_buffer_size(fmt: int, w: int, h: int, align: int) => int;
extern "C" fn av_image_fill_arrays(data: u8**, lines: int*, buf: u8*, fmt: int, w: int, h: int, align: int) => int;
// NOTE: reading width/height/pts/duration needs AVFrame/AVStream/AVFormatContext field offsets (version-specific): use a C shim.
