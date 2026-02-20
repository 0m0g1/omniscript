#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// C-linkage (should be kept even when parsing as C++)
int add_i32(int a, int b);

// C-linkage with pointers
void fill_buffer(uint8_t* out, uint32_t n);

// varargs (C)
int c_printf(const char* fmt, ...);

// C global
extern int g_counter;

#ifdef __cplusplus
} // extern "C"
#endif


// -------------------------
// C++ (should be SKIPPED by default when linkage-mode is C++ and allowMangled=false)
// -------------------------

namespace gfx {

enum class Mode : uint32_t { A = 1, B = 2 };

struct Vec2 { float x, y; };

int overloaded(int x);
int overloaded(double x);

class Renderer {
public:
    Renderer();
    ~Renderer();

    void draw(Vec2 p) const noexcept;
    static int static_fn(int x);
};

template <typename T>
T templated(T v) { return v; }

} // namespace gfx