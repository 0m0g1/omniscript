# Alternative GNU Make project makefile autogenerated by Premake

ifndef config
  config=debug
endif

ifndef verbose
  SILENT = @
endif

.PHONY: clean prebuild

SHELLTYPE := posix
ifeq ($(shell echo "test"), "test")
	SHELLTYPE := msdos
endif

# Configurations
# #############################################

ifeq ($(origin CC), default)
  CC = gcc
endif
ifeq ($(origin CXX), default)
  CXX = g++
endif
ifeq ($(origin AR), default)
  AR = ar
endif
RESCOMP = windres
INCLUDES += -Iinclude -Idependencies/llvm/include
FORCE_INCLUDE +=
ALL_CPPFLAGS += $(CPPFLAGS) -MD -MP $(DEFINES) $(INCLUDES)
ALL_RESFLAGS += $(RESFLAGS) $(DEFINES) $(INCLUDES)
LIBS += -lLLVM-19 -lpthread
LDDEPS +=
LINKCMD = $(CXX) -o "$@" $(OBJECTS) $(RESOURCES) $(ALL_LDFLAGS) $(LIBS)
define PREBUILDCMDS
endef
define PRELINKCMDS
endef
define POSTBUILDCMDS
endef

ifeq ($(config),debug)
TARGETDIR = bin/Debug-windows-x86_64
TARGET = $(TARGETDIR)/Osengine.exe
OBJDIR = bin-int/Debug-windows-x86_64
DEFINES += -DDEBUG -DPLATFORM_WINDOWS
ALL_CFLAGS += $(CFLAGS) $(ALL_CPPFLAGS) -m64 -g
ALL_CXXFLAGS += $(CXXFLAGS) $(ALL_CPPFLAGS) -m64 -g -std=c++23
ALL_LDFLAGS += $(LDFLAGS) -Ldependencies/llvm/lib -L/usr/lib64 -m64

else ifeq ($(config),release)
TARGETDIR = bin/Release-windows-x86_64
TARGET = $(TARGETDIR)/Osengine.exe
OBJDIR = bin-int/Release-windows-x86_64
DEFINES += -DPLATFORM_WINDOWS
ALL_CFLAGS += $(CFLAGS) $(ALL_CPPFLAGS) -m64
ALL_CXXFLAGS += $(CXXFLAGS) $(ALL_CPPFLAGS) -m64 -std=c++23
ALL_LDFLAGS += $(LDFLAGS) -Ldependencies/llvm/lib -L/usr/lib64 -m64 -s

endif

# Per File Configurations
# #############################################


# File sets
# #############################################

GENERATED :=
OBJECTS :=

GENERATED += $(OBJDIR)/Allocator.o
GENERATED += $(OBJDIR)/Application.o
GENERATED += $(OBJDIR)/Audio.o
GENERATED += $(OBJDIR)/AudioAccess.o
GENERATED += $(OBJDIR)/Class.o
GENERATED += $(OBJDIR)/Context2D.o
GENERATED += $(OBJDIR)/Context3D.o
GENERATED += $(OBJDIR)/Core.o
GENERATED += $(OBJDIR)/Date.o
GENERATED += $(OBJDIR)/EngineConfigs.o
GENERATED += $(OBJDIR)/Enum.o
GENERATED += $(OBJDIR)/Event.o
GENERATED += $(OBJDIR)/File.o
GENERATED += $(OBJDIR)/FileAccess.o
GENERATED += $(OBJDIR)/Function.o
GENERATED += $(OBJDIR)/Http.o
GENERATED += $(OBJDIR)/IRGenerator.o
GENERATED += $(OBJDIR)/JITCompiler.o
GENERATED += $(OBJDIR)/Json.o
GENERATED += $(OBJDIR)/KeyEvent.o
GENERATED += $(OBJDIR)/Math.o
GENERATED += $(OBJDIR)/MouseEvent.o
GENERATED += $(OBJDIR)/Namespace.o
GENERATED += $(OBJDIR)/Number.o
GENERATED += $(OBJDIR)/OS.o
GENERATED += $(OBJDIR)/Path.o
GENERATED += $(OBJDIR)/Pointer.o
GENERATED += $(OBJDIR)/RenderingContext.o
GENERATED += $(OBJDIR)/Statement.o
GENERATED += $(OBJDIR)/String.o
GENERATED += $(OBJDIR)/Time.o
GENERATED += $(OBJDIR)/Window.o
GENERATED += $(OBJDIR)/canvas.o
GENERATED += $(OBJDIR)/console.o
GENERATED += $(OBJDIR)/lexer.o
GENERATED += $(OBJDIR)/main.o
GENERATED += $(OBJDIR)/object.o
GENERATED += $(OBJDIR)/omniscript_pch.o
GENERATED += $(OBJDIR)/parser.o
GENERATED += $(OBJDIR)/symboltable.o
GENERATED += $(OBJDIR)/tokens.o
GENERATED += $(OBJDIR)/utils.o
OBJECTS += $(OBJDIR)/Allocator.o
OBJECTS += $(OBJDIR)/Application.o
OBJECTS += $(OBJDIR)/Audio.o
OBJECTS += $(OBJDIR)/AudioAccess.o
OBJECTS += $(OBJDIR)/Class.o
OBJECTS += $(OBJDIR)/Context2D.o
OBJECTS += $(OBJDIR)/Context3D.o
OBJECTS += $(OBJDIR)/Core.o
OBJECTS += $(OBJDIR)/Date.o
OBJECTS += $(OBJDIR)/EngineConfigs.o
OBJECTS += $(OBJDIR)/Enum.o
OBJECTS += $(OBJDIR)/Event.o
OBJECTS += $(OBJDIR)/File.o
OBJECTS += $(OBJDIR)/FileAccess.o
OBJECTS += $(OBJDIR)/Function.o
OBJECTS += $(OBJDIR)/Http.o
OBJECTS += $(OBJDIR)/IRGenerator.o
OBJECTS += $(OBJDIR)/JITCompiler.o
OBJECTS += $(OBJDIR)/Json.o
OBJECTS += $(OBJDIR)/KeyEvent.o
OBJECTS += $(OBJDIR)/Math.o
OBJECTS += $(OBJDIR)/MouseEvent.o
OBJECTS += $(OBJDIR)/Namespace.o
OBJECTS += $(OBJDIR)/Number.o
OBJECTS += $(OBJDIR)/OS.o
OBJECTS += $(OBJDIR)/Path.o
OBJECTS += $(OBJDIR)/Pointer.o
OBJECTS += $(OBJDIR)/RenderingContext.o
OBJECTS += $(OBJDIR)/Statement.o
OBJECTS += $(OBJDIR)/String.o
OBJECTS += $(OBJDIR)/Time.o
OBJECTS += $(OBJDIR)/Window.o
OBJECTS += $(OBJDIR)/canvas.o
OBJECTS += $(OBJDIR)/console.o
OBJECTS += $(OBJDIR)/lexer.o
OBJECTS += $(OBJDIR)/main.o
OBJECTS += $(OBJDIR)/object.o
OBJECTS += $(OBJDIR)/omniscript_pch.o
OBJECTS += $(OBJDIR)/parser.o
OBJECTS += $(OBJDIR)/symboltable.o
OBJECTS += $(OBJDIR)/tokens.o
OBJECTS += $(OBJDIR)/utils.o

# Rules
# #############################################

all: $(TARGET)
	@:

$(TARGET): $(GENERATED) $(OBJECTS) $(LDDEPS) | $(TARGETDIR)
	$(PRELINKCMDS)
	@echo Linking Osengine
	$(SILENT) $(LINKCMD)
	$(POSTBUILDCMDS)

$(TARGETDIR):
	@echo Creating $(TARGETDIR)
ifeq (posix,$(SHELLTYPE))
	$(SILENT) mkdir -p $(TARGETDIR)
else
	$(SILENT) mkdir $(subst /,\\,$(TARGETDIR))
endif

$(OBJDIR):
	@echo Creating $(OBJDIR)
ifeq (posix,$(SHELLTYPE))
	$(SILENT) mkdir -p $(OBJDIR)
else
	$(SILENT) mkdir $(subst /,\\,$(OBJDIR))
endif

clean:
	@echo Cleaning Osengine
ifeq (posix,$(SHELLTYPE))
	$(SILENT) rm -f  $(TARGET)
	$(SILENT) rm -rf $(GENERATED)
	$(SILENT) rm -rf $(OBJDIR)
else
	$(SILENT) if exist $(subst /,\\,$(TARGET)) del $(subst /,\\,$(TARGET))
	$(SILENT) if exist $(subst /,\\,$(GENERATED)) del /s /q $(subst /,\\,$(GENERATED))
	$(SILENT) if exist $(subst /,\\,$(OBJDIR)) rmdir /s /q $(subst /,\\,$(OBJDIR))
endif

prebuild: | $(OBJDIR)
	$(PREBUILDCMDS)

ifneq (,$(PCH))
$(OBJECTS): $(GCH) | $(PCH_PLACEHOLDER)
$(GCH): $(PCH) | prebuild
	@echo $(notdir $<)
	$(SILENT) $(CXX) -x c++-header $(ALL_CXXFLAGS) -o "$@" -MF "$(@:%.gch=%.d)" -c "$<"
$(PCH_PLACEHOLDER): $(GCH) | $(OBJDIR)
ifeq (posix,$(SHELLTYPE))
	$(SILENT) touch "$@"
else
	$(SILENT) echo $null >> "$@"
endif
else
$(OBJECTS): | prebuild
endif


# File Rules
# #############################################

$(OBJDIR)/Core.o: src/Core.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Allocator.o: src/engine/Allocator.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/EngineConfigs.o: src/engine/EngineConfigs.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/IRGenerator.o: src/engine/IRGenerator.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/JITCompiler.o: src/engine/JITCompiler.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/lexer.o: src/engine/lexer.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/parser.o: src/engine/parser.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/tokens.o: src/engine/tokens.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/main.o: src/main.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/omniscript_pch.o: src/omniscript_pch.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Class.o: src/runtime/Class.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Date.o: src/runtime/Date/Date.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Enum.o: src/runtime/Enum.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Function.o: src/runtime/Function.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Http.o: src/runtime/Http/Http.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Json.o: src/runtime/Json/Json.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Math.o: src/runtime/Math/Math.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Namespace.o: src/runtime/Namespace.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Number.o: src/runtime/Number.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/OS.o: src/runtime/OS/OS.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Path.o: src/runtime/Path/Path.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Pointer.o: src/runtime/Pointer.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Statement.o: src/runtime/Statement.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/String.o: src/runtime/String.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Time.o: src/runtime/Time/Time.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Audio.o: src/runtime/audio/Audio.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/AudioAccess.o: src/runtime/audio/AudioAccess.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Context2D.o: src/runtime/graphics/Context2D.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Context3D.o: src/runtime/graphics/Context3D.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Application.o: src/runtime/graphics/Events/Application.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Event.o: src/runtime/graphics/Events/Event.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/KeyEvent.o: src/runtime/graphics/Events/KeyEvent.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/MouseEvent.o: src/runtime/graphics/Events/MouseEvent.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/RenderingContext.o: src/runtime/graphics/RenderingContext.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/Window.o: src/runtime/graphics/Window.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/canvas.o: src/runtime/graphics/canvas.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/File.o: src/runtime/io/File.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/FileAccess.o: src/runtime/io/FileAccess.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/console.o: src/runtime/io/console.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/object.o: src/runtime/object.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/symboltable.o: src/runtime/symboltable.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"
$(OBJDIR)/utils.o: src/utils.cpp
	@echo "$(notdir $<)"
	$(SILENT) $(CXX) $(ALL_CXXFLAGS) $(FORCE_INCLUDE) -o "$@" -MF "$(@:%.o=%.d)" -c "$<"

-include $(OBJECTS:%.o=%.d)
ifneq (,$(PCH))
  -include $(PCH_PLACEHOLDER).d
endif