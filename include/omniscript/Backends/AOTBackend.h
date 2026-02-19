// AOTBackend.h
#pragma once

#include <omniscript/Backend.h>

namespace Omniscript {
class AOTBackend : public Backend {
public:
    virtual ~AOTBackend() = default;

    /// Emit to an object file or intermediate representation
    virtual void initialize() = 0;
    virtual void emitToFile(const Config& config) = 0;
};

} // namespace Omniscript
