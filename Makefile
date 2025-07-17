# Makefile for Tactical Aim Assist v3.0 - Ultra Optimized
#
# description: Complete build system for Phase 3 optimizations
# developer: ingekastel
# license: GNU General Public License v3.0
# version: 3.0.2 - Phase 3 Ultra Optimized Build System
# date: 2025-07-16
# project: Tactical Aim Assist

# =============================================================================
# COMPILER AND BUILD CONFIGURATION
# =============================================================================
CXX = g++
AR = ar
STRIP = strip

# Base compiler flags
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic
ifeq ($(ENABLE_EXCEPTIONS),1)
    CXXFLAGS += -fexceptions
else
    CXXFLAGS += -fno-exceptions -DUSE_ERROR_CODES
endif

CPPFLAGS = -DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_WIN32_WINNT=0x0601

# Optimization flags
OPT_FLAGS = -O3 -march=native -mtune=native -flto -ffast-math
OPT_FLAGS += -funroll-loops -fomit-frame-pointer -finline-functions

# SIMD optimizations
SIMD_FLAGS = -mavx2 -msse4.2 -mfma
SIMD_FLAGS += -DENABLE_SIMD -DENABLE_AVX2

# Security and hardening flags
SECURITY_FLAGS = -fstack-protector-strong -D_FORTIFY_SOURCE=2
SECURITY_FLAGS += -Wformat -Wformat-security

# Debug vs Release configuration
ifeq ($(BUILD_TYPE),debug)
    CXXFLAGS += -g3 -O0 -DDEBUG -D_DEBUG -DENABLE_PROFILING
    OPT_FLAGS = -O0
    SIMD_FLAGS = 
else
    CXXFLAGS += -DNDEBUG -DRELEASE
    CXXFLAGS += $(OPT_FLAGS) $(SIMD_FLAGS) $(SECURITY_FLAGS)
endif

# Linker flags
LDFLAGS = -static -static-libgcc -static-libstdc++
LDFLAGS += -Wl,--gc-sections -Wl,--strip-all
ifeq ($(BUILD_TYPE),debug)
    LDFLAGS = -static-libgcc -static-libstdc++
endif

# =============================================================================
# DIRECTORY STRUCTURE (Updated for v3.0)
# =============================================================================
# Source directories
SRCDIR = src
CORE_SRCDIR = $(SRCDIR)/core
SYSTEMS_SRCDIR = $(SRCDIR)/systems
MODULAR_SRCDIR = $(SRCDIR)/modular
OPTIMIZATION_SRCDIR = $(SRCDIR)/optimization
WEAPONS_SRCDIR = $(SRCDIR)/weapons
GUI_SRCDIR = $(SRCDIR)/gui

# Build directories
BUILDDIR = build
OBJDIR = $(BUILDDIR)/obj
DEPDIR = $(BUILDDIR)/deps
LIBDIR = $(BUILDDIR)/lib
BINDIR = $(BUILDDIR)/bin

# Configuration and output directories
CONFIGDIR = config
PERFDIR = performance
DOCSDIR = docs
TOOLSDIR = tools

# External dependencies
PORTAUDIO_PATH = external/portaudio
FFTW_PATH = external/fftw3
JSON_PATH = external/nlohmann
IMGUI_PATH = external/imgui

# =============================================================================
# TARGET EXECUTABLES
# =============================================================================
TARGET = TacticalAimAssist.exe
TARGET_DEBUG = TacticalAimAssist_debug.exe
TARGET_PROFILE = TacticalAimAssist_profile.exe

ifeq ($(BUILD_TYPE),debug)
    MAIN_TARGET = $(BINDIR)/$(TARGET_DEBUG)
else ifeq ($(BUILD_TYPE),profile)
    MAIN_TARGET = $(BINDIR)/$(TARGET_PROFILE)
    CXXFLAGS += -DENABLE_PROFILING -pg
    LDFLAGS += -pg
else
    MAIN_TARGET = $(BINDIR)/$(TARGET)
endif

# =============================================================================
# SOURCE FILES (Organized by Phase)
# =============================================================================
# Core system files
CORE_SRCS = $(wildcard $(CORE_SRCDIR)/*.cpp)

# Legacy systems (maintaining compatibility)
SYSTEMS_SRCS = $(wildcard $(SYSTEMS_SRCDIR)/*.cpp)
SYSTEMS_SRCS := $(filter-out $(SYSTEMS_SRCDIR)/assist.cpp, $(SYSTEMS_SRCS))  # Remove old assist.cpp
SYSTEMS_SRCS := $(filter-out $(SYSTEMS_SRCDIR)/audio.cpp, $(SYSTEMS_SRCS))   # Remove old audio.cpp

# Phase 2: Modular systems
MODULAR_SRCS = $(wildcard $(MODULAR_SRCDIR)/*.cpp)

# Phase 3: Optimization systems
OPTIMIZATION_SRCS = $(wildcard $(OPTIMIZATION_SRCDIR)/*.cpp)

# Add optimized assist system
OPTIMIZATION_SRCS += $(SYSTEMS_SRCDIR)/assist_optimized.cpp

# Weapon and GUI systems
WEAPONS_SRCS = $(wildcard $(WEAPONS_SRCDIR)/*.cpp)
GUI_SRCS = $(wildcard $(GUI_SRCDIR)/*.cpp)

# Combine all sources
ALL_SRCS = $(CORE_SRCS) $(SYSTEMS_SRCS) $(MODULAR_SRCS) $(OPTIMIZATION_SRCS) $(WEAPONS_SRCS) $(GUI_SRCS)

# Generate object file paths
ALL_OBJS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(ALL_SRCS))

# Generate dependency files
DEPS = $(patsubst $(SRCDIR)/%.cpp,$(DEPDIR)/%.d,$(ALL_SRCS))

# =============================================================================
# INCLUDE PATHS
# =============================================================================
INC_FLAGS = -I$(SRCDIR) \
            -I$(CORE_SRCDIR) \
            -I$(SYSTEMS_SRCDIR) \
            -I$(MODULAR_SRCDIR) \
            -I$(OPTIMIZATION_SRCDIR) \
            -I$(WEAPONS_SRCDIR) \
            -I$(GUI_SRCDIR) \
            -I$(PORTAUDIO_PATH)/include \
            -I$(FFTW_PATH)/include \
            -I$(JSON_PATH)/include \
            -I$(IMGUI_PATH) \
            -I$(IMGUI_PATH)/backends

# =============================================================================
# EXTERNAL LIBRARIES
# =============================================================================
# PortAudio library
PORTAUDIO_LIB = $(PORTAUDIO_PATH)/lib/.libs/libportaudio.a

# FFTW library (for audio processing)
FFTW_LIB = $(FFTW_PATH)/lib/libfftw3f.a

# ImGui library (will be built)
IMGUI_LIB = $(LIBDIR)/libimgui.a

# System libraries for Windows
SYSTEM_LIBS = -luser32 -lgdi32 -lkernel32 -lshell32 -lole32 -luuid
SYSTEM_LIBS += -lwinmm -ldsound -lksguid -ladvapi32 -lpsapi -lpdh
SYSTEM_LIBS += -ld3d11 -ldxgi -ld3dcompiler -lcomctl32

# OpenGL libraries for ImGui
OPENGL_LIBS = -lopengl32 -lglu32

# All external libraries combined
EXTERNAL_LIBS = $(IMGUI_LIB) $(PORTAUDIO_LIB) $(FFTW_LIB)
ALL_LIBS = $(EXTERNAL_LIBS) $(SYSTEM_LIBS) $(OPENGL_LIBS)

# =============================================================================
# IMGUI SOURCE FILES
# =============================================================================
IMGUI_SRCS = $(IMGUI_PATH)/imgui.cpp \
             $(IMGUI_PATH)/imgui_demo.cpp \
             $(IMGUI_PATH)/imgui_draw.cpp \
             $(IMGUI_PATH)/imgui_tables.cpp \
             $(IMGUI_PATH)/imgui_widgets.cpp \
             $(IMGUI_PATH)/backends/imgui_impl_win32.cpp \
             $(IMGUI_PATH)/backends/imgui_impl_opengl3.cpp

IMGUI_OBJS = $(patsubst $(IMGUI_PATH)/%.cpp,$(OBJDIR)/imgui/%.o,$(IMGUI_SRCS))

# =============================================================================
# BUILD RULES
# =============================================================================
.PHONY: all clean debug release profile install uninstall setup help test benchmark docs

# Default target
all: release

# Build targets
release:
	@$(MAKE) build BUILD_TYPE=release

debug:
	@$(MAKE) build BUILD_TYPE=debug

profile:
	@$(MAKE) build BUILD_TYPE=profile

# Main build rule
build: setup $(MAIN_TARGET)

# Create main executable
$(MAIN_TARGET): $(ALL_OBJS) $(IMGUI_LIB) | $(BINDIR)
	@echo "🔗 Linking $(notdir $@) with Phase 3 optimizations..."
	@echo "   • Core Systems: $(words $(CORE_SRCS)) files"
	@echo "   • Legacy Systems: $(words $(SYSTEMS_SRCS)) files"
	@echo "   • Modular Systems: $(words $(MODULAR_SRCS)) files"
	@echo "   • Optimization Systems: $(words $(OPTIMIZATION_SRCS)) files"
	@echo "   • Total Objects: $(words $(ALL_OBJS)) files"
	$(CXX) $(ALL_OBJS) $(ALL_LIBS) -o $@ $(LDFLAGS)
	@if [ "$(BUILD_TYPE)" != "debug" ]; then \
		echo "🗜️  Stripping debug symbols..."; \
		$(STRIP) $@; \
	fi
	@echo "✅ Build completed: $@"
	@echo "📊 Binary size: $$(du -h $@ | cut -f1)"

# Build ImGui library
$(IMGUI_LIB): $(IMGUI_OBJS) | $(LIBDIR)
	@echo "📚 Creating ImGui static library..."
	$(AR) rcs $@ $(IMGUI_OBJS)
	@echo "✅ ImGui library created: $@"

# =============================================================================
# OBJECT FILE COMPILATION RULES
# =============================================================================
# Core system objects
$(OBJDIR)/core/%.o: $(CORE_SRCDIR)/%.cpp | $(OBJDIR)/core $(DEPDIR)/core
	@echo "🔧 Compiling core: $(notdir $<)"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INC_FLAGS) -MMD -MP -MF $(DEPDIR)/core/$*.d -c $< -o $@

# Systems objects
$(OBJDIR)/systems/%.o: $(SYSTEMS_SRCDIR)/%.cpp | $(OBJDIR)/systems $(DEPDIR)/systems
	@echo "🎯 Compiling system: $(notdir $<)"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INC_FLAGS) -MMD -MP -MF $(DEPDIR)/systems/$*.d -c $< -o $@

# Modular systems objects (Phase 2)
$(OBJDIR)/modular/%.o: $(MODULAR_SRCDIR)/%.cpp | $(OBJDIR)/modular $(DEPDIR)/modular
	@echo "🔧 Compiling modular: $(notdir $<)"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INC_FLAGS) -MMD -MP -MF $(DEPDIR)/modular/$*.d -c $< -o $@

# Optimization objects (Phase 3)
$(OBJDIR)/optimization/%.o: $(OPTIMIZATION_SRCDIR)/%.cpp | $(OBJDIR)/optimization $(DEPDIR)/optimization
	@echo "⚡ Compiling optimization: $(notdir $<)"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INC_FLAGS) -MMD -MP -MF $(DEPDIR)/optimization/$*.d -c $< -o $@

# Weapons objects
$(OBJDIR)/weapons/%.o: $(WEAPONS_SRCDIR)/%.cpp | $(OBJDIR)/weapons $(DEPDIR)/weapons
	@echo "🔫 Compiling weapon: $(notdir $<)"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INC_FLAGS) -MMD -MP -MF $(DEPDIR)/weapons/$*.d -c $< -o $@

# GUI objects
$(OBJDIR)/gui/%.o: $(GUI_SRCDIR)/%.cpp | $(OBJDIR)/gui $(DEPDIR)/gui
	@echo "🖥️  Compiling GUI: $(notdir $<)"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INC_FLAGS) -MMD -MP -MF $(DEPDIR)/gui/$*.d -c $< -o $@

# ImGui objects
$(OBJDIR)/imgui/%.o: $(IMGUI_PATH)/%.cpp | $(OBJDIR)/imgui
	@echo "🎨 Compiling ImGui: $(notdir $<)"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INC_FLAGS) -c $< -o $@

$(OBJDIR)/imgui/backends/%.o: $(IMGUI_PATH)/backends/%.cpp | $(OBJDIR)/imgui/backends
	@echo "🎨 Compiling ImGui backend: $(notdir $<)"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INC_FLAGS) -c $< -o $@

# =============================================================================
# DIRECTORY CREATION
# =============================================================================
$(OBJDIR) $(OBJDIR)/core $(OBJDIR)/systems $(OBJDIR)/modular $(OBJDIR)/optimization $(OBJDIR)/weapons $(OBJDIR)/gui $(OBJDIR)/imgui $(OBJDIR)/imgui/backends:
	@mkdir -p $@

$(DEPDIR) $(DEPDIR)/core $(DEPDIR)/systems $(DEPDIR)/modular $(DEPDIR)/optimization $(DEPDIR)/weapons $(DEPDIR)/gui:
	@mkdir -p $@

$(LIBDIR) $(BINDIR) $(CONFIGDIR) $(PERFDIR) $(DOCSDIR) $(TOOLSDIR):
	@mkdir -p $@

# =============================================================================
# SETUP AND INSTALLATION
# =============================================================================
setup: $(OBJDIR) $(DEPDIR) $(LIBDIR) $(BINDIR) $(CONFIGDIR) $(PERFDIR) $(DOCSDIR) $(TOOLSDIR)
	@echo "📁 Directory structure created"
	@if [ ! -f $(CONFIGDIR)/config.json ]; then \
		echo "📝 Creating default configuration files..."; \
		echo '{"general":{"assist_enabled":true,"debug_mode":false},"performance":{"enable_profiling":true}}' > $(CONFIGDIR)/config.json; \
	fi
	@if [ ! -f $(CONFIGDIR)/weapon_profiles.json ]; then \
		echo '{"weapon_profiles":[{"name":"Default","sensitivity":1.0,"smoothing":0.8}]}' > $(CONFIGDIR)/weapon_profiles.json; \
	fi

install: release
	@echo "📦 Installing Tactical Aim Assist v3.0..."
	@mkdir -p /usr/local/bin
	@mkdir -p /usr/local/share/tactical-aim-assist
	@cp $(MAIN_TARGET) /usr/local/bin/
	@cp -r $(CONFIGDIR) /usr/local/share/tactical-aim-assist/
	@cp -r $(DOCSDIR) /usr/local/share/tactical-aim-assist/
	@echo "✅ Installation completed"

uninstall:
	@echo "🗑️  Uninstalling Tactical Aim Assist..."
	@rm -f /usr/local/bin/$(TARGET)
	@rm -rf /usr/local/share/tactical-aim-assist
	@echo "✅ Uninstallation completed"

# =============================================================================
# TESTING AND BENCHMARKING
# =============================================================================
test: debug
	@echo "🧪 Running tests..."
	@$(BINDIR)/$(TARGET_DEBUG) --test-mode
	@echo "✅ Tests completed"

benchmark: profile
	@echo "📊 Running benchmarks..."
	@$(BINDIR)/$(TARGET_PROFILE) --benchmark-mode
	@echo "✅ Benchmarks completed"

performance-test: release
	@echo "⚡ Running performance tests..."
	@$(MAIN_TARGET) --performance-test --duration=60
	@echo "📈 Performance test results saved to $(PERFDIR)/"

# =============================================================================
# DOCUMENTATION
# =============================================================================
docs:
	@echo "📚 Generating documentation..."
	@if command -v doxygen >/dev/null 2>&1; then \
		doxygen Doxyfile; \
		echo "✅ Doxygen documentation generated"; \
	else \
		echo "⚠️  Doxygen not found, skipping API documentation"; \
	fi
	@echo "📝 Documentation available in $(DOCSDIR)/"

# =============================================================================
# CLEANING
# =============================================================================
clean:
	@echo "🧹 Cleaning build artifacts..."
	@rm -rf $(BUILDDIR)
	@echo "✅ Cleanup completed"

clean-all: clean
	@echo "🧹 Deep cleaning (including external libraries)..."
	@rm -rf $(PERFDIR)/logs/*
	@rm -rf $(PERFDIR)/reports/*
	@rm -rf $(PERFDIR)/metrics/*
	@echo "✅ Deep cleanup completed"

# =============================================================================
# DEVELOPMENT HELPERS
# =============================================================================
format:
	@echo "🎨 Formatting code..."
	@find $(SRCDIR) -name "*.cpp" -o -name "*.h" | xargs clang-format -i
	@echo "✅ Code formatting completed"

lint:
	@echo "🔍 Running static analysis..."
	@if command -v cppcheck >/dev/null 2>&1; then \
		cppcheck --enable=all --inconclusive --std=c++17 $(SRCDIR); \
	else \
		echo "⚠️  cppcheck not found, skipping static analysis"; \
	fi

analyze: lint
	@echo "📊 Running additional analysis..."
	@if command -v clang-tidy >/dev/null 2>&1; then \
		find $(SRCDIR) -name "*.cpp" | xargs clang-tidy; \
	fi

# =============================================================================
# HELP AND INFORMATION
# =============================================================================
help:
	@echo "🎯 Tactical Aim Assist v3.0 - Build System"
	@echo ""
	@echo "📋 Available targets:"
	@echo "  all          - Build release version (default)"
	@echo "  release      - Build optimized release version"
	@echo "  debug        - Build debug version with symbols"
	@echo "  profile      - Build with profiling enabled"
	@echo ""
	@echo "🔧 Development:"
	@echo "  test         - Run test suite"
	@echo "  benchmark    - Run performance benchmarks"
	@echo "  format       - Format source code"
	@echo "  lint         - Run static analysis"
	@echo "  analyze      - Run comprehensive analysis"
	@echo ""
	@echo "📚 Documentation:"
	@echo "  docs         - Generate documentation"
	@echo ""
	@echo "📦 Installation:"
	@echo "  install      - Install to system"
	@echo "  uninstall    - Remove from system"
	@echo ""
	@echo "🧹 Cleanup:"
	@echo "  clean        - Remove build artifacts"
	@echo "  clean-all    - Deep clean including logs"
	@echo ""
	@echo "💡 Build options:"
	@echo "  BUILD_TYPE=debug|release|profile"
	@echo "  ENABLE_SIMD=1|0"
	@echo "  ENABLE_PROFILING=1|0"

info:
	@echo "📊 Build Information:"
	@echo "  Version: 3.0.2"
	@echo "  Compiler: $(CXX)"
	@echo "  Build Type: $(BUILD_TYPE)"
	@echo "  Target: $(MAIN_TARGET)"
	@echo "  Sources: $(words $(ALL_SRCS)) files"
	@echo "  Include Paths: $(words $(subst -I, ,$(INC_FLAGS))) directories"
	@echo "  Libraries: $(words $(ALL_LIBS)) linked"
	@echo ""
	@echo "🔧 Phase 3 Optimizations:"
	@echo "  • Memory Pooling: Enabled"
	@echo "  • SIMD Operations: $(if $(findstring -mavx2,$(SIMD_FLAGS)),Enabled (AVX2),Disabled)"
	@echo "  • Performance Profiling: $(if $(findstring ENABLE_PROFILING,$(CXXFLAGS)),Enabled,Disabled)"
	@echo "  • Link Time Optimization: $(if $(findstring -flto,$(OPT_FLAGS)),Enabled,Disabled)"

# =============================================================================
# DEPENDENCY INCLUSION
# =============================================================================
# Include dependency files if they exist
-include $(DEPS)

# =============================================================================
# MAKEFILE VALIDATION
# =============================================================================
validate:
	@echo "✅ Validating Makefile configuration..."
	@echo "📁 Checking source directories..."
	@test -d $(SRCDIR) || (echo "❌ Source directory not found: $(SRCDIR)" && exit 1)
	@test -d $(CORE_SRCDIR) || (echo "❌ Core source directory not found: $(CORE_SRCDIR)" && exit 1)
	@test -d $(MODULAR_SRCDIR) || (echo "❌ Modular source directory not found: $(MODULAR_SRCDIR)" && exit 1)
	@test -d $(OPTIMIZATION_SRCDIR) || (echo "❌ Optimization source directory not found: $(OPTIMIZATION_SRCDIR)" && exit 1)
	@echo "📦 Checking external dependencies..."
	@test -d $(IMGUI_PATH) || (echo "⚠️  ImGui directory not found: $(IMGUI_PATH)" && exit 0)
	@echo "✅ Makefile validation passed"

# Special target to show all available sources
list-sources:
	@echo "📋 Source Files by Category:"
	@echo ""
	@echo "🔧 Core Systems ($(words $(CORE_SRCS)) files):"
	@$(foreach src,$(CORE_SRCS),echo "  $(src)";)
	@echo ""
	@echo "🎯 Legacy Systems ($(words $(SYSTEMS_SRCS)) files):"
	@$(foreach src,$(SYSTEMS_SRCS),echo "  $(src)";)
	@echo ""
	@echo "🔧 Modular Systems ($(words $(MODULAR_SRCS)) files):"
	@$(foreach src,$(MODULAR_SRCS),echo "  $(src)";)
	@echo ""
	@echo "⚡ Optimization Systems ($(words $(OPTIMIZATION_SRCS)) files):"
	@$(foreach src,$(OPTIMIZATION_SRCS),echo "  $(src)";)
	@echo ""
	@echo "🔫 Weapon Systems ($(words $(WEAPONS_SRCS)) files):"
	@$(foreach src,$(WEAPONS_SRCS),echo "  $(src)";)
	@echo ""
	@echo "🖥️  GUI Systems ($(words $(GUI_SRCS)) files):"
	@$(foreach src,$(GUI_SRCS),echo "  $(src)";)

# =============================================================================
# SPECIAL TARGETS FOR CI/CD
# =============================================================================
ci-build: clean validate setup release test
	@echo "🚀 CI/CD build completed successfully"

ci-test: debug test benchmark
	@echo "🧪 CI/CD testing completed"

# =============================================================================
# MAKEFILE VERSION INFO
# =============================================================================
version:
	@echo "Tactical Aim Assist Build System v3.0.2"
	@echo "Phase 3 Ultra Optimized - Complete Integration"
	@echo "Support for: Memory Pooling, SIMD, Performance Profiling"
	@echo "Developer: ingekastel"
	@echo "Date: 2025-07-16"