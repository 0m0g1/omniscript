// include/omniscript/backends/JITBackend.h
#pragma once

#include <omniscript/backends/Backend.h>

namespace Omniscript {

// Marker interface for JIT-style backends (execute == run)
class JITBackend : public Backend {
public:
    ~JITBackend() override = default;
};

} // namespace Omniscript