#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <queue>
#include <memory>
#include <variant>
#include <functional>
#include <algorithm>
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <type_traits>
#include <utility>
#include <stdexcept>
#include <cstdlib>
#include <cctype>
#include <inttypes.h>
#include <cstddef>
#include <stack>
#include <atomic>
#include <numeric>
#include <string>
#include <optional>
#include <filesystem>
#include <thread>
#include <regex>
#include <future>
#include <cassert>
#include <cstdarg>
#include <string_view>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <future>
#include <span>
#include <expected>

// System-Specific Headers
#ifdef _WIN32
    #include <winsock2.h>
    #include <Windows.h>
    #include <psapi.h>
#elif defined(__linux__) || defined(__APPLE__)
    #include <sys/resource.h>
    #include <unistd.h>
    if defined(__APPLE__)
        #include <mach/mach.h>
        #include <sys/utsname.h>
        #include <TargetConditionals.h>
    #elif defined(__linux__)
        #include <sys/utsname.h>
        #include <malloc.h>
#endif

#if defined(__AVX512F__)
    #include <immintrin.h>
    #define SIMD_OPTIMIZATION_LEVEL 512
#elif defined(__AVX2__)
    #include <immintrin.h>
    #define SIMD_OPTIMIZATION_LEVEL 256
#elif defined(__AVX__)
    #include <immintrin.h>
    #define SIMD_OPTIMIZATION_LEVEL 128
#else
    #define SIMD_OPTIMIZATION_LEVEL 64
#endif