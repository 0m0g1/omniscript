// #ifndef AudioAccess_H
// #define AudioAccess_H

// #include <omniscript/omniscript_pch.h>
// #include <omniscript/runtime/object.h>
// #include <omniscript/runtime/Audio/Audio.h>

// class AudioAccess : public Object {
//     public:

//         // TODO:: Have a method to check all available sound outputting devices
//         // then in open you can choose which device plays a certain audio or in the file itself
//         // sound buffer, sound device, audio bus class, and sound source classes
//         AudioAccess();

//         ~AudioAccess();

//         std::string toString(int indentLevel = 0) const override {
//             return "[AudioAccess]";
//         }

//         static void play(ALuint source);
//         std::shared_ptr<Audio> open(const std::string& path);
//         std::string getAvailableDevices();
//     private:
//         static ALCdevice* SoundDevice;
//         static ALCcontext* SoundContext;
// };

// #endif