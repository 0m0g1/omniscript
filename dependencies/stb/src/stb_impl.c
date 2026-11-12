#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_truetype.h"
#include <stdlib.h>

__declspec(dllexport) void* om_alloc(unsigned long long n) { return malloc(n); }
__declspec(dllexport) void  om_free(void* p) { free(p); }