#include <set>
#include <map>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <filesystem>
#include <omniscript/backends/llvm/IRGenerator.h>
#include <omniscript/backends/llvm/LLVMExternalFunctionResolver.h>
#include <omniscript/backends/llvm/ExternalFunctionResolvers/CLLVMResolver.h>
#include <omniscript/backends/llvm/ExternalFunctionResolvers/LinuxLLVMResolver.h>
#include <omniscript/backends/llvm/ExternalFunctionResolvers/PosixLLVMResolver.h>
#include <omniscript/backends/llvm/ExternalFunctionResolvers/DarwinLLVMResolver.h>
#include <omniscript/backends/llvm/ExternalFunctionResolvers/AndroidLLVMResolver.h>
#include <omniscript/backends/llvm/ExternalFunctionResolvers/WindowsAPILLVMResolver.h>
#include <omniscript/backends/llvm/ExternalFunctionResolvers/WebAssemblyLLVMResolver.h>
#include <omniscript/backends/llvm/ExternalFunctionResolvers/SmartPlatformLLVMResolver.h>
#include <omniscript/backends/llvm/ExternalFunctionResolvers/StaticLibraryLLVMResolver.h>
#include <omniscript/backends/llvm/ExternalFunctionResolvers/DynamicLibraryLLVMResolver.h>

namespace Omniscript {
bool ExternalFunctionResolver::isSystemLibrary(const std::string& libPath) {
    static const std::unordered_set<std::string> systemLibs = {
        // Windows System Libraries (.lib for static, .dll for dynamic)
        // Core Windows APIs
        "kernel32.lib", "kernel32.dll",
        "user32.lib", "user32.dll",
        "gdi32.lib", "gdi32.dll",
        "shell32.lib", "shell32.dll",
        "ntdll.lib", "ntdll.dll",
        "advapi32.lib", "advapi32.dll",
        "ole32.lib", "ole32.dll",
        "oleaut32.lib", "oleaut32.dll",
        "uuid.lib",
        "ws2_32.lib", "ws2_32.dll",
        "winmm.lib", "winmm.dll",
        "comctl32.lib", "comctl32.dll",
        "comdlg32.lib", "comdlg32.dll",
        "winspool.lib", "winspool.dll",
        "wininet.lib", "wininet.dll",
        "crypt32.lib", "crypt32.dll",
        "secur32.lib", "secur32.dll",
        "netapi32.lib", "netapi32.dll",
        "version.lib", "version.dll",
        "imm32.lib", "imm32.dll",
        "psapi.lib", "psapi.dll",
        "iphlpapi.lib", "iphlpapi.dll",
        "setupapi.lib", "setupapi.dll",
        "dbghelp.lib", "dbghelp.dll",
        "imagehlp.lib", "imagehlp.dll",
        "shlwapi.lib", "shlwapi.dll",
        "wtsapi32.lib", "wtsapi32.dll",
        "userenv.lib", "userenv.dll",
        "mpr.lib", "mpr.dll",
        "cabinet.lib", "cabinet.dll",
        "rpcrt4.lib", "rpcrt4.dll",
        "powrprof.lib", "powrprof.dll",
        "dwmapi.lib", "dwmapi.dll",
        "uxtheme.lib", "uxtheme.dll",
        "bcrypt.lib", "bcrypt.dll",
        "ncrypt.lib", "ncrypt.dll",
        
        // Windows CRT Libraries
        "msvcrt.lib", "msvcrt.dll",
        "ucrt.lib", "ucrtbase.dll",
        "vcruntime.lib", "vcruntime140.dll",
        "vcruntime140d.dll", "vcruntime140_1.dll",
        "concrt140.lib", "concrt140.dll",
        "msvcp140.lib", "msvcp140.dll",
        "vcomp140.lib", "vcomp140.dll",
        "msvcr120.lib", "msvcr120.dll",
        "msvcp120.lib", "msvcp120.dll",
        "msvcr110.lib", "msvcr110.dll",
        "msvcp110.lib", "msvcp110.dll",
        "msvcr100.lib", "msvcr100.dll",
        "msvcp100.lib", "msvcp100.dll",
        
        // Windows Graphics and Media
        "d3d9.lib", "d3d9.dll",
        "d3d11.lib", "d3d11.dll",
        "d3d12.lib", "d3d12.dll",
        "dxgi.lib", "dxgi.dll",
        "d3dcompiler.lib", "d3dcompiler_47.dll",
        "dinput8.lib", "dinput8.dll",
        "dsound.lib", "dsound.dll",
        "dxguid.lib",
        "opengl32.lib", "opengl32.dll",
        "glu32.lib", "glu32.dll",
        "gdi32.lib", "gdi32.dll",
        "mf.lib", "mf.dll",
        "mfplat.lib", "mfplat.dll",
        "mfreadwrite.lib", "mfreadwrite.dll",
        "mfuuid.lib",
        "strmiids.lib",
        "quartz.lib", "quartz.dll",
        "wmcodecdspuuid.lib",
        
        // Linux System Libraries (.so for shared, .a for static)
        // Core C libraries
        "libc.so", "libc.so.6", "libc.a",
        "libm.so", "libm.so.6", "libm.a",
        "libdl.so", "libdl.so.2", "libdl.a",
        "libpthread.so", "libpthread.so.0", "libpthread.a",
        "librt.so", "librt.so.1", "librt.a",
        "libresolv.so", "libresolv.so.2", "libresolv.a",
        "libutil.so", "libutil.so.1", "libutil.a",
        "libcrypt.so", "libcrypt.so.1", "libcrypt.a",
        "libnsl.so", "libnsl.so.1", "libnsl.a",
        "libz.so", "libz.so.1", "libz.a",
        "libbz2.so", "libbz2.so.1", "libbz2.a",
        
        // Linux system integration
        "libgcc_s.so", "libgcc_s.so.1",
        "libstdc++.so", "libstdc++.so.6", "libstdc++.a",
        "ld-linux.so", "ld-linux.so.2",
        "ld-linux-x86-64.so", "ld-linux-x86-64.so.2",
        "ld-linux-aarch64.so", "ld-linux-aarch64.so.1",
        "ld-linux-armhf.so", "ld-linux-armhf.so.3",
        
        // Linux X11 and graphics
        "libX11.so", "libX11.so.6", "libX11.a",
        "libXext.so", "libXext.so.6", "libXext.a",
        "libXrender.so", "libXrender.so.1", "libXrender.a",
        "libXrandr.so", "libXrandr.so.2", "libXrandr.a",
        "libXinerama.so", "libXinerama.so.1", "libXinerama.a",
        "libXcursor.so", "libXcursor.so.1", "libXcursor.a",
        "libXi.so", "libXi.so.6", "libXi.a",
        "libXxf86vm.so", "libXxf86vm.so.1", "libXxf86vm.a",
        "libGL.so", "libGL.so.1", "libGL.a",
        "libGLU.so", "libGLU.so.1", "libGLU.a",
        "libEGL.so", "libEGL.so.1", "libEGL.a",
        "libGLESv2.so", "libGLESv2.so.2", "libGLESv2.a",
        "libdrm.so", "libdrm.so.2", "libdrm.a",
        "libgbm.so", "libgbm.so.1", "libgbm.a",
        
        // Linux audio
        "libasound.so", "libasound.so.2", "libasound.a",
        "libpulse.so", "libpulse.so.0", "libpulse.a",
        "libpulse-simple.so", "libpulse-simple.so.0", "libpulse-simple.a",
        "libjack.so", "libjack.so.0", "libjack.a",
        
        // Linux network and security
        "libssl.so", "libssl.so.1.1", "libssl.so.3", "libssl.a",
        "libcrypto.so", "libcrypto.so.1.1", "libcrypto.so.3", "libcrypto.a",
        "libcurl.so", "libcurl.so.4", "libcurl.a",
        "libssh2.so", "libssh2.so.1", "libssh2.a",
        
        // Linux desktop integration
        "libgtk-3.so", "libgtk-3.so.0", "libgtk-3.a",
        "libgdk-3.so", "libgdk-3.so.0", "libgdk-3.a",
        "libglib-2.0.so", "libglib-2.0.so.0", "libglib-2.0.a",
        "libgobject-2.0.so", "libgobject-2.0.so.0", "libgobject-2.0.a",
        "libgio-2.0.so", "libgio-2.0.so.0", "libgio-2.0.a",
        "libpango-1.0.so", "libpango-1.0.so.0", "libpango-1.0.a",
        "libcairo.so", "libcairo.so.2", "libcairo.a",
        "libatk-1.0.so", "libatk-1.0.so.0", "libatk-1.0.a",
        "libgdk_pixbuf-2.0.so", "libgdk_pixbuf-2.0.so.0", "libgdk_pixbuf-2.0.a",
        "libfontconfig.so", "libfontconfig.so.1", "libfontconfig.a",
        "libfreetype.so", "libfreetype.so.6", "libfreetype.a",
        "libharfbuzz.so", "libharfbuzz.so.0", "libharfbuzz.a",
        "libpng16.so", "libpng16.so.16", "libpng16.a",
        "libjpeg.so", "libjpeg.so.8", "libjpeg.a",
        
        // macOS/Darwin System Libraries (.dylib for dynamic, .a for static)
        // Core system
        "libSystem.dylib", "libSystem.a",
        "libc.dylib", "libc.a",
        "libm.dylib", "libm.a",
        "libdl.dylib", "libdl.a",
        "libpthread.dylib", "libpthread.a",
        "libresolv.dylib", "libresolv.a",
        "libutil.dylib", "libutil.a",
        "libz.dylib", "libz.a",
        "libbz2.dylib", "libbz2.a",
        "libiconv.dylib", "libiconv.a",
        "libxml2.dylib", "libxml2.a",
        "libxslt.dylib", "libxslt.a",
        "libsqlite3.dylib", "libsqlite3.a",
        "libcurl.dylib", "libcurl.a",
        "libssl.dylib", "libssl.a",
        "libcrypto.dylib", "libcrypto.a",
        
        // macOS frameworks (as dylibs)
        "CoreFoundation.dylib", "CoreFoundation.a",
        "Foundation.dylib", "Foundation.a",
        "AppKit.dylib", "AppKit.a",
        "Cocoa.dylib", "Cocoa.a",
        "Carbon.dylib", "Carbon.a",
        "CoreServices.dylib", "CoreServices.a",
        "CoreGraphics.dylib", "CoreGraphics.a",
        "CoreText.dylib", "CoreText.a",
        "CoreImage.dylib", "CoreImage.a",
        "CoreAnimation.dylib", "CoreAnimation.a",
        "QuartzCore.dylib", "QuartzCore.a",
        "ImageIO.dylib", "ImageIO.a",
        "CoreVideo.dylib", "CoreVideo.a",
        "CoreMedia.dylib", "CoreMedia.a",
        "AVFoundation.dylib", "AVFoundation.a",
        "AudioToolbox.dylib", "AudioToolbox.a",
        "AudioUnit.dylib", "AudioUnit.a",
        "CoreAudio.dylib", "CoreAudio.a",
        "OpenGL.dylib", "OpenGL.a",
        "Metal.dylib", "Metal.a",
        "MetalKit.dylib", "MetalKit.a",
        "IOKit.dylib", "IOKit.a",
        "Security.dylib", "Security.a",
        "SystemConfiguration.dylib", "SystemConfiguration.a",
        "CFNetwork.dylib", "CFNetwork.a",
        "NetworkExtension.dylib", "NetworkExtension.a",
        
        // iOS specific (subset of macOS)
        "UIKit.dylib", "UIKit.a",
        "CoreLocation.dylib", "CoreLocation.a",
        "CoreMotion.dylib", "CoreMotion.a",
        "CoreBluetooth.dylib", "CoreBluetooth.a",
        "GameKit.dylib", "GameKit.a",
        "StoreKit.dylib", "StoreKit.a",
        "MapKit.dylib", "MapKit.a",
        "MessageUI.dylib", "MessageUI.a",
        "AddressBook.dylib", "AddressBook.a",
        "AddressBookUI.dylib", "AddressBookUI.a",
        "EventKit.dylib", "EventKit.a",
        "EventKitUI.dylib", "EventKitUI.a",
        "Photos.dylib", "Photos.a",
        "PhotosUI.dylib", "PhotosUI.a",
        "HealthKit.dylib", "HealthKit.a",
        "WatchKit.dylib", "WatchKit.a",
        "CloudKit.dylib", "CloudKit.a",
        
        // Android System Libraries (.so)
        "libc.so", "libm.so", "libdl.so", "liblog.so",
        "libz.so", "libcutils.so", "libutils.so",
        "libandroid.so", "libandroid_runtime.so",
        "libbinder.so", "libui.so", "libgui.so",
        "libEGL.so", "libGLESv1_CM.so", "libGLESv2.so", "libGLESv3.so",
        "libOpenSLES.so", "libmediandk.so", "libcamera2ndk.so",
        "libjnigraphics.so", "libaaudio.so", "libamidi.so",
        "libneuralnetworks.so", "libvulkan.so",
        
        // FreeBSD System Libraries
        "libc.so", "libc.so.7", "libc.a",
        "libm.so", "libm.so.5", "libm.a",
        "libpthread.so", "libpthread.so.3", "libpthread.a",
        "librt.so", "librt.so.1", "librt.a",
        "libutil.so", "libutil.so.9", "libutil.a",
        "libkvm.so", "libkvm.so.7", "libkvm.a",
        "libprocstat.so", "libprocstat.so.1", "libprocstat.a",
        "libgeom.so", "libgeom.so.1", "libgeom.a",
        "libjail.so", "libjail.so.1", "libjail.a",
        "libdevstat.so", "libdevstat.so.7", "libdevstat.a",
        "libelf.so", "libelf.so.2", "libelf.a",
        "libdwarf.so", "libdwarf.so.3", "libdwarf.a",
        "libexecinfo.so", "libexecinfo.so.1", "libexecinfo.a",
        "libthr.so", "libthr.so.3", "libthr.a",
        "libgcc_s.so", "libgcc_s.so.1",
        "libstdc++.so", "libstdc++.so.6", "libstdc++.a",
        "libcxxrt.so", "libcxxrt.so.1", "libcxxrt.a",
        "libc++.so", "libc++.so.1", "libc++.a",
        
        // OpenBSD System Libraries
        "libc.so", "libc.so.96", "libc.a",
        "libm.so", "libm.so.10", "libm.a",
        "libpthread.so", "libpthread.so.26", "libpthread.a",
        "libutil.so", "libutil.so.13", "libutil.a",
        "libkvm.so", "libkvm.so.18", "libkvm.a",
        "libossaudio.so", "libossaudio.so.7", "libossaudio.a",
        
        // NetBSD System Libraries
        "libc.so", "libc.so.12", "libc.a",
        "libm.so", "libm.so.0", "libm.a",
        "libpthread.so", "libpthread.so.1", "libpthread.a",
        "libutil.so", "libutil.so.7", "libutil.a",
        "libkvm.so", "libkvm.so.7", "libkvm.a",
        "libossaudio.so", "libossaudio.so.1", "libossaudio.a",
        
        // WebAssembly System Libraries (WASI)
        "libc.wasm", "libc.a",
        "libm.wasm", "libm.a",
        "libwasi-emulated-mman.wasm", "libwasi-emulated-mman.a",
        "libwasi-emulated-process-clocks.wasm", "libwasi-emulated-process-clocks.a",
        "libwasi-emulated-getpid.wasm", "libwasi-emulated-getpid.a",
        "libwasi-emulated-signal.wasm", "libwasi-emulated-signal.a",
        "libdl.wasm", "libdl.a",
        "libpthread.wasm", "libpthread.a",
        
        // Emscripten specific
        "libgl.js", "libgl.a",
        "libal.js", "libal.a",
        "libhtml5.js", "libhtml5.a",
        "libembind.js", "libembind.a",
        "libnodefs.js", "libnodefs.a",
        "libproxyfs.js", "libproxyfs.a",
        "libworkerfs.js", "libworkerfs.a",
        "libidbfs.js", "libidbfs.a",
        "libwebgl.js", "libwebgl.a",
        "libwebgpu.js", "libwebgpu.a",
        
        // Solaris/Illumos System Libraries
        "libc.so", "libc.so.1", "libc.a",
        "libm.so", "libm.so.2", "libm.a",
        "libpthread.so", "libpthread.so.1", "libpthread.a",
        "librt.so", "librt.so.1", "librt.a",
        "libsocket.so", "libsocket.so.1", "libsocket.a",
        "libnsl.so", "libnsl.so.1", "libnsl.a",
        "libdl.so", "libdl.so.1", "libdl.a",
        "libkstat.so", "libkstat.so.1", "libkstat.a",
        "libproc.so", "libproc.so.1", "libproc.a",
        "libcontract.so", "libcontract.so.1", "libcontract.a",
        "libscf.so", "libscf.so.1", "libscf.a",
        "libnvpair.so", "libnvpair.so.1", "libnvpair.a",
        "libzfs.so", "libzfs.so.1", "libzfs.a",
        "libzpool.so", "libzpool.so.1", "libzpool.a",
        
        // Haiku System Libraries
        "libroot.so", "libroot.a",
        "libnetwork.so", "libnetwork.a",
        "libbe.so", "libbe.a",
        "libtracker.so", "libtracker.a",
        "libtranslation.so", "libtranslation.a",
        "libmedia.so", "libmedia.a",
        "libgame.so", "libgame.a",
        "libdevice.so", "libdevice.a",
        "libmail.so", "libmail.a",
        "libmidi.so", "libmidi.a",
        "libmidi2.so", "libmidi2.a",
        "libGL.so", "libGL.a",
        "libGLU.so", "libGLU.a"
    };
    
    std::filesystem::path path(libPath);
    std::string filename = path.filename().string();
    
    return systemLibs.find(filename) != systemLibs.end();
}

void LinkDependencies::addRequiredLibrary(const std::string& libName, const LibraryInfo& info) {
    requiredLibraries_.insert(libName);
    libraryInfo_[libName] = info;
}

void LinkDependencies::addLibrarySearchPath(const std::string& path) {
    if (!path.empty()) {
        librarySearchPaths_.insert(path);
    }
}

std::vector<std::string> LinkDependencies::getLinkerFlags() const {
    std::vector<std::string> flags;
    std::set<std::string> libDirs; // Deduplicate -L paths
    static std::map<std::string, std::string> importLibCache; // Cache for found import libs

    // Platform-specific extensions
    #if defined(_WIN32)
    const std::string sharedExt = ".dll";
    const std::vector<std::string> importExts = {".dll.a", ".lib", ".a"};
    const std::string libPrefix = "lib";
    #elif defined(__linux__)
    const std::string sharedExt = ".so";
    const std::vector<std::string> importExts = {".so", ".a"};
    const std::string libPrefix = "lib";
    #elif defined(__APPLE__)
    const std::string sharedExt = ".dylib";
    const std::vector<std::string> importExts = {".dylib", ".a"};
    const std::string libPrefix = "lib";
    #endif

    namespace fs = std::filesystem;

    auto getBaseName = [&](const std::string& libPath) -> std::string {
        std::string filename = fs::path(libPath).filename().string();
        std::string baseName = fs::path(libPath).stem().string();
        
        // Remove lib prefix if present
        if (baseName.rfind(libPrefix, 0) == 0) {
            baseName = baseName.substr(libPrefix.length());
        }
        
        // Remove version numbers (e.g., libname.so.1.2.3 -> libname)
        size_t pos = baseName.find('.');
        if (pos != std::string::npos) {
            baseName = baseName.substr(0, pos);
        }
        
        return baseName;
    };

    auto isSharedLibrary = [&](const std::string& libPath) -> bool {
        return libPath.ends_with(sharedExt) || 
               libPath.find(sharedExt + ".") != std::string::npos; // versioned .so files
    };

    auto findImportLibrary = [&](const std::string& sharedLibPath) -> std::string {
        // Check cache first
        if (importLibCache.find(sharedLibPath) != importLibCache.end()) {
            return importLibCache[sharedLibPath];
        }
        
        std::string libDir = fs::path(sharedLibPath).parent_path().string();
        std::string baseName = getBaseName(sharedLibPath);
        
        // Generate all possible import library names
        std::vector<std::string> candidates;
        for (const auto& ext : importExts) {
            candidates.push_back(libDir + "/" + libPrefix + baseName + ext);
            candidates.push_back(libDir + "/" + baseName + ext);
            candidates.push_back(libDir + "/" + libPrefix + baseName + "dll" + ext);
            candidates.push_back(libDir + "/" + baseName + "dll" + ext);
            
            // Special Windows patterns
            #if defined(_WIN32)
            candidates.push_back(libDir + "/" + libPrefix + baseName + "dll.a");
            candidates.push_back(libDir + "/" + baseName + "dll.a");
            candidates.push_back(libDir + "/" + baseName + ".lib");
            #endif
        }
        
        // Find first existing candidate
        for (const auto& candidate : candidates) {
            if (fs::exists(candidate)) {
                importLibCache[sharedLibPath] = candidate;
                return candidate;
            }
        }
        
        importLibCache[sharedLibPath] = "";
        return "";
    };

    for (const auto& libName : requiredLibraries_) {
        auto it = libraryInfo_.find(libName);
        
        // 1. Use custom linker flags if provided (user override)
        if (it != libraryInfo_.end() && !it->second.linkerFlags.empty()) {
            flags.insert(flags.end(), it->second.linkerFlags.begin(), it->second.linkerFlags.end());
            continue;
        }

        std::string libPath;
        if (it != libraryInfo_.end() && !it->second.path.empty()) {
            libPath = it->second.path;

            // 2. If path is a dynamic library, find its import/static version
            if (isSharedLibrary(libPath)) {
                std::string importLib = findImportLibrary(libPath);
                if (!importLib.empty()) {
                    flags.push_back(importLib);  // Use the import lib (e.g., libglfw3.dll.a)
                    libDirs.insert(fs::path(importLib).parent_path().string());
                    continue;
                } else {
                    // If no import lib found, fall back to -l (e.g., -lKernel32)
                     // 3. No explicit path? Fall back to -l
                    std::string shortName = libName;
                    if (libName.find('/') != std::string::npos || libName.find('\\') != std::string::npos) {
                        std::string filename = fs::path(libName).filename().string();
                        shortName = fs::path(filename).stem().string(); // Remove extension
                        if (shortName.rfind("lib", 0) == 0)
                            shortName = shortName.substr(3); // Strip leading "lib" if present
                    }
                    flags.push_back("-l" + shortName);
                    continue;
                }
            }
        } else {
            // 3. No explicit path? Fall back to -l
            std::string shortName = libName;
            if (libName.find('/') != std::string::npos || libName.find('\\') != std::string::npos) {
                std::string filename = fs::path(libName).filename().string();
                shortName = fs::path(filename).stem().string(); // Remove extension
                if (shortName.rfind("lib", 0) == 0)
                    shortName = shortName.substr(3); // Strip leading "lib" if present
            }
            flags.push_back("-l" + shortName);

            continue;
        }

        // 4. If we get here, it's a static/import lib → link directly
        flags.push_back(libPath);
    }

    // Add unique -L flags
    for (const auto& dir : libDirs) {
        flags.push_back("-L" + dir);
    }

    return flags;
}

bool LinkDependencies::hasLibrary(const std::string& libName) const {
    return requiredLibraries_.find(libName) != requiredLibraries_.end();
}

void LinkDependencies::clear() {
    requiredLibraries_.clear();
    libraryInfo_.clear();
    librarySearchPaths_.clear();
}

std::vector<std::string> LinkDependencies::getRequiredLibraries() const {
    return std::vector<std::string>(requiredLibraries_.begin(), requiredLibraries_.end());
}

const LinkDependencies::LibraryInfo* LinkDependencies::getLibraryInfo(const std::string& libName) const {
    auto it = libraryInfo_.find(libName);
    return (it != libraryInfo_.end()) ? &it->second : nullptr;
}

// Platform detection implementation
PlatformInfo::Platform PlatformInfo::getCurrentPlatform() {
#ifdef _WIN32
    return Windows;
#elif defined(__ANDROID__)
    return Android;
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
        return iOS;
    #else
        return MacOS;
    #endif
#elif defined(__linux__)
    return Linux;
#elif defined(__FreeBSD__)
    return FreeBSD;
#elif defined(__EMSCRIPTEN__)
    return WebAssembly;
#else
    return Unknown;
#endif
}

PlatformInfo::Architecture PlatformInfo::getCurrentArchitecture() {
#if defined(_M_X64) || defined(__x86_64__)
    return x86_64;
#elif defined(_M_IX86) || defined(__i386__)
    return x86;
#elif defined(_M_ARM64) || defined(__aarch64__)
    return ARM64;
#elif defined(_M_ARM) || defined(__arm__)
    return ARM;
#elif defined(__mips__)
    return MIPS;
#elif defined(__riscv)
    return RISC_V;
#elif defined(__EMSCRIPTEN__)
    return WebAsm;
#else
    return UnknownArch;
#endif
}

bool PlatformInfo::isUnixLike() {
    auto platform = getCurrentPlatform();
    return platform == Linux || platform == MacOS || platform == FreeBSD || platform == Android;
}

bool PlatformInfo::isApple() {
    auto platform = getCurrentPlatform();
    return platform == MacOS || platform == iOS;
}

std::string PlatformInfo::getPlatformString() {
    switch (getCurrentPlatform()) {
        case Windows: return "windows";
        case Linux: return "linux";
        case MacOS: return "macos";
        case Android: return "android";
        case iOS: return "ios";
        case FreeBSD: return "freebsd";
        case WebAssembly: return "wasm";
        default: return "unknown";
    }
    return "unknown";
}

std::string PlatformInfo::getArchString() {
    switch (getCurrentArchitecture()) {
        case x86: return "x86";
        case x86_64: return "x86_64";
        case ARM: return "ARM";
        case ARM64: return "ARM64";
        case MIPS: return "MIPS";
        case RISC_V: return "RISC-V";
        case WebAsm: return "WebAssembly";
        default: return "Unknown";
    }
    return "Unknown";
}

}
