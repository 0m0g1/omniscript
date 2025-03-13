// #ifndef Audio_H
// #define Audio_H

// #include <omniscript/omniscript_pch.h>
// #include <omniscript/runtime/object.h>


// class Audio : public Object {
// public:
//     Audio(const std::string& path);
//     ~Audio();

//     void play();
//     void pause();
//     void seek(int time);
//     void seek(const std::string& mark);
//     void setLoop(bool enable = true);
//     int getLength();
//     void trim(int start, int end);
//     // Todo:: a method to remove a section of audio and leave a
//     // blanck space between and one to remove and join the separated segments
//     void cut(int start, int end);
//     void crop(int start, int end);
//     void merge(std::shared_ptr<Audio> other, int start);
//     void mark(int time, const std::string& name);
//     void removeMark(const std::string& name);
//     void removeEffect(const std::string& name, int start, int end);
//     void applyEffect(const std::string& name, int start, int end);
//     void setVolume(float volume);
//     Audio clone();

// private:
//     float volume;

//     ALuint buffer;
//     ALuint source;
    
//     bool loop = false;
//     std::string path;
//     bool isPlaying = false;
//     std::unordered_map<std::string, int> marks;

// };

// #endif
