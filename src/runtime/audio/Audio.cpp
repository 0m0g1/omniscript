// #include <omniscript/omniscript_pch.h>
// #include <omniscript/runtime/Audio/Audio.h>
// #include <omniscript/runtime/Audio/AudioAccess.h>
// #include <omniscript/debuggingtools/console.h>

// // Audio class constructor
// Audio::Audio(const std::string& filePath) : path(path) {
//     AVFormatContext* formatCtx = nullptr;
//     if (avformat_open_input(&formatCtx, filePath.c_str(), nullptr, nullptr) < 0) {
//         throw std::runtime_error("Could not open audio file: " + filePath);
//     }

//     // Find the audio stream
//     if (avformat_find_stream_info(formatCtx, nullptr) < 0) {
//         avformat_close_input(&formatCtx);
//         throw std::runtime_error("Could not retrieve stream info from file.");
//     }

//     const AVCodec* codec = nullptr; // Make this a const AVCodec*
//     int streamIndex = av_find_best_stream(formatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);

//     if (streamIndex < 0) {
//         console.error("Error: Failed to find the best stream.");
//     }


//     // Open the codec context
//     AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
//     if (!codecCtx) {
//         avformat_close_input(&formatCtx);
//         throw std::runtime_error("Could not allocate codec context.");
//     }

//     avcodec_parameters_to_context(codecCtx, formatCtx->streams[streamIndex]->codecpar);
//     if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
//         avcodec_free_context(&codecCtx);
//         avformat_close_input(&formatCtx);
//         throw std::runtime_error("Could not open codec.");
//     }

//     // Resampler setup
//     AVChannelLayout out_ch_layout = {};
//     av_channel_layout_default(&out_ch_layout, AV_CH_LAYOUT_MONO);

//     AVChannelLayout in_ch_layout = {};
//     av_channel_layout_default(&in_ch_layout, codecCtx->ch_layout.nb_channels);

//     // Allocate SwrContext
//     SwrContext* swrCtx = nullptr;  // Initialize as a nullptr

//     int ret = swr_alloc_set_opts2(
//         &swrCtx,                // Pass the address of swrCtx
//         &out_ch_layout,         // Output channel layout
//         AV_SAMPLE_FMT_S16,      // Output format
//         codecCtx->sample_rate,  // Output sample rate
//         &codecCtx->ch_layout,   // Input channel layout
//         codecCtx->sample_fmt,   // Input format
//         codecCtx->sample_rate,  // Input sample rate
//         0,                      // Flags (optional)
//         nullptr                 // Log context (optional)
//     );

//     if (ret < 0 || !swrCtx) {
//         // Ensure swrCtx is freed if allocated, even on error
//         if (swrCtx) {
//             swr_free(&swrCtx);  // Free SwrContext if allocated
//         }
//         avcodec_free_context(&codecCtx);
//         avformat_close_input(&formatCtx);
//         console.error("Error initializing the SwrContext (" + std::to_string(ret) + "), Could not initialize the audio resampler.");
//         return;
//     }

//     // Decode audio and convert to PCM
//     AVPacket* packet = av_packet_alloc();
//     AVFrame* frame = av_frame_alloc();
//     std::vector<uint8_t> pcmData;

//     while (av_read_frame(formatCtx, packet) >= 0) {
//         if (packet->stream_index == streamIndex) {
//             if (avcodec_send_packet(codecCtx, packet) == 0) {
//                 while (avcodec_receive_frame(codecCtx, frame) == 0) {
//                     int dstLinesize = 0;
//                     uint8_t* dstData = nullptr;
//                     int dstSamples = swr_convert(
//                         swrCtx,
//                         &dstData,
//                         frame->nb_samples,
//                         (const uint8_t**)frame->data,
//                         frame->nb_samples
//                     );

//                     if (dstSamples > 0) {
//                         pcmData.insert(
//                             pcmData.end(),
//                             dstData,
//                             dstData + dstSamples * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16)
//                         );
//                     }
//                 }
//             }
//         }
//         av_packet_unref(packet);
//     }

//     // Free FFmpeg resources
//     av_frame_free(&frame);
//     av_packet_free(&packet);
//     swr_free(&swrCtx);
//     avcodec_free_context(&codecCtx);
//     avformat_close_input(&formatCtx);

//     // Create OpenAL buffer and source
//     // ALuint buffer;
//     // Generate OpenAL buffer and source
//     alGenBuffers(1, &buffer);
//     alGenSources(1, &source);
//     alBufferData(buffer, AL_FORMAT_MONO16, pcmData.data(), pcmData.size(), codecCtx->sample_rate);
    
//     // Attach buffer to source
//     alSourcei(source, AL_BUFFER, buffer);

//     if (alGetError() != AL_NO_ERROR) {
//         alDeleteBuffers(1, &buffer);
//         throw std::runtime_error("Failed to create OpenAL buffer.");
//     }

//     addMethod("play", [this](const ArgumentDefinition& args) {
//         if (args.size() > 0) {
//             console.error("Audio.play() does not take in any arguments");
//         }
//         play();
//         return SymbolTable::ValueType{};
//     });

//     addMethod("pause", [this](const ArgumentDefinition& args) {
//         if (args.size() > 0) {
//             console.error("Audio.pause() does not take in any arguments");
//         }
//         pause();
//         return SymbolTable::ValueType{};
//     });
// }

// // Audio class destructor
// Audio::~Audio() {
//     alDeleteSources(1, &source);
//     alDeleteBuffers(1, &buffer);
// }

// // Play the audio
// void Audio::play() {
//     AudioAccess::play(source);
//     if (alGetError() != AL_NO_ERROR) {
//         console.error("Error: Failed to play audio");
//     }
// }

// // Pause the audio
// void Audio::pause() {
//     alSourcePause(source);
//     if (alGetError() != AL_NO_ERROR) {
//         console.error("Error: Failed to pause audio");
//     }
// }

// // Seek the audio to a specific time in seconds
// void Audio::seek(int time) {
//     // OpenAL doesn't support seeking directly, but you can stop and restart playback
//     // This is a placeholder for an actual seek implementation
//     console.error("Seek not implemented in OpenAL, needs custom seek logic.");
// }

// // Seek to a specific mark by name
// void Audio::seek(const std::string& mark) {
//     if (marks.find(mark) != marks.end()) {
//         seek(marks[mark]); // Use the time for the mark
//     } else {
//         console.error("Mark not found: " + mark);
//     }
// }

// // Set looping for the audio
// void Audio::setLoop(bool enable) {
//     loop = enable;
//     alSourcei(source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
// }

// // Get the length of the audio
// int Audio::getLength() {
//     // OpenAL doesn't directly support getting the length, but you can approximate it
//     // based on the sample rate and the number of samples in the buffer
//     // This needs implementation based on your decoded data
//     return 0; // Placeholder
// }

// // Trim the audio (remove part of it from start to end)
// void Audio::trim(int start, int end) {
//     // Implement trimming logic (removing parts from the buffer)
// }

// // Cut the audio (remove part from start to end)
// void Audio::cut(int start, int end) {
//     // Implement cut logic (removing part of the buffer)
// }

// // Crop the audio (keep only part from start to end)
// void Audio::crop(int start, int end) {
//     // Implement crop logic (creating a new buffer with a specific part of the audio)
// }

// // Merge this audio with another audio (start position in the current audio)
// void Audio::merge(std::shared_ptr<Audio> other, int start) {
//     // Implement merge logic (combine two buffers into one)
// }

// // Mark a specific point in time with a name
// void Audio::mark(int time, const std::string& name) {
//     marks[name] = time;
// }

// // Remove a specific mark by name
// void Audio::removeMark(const std::string& name) {
//     marks.erase(name);
// }

// // Remove effect from a section of the audio
// void Audio::removeEffect(const std::string& name, int start, int end) {
//     // Implement remove effect logic
// }

// // Apply an effect to a section of the audio
// void Audio::applyEffect(const std::string& name, int start, int end) {
//     // Implement apply effect logic
// }

// // Set the volume of the audio
// void Audio::setVolume(float volume) {
//     this->volume = volume;
//     alSourcef(source, AL_GAIN, volume);
// }

// // Clone the audio object
// Audio Audio::clone() {
//     return Audio(path); // Simple clone returning a new Audio object
// }
