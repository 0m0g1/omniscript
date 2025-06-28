# GLAD Build Makefile for Windows (MinGW/MSYS2)
# Builds OpenGL, OpenGL ES 2.0, and Vulkan libraries

# Compiler and tools
CC = gcc
AR = ar
DLLTOOL = dlltool
GENDEF = gendef

# Compiler flags
CFLAGS = -Wall -O2
SHARED_FLAGS = -shared
EXPORT_FLAGS = -DGLAD_API_CALL_EXPORT -DGLAD_API_CALL_EXPORT_BUILD

# Directories
GL_SRC_DIR = gl/src
GL_INC_DIR = gl/include
GL_BIN_DIR = gl/bin
GL_INT_DIR = gl/bin-int

GLES2_SRC_DIR = gles2/src
GLES2_INC_DIR = gles2/include
GLES2_BIN_DIR = gles2/bin
GLES2_INT_DIR = gles2/bin-int

VULKAN_SRC_DIR = vulkan/src
VULKAN_INC_DIR = vulkan/include
VULKAN_BIN_DIR = vulkan/bin
VULKAN_INT_DIR = vulkan/bin-int

# Source files
GL_SOURCES = $(GL_SRC_DIR)/gl.c
GLES2_SOURCES = $(GLES2_SRC_DIR)/gles2.c $(GLES2_SRC_DIR)/egl.c
VULKAN_SOURCES = $(VULKAN_SRC_DIR)/vulkan.c

# Object files
GL_OBJECTS = $(GL_INT_DIR)/gl.o
GLES2_OBJECTS = $(GLES2_INT_DIR)/gles2.o $(GLES2_INT_DIR)/egl.o
VULKAN_OBJECTS = $(VULKAN_INT_DIR)/vulkan.o

# Output files
GL_STATIC_LIB = $(GL_BIN_DIR)/libglad_gl.a
GL_SHARED_LIB = $(GL_BIN_DIR)/glad_gl.dll
GL_IMPORT_LIB = $(GL_BIN_DIR)/glad_gl.lib

GLES2_STATIC_LIB = $(GLES2_BIN_DIR)/libglad_gles2.a
GLES2_SHARED_LIB = $(GLES2_BIN_DIR)/glad_gles2.dll
GLES2_IMPORT_LIB = $(GLES2_BIN_DIR)/glad_gles2.lib

VULKAN_STATIC_LIB = $(VULKAN_BIN_DIR)/libglad_vulkan.a
VULKAN_SHARED_LIB = $(VULKAN_BIN_DIR)/glad_vulkan.dll
VULKAN_IMPORT_LIB = $(VULKAN_BIN_DIR)/glad_vulkan.lib

# Default target
all: glad_gl glad_gles2 glad_vulkan
	@echo All builds complete! Cleaning all temporary files...
	@if exist "$(GL_INT_DIR)" rmdir /s /q "$(GL_INT_DIR)"
	@if exist "$(GLES2_INT_DIR)" rmdir /s /q "$(GLES2_INT_DIR)"
	@if exist "$(VULKAN_INT_DIR)" rmdir /s /q "$(VULKAN_INT_DIR)"

# Create directories
prepare_dirs:
	@if not exist "$(GL_BIN_DIR)" mkdir "$(GL_BIN_DIR)"
	@if not exist "$(GL_INT_DIR)" mkdir "$(GL_INT_DIR)"
	@if not exist "$(GLES2_BIN_DIR)" mkdir "$(GLES2_BIN_DIR)"
	@if not exist "$(GLES2_INT_DIR)" mkdir "$(GLES2_INT_DIR)"
	@if not exist "$(VULKAN_BIN_DIR)" mkdir "$(VULKAN_BIN_DIR)"
	@if not exist "$(VULKAN_INT_DIR)" mkdir "$(VULKAN_INT_DIR)"

# OpenGL targets
glad_gl: prepare_dirs $(GL_STATIC_LIB) $(GL_SHARED_LIB) $(GL_IMPORT_LIB)
	@echo Building GLAD OpenGL... Done!
	@echo Cleaning OpenGL temporary files...
	@if exist "$(GL_INT_DIR)" rmdir /s /q "$(GL_INT_DIR)"

$(GL_INT_DIR)/gl.o: $(GL_SRC_DIR)/gl.c
	@echo Compiling OpenGL object file...
	$(CC) $(CFLAGS) $(EXPORT_FLAGS) -c $< -I$(GL_INC_DIR) -o $@

$(GL_STATIC_LIB): $(GL_OBJECTS)
	@echo Creating OpenGL static library...
	$(AR) rcs $@ $^

$(GL_SHARED_LIB): $(GL_OBJECTS)
	@echo Creating OpenGL shared library...
	$(CC) $(SHARED_FLAGS) $(EXPORT_FLAGS) -o $@ $^

$(GL_IMPORT_LIB): $(GL_SHARED_LIB)
	@echo Creating OpenGL import library...
	$(GENDEF) $< > $(GL_BIN_DIR)/glad_gl.def
	$(DLLTOOL) -d $(GL_BIN_DIR)/glad_gl.def -l $@

# OpenGL ES 2.0 targets
glad_gles2: prepare_dirs $(GLES2_STATIC_LIB) $(GLES2_SHARED_LIB) $(GLES2_IMPORT_LIB)
	@echo Building GLAD OpenGL ES 2.0... Done!
	@echo Cleaning OpenGL ES 2.0 temporary files...
	@if exist "$(GLES2_INT_DIR)" rmdir /s /q "$(GLES2_INT_DIR)"

$(GLES2_INT_DIR)/gles2.o: $(GLES2_SRC_DIR)/gles2.c
	@echo Compiling OpenGL ES 2.0 object file...
	$(CC) $(CFLAGS) $(EXPORT_FLAGS) -c $< -I$(GLES2_INC_DIR) -o $@

$(GLES2_INT_DIR)/egl.o: $(GLES2_SRC_DIR)/egl.c
	@echo Compiling EGL object file...
	$(CC) $(CFLAGS) $(EXPORT_FLAGS) -c $< -I$(GLES2_INC_DIR) -o $@

$(GLES2_STATIC_LIB): $(GLES2_OBJECTS)
	@echo Creating OpenGL ES 2.0 static library...
	$(AR) rcs $@ $^

$(GLES2_SHARED_LIB): $(GLES2_OBJECTS)
	@echo Creating OpenGL ES 2.0 shared library...
	$(CC) $(SHARED_FLAGS) $(EXPORT_FLAGS) -o $@ $^

$(GLES2_IMPORT_LIB): $(GLES2_SHARED_LIB)
	@echo Creating OpenGL ES 2.0 import library...
	$(GENDEF) $< > $(GLES2_BIN_DIR)/glad_gles2.def
	$(DLLTOOL) -d $(GLES2_BIN_DIR)/glad_gles2.def -l $@

# Vulkan targets
glad_vulkan: prepare_dirs $(VULKAN_STATIC_LIB) $(VULKAN_SHARED_LIB) $(VULKAN_IMPORT_LIB)
	@echo Building GLAD Vulkan... Done!
	@echo Cleaning Vulkan temporary files...
	@if exist "$(VULKAN_INT_DIR)" rmdir /s /q "$(VULKAN_INT_DIR)"

$(VULKAN_INT_DIR)/vulkan.o: $(VULKAN_SRC_DIR)/vulkan.c
	@echo Compiling Vulkan object file...
	$(CC) $(CFLAGS) $(EXPORT_FLAGS) -c $< -I$(VULKAN_INC_DIR) -o $@

$(VULKAN_STATIC_LIB): $(VULKAN_OBJECTS)
	@echo Creating Vulkan static library...
	$(AR) rcs $@ $^

$(VULKAN_SHARED_LIB): $(VULKAN_OBJECTS)
	@echo Creating Vulkan shared library...
	$(CC) $(SHARED_FLAGS) $(EXPORT_FLAGS) -o $@ $^

$(VULKAN_IMPORT_LIB): $(VULKAN_SHARED_LIB)
	@echo Creating Vulkan import library...
	$(GENDEF) $< > $(VULKAN_BIN_DIR)/glad_vulkan.def
	$(DLLTOOL) -d $(VULKAN_BIN_DIR)/glad_vulkan.def -l $@

# Clean targets
clean:
	@echo Cleaning temporary files...
	@if exist "$(GL_INT_DIR)" rmdir /s /q "$(GL_INT_DIR)"
	@if exist "$(GLES2_INT_DIR)" rmdir /s /q "$(GLES2_INT_DIR)"
	@if exist "$(VULKAN_INT_DIR)" rmdir /s /q "$(VULKAN_INT_DIR)"

clean_all: clean
	@echo Cleaning all build files...
	@if exist "$(GL_BIN_DIR)" rmdir /s /q "$(GL_BIN_DIR)"
	@if exist "$(GLES2_BIN_DIR)" rmdir /s /q "$(GLES2_BIN_DIR)"
	@if exist "$(VULKAN_BIN_DIR)" rmdir /s /q "$(VULKAN_BIN_DIR)"

# Individual clean targets
clean_gl:
	@if exist "$(GL_INT_DIR)" rmdir /s /q "$(GL_INT_DIR)"
	@if exist "$(GL_BIN_DIR)" rmdir /s /q "$(GL_BIN_DIR)"

clean_gles2:
	@if exist "$(GLES2_INT_DIR)" rmdir /s /q "$(GLES2_INT_DIR)"
	@if exist "$(GLES2_BIN_DIR)" rmdir /s /q "$(GLES2_BIN_DIR)"

clean_vulkan:
	@if exist "$(VULKAN_INT_DIR)" rmdir /s /q "$(VULKAN_INT_DIR)"
	@if exist "$(VULKAN_BIN_DIR)" rmdir /s /q "$(VULKAN_BIN_DIR)"

# Help target
help:
	@echo Available targets:
	@echo   all          - Build all libraries (default)
	@echo   glad_gl      - Build OpenGL library only
	@echo   glad_gles2   - Build OpenGL ES 2.0 library only
	@echo   glad_vulkan  - Build Vulkan library only
	@echo   clean        - Clean temporary files
	@echo   clean_all    - Clean all build files
	@echo   clean_gl     - Clean OpenGL build files
	@echo   clean_gles2  - Clean OpenGL ES 2.0 build files
	@echo   clean_vulkan - Clean Vulkan build files
	@echo   help         - Show this help message

.PHONY: all prepare_dirs glad_gl glad_gles2 glad_vulkan clean clean_all clean_gl clean_gles2 clean_vulkan help