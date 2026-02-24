// include/omniscript/Backends/AOTBackend.h
#pragma once

#include <omniscript/backends/Backend.h>

namespace Omniscript {

// AOT backends typically generate output files (obj/exe/ir/etc)
class AOTBackend : public Backend {
public:
    ~AOTBackend() override = default;

    // Emit to file(s) based on config.aot.outputFormat / config.outputPath
    virtual void emitToFile(const Config& config) = 0;
};

} // namespace Omniscript