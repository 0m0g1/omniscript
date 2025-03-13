// #include <omniscript/Core.h>
// #include <omniscript/runtime/Audio/AudioAccess.h>
// #include <omniscript/debuggingtools/console.h>

// // Define the static members
// ALCdevice* AudioAccess::SoundDevice = nullptr;  // Initialize to nullptr or a valid device
// ALCcontext* AudioAccess::SoundContext = nullptr;  // Initialize to nullptr or a valid context

// AudioAccess::AudioAccess() {
//     // // Initialize FFmpeg
//     // av_register_all(); // Deprecated in newer versions of FFmpeg
//     // avformat_network_init(); // Optional, initialize network components for FFmpeg

//     // Initialize OpenAL
//     ALCdevice* device = alcOpenDevice(nullptr);
//     if (!device) {
//         console.error("Failed to initialize OpenAL device.");
//         return;
//     }

//     ALCcontext* context = alcCreateContext(device, nullptr);
//     if (!context || alcMakeContextCurrent(context) == ALC_FALSE) {
//         console.error("Failed to create or make OpenAL context current.");
//         alcCloseDevice(device);
//         return;
//     }

//     // Store the OpenAL device and context in the class for later use
//     SoundDevice = device;
//     SoundContext = context;

//     console.debug("OpenAL initialized successfully.");

//     // Add methods to the class
//     addMethod("open", [this](const ArgumentDefinition& args) -> std::shared_ptr<Audio> {
//         if (args.size() != 1 || !std::holds_alternative<std::string>(args[0])) {
//             console.error("AudioAccess::open requires a single argument: the path to the audio file.");
//         }
//         std::string path = std::get<std::string>(args[0]);
//         return std::make_shared<Audio>(path);
//     });

//     addMethod("getAvailableSoundDevices", [this](const ArgumentDefinition& args) {
//         if (!args.empty()) {
//             console.error("AudioAccess::getAvailableSoundDevices does not accept any arguments.");
//         }
//         return SymbolTable::ValueType(getAvailableDevices());
//     });
// }

// // Destructor to clean up OpenAL resources
// AudioAccess::~AudioAccess() {
//     if (SoundContext) {
//         alcMakeContextCurrent(nullptr);
//         alcDestroyContext(SoundContext);
//     }
//     if (SoundDevice) {
//         alcCloseDevice(SoundDevice);
//     }
//     // avformat_network_deinit(); // Clean up FFmpeg network components
//     console.debug("AudioAccess cleaned up successfully.");
// }



// std::shared_ptr<Audio> AudioAccess::open(const std::string& path) {
//     return std::make_shared<Audio>(path);
// }

// void AudioAccess::play(ALuint source) {
//     if (!SoundContext) {
//         console.error("Error: SoundContext is not initialized");
//         return;
//     }

//     // Make the context current
//     alcMakeContextCurrent(SoundContext);

//     // Play the audio source
//     alSourcePlay(source);
//     if (alGetError() != AL_NO_ERROR) {
//         console.error("Error: Failed to play audio");
//         return;
//     }

//     // Wait until the source is no longer playing
//     ALint state;
//     Omniscript::allThreadsDone = false;
//     do {
//         alGetSourcei(source, AL_SOURCE_STATE, &state);
//         std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 60));
//     } while (state == AL_PLAYING);
//     Omniscript::allThreadsDone = true;

//     if (alGetError() != AL_NO_ERROR) {
//         console.error("Error: Playback encountered an issue");
//     }
// }


// std::string AudioAccess::getAvailableDevices() {
//     // Check if enumeration is supported
//     if (!alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT")) {
//         return "Device enumeration not supported!";
//     }

//     // Get the list of devices
//     const ALCchar* deviceList = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
//     if (!deviceList) {
//         return "Failed to get device list!";
//     }

//     // Parse the null-separated list into a single string
//     std::ostringstream result;
//     const ALCchar* device = deviceList;
//     while (*device != '\0') {
//         result << device << '\n';
//         device += strlen(device) + 1; // Move to the next device
//     }

//     return result.str();
// }