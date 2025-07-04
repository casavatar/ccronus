# Makefile for the Tactical Aim Assist Project
#
# developer: ingekastel (with AI assistance)
# version: 2.1.0 (Final Linker Fix with User-Provided Paths)
# date: 2025-06-26
# project: Tactical Aim Assist

# --- Compiler and Flags ---
CXX = x86_64-w64-mingw32-g++-posix
CXXFLAGS = -std=c++17 -g -Wall -Wextra -O2
LDFLAGS = -mwindows -static-libgcc -static-libstdc++

# --- Directories and Paths (User-Provided) ---
SRCDIR = src
OBJDIR = obj
AUBIO_PATH = aubio
PORTAUDIO_PATH = portaudio
WINSDK_PATH = winsdk-7-master/v7.1A

# --- Files and Executable ---
TARGET = TacticalAimAssist.exe
SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

# --- Include Paths ---
# Added the specific path to the Windows SDK Include directory.
INC_FLAGS = -I$(SRCDIR) \
            -I$(AUBIO_PATH)/src \
            -I$(PORTAUDIO_PATH)/include \
            -I$(WINSDK_PATH)/Include

# --- Library Paths and Files ---
# Define search paths for the linker.
LIB_PATHS = -L.

# FIX: Using the exact filenames provided by the user.
# The linker will now be pointed directly to these files.
AUBIO_LIB_FILE = -l:libaubio-5.dll
PORTAUDIO_LIB_FILE = -l:libportaudio-2.dll
SNDFILE_LIB_FILE = -l:libsndfile-1.dll

# System libraries required for PortAudio and Windows GUI functions.
SYSTEM_LIBS = -lwinmm -lole32 -luuid -lksguid -lgdi32

# --- Build Rules ---
all: $(TARGET)

# The linker command now uses the explicit library file paths.
# This is the most robust way to ensure the linker finds everything.
$(TARGET): $(OBJS)
	@echo "Linking executable with explicit library paths..."
	$(CXX) $(OBJS) -o $@ $(LIB_PATHS) $(AUBIO_LIB_FILE) $(PORTAUDIO_LIB_FILE) $(SNDFILE_LIB_FILE) $(SYSTEM_LIBS) $(LDFLAGS)
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