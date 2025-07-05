# Makefile for the Tactical Aim Assist Project
#
# developer: ingekastel (with AI assistance)
# version: 5.1.0 (Final - Explicit Library Path Linking)
# date: 2025-07-04
# project: Tactical Aim Assist

# --- Compiler and Flags ---
CXX = g++
CXXFLAGS = -std=c++17 -g -Wall -Wextra -O2
LDFLAGS = -mwindows -static

# --- Directories and Paths ---
SRCDIR = src
OBJDIR = obj
AUBIO_PATH = aubio
PORTAUDIO_PATH = portaudio

# --- Files and Executable ---
TARGET = TacticalAimAssist.exe
SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

# --- Include Paths ---
INC_FLAGS = -I$(SRCDIR) \
            -I$(AUBIO_PATH)/src \
            -I$(PORTAUDIO_PATH)/include

# --- Library Paths and Files ---
# Define search paths for the linker (still useful for system libs).
LIB_PATHS = -L$(AUBIO_PATH)/build/src \
            -L$(PORTAUDIO_PATH)/lib/.libs

# FIX: Define explicit paths to the library files to avoid ambiguity.
# This uses the exact filenames provided by the user.
AUBIO_LIB_FILE = $(AUBIO_PATH)/build/src/libaubio.a
PORTAUDIO_LIB_FILE = $(PORTAUDIO_PATH)/lib/.libs/libportaudio.dll.a

# System libraries required for PortAudio and Windows GUI functions.
SYSTEM_LIBS = -lfftw3f -ldsound -lwinmm -lole32 -luuid -lksguid -lgdi32

# --- Build Rules ---
all: $(TARGET)

# The linker command now uses the explicit library file paths instead of -l flags
# for aubio and portaudio. The order is critical.
$(TARGET): $(OBJS)
	@echo "Linking executable with explicit library paths..."
	$(CXX) $(OBJS) -o $@ $(LIB_PATHS) $(AUBIO_LIB_FILE) $(PORTAUDIO_LIB_FILE) $(SYSTEM_LIBS) $(LDFLAGS)
	@echo "Build finished: $(TARGET)"

# Compile each source file into an object file
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@echo "Compiling $<..."
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(INC_FLAGS) -c $< -o $@

# Clean up generated files
clean:
	@echo "Cleaning up..."
	rm -f $(OBJDIR)/*.o $(TARGET)
	@echo "Cleanup complete."

.PHONY: all clean