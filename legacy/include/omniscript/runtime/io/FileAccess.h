#ifndef FileAccess_H
#define FileAccess_H

#include <omniscript/omniscript_pch.h>
#include <omniscript/runtime/object.h>
#include <omniscript/runtime/io/File.h>

class FileAccess : public Object {
public:
    enum Mode {READ, WRITE, READWRITE, WRITEREAD, APPEND, BINARY, ATEND};

    FileAccess();
    ~FileAccess() {};

    std::string toString(int indentLevel = 0) const override {
        return "[FileAccess]";
    }
    
    std::shared_ptr<File> open(std::string& path, Mode mode);

};

#endif