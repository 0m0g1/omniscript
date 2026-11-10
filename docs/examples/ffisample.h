#ifndef T1_H
#define T1_H
#include <stdint.h>
#define VERSION_MAJOR 2
#define VERSION_STR "1.2.3"
#define BUF_SIZE (VERSION_MAJOR * 8)
#define EXPORT __declspec(dllexport)
#define CALL
#define FLAG_A 0x01u
#define BIG 0xFFFFFFFFFFULL
#define PI 3.14159f
#if defined(_WIN32) && !defined(FOO)
#define PLATFORM 1
#else
#define PLATFORM 2
#endif
#if 0
int hidden(int);
#endif
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _Vec2 { float x, y; } Vec2;
typedef struct { int w, h; unsigned flags; char name[BUF_SIZE]; void (*cb)(int); } Rect;
typedef struct Opaque Opaque;
typedef enum { RED, GREEN = 5, BLUE } Color;
enum { ANON_A = 1, ANON_B = 1 << 3 };
typedef void (*cb_t)(int, void*);
typedef char* (*namefn)(void);
typedef unsigned char BYTE;
typedef uint32_t u32_alias;
EXPORT int CALL add(int x, int y);
const char* name(void);
void fill(BYTE* buf, size_t n, cb_t cb);
int printf(const char* fmt, ...);
extern int g_counter;
extern const int g_limit;
static const int LOCAL_K = 7;
Vec2 make(float x, float y);
Opaque* open_it(const char* path);
void close_it(Opaque* o);
struct Opaque* open2(void);
enum Color pick(enum Color c);
Color pick2(Color c);
unsigned int uadd(unsigned a, unsigned long long b);
static inline int sq(int x) { return x * x; }
int arr_param(int a[], const Rect* r);
void reg(namefn f);
#ifdef __cplusplus
}
#endif
#endif