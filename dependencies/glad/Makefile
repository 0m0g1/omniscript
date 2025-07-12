# Platform-specific configuration
ifeq ($(TARGET_OS),Windows)
    STATIC_EXT := .a
    SHARED_EXT := .dll
    IMPORT_EXT := .lib
    STATIC_PREFIX := lib
    SHARED_PREFIX := 
    IMPORT_PREFIX := 
    EXPORT_FLAGS := -DGLAD_API_CALL_EXPORT -DGLAD_API_CALL_EXPORT_BUILD
    SHARED_FLAGS := -shared -Wl,--out-implib,
else ifeq ($(TARGET_OS),macOS)
    STATIC_EXT := .a
    SHARED_EXT := .dylib
    IMPORT_EXT := 
    STATIC_PREFIX := lib
    SHARED_PREFIX := lib
    IMPORT_PREFIX := 
    EXPORT_FLAGS := -DGLAD_API_CALL_EXPORT -DGLAD_API_CALL_EXPORT_BUILD
    SHARED_FLAGS := -dynamiclib -install_name @rpath/
else ifeq ($(TARGET_OS),Linux)
    STATIC_EXT := .a
    SHARED_EXT := .so
    IMPORT_EXT := 
    STATIC_PREFIX := lib
    SHARED_PREFIX := lib
    IMPORT_PREFIX := 
    EXPORT_FLAGS := -DGLAD_API_CALL_EXPORT -DGLAD_API_CALL_EXPORT_BUILD -fPIC
    SHARED_FLAGS := -shared -fPIC
endif# GLAD Cross-Platform Build Makefile
# Builds OpenGL, OpenGL ES 2.0, and Vulkan libraries for Windows, macOS, and Linux

# OS Detection
ifeq ($(OS),Windows_NT)
    DETECTED_OS := Windows
else
    UNAME_S := $(shell uname -s 2>/dev/null || echo Unknown)
    ifeq ($(UNAME_S),Linux)
        DETECTED_OS := Linux
    endif
    ifeq ($(UNAME_S),Darwin)
        DETECTED_OS := macOS
    endif
    ifndef DETECTED_OS
        DETECTED_OS := Linux
    endif
endif

# Allow OS override
ifndef TARGET_OS
    TARGET_OS := $(DETECTED_OS)
endif

# Cross-compilation toolchain configuration
ifeq ($(TARGET_OS),Linux)
    ifeq ($(DETECTED_OS),Windows)
        # Cross-compilation from Windows to Linux using MinGW-w64
        CC := x86_64-linux-gnu-gcc
        AR := x86_64-linux-gnu-ar
        STRIP := x86_64-linux-gnu-strip
        CROSS_COMPILE := 1
    else
        CC := gcc
        AR := ar
        STRIP := strip
    endif
else ifeq ($(TARGET_OS),macOS)
    ifeq ($(DETECTED_OS),Windows)
        # Cross-compilation from Windows to macOS (requires OSXCross)
        CC := x86_64-apple-darwin20-clang
        AR := x86_64-apple-darwin20-ar
        STRIP := x86_64-apple-darwin20-strip
        CROSS_COMPILE := 1
    else
        CC := clang
        AR := ar
        STRIP := strip
    endif
else ifeq ($(TARGET_OS),Windows)
    ifeq ($(DETECTED_OS),Windows)
        # Native Windows compilation
        CC := gcc
        AR := ar
        STRIP := strip
        DLLTOOL := dlltool
        GENDEF := gendef
    else
        # Cross-compilation to Windows from Unix-like systems
        CC := x86_64-w64-mingw32-gcc
        AR := x86_64-w64-mingw32-ar
        STRIP := x86_64-w64-mingw32-strip
        DLLTOOL := x86_64-w64-mingw32-dlltool
        GENDEF := x86_64-w64-mingw32-gendef
        CROSS_COMPILE := 1
    endif
endif

# Compiler flags
CFLAGS := -Wall -O2
DEBUG_FLAGS := -g -DDEBUG
RELEASE_FLAGS := -DNDEBUG

# Build configuration
ifndef CONFIG
    CONFIG := Release
endif

ifeq ($(CONFIG),Debug)
    CFLAGS += $(DEBUG_FLAGS)
    CONFIG_SUFFIX := _debug
else
    CFLAGS += $(RELEASE_FLAGS)
    CONFIG_SUFFIX := 
endif

# Architecture detection
ifndef ARCH
    ifeq ($(TARGET_OS),Windows)
        ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
            ARCH := x86_64
        else ifeq ($(PROCESSOR_ARCHITECTURE),x86)
            ARCH := i386
        else ifeq ($(PROCESSOR_ARCHITECTURE),ARM64)
            ARCH := arm64
        else
            ARCH := x86_64
        endif
    else
        ARCH := x86_64
    endif
endif

# Normalize architecture names
ifeq ($(ARCH),x86_64)
    ARCH_SUFFIX := _x64
else ifeq ($(ARCH),amd64)
    ARCH_SUFFIX := _x64
else ifeq ($(ARCH),i386)
    ARCH_SUFFIX := _x86
else ifeq ($(ARCH),i686)
    ARCH_SUFFIX := _x86
else ifeq ($(ARCH),arm64)
    ARCH_SUFFIX := _arm64
else ifeq ($(ARCH),aarch64)
    ARCH_SUFFIX := _arm64
else
    ARCH_SUFFIX := _$(ARCH)
endif

# Directory structure
BASE_DIR := .
ifeq ($(DETECTED_OS),Windows)
    BUILD_DIR := build\$(TARGET_OS)$(ARCH_SUFFIX)$(CONFIG_SUFFIX)
    OBJ_DIR := $(BUILD_DIR)\obj
    LIB_DIR := $(BUILD_DIR)\lib
    BIN_DIR := $(BUILD_DIR)\bin
    
    # API-specific directories
    GL_SRC_DIR := gl\src
    GL_INC_DIR := gl\include
    GL_OBJ_DIR := $(OBJ_DIR)\gl
    
    GLES2_SRC_DIR := gles2\src
    GLES2_INC_DIR := gles2\include
    GLES2_OBJ_DIR := $(OBJ_DIR)\gles2
    
    VULKAN_SRC_DIR := vulkan\src
    VULKAN_INC_DIR := vulkan\include
    VULKAN_OBJ_DIR := $(OBJ_DIR)\vulkan
    
    # Source files
    GL_SOURCES := $(GL_SRC_DIR)\gl.c
    GLES2_SOURCES := $(GLES2_SRC_DIR)\gles2.c $(GLES2_SRC_DIR)\egl.c
    VULKAN_SOURCES := $(VULKAN_SRC_DIR)\vulkan.c
    
    # Object files
    GL_OBJECTS := $(GL_OBJ_DIR)\gl.o
    GLES2_OBJECTS := $(GLES2_OBJ_DIR)\gles2.o $(GLES2_OBJ_DIR)\egl.o
    VULKAN_OBJECTS := $(VULKAN_OBJ_DIR)\vulkan.o
    
    # Library names
    GL_STATIC_LIB := $(LIB_DIR)\$(STATIC_PREFIX)glad_gl$(STATIC_EXT)
    GL_SHARED_LIB := $(LIB_DIR)\$(SHARED_PREFIX)glad_gl$(SHARED_EXT)
    
    GLES2_STATIC_LIB := $(LIB_DIR)\$(STATIC_PREFIX)glad_gles2$(STATIC_EXT)
    GLES2_SHARED_LIB := $(LIB_DIR)\$(SHARED_PREFIX)glad_gles2$(SHARED_EXT)
    
    VULKAN_STATIC_LIB := $(LIB_DIR)\$(STATIC_PREFIX)glad_vulkan$(STATIC_EXT)
    VULKAN_SHARED_LIB := $(LIB_DIR)\$(SHARED_PREFIX)glad_vulkan$(SHARED_EXT)
else
    BUILD_DIR := build/$(TARGET_OS)$(ARCH_SUFFIX)$(CONFIG_SUFFIX)
    OBJ_DIR := $(BUILD_DIR)/obj
    LIB_DIR := $(BUILD_DIR)/lib
    BIN_DIR := $(BUILD_DIR)/bin
    
    # API-specific directories
    GL_SRC_DIR := gl/src
    GL_INC_DIR := gl/include
    GL_OBJ_DIR := $(OBJ_DIR)/gl
    
    GLES2_SRC_DIR := gles2/src
    GLES2_INC_DIR := gles2/include
    GLES2_OBJ_DIR := $(OBJ_DIR)/gles2
    
    VULKAN_SRC_DIR := vulkan/src
    VULKAN_INC_DIR := vulkan/include
    VULKAN_OBJ_DIR := $(OBJ_DIR)/vulkan
    
    # Source files
    GL_SOURCES := $(GL_SRC_DIR)/gl.c
    GLES2_SOURCES := $(GLES2_SRC_DIR)/gles2.c $(GLES2_SRC_DIR)/egl.c
    VULKAN_SOURCES := $(VULKAN_SRC_DIR)/vulkan.c
    
    # Object files
    GL_OBJECTS := $(GL_OBJ_DIR)/gl.o
    GLES2_OBJECTS := $(GLES2_OBJ_DIR)/gles2.o $(GLES2_OBJ_DIR)/egl.o
    VULKAN_OBJECTS := $(VULKAN_OBJ_DIR)/vulkan.o
    
    # Library names
    GL_STATIC_LIB := $(LIB_DIR)/$(STATIC_PREFIX)glad_gl$(STATIC_EXT)
    GL_SHARED_LIB := $(LIB_DIR)/$(SHARED_PREFIX)glad_gl$(SHARED_EXT)
    
    GLES2_STATIC_LIB := $(LIB_DIR)/$(STATIC_PREFIX)glad_gles2$(STATIC_EXT)
    GLES2_SHARED_LIB := $(LIB_DIR)/$(SHARED_PREFIX)glad_gles2$(SHARED_EXT)
    
    VULKAN_STATIC_LIB := $(LIB_DIR)/$(STATIC_PREFIX)glad_vulkan$(STATIC_EXT)
    VULKAN_SHARED_LIB := $(LIB_DIR)/$(SHARED_PREFIX)glad_vulkan$(SHARED_EXT)
endif

# Windows-specific import libraries
ifeq ($(TARGET_OS),Windows)
    ifeq ($(DETECTED_OS),Windows)
        GL_IMPORT_LIB := $(LIB_DIR)\$(IMPORT_PREFIX)glad_gl$(IMPORT_EXT)
        GLES2_IMPORT_LIB := $(LIB_DIR)\$(IMPORT_PREFIX)glad_gles2$(IMPORT_EXT)
        VULKAN_IMPORT_LIB := $(LIB_DIR)\$(IMPORT_PREFIX)glad_vulkan$(IMPORT_EXT)
    else
        GL_IMPORT_LIB := $(LIB_DIR)/$(IMPORT_PREFIX)glad_gl$(IMPORT_EXT)
        GLES2_IMPORT_LIB := $(LIB_DIR)/$(IMPORT_PREFIX)glad_gles2$(IMPORT_EXT)
        VULKAN_IMPORT_LIB := $(LIB_DIR)/$(IMPORT_PREFIX)glad_vulkan$(IMPORT_EXT)
    endif
    GL_TARGETS := $(GL_STATIC_LIB) $(GL_SHARED_LIB) $(GL_IMPORT_LIB)
    GLES2_TARGETS := $(GLES2_STATIC_LIB) $(GLES2_SHARED_LIB) $(GLES2_IMPORT_LIB)
    VULKAN_TARGETS := $(VULKAN_STATIC_LIB) $(VULKAN_SHARED_LIB) $(VULKAN_IMPORT_LIB)
else
    GL_TARGETS := $(GL_STATIC_LIB) $(GL_SHARED_LIB)
    GLES2_TARGETS := $(GLES2_STATIC_LIB) $(GLES2_SHARED_LIB)
    VULKAN_TARGETS := $(VULKAN_STATIC_LIB) $(VULKAN_SHARED_LIB)
endif

# Default target
all: info glad_gl glad_gles2 glad_vulkan
	@echo "Build complete for $(TARGET_OS) $(ARCH) $(CONFIG)"

# Show build information
info:
	@echo "=== GLAD Cross-Platform Build ==="
	@echo "Host OS: $(DETECTED_OS)"
	@echo "Target OS: $(TARGET_OS)"
	@echo "Architecture: $(ARCH)"
	@echo "Configuration: $(CONFIG)"
	@echo "Compiler: $(CC)"
	@echo "Build Directory: $(BUILD_DIR)"
	@echo "================================="

# Create directories
prepare_dirs:
ifeq ($(DETECTED_OS),Windows)
	@if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
	@if not exist "$(OBJ_DIR)" mkdir "$(OBJ_DIR)"
	@if not exist "$(LIB_DIR)" mkdir "$(LIB_DIR)"
	@if not exist "$(BIN_DIR)" mkdir "$(BIN_DIR)"
	@if not exist "$(GL_OBJ_DIR)" mkdir "$(GL_OBJ_DIR)"
	@if not exist "$(GLES2_OBJ_DIR)" mkdir "$(GLES2_OBJ_DIR)"
	@if not exist "$(VULKAN_OBJ_DIR)" mkdir "$(VULKAN_OBJ_DIR)"
else
	@mkdir -p $(BUILD_DIR) $(OBJ_DIR) $(LIB_DIR) $(BIN_DIR)
	@mkdir -p $(GL_OBJ_DIR) $(GLES2_OBJ_DIR) $(VULKAN_OBJ_DIR)
endif

# OpenGL targets
glad_gl: prepare_dirs $(GL_TARGETS)
	@echo "OpenGL libraries built successfully"

ifeq ($(DETECTED_OS),Windows)
$(GL_OBJ_DIR)\gl.o: $(GL_SRC_DIR)\gl.c
	@echo "Compiling OpenGL source..."
	$(CC) $(CFLAGS) $(EXPORT_FLAGS) -c $< -I$(GL_INC_DIR) -o $@
else
$(GL_OBJ_DIR)/gl.o: $(GL_SRC_DIR)/gl.c
	@echo "Compiling OpenGL source..."
	$(CC) $(CFLAGS) $(EXPORT_FLAGS) -c $< -I$(GL_INC_DIR) -o $@
endif

$(GL_STATIC_LIB): $(GL_OBJECTS)
	@echo "Creating OpenGL static library..."
	$(AR) rcs $@ $^

ifeq ($(TARGET_OS),Windows)
$(GL_SHARED_LIB): $(GL_OBJECTS)
	@echo "Creating OpenGL shared library..."
	$(CC) $(SHARED_FLAGS)$(GL_IMPORT_LIB) -o $@ $^

$(GL_IMPORT_LIB): $(GL_SHARED_LIB)
	@echo "OpenGL import library created"
else ifeq ($(TARGET_OS),macOS)
$(GL_SHARED_LIB): $(GL_OBJECTS)
	@echo "Creating OpenGL shared library..."
	$(CC) $(SHARED_FLAGS)$(notdir $@) -o $@ $^
else
$(GL_SHARED_LIB): $(GL_OBJECTS)
	@echo "Creating OpenGL shared library..."
	$(CC) $(SHARED_FLAGS) -o $@ $^
endif

# OpenGL ES 2.0 targets
glad_gles2: prepare_dirs $(GLES2_TARGETS)
	@echo "OpenGL ES 2.0 libraries built successfully"

ifeq ($(DETECTED_OS),Windows)
$(GLES2_OBJ_DIR)\gles2.o: $(GLES2_SRC_DIR)\gles2.c
	@echo "Compiling OpenGL ES 2.0 source..."
	$(CC) $(CFLAGS) $(EXPORT_FLAGS) -c $< -I$(GLES2_INC_DIR) -o $@

$(GLES2_OBJ_DIR)\egl.o: $(GLES2_SRC_DIR)\egl.c
	@echo "Compiling EGL source..."
	$(CC) $(CFLAGS) $(EXPORT_FLAGS) -c $< -I$(GLES2_INC_DIR) -o $@
else
$(GLES2_OBJ_DIR)/gles2.o: $(GLES2_SRC_DIR)/gles2.c
	@echo "Compiling OpenGL ES 2.0 source..."
	$(CC) $(CFLAGS) $(EXPORT_FLAGS) -c $< -I$(GLES2_INC_DIR) -o $@

$(GLES2_OBJ_DIR)/egl.o: $(GLES2_SRC_DIR)/egl.c
	@echo "Compiling EGL source..."
	$(CC) $(CFLAGS) $(EXPORT_FLAGS) -c $< -I$(GLES2_INC_DIR) -o $@
endif

$(GLES2_STATIC_LIB): $(GLES2_OBJECTS)
	@echo "Creating OpenGL ES 2.0 static library..."
	$(AR) rcs $@ $^

ifeq ($(TARGET_OS),Windows)
$(GLES2_SHARED_LIB): $(GLES2_OBJECTS)
	@echo "Creating OpenGL ES 2.0 shared library..."
	$(CC) $(SHARED_FLAGS)$(GLES2_IMPORT_LIB) -o $@ $^

$(GLES2_IMPORT_LIB): $(GLES2_SHARED_LIB)
	@echo "OpenGL ES 2.0 import library created"
else ifeq ($(TARGET_OS),macOS)
$(GLES2_SHARED_LIB): $(GLES2_OBJECTS)
	@echo "Creating OpenGL ES 2.0 shared library..."
	$(CC) $(SHARED_FLAGS)$(notdir $@) -o $@ $^
else
$(GLES2_SHARED_LIB): $(GLES2_OBJECTS)
	@echo "Creating OpenGL ES 2.0 shared library..."
	$(CC) $(SHARED_FLAGS) -o $@ $^
endif

# Vulkan targets
glad_vulkan: prepare_dirs $(VULKAN_TARGETS)
	@echo "Vulkan libraries built successfully"

ifeq ($(DETECTED_OS),Windows)
$(VULKAN_OBJ_DIR)\vulkan.o: $(VULKAN_SRC_DIR)\vulkan.c
	@echo "Compiling Vulkan source..."
	$(CC) $(CFLAGS) $(EXPORT_FLAGS) -c $< -I$(VULKAN_INC_DIR) -o $@
else
$(VULKAN_OBJ_DIR)/vulkan.o: $(VULKAN_SRC_DIR)/vulkan.c
	@echo "Compiling Vulkan source..."
	$(CC) $(CFLAGS) $(EXPORT_FLAGS) -c $< -I$(VULKAN_INC_DIR) -o $@
endif

$(VULKAN_STATIC_LIB): $(VULKAN_OBJECTS)
	@echo "Creating Vulkan static library..."
	$(AR) rcs $@ $^

ifeq ($(TARGET_OS),Windows)
$(VULKAN_SHARED_LIB): $(VULKAN_OBJECTS)
	@echo "Creating Vulkan shared library..."
	$(CC) $(SHARED_FLAGS)$(VULKAN_IMPORT_LIB) -o $@ $^

$(VULKAN_IMPORT_LIB): $(VULKAN_SHARED_LIB)
	@echo "Vulkan import library created"
else ifeq ($(TARGET_OS),macOS)
$(VULKAN_SHARED_LIB): $(VULKAN_OBJECTS)
	@echo "Creating Vulkan shared library..."
	$(CC) $(SHARED_FLAGS)$(notdir $@) -o $@ $^
else
$(VULKAN_SHARED_LIB): $(VULKAN_OBJECTS)
	@echo "Creating Vulkan shared library..."
	$(CC) $(SHARED_FLAGS) -o $@ $^
endif

# Platform-specific build targets
windows: TARGET_OS := Windows
windows: all

macos: TARGET_OS := macOS
macos: all

linux: TARGET_OS := Linux
linux: all

# Cross-compilation targets with toolchain checks
cross_linux: 
	@echo "Cross-compiling for Linux..."
	@where x86_64-linux-gnu-gcc >nul 2>&1 || (echo "Error: Linux cross-compiler not found!" && echo "Install Ubuntu/Debian cross-compilation tools" && exit 1)
	@$(MAKE) TARGET_OS=Linux all

cross_macos:
	@echo "Cross-compiling for macOS..."
	@where x86_64-apple-darwin20-clang >nul 2>&1 || (echo "Error: OSXCross toolchain not found!" && echo "Please install OSXCross: https://github.com/tpoechtrager/osxcross" && exit 1)
	@$(MAKE) TARGET_OS=macOS all

cross_windows:
	@echo "Cross-compiling for Windows..."
	@where x86_64-w64-mingw32-gcc >nul 2>&1 || (echo "Error: MinGW-w64 cross-compiler not found!" && echo "Install MinGW-w64 toolchain" && exit 1)
	@$(MAKE) TARGET_OS=Windows all

# Check available toolchains
check_toolchains:
	@echo "Checking available cross-compilation toolchains..."
	@echo "Current OS: $(DETECTED_OS)"
	@echo ""
	@echo "Windows (native/cross):"
	@where gcc >nul 2>&1 && echo "  ✓ GCC found" || echo "  ✗ GCC not found"
	@where x86_64-w64-mingw32-gcc >nul 2>&1 && echo "  ✓ MinGW-w64 found" || echo "  ✗ MinGW-w64 not found"
	@echo ""
	@echo "Linux cross-compilation:"
	@where x86_64-linux-gnu-gcc >nul 2>&1 && echo "  ✓ Linux GCC found" || echo "  ✗ Linux GCC not found"
	@echo ""
	@echo "macOS cross-compilation:"
	@where x86_64-apple-darwin20-clang >nul 2>&1 && echo "  ✓ OSXCross found" || echo "  ✗ OSXCross not found"
	@echo ""
	@echo "Installation instructions:"
	@echo "  MinGW-w64: Install via MSYS2: pacman -S mingw-w64-x86_64-toolchain"
	@echo "  Linux GCC: Install via package manager or build from source"
	@echo "  OSXCross:  Build from https://github.com/tpoechtrager/osxcross"

# Install required toolchains on Windows
install_toolchains:
	@echo "Installing cross-compilation toolchains..."
	@echo "This requires MSYS2 or similar package manager"
	@echo ""
	@echo "For MSYS2 (run in MSYS2 terminal):"
	@echo "  pacman -S mingw-w64-x86_64-toolchain"
	@echo "  pacman -S mingw-w64-x86_64-gcc"
	@echo "  pacman -S mingw-w64-x86_64-binutils"
	@echo ""
	@echo "For Linux cross-compilation, you may need to build your own toolchain"
	@echo "or use Docker containers with the appropriate tools."

# Multi-platform build with proper cross-compilation
build_all_platforms: check_toolchains
	@echo "Building for all available platforms..."
	@echo ""
	@echo "Building Windows..."
	@$(MAKE) TARGET_OS=Windows all
	@echo ""
	@echo "Checking for Linux cross-compilation..."
	@where x86_64-linux-gnu-gcc >nul 2>&1 && (echo "Building Linux..." && $(MAKE) TARGET_OS=Linux all) || echo "Skipping Linux - cross-compiler not available"
	@echo ""
	@echo "Checking for macOS cross-compilation..."
	@where x86_64-apple-darwin20-clang >nul 2>&1 && (echo "Building macOS..." && $(MAKE) TARGET_OS=macOS all) || echo "Skipping macOS - OSXCross not available"
	@echo ""
	@echo "Multi-platform build completed!"

# Force build all platforms (will fail if toolchains missing)
build_all_force:
	@echo "Force building for all platforms..."
	@$(MAKE) TARGET_OS=Windows all
	@$(MAKE) TARGET_OS=Linux all
	@$(MAKE) TARGET_OS=macOS all
	@echo "All platforms built successfully!"

# Docker-based cross-compilation (alternative approach)
docker_build_linux:
	@echo "Building Linux binaries using Docker..."
	@docker run --rm -v $(shell pwd):/src -w /src ubuntu:20.04 bash -c "apt-get update && apt-get install -y build-essential && make TARGET_OS=Linux all"

docker_build_macos:
	@echo "Building macOS binaries using Docker..."
	@echo "This requires a Docker image with OSXCross pre-installed"
	@docker run --rm -v $(shell pwd):/src -w /src osxcross:latest make TARGET_OS=macOS all

# Debug builds
debug: CONFIG := Debug
debug: all

debug_all: CONFIG := Debug
debug_all: build_all_platforms

# Clean targets
clean:
	@echo "Cleaning build files for $(TARGET_OS)..."
ifeq ($(DETECTED_OS),Windows)
	@if exist "$(BUILD_DIR)" rmdir /s /q "$(BUILD_DIR)" 2>nul || echo "Build directory already clean"
else
	@if [ -d "$(BUILD_DIR)" ]; then rm -rf "$(BUILD_DIR)"; fi
endif

clean_all:
	@echo "Cleaning all build files..."
ifeq ($(DETECTED_OS),Windows)
	@if exist "build" rmdir /s /q "build" 2>nul || echo "Build directory already clean"
else
	@if [ -d "build" ]; then rm -rf "build"; fi
endif

# Create distribution package
dist: build_all_platforms
	@echo "Creating distribution package..."
ifeq ($(DETECTED_OS),Windows)
	@if not exist "dist" mkdir "dist"
	@xcopy build dist\ /E /I /H /Y
else
	@mkdir -p dist
	@cp -r build/* dist/
endif
	@echo "Distribution package created in dist/"

# Help target
help:
	@echo "GLAD Cross-Platform Build System"
	@echo "================================="
	@echo "Usage: make [target] [OPTIONS]"
	@echo ""
	@echo "Main Targets:"
	@echo "  all                    - Build all libraries for detected OS"
	@echo "  build_all_platforms    - Build for all available platforms"
	@echo "  build_all_force        - Force build all platforms (fails if missing tools)"
	@echo "  windows                - Build for Windows"
	@echo "  linux                  - Build for Linux"  
	@echo "  macos                  - Build for macOS"
	@echo ""
	@echo "Cross-compilation Targets:"
	@echo "  cross_linux            - Cross-compile for Linux"
	@echo "  cross_macos            - Cross-compile for macOS"
	@echo "  cross_windows          - Cross-compile for Windows"
	@echo ""
	@echo "Individual API Targets:"
	@echo "  glad_gl                - Build OpenGL library only"
	@echo "  glad_gles2             - Build OpenGL ES 2.0 library only"
	@echo "  glad_vulkan            - Build Vulkan library only"
	@echo ""
	@echo "Utility Targets:"
	@echo "  check_toolchains       - Check available cross-compilation tools"
	@echo "  install_toolchains     - Show toolchain installation instructions"
	@echo "  docker_build_linux     - Build Linux using Docker"
	@echo "  docker_build_macos     - Build macOS using Docker"
	@echo "  debug                  - Build debug version"
	@echo "  debug_all              - Build debug version for all platforms"
	@echo "  clean                  - Clean current build files"
	@echo "  clean_all              - Clean all build files"
	@echo "  dist                   - Create distribution package"
	@echo "  help                   - Show this help message"
	@echo ""
	@echo "Options:"
	@echo "  TARGET_OS=[Windows|macOS|Linux] - Override target OS"
	@echo "  CONFIG=[Debug|Release]          - Build configuration"
	@echo "  ARCH=[x86_64|i386|arm64|...]    - Target architecture"
	@echo "  CC=[gcc|clang|...]              - Compiler to use"
	@echo ""
	@echo "Examples:"
	@echo "  make build_all_platforms        - Build for all available platforms"
	@echo "  make TARGET_OS=Linux            - Cross-compile for Linux"
	@echo "  make CONFIG=Debug linux         - Build debug version for Linux"
	@echo "  make check_toolchains           - Check what tools are available"

.PHONY: all info prepare_dirs glad_gl glad_gles2 glad_vulkan windows macos linux \
        cross_linux cross_macos cross_windows build_all_platforms build_all_force \
        docker_build_linux docker_build_macos debug debug_all clean clean_all dist \
        check_toolchains install_toolchains help