#pragma once

// Standard Library Includes (Order Matters!)
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
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
#include <cstdlib> // Keep this after defining _GLIBCXX_HAVE_TIMESPEC_GET
#include <cctype>
#include <inttypes.h>
#include <cstddef>

// System-Specific Headers
#ifdef _WIN32
    #include <winsock2.h>  // Include before <Windows.h>
    #include <Windows.h>
#elif defined(__linux__) || defined(__APPLE__)
    #include <sys/resource.h>
    #include <unistd.h>
#endif

// Uncomment if you need them
// #include <curl/curl.h>
// #include <AL/al.h>
// #include <AL/alc.h>
// #include <AL/alext.h>
