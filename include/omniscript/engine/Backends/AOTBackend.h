// AOTBackend.h
#pragma once

#include <omniscript/engine/Backend.h>

class AOTBackend : public Backend {
public:
    virtual ~AOTBackend() = default;

    /// Emit to an object file or intermediate representation
    virtual void emitToFile(const Config& config) = 0;
};
